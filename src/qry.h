#ifndef QRY_H
#define QRY_H

/*
 * qry.h — Módulo de Consultas e Atualizações (.qry)
 *
 * Interpreta e executa os comandos do arquivo .qry:
 *   rq    — remove quadra
 *   pq    — conta moradores da quadra
 *   censo — estatísticas gerais
 *   h?    — dados de um habitante
 *   nasc  — novo habitante
 *   rip   — habitante falece
 *   mud   — morador muda de endereço
 *   dspj  — morador é despejado
 */

#include "geo.h"
#include "pm.h"
#include "svg.h"

/* Contexto de execução do .qry */
typedef struct
{
  GeoCtx *geo;
  PMCtx *pm;
  SVGCtx *svg;       /* pode ser NULL se saída SVG não foi aberta */
  FILE *txt_out;     /* saída texto (pode ser stdout)              */
  QuadraStyle style; /* estilo corrente das quadras                */
} QryCtx;

/*
 * qry_init — inicializa contexto com os módulos já abertos.
 */
void qry_init(QryCtx *qry, GeoCtx *geo, PMCtx *pm,
              SVGCtx *svg, FILE *txt_out);

/*
 * qry_execute_file — lê e executa todos os comandos do arquivo .qry.
 * Retorna número de comandos executados, -1 em falha de abertura.
 */
int qry_execute_file(QryCtx *qry, const char *path_qry);

/*
 * Funções individuais de comando (úteis para testes unitários)
 */

/* rq cep — remove quadra */
void qry_cmd_rq(QryCtx *qry, const char *cep);

/* pq cep — conta moradores da quadra */
void qry_cmd_pq(QryCtx *qry, const char *cep);

/* censo — estatísticas gerais */
void qry_cmd_censo(QryCtx *qry);

/* h? cpf — dados do habitante */
void qry_cmd_h(QryCtx *qry, const char *cpf);

/* nasc cpf nome sobrenome sexo nasc — novo habitante */
void qry_cmd_nasc(QryCtx *qry, const char *cpf, const char *nome,
                  const char *sobrenome, char sexo, const char *nasc);

/* rip cpf — habitante falece */
void qry_cmd_rip(QryCtx *qry, const char *cpf);

/* mud cpf cep face num compl — morador muda */
void qry_cmd_mud(QryCtx *qry, const char *cpf, const char *cep,
                 char face, int num, const char *compl);

/* dspj cpf — morador despejado */
void qry_cmd_dspj(QryCtx *qry, const char *cpf);

#endif /* QRY_H */