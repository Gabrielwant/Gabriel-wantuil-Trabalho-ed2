#ifndef PM_H
#define PM_H

/*
 * pm.h — Módulo de Pessoas e Moradores
 *
 * Gerencia habitantes (pessoas) e moradores (subconjunto com endereço).
 * Sem-tetos são habitantes sem endereço associado.
 *
 * Armazenamento em dois hashfiles:
 *   pessoas.hf   — todos os habitantes, indexados por CPF
 *   moradores.hf — endereços de moradores, indexados por CPF
 */

#include "hashfile.h"
#include "geo.h"

#define CPF_MAX 16
#define NOME_MAX 64
#define DATA_MAX 12 /* dd/mm/aaaa */
#define COMPL_MAX 32

/* Habitante */
typedef struct
{
  char cpf[CPF_MAX];
  char nome[NOME_MAX];
  char sobrenome[NOME_MAX];
  char sexo;           /* 'M' ou 'F'   */
  char nasc[DATA_MAX]; /* dd/mm/aaaa   */
  int vivo;            /* 1=vivo 0=falecido */
} Habitante;

/* Endereço de um morador */
typedef struct
{
  char cpf[CPF_MAX];
  char cep[CEP_MAX];
  char face;
  int numero;
  char compl[COMPL_MAX];
} Endereco;

/* Contexto do módulo pm (opaco) */
typedef struct PMCtx PMCtx;

/*
 * pm_create — inicializa contexto e cria hashfiles.
 */
PMCtx *pm_create(const char *path_hf_pessoas, const char *path_hf_moradores);

/*
 * pm_open — abre hashfiles existentes.
 */
PMCtx *pm_open(const char *path_hf_pessoas, const char *path_hf_moradores);

/*
 * pm_free — libera recursos.
 */
void pm_free(PMCtx *ctx);

/*
 * pm_inserir_habitante — insere novo habitante (ou atualiza se já existe).
 * Retorna 0 em sucesso, -1 em falha.
 */
int pm_inserir_habitante(PMCtx *ctx, const Habitante *h);

/*
 * pm_buscar_habitante — busca habitante pelo CPF.
 * Preenche h_out se encontrado.
 * Retorna 0 em sucesso, -1 se não encontrado.
 */
int pm_buscar_habitante(PMCtx *ctx, const char *cpf, Habitante *h_out);

/*
 * pm_remover_habitante — remove registro de habitante.
 * Retorna 0 em sucesso, -1 se não encontrado.
 */
int pm_remover_habitante(PMCtx *ctx, const char *cpf);

/*
 * pm_registrar_endereco — associa habitante a endereço (torna-o morador).
 * Retorna 0 em sucesso, -1 em falha.
 */
int pm_registrar_endereco(PMCtx *ctx, const Endereco *e);

/*
 * pm_buscar_endereco — busca endereço de um morador pelo CPF.
 * Retorna 0 em sucesso, -1 se não for morador.
 */
int pm_buscar_endereco(PMCtx *ctx, const char *cpf, Endereco *e_out);

/*
 * pm_remover_endereco — remove endereço (morador vira sem-teto).
 * Retorna 0 em sucesso, -1 se não tinha endereço.
 */
int pm_remover_endereco(PMCtx *ctx, const char *cpf);

/*
 * pm_eh_morador — retorna 1 se o CPF tem endereço, 0 caso contrário.
 */
int pm_eh_morador(PMCtx *ctx, const char *cpf);

/*
 * pm_parse_pm_file — lê arquivo .pm e popula os hashfiles.
 * Interpreta comandos 'p' (pessoa) e 'm' (morador).
 * Retorna número de registros processados, -1 em falha.
 */
int pm_parse_pm_file(PMCtx *ctx, const char *path_pm);

/*
 * pm_for_each_habitante — itera sobre todos os habitantes.
 */
void pm_for_each_habitante(PMCtx *ctx,
                           void (*cb)(const Habitante *h, void *ud),
                           void *ud);

/*
 * pm_for_each_morador — itera sobre todos os endereços registrados.
 */
void pm_for_each_morador(PMCtx *ctx,
                         void (*cb)(const Endereco *e, void *ud),
                         void *ud);

/*
 * pm_moradores_da_quadra — conta moradores de uma quadra por face e total.
 * contagem[0]=N, [1]=S, [2]=L, [3]=O; retorna total.
 */
int pm_moradores_da_quadra(PMCtx *ctx, const char *cep, int contagem[4]);

/*
 * pm_dump — gera arquivos .hfd para pessoas e moradores.
 */
void pm_dump(PMCtx *ctx, const char *path_hfd_pessoas,
             const char *path_hfd_moradores);

#endif /* PM_H */