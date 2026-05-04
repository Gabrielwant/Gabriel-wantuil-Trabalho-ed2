
#include "hashfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- structs internas (privadas) ---------- */

typedef struct
{
  char key[HF_MAX_KEY];
  char data[HF_MAX_DATA];
  int valid; /* 1 = ocupado, 0 = vazio/removido */
} HFRecord;

typedef struct
{
  int local_depth;
  int count;
  int bucket_prefix; /* prefixo canônico deste bucket no diretório */
  HFRecord records[BUCKET_CAPACITY];
} HFBucket;

typedef struct
{
  int global_depth;
  int dir_size;
  int bucket_count;
} HFHeader;

#define MAX_SPLITS 1024

typedef struct
{
  int global_depth_before;
  int global_depth_after;
  char key_trigger[HF_MAX_KEY];
  int bucket_index;
} SplitEvent;

struct HFFile
{
  FILE *fp;
  char path[512];
  HFHeader header;
  long *dir; /* dir[i] = offset do bucket no arquivo */
  SplitEvent splits[MAX_SPLITS];
  int split_count;
};

/* ---------- funções auxiliares privadas ---------- */

/* djb2: retorna os últimos 'depth' bits */
static unsigned int hf_hash(const char *key, int depth)
{
  unsigned long hash = 5381;
  int c;
  while ((c = (unsigned char)*key++))
    hash = ((hash << 5) + hash) + c;
  if (depth == 0)
    return 0;
  return (unsigned int)(hash & ((1u << depth) - 1));
}

static long bucket_offset(int bucket_pos)
{
  return (long)sizeof(HFHeader) + (long)bucket_pos * (long)sizeof(HFBucket);
}

static int read_bucket(FILE *fp, long offset, HFBucket *bkt)
{
  if (fseek(fp, offset, SEEK_SET) != 0)
    return -1;
  if (fread(bkt, sizeof(HFBucket), 1, fp) != 1)
    return -1;
  return 0;
}

static int write_bucket(FILE *fp, long offset, const HFBucket *bkt)
{
  if (fseek(fp, offset, SEEK_SET) != 0)
    return -1;
  if (fwrite(bkt, sizeof(HFBucket), 1, fp) != 1)
    return -1;
  fflush(fp);
  return 0;
}

static int write_header(HFFile *hf)
{
  if (fseek(hf->fp, 0, SEEK_SET) != 0)
    return -1;
  if (fwrite(&hf->header, sizeof(HFHeader), 1, hf->fp) != 1)
    return -1;
  fflush(hf->fp);
  return 0;
}

/* aloca novo bucket com prefix canônico */
static long alloc_bucket(HFFile *hf, int local_depth, int prefix)
{
  HFBucket bkt;
  memset(&bkt, 0, sizeof(HFBucket));
  bkt.local_depth = local_depth;
  bkt.bucket_prefix = prefix;
  bkt.count = 0;

  long offset = bucket_offset(hf->header.bucket_count);
  hf->header.bucket_count++;
  write_header(hf);
  write_bucket(hf->fp, offset, &bkt);
  return offset;
}

static int double_directory(HFFile *hf)
{
  int old_size = hf->header.dir_size;
  int new_size = old_size * 2;

  long *new_dir = realloc(hf->dir, (size_t)new_size * sizeof(long));
  if (!new_dir)
    return -1;

  for (int i = 0; i < old_size; i++)
    new_dir[old_size + i] = new_dir[i];

  hf->dir = new_dir;
  hf->header.global_depth++;
  hf->header.dir_size = new_size;
  write_header(hf);
  return 0;
}

