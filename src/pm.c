/*
 * pm.c — Implementação do módulo de Pessoas e Moradores
 */

#include "pm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- serialização ---------- */

static void habitante_serialize(const Habitante *h, char *buf, int len)
{
  snprintf(buf, len, "%s|%s|%s|%c|%s|%d",
           h->cpf, h->nome, h->sobrenome, h->sexo, h->nasc, h->vivo);
}

static int habitante_deserialize(const char *buf, Habitante *h)
{
  char tmp[HF_MAX_DATA];
  strncpy(tmp, buf, HF_MAX_DATA - 1);
  tmp[HF_MAX_DATA - 1] = '\0';

  char *tok = strtok(tmp, "|");
  if (!tok)
    return -1;
  strncpy(h->cpf, tok, CPF_MAX - 1);

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  strncpy(h->nome, tok, NOME_MAX - 1);

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  strncpy(h->sobrenome, tok, NOME_MAX - 1);

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  h->sexo = tok[0];

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  strncpy(h->nasc, tok, DATA_MAX - 1);

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  h->vivo = atoi(tok);

  return 0;
}

static void endereco_serialize(const Endereco *e, char *buf, int len)
{
  snprintf(buf, len, "%s|%s|%c|%d|%s",
           e->cpf, e->cep, e->face, e->numero, e->compl);
}

static int endereco_deserialize(const char *buf, Endereco *e)
{
  char tmp[HF_MAX_DATA];
  strncpy(tmp, buf, HF_MAX_DATA - 1);
  tmp[HF_MAX_DATA - 1] = '\0';

  char *tok = strtok(tmp, "|");
  if (!tok)
    return -1;
  strncpy(e->cpf, tok, CPF_MAX - 1);

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  strncpy(e->cep, tok, CEP_MAX - 1);

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  e->face = tok[0];

  tok = strtok(NULL, "|");
  if (!tok)
    return -1;
  e->numero = atoi(tok);

  tok = strtok(NULL, "|");
  if (tok)
    strncpy(e->compl, tok, COMPL_MAX - 1);
  else
    e->compl[0] = '\0';

  return 0;
}

/* ---------- estrutura interna ---------- */

struct PMCtx
{
  HFFile *hf_pessoas;
  HFFile *hf_moradores;
  char path_p[512];
  char path_m[512];
};

/* ---------- API pública ---------- */

PMCtx *pm_create(const char *path_hf_pessoas, const char *path_hf_moradores)
{
  PMCtx *ctx = calloc(1, sizeof(PMCtx));
  if (!ctx)
    return NULL;
  strncpy(ctx->path_p, path_hf_pessoas, sizeof(ctx->path_p) - 1);
  strncpy(ctx->path_m, path_hf_moradores, sizeof(ctx->path_m) - 1);
  ctx->hf_pessoas = hf_create(path_hf_pessoas, 1);
  ctx->hf_moradores = hf_create(path_hf_moradores, 1);
  if (!ctx->hf_pessoas || !ctx->hf_moradores)
  {
    if (ctx->hf_pessoas)
      hf_close(ctx->hf_pessoas);
    if (ctx->hf_moradores)
      hf_close(ctx->hf_moradores);
    free(ctx);
    return NULL;
  }
  return ctx;
}

PMCtx *pm_open(const char *path_hf_pessoas, const char *path_hf_moradores)
{
  PMCtx *ctx = calloc(1, sizeof(PMCtx));
  if (!ctx)
    return NULL;
  strncpy(ctx->path_p, path_hf_pessoas, sizeof(ctx->path_p) - 1);
  strncpy(ctx->path_m, path_hf_moradores, sizeof(ctx->path_m) - 1);
  ctx->hf_pessoas = hf_open(path_hf_pessoas);
  ctx->hf_moradores = hf_open(path_hf_moradores);
  if (!ctx->hf_pessoas || !ctx->hf_moradores)
  {
    if (ctx->hf_pessoas)
      hf_close(ctx->hf_pessoas);
    if (ctx->hf_moradores)
      hf_close(ctx->hf_moradores);
    free(ctx);
    return NULL;
  }
  return ctx;
}

void pm_free(PMCtx *ctx)
{
  if (!ctx)
    return;
  hf_close(ctx->hf_pessoas);
  hf_close(ctx->hf_moradores);
  free(ctx);
}

int pm_inserir_habitante(PMCtx *ctx, const Habitante *h)
{
  if (!ctx || !h)
    return -1;
  char buf[HF_MAX_DATA];
  habitante_serialize(h, buf, sizeof(buf));
  return hf_insert(ctx->hf_pessoas, h->cpf, buf);
}

int pm_buscar_habitante(PMCtx *ctx, const char *cpf, Habitante *h_out)
{
  if (!ctx || !cpf || !h_out)
    return -1;
  char buf[HF_MAX_DATA];
  if (hf_search(ctx->hf_pessoas, cpf, buf) != 0)
    return -1;
  return habitante_deserialize(buf, h_out);
}

int pm_remover_habitante(PMCtx *ctx, const char *cpf)
{
  if (!ctx || !cpf)
    return -1;
  return hf_delete(ctx->hf_pessoas, cpf);
}

int pm_registrar_endereco(PMCtx *ctx, const Endereco *e)
{
  if (!ctx || !e)
    return -1;
  char buf[HF_MAX_DATA];
  endereco_serialize(e, buf, sizeof(buf));
  return hf_insert(ctx->hf_moradores, e->cpf, buf);
}

