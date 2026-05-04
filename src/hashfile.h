#ifndef HASHFILE_H
#define HASHFILE_H

#include <stdio.h>

/* ---------- constantes configuráveis ---------- */
#define HF_MAX_KEY 32     /* tamanho máximo da chave (string)      */
#define HF_MAX_DATA 256   /* tamanho máximo do valor serializado    */
#define BUCKET_CAPACITY 4 /* registros por bucket                   */

/* Handle do hashfile (tipo opaco — implementação em hashfile.c) */
typedef struct HFFile HFFile;

/* ---------- API pública ---------- */
HFFile *hf_create(const char *path, int init_depth);

/*
 * hf_open — abre um hashfile existente.
 * Retorna ponteiro para HFFile em caso de sucesso, NULL se não existir.
 */
HFFile *hf_open(const char *path);

/*
 * hf_close — fecha e libera recursos do hashfile.
 */
void hf_close(HFFile *hf);

/*
 * hf_insert — insere um par (key, data) no hashfile.
 * Se a chave já existir, sobrescreve o dado.
 * Retorna 0 em sucesso, -1 em falha.
 */
int hf_insert(HFFile *hf, const char *key, const char *data);

/*
 * hf_search — busca a chave no hashfile.
 * Se encontrada, copia o dado em data_out (buffer de pelo menos HF_MAX_DATA bytes).
 * Retorna 0 em sucesso, -1 se não encontrado.
 */
int hf_search(HFFile *hf, const char *key, char *data_out);

/*
 * hf_delete — marca o registro com a chave como removido (tombstone).
 * Retorna 0 em sucesso, -1 se não encontrado.
 */
int hf_delete(HFFile *hf, const char *key);

/*
 * hf_dump — gera arquivo texto .hfd com representação legível do hashfile.
 * Registra também as expansões (splits) ocorridas durante a sessão.
 *   path_hfd : caminho do arquivo de saída
 */
void hf_dump(HFFile *hf, const char *path_hfd);

/*
 * hf_for_each — itera sobre todos os registros válidos.
 * Chama callback(key, data, userdata) para cada registro ativo.
 */
void hf_for_each(HFFile *hf,
                 void (*callback)(const char *key, const char *data,
                                  void *userdata),
                 void *userdata);

#endif /* HASHFILE_H */