static int split_bucket(HFFile *hf, int dir_index, const char *trigger_key)
{
  long old_offset = hf->dir[dir_index];
  HFBucket old_bkt;
  if (read_bucket(hf->fp, old_offset, &old_bkt) != 0)
    return -1;

  int old_depth = old_bkt.local_depth;
  int new_depth = old_depth + 1;

  /* registra evento */
  if (hf->split_count < MAX_SPLITS)
  {
    SplitEvent *ev = &hf->splits[hf->split_count++];
    ev->global_depth_before = hf->header.global_depth;
    ev->bucket_index = dir_index;
    strncpy(ev->key_trigger, trigger_key, HF_MAX_KEY - 1);
    ev->key_trigger[HF_MAX_KEY - 1] = '\0';
  }

  if (new_depth > hf->header.global_depth)
    if (double_directory(hf) != 0)
      return -1;

  /* prefixos canônicos dos dois buckets filhos */
  int old_prefix = dir_index & ((1 << old_depth) - 1);
  int new_prefix = old_prefix | (1 << old_depth);

  long new_offset = alloc_bucket(hf, new_depth, new_prefix);

  /* atualiza prefixo do bucket antigo */
  old_bkt.local_depth = new_depth;
  old_bkt.bucket_prefix = old_prefix;

  /* redistribui entradas do diretório */
  int mask = (1 << new_depth) - 1;
  for (int i = 0; i < hf->header.dir_size; i++)
  {
    if ((i & mask) == (new_prefix & mask))
      hf->dir[i] = new_offset;
    else if ((i & mask) == (old_prefix & mask))
      hf->dir[i] = old_offset;
  }

  /* redistribui registros */
  HFBucket new_bkt, tmp_old;
  memset(&new_bkt, 0, sizeof(HFBucket));
  memset(&tmp_old, 0, sizeof(HFBucket));
  new_bkt.local_depth = new_depth;
  new_bkt.bucket_prefix = new_prefix;
  tmp_old.local_depth = new_depth;
  tmp_old.bucket_prefix = old_prefix;

  for (int i = 0; i < BUCKET_CAPACITY; i++)
  {
    if (!old_bkt.records[i].valid)
      continue;
    unsigned int h = hf_hash(old_bkt.records[i].key, new_depth);
    if ((int)(h & mask) == (new_prefix & mask))
    {
      if (new_bkt.count < BUCKET_CAPACITY)
        new_bkt.records[new_bkt.count++] = old_bkt.records[i];
    }
    else
    {
      if (tmp_old.count < BUCKET_CAPACITY)
        tmp_old.records[tmp_old.count++] = old_bkt.records[i];
    }
  }

  write_bucket(hf->fp, old_offset, &tmp_old);
  write_bucket(hf->fp, new_offset, &new_bkt);

  if (hf->split_count > 0)
    hf->splits[hf->split_count - 1].global_depth_after = hf->header.global_depth;

  return 0;
}

/* ---------- API pública ---------- */

HFFile *hf_create(const char *path, int init_depth)
{
  HFFile *hf = calloc(1, sizeof(HFFile));
  if (!hf)
    return NULL;

  strncpy(hf->path, path, sizeof(hf->path) - 1);
  hf->fp = fopen(path, "w+b");
  if (!hf->fp)
  {
    free(hf);
    return NULL;
  }

  int dir_size = (1 << init_depth);
  hf->header.global_depth = init_depth;
  hf->header.dir_size = dir_size;
  hf->header.bucket_count = 0;

  hf->dir = malloc((size_t)dir_size * sizeof(long));
  if (!hf->dir)
  {
    fclose(hf->fp);
    free(hf);
    return NULL;
  }

  write_header(hf);

  /* cria um bucket por entrada do diretório, com prefix canônico */
  for (int i = 0; i < dir_size; i++)
  {
    long off = alloc_bucket(hf, init_depth, i);
    hf->dir[i] = off;
  }

  hf->split_count = 0;
  return hf;
}

/*
 * hf_open — reabre um hashfile existente.
 *
 * Reconstrução do diretório:
 *   Para cada bucket lido do arquivo, usamos bucket_prefix e local_depth
 *   para mapear todas as entradas do diretório com o mesmo sufixo de
 *   local_depth bits para esse bucket.
 *   Buckets mais "profundos" têm prioridade (não são sobrescritos por
 *   buckets de profundidade menor).
 */