int pm_buscar_endereco(PMCtx *ctx, const char *cpf, Endereco *e_out)
{
  if (!ctx || !cpf || !e_out)
    return -1;
  char buf[HF_MAX_DATA];
  if (hf_search(ctx->hf_moradores, cpf, buf) != 0)
    return -1;
  return endereco_deserialize(buf, e_out);
}

int pm_remover_endereco(PMCtx *ctx, const char *cpf)
{
  if (!ctx || !cpf)
    return -1;
  return hf_delete(ctx->hf_moradores, cpf);
}

int pm_eh_morador(PMCtx *ctx, const char *cpf)
{
  if (!ctx || !cpf)
    return 0;
  char buf[HF_MAX_DATA];
  return (hf_search(ctx->hf_moradores, cpf, buf) == 0) ? 1 : 0;
}

int pm_parse_pm_file(PMCtx *ctx, const char *path_pm)
{
  if (!ctx || !path_pm)
    return -1;
  FILE *fp = fopen(path_pm, "r");
  if (!fp)
    return -1;

  char line[512];
  int count = 0;

  while (fgets(line, sizeof(line), fp))
  {
    char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0')
      continue;

    char cmd[8];
    if (sscanf(p, "%7s", cmd) != 1)
      continue;

    if (strcmp(cmd, "p") == 0)
    {
      Habitante h;
      memset(&h, 0, sizeof(h));
      h.vivo = 1;
      if (sscanf(p, "%*s %15s %63s %63s %c %11s",
                 h.cpf, h.nome, h.sobrenome, &h.sexo, h.nasc) == 5)
      {
        pm_inserir_habitante(ctx, &h);
        count++;
      }
    }
    else if (strcmp(cmd, "m") == 0)
    {
      Endereco e;
      memset(&e, 0, sizeof(e));
      char face_str[4];
      if (sscanf(p, "%*s %15s %31s %3s %d %31s",
                 e.cpf, e.cep, face_str, &e.numero, e.compl) >= 4)
      {
        e.face = face_str[0];
        pm_registrar_endereco(ctx, &e);
        count++;
      }
    }
  }

  fclose(fp);
  return count;
}

/* ---------- iteração de habitantes ---------- */

typedef struct
{
  void (*cb)(const Habitante *, void *);
  void *ud;
} HabIterCtx;

static void hab_iter_cb(const char *key, const char *data, void *ud)
{
  (void)key;
  HabIterCtx *ic = (HabIterCtx *)ud;
  Habitante h;
  if (habitante_deserialize(data, &h) == 0)
    ic->cb(&h, ic->ud);
}

void pm_for_each_habitante(PMCtx *ctx,
                           void (*cb)(const Habitante *, void *), void *ud)
{
  if (!ctx || !cb)
    return;
  HabIterCtx ic = {cb, ud};
  hf_for_each(ctx->hf_pessoas, hab_iter_cb, &ic);
}

/* ---------- iteração de moradores ---------- */

typedef struct
{
  void (*cb)(const Endereco *, void *);
  void *ud;
} EndIterCtx;

static void end_iter_cb(const char *key, const char *data, void *ud)
{
  (void)key;
  EndIterCtx *ic = (EndIterCtx *)ud;
  Endereco e;
  if (endereco_deserialize(data, &e) == 0)
    ic->cb(&e, ic->ud);
}

void pm_for_each_morador(PMCtx *ctx,
                         void (*cb)(const Endereco *, void *), void *ud)
{
  if (!ctx || !cb)
    return;
  EndIterCtx ic = {cb, ud};
  hf_for_each(ctx->hf_moradores, end_iter_cb, &ic);
}

/* ---------- contagem de moradores por quadra ---------- */

typedef struct
{
  const char *cep;
  int contagem[4]; /* [0]=N [1]=S [2]=L [3]=O */
} ContCtx;

static void cont_cb(const Endereco *e, void *ud)
{
  ContCtx *cc = (ContCtx *)ud;
  if (strncmp(e->cep, cc->cep, CEP_MAX) != 0)
    return;
  switch (e->face)
  {
  case 'N':
    cc->contagem[0]++;
    break;
  case 'S':
    cc->contagem[1]++;
    break;
  case 'L':
    cc->contagem[2]++;
    break;
  case 'O':
    cc->contagem[3]++;
    break;
  default:
    break;
  }
}

int pm_moradores_da_quadra(PMCtx *ctx, const char *cep, int contagem[4])
{
  if (!ctx || !cep || !contagem)
    return 0;
  ContCtx cc;
  cc.cep = cep;
  memset(cc.contagem, 0, sizeof(cc.contagem));
  pm_for_each_morador(ctx, cont_cb, &cc);
  memcpy(contagem, cc.contagem, sizeof(cc.contagem));
  return cc.contagem[0] + cc.contagem[1] + cc.contagem[2] + cc.contagem[3];
}

void pm_dump(PMCtx *ctx, const char *path_hfd_pessoas,
             const char *path_hfd_moradores)
{
  if (!ctx)
    return;
  if (path_hfd_pessoas)
    hf_dump(ctx->hf_pessoas, path_hfd_pessoas);
  if (path_hfd_moradores)
    hf_dump(ctx->hf_moradores, path_hfd_moradores);
}