HFFile *hf_open(const char *path)
{
  HFFile *hf = calloc(1, sizeof(HFFile));
  if (!hf)
    return NULL;

  strncpy(hf->path, path, sizeof(hf->path) - 1);
  hf->fp = fopen(path, "r+b");
  if (!hf->fp)
  {
    free(hf);
    return NULL;
  }

  if (fread(&hf->header, sizeof(HFHeader), 1, hf->fp) != 1)
  {
    fclose(hf->fp);
    free(hf);
    return NULL;
  }

  hf->dir = malloc((size_t)hf->header.dir_size * sizeof(long));
  if (!hf->dir)
  {
    fclose(hf->fp);
    free(hf);
    return NULL;
  }

  /* inicializa diretório com -1 (não mapeado) */
  for (int i = 0; i < hf->header.dir_size; i++)
    hf->dir[i] = -1L;

  /*
   * Percorre todos os buckets físicos.
   * Para cada bucket, mapeia as entradas do diretório cujo sufixo de
   * local_depth bits coincide com bucket_prefix, mas somente se a entrada
   * ainda não foi mapeada ou foi mapeada por bucket menos profundo.
   */
  /* depth_of[i] = local_depth do bucket que mapeou dir[i] (-1 = nenhum) */
  int *depth_of = malloc((size_t)hf->header.dir_size * sizeof(int));
  if (!depth_of)
  {
    fclose(hf->fp);
    free(hf->dir);
    free(hf);
    return NULL;
  }
  for (int i = 0; i < hf->header.dir_size; i++)
    depth_of[i] = -1;

  for (int b = 0; b < hf->header.bucket_count; b++)
  {
    long off = bucket_offset(b);
    HFBucket bkt;
    if (read_bucket(hf->fp, off, &bkt) != 0)
      continue;

    int depth = bkt.local_depth;
    int prefix = bkt.bucket_prefix;
    int mask = (depth > 0) ? ((1 << depth) - 1) : 0;

    for (int i = 0; i < hf->header.dir_size; i++)
    {
      if ((i & mask) == (prefix & mask))
      {
        /* mapeia se não mapeado ou se o atual tem profundidade menor */
        if (depth_of[i] < depth)
        {
          hf->dir[i] = off;
          depth_of[i] = depth;
        }
      }
    }
  }

  free(depth_of);

  /* fallback: entradas ainda -1 apontam para bucket 0 */
  for (int i = 0; i < hf->header.dir_size; i++)
    if (hf->dir[i] == -1L)
      hf->dir[i] = bucket_offset(0);

  hf->split_count = 0;
  return hf;
}

void hf_close(HFFile *hf)
{
  if (!hf)
    return;
  fclose(hf->fp);
  free(hf->dir);
  free(hf);
}

int hf_insert(HFFile *hf, const char *key, const char *data)
{
  if (!hf || !key || !data)
    return -1;

  int attempts = 0;
  while (attempts++ < 128)
  {
    unsigned int h = hf_hash(key, hf->header.global_depth);
    long offset = hf->dir[h];
    HFBucket bkt;
    if (read_bucket(hf->fp, offset, &bkt) != 0)
      return -1;

    /* atualiza se chave já existe */
    for (int i = 0; i < BUCKET_CAPACITY; i++)
    {
      if (bkt.records[i].valid &&
          strncmp(bkt.records[i].key, key, HF_MAX_KEY) == 0)
      {
        strncpy(bkt.records[i].data, data, HF_MAX_DATA - 1);
        bkt.records[i].data[HF_MAX_DATA - 1] = '\0';
        return write_bucket(hf->fp, offset, &bkt);
      }
    }

    /* slot vazio */
    for (int i = 0; i < BUCKET_CAPACITY; i++)
    {
      if (!bkt.records[i].valid)
      {
        strncpy(bkt.records[i].key, key, HF_MAX_KEY - 1);
        bkt.records[i].key[HF_MAX_KEY - 1] = '\0';
        strncpy(bkt.records[i].data, data, HF_MAX_DATA - 1);
        bkt.records[i].data[HF_MAX_DATA - 1] = '\0';
        bkt.records[i].valid = 1;
        bkt.count++;
        return write_bucket(hf->fp, offset, &bkt);
      }
    }

    /* bucket cheio: split */
    if (split_bucket(hf, (int)h, key) != 0)
      return -1;
  }
  return -1;
}

int hf_search(HFFile *hf, const char *key, char *data_out)
{
  if (!hf || !key || !data_out)
    return -1;

  unsigned int h = hf_hash(key, hf->header.global_depth);
  long offset = hf->dir[h];
  HFBucket bkt;
  if (read_bucket(hf->fp, offset, &bkt) != 0)
    return -1;

  for (int i = 0; i < BUCKET_CAPACITY; i++)
  {
    if (bkt.records[i].valid &&
        strncmp(bkt.records[i].key, key, HF_MAX_KEY) == 0)
    {
      strncpy(data_out, bkt.records[i].data, HF_MAX_DATA);
      return 0;
    }
  }
  return -1;
}

int hf_delete(HFFile *hf, const char *key)
{
  if (!hf || !key)
    return -1;

  unsigned int h = hf_hash(key, hf->header.global_depth);
  long offset = hf->dir[h];
  HFBucket bkt;
  if (read_bucket(hf->fp, offset, &bkt) != 0)
    return -1;

  for (int i = 0; i < BUCKET_CAPACITY; i++)
  {
    if (bkt.records[i].valid &&
        strncmp(bkt.records[i].key, key, HF_MAX_KEY) == 0)
    {
      bkt.records[i].valid = 0;
      if (bkt.count > 0)
        bkt.count--;
      return write_bucket(hf->fp, offset, &bkt);
    }
  }
  return -1;
}

void hf_dump(HFFile *hf, const char *path_hfd)
{
  if (!hf || !path_hfd)
    return;

  FILE *out = fopen(path_hfd, "w");
  if (!out)
    return;

  fprintf(out, "=== HASHFILE DUMP: %s ===\n", hf->path);
  fprintf(out, "Profundidade global : %d\n", hf->header.global_depth);
  fprintf(out, "Tamanho do diretorio: %d\n", hf->header.dir_size);
  fprintf(out, "Numero de buckets   : %d\n\n", hf->header.bucket_count);

  fprintf(out, "--- DIRETORIO ---\n");
  for (int i = 0; i < hf->header.dir_size; i++)
    fprintf(out, "  dir[%3d] -> offset %ld\n", i, hf->dir[i]);
  fprintf(out, "\n");

  fprintf(out, "--- BUCKETS ---\n");
  for (int b = 0; b < hf->header.bucket_count; b++)
  {
    long off = bucket_offset(b);
    HFBucket bkt;
    if (read_bucket(hf->fp, off, &bkt) != 0)
      continue;

    fprintf(out, "Bucket %d (offset=%ld, local_depth=%d, prefix=%d, count=%d)\n",
            b, off, bkt.local_depth, bkt.bucket_prefix, bkt.count);
    for (int r = 0; r < BUCKET_CAPACITY; r++)
    {
      if (bkt.records[r].valid)
        fprintf(out, "    [%d] key=\"%s\" data=\"%s\"\n",
                r, bkt.records[r].key, bkt.records[r].data);
      else
        fprintf(out, "    [%d] (vazio)\n", r);
    }
  }

  fprintf(out, "\n--- EXPANSOES (SPLITS) ---\n");
  if (hf->split_count == 0)
    fprintf(out, "  Nenhuma expansao ocorreu.\n");
  else
    for (int i = 0; i < hf->split_count; i++)
    {
      SplitEvent *ev = &hf->splits[i];
      fprintf(out, "  Split %d: chave=\"%s\", bucket_dir=%d, depth %d -> %d\n",
              i + 1, ev->key_trigger, ev->bucket_index,
              ev->global_depth_before, ev->global_depth_after);
    }

  fclose(out);
}

void hf_for_each(HFFile *hf,
                 void (*callback)(const char *key, const char *data,
                                  void *userdata),
                 void *userdata)
{
  if (!hf || !callback)
    return;

  for (int b = 0; b < hf->header.bucket_count; b++)
  {
    long off = bucket_offset(b);
    HFBucket bkt;
    if (read_bucket(hf->fp, off, &bkt) != 0)
      continue;
    for (int r = 0; r < BUCKET_CAPACITY; r++)
      if (bkt.records[r].valid)
        callback(bkt.records[r].key, bkt.records[r].data, userdata);
  }
}