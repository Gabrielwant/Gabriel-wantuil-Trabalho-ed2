#ifndef SVG_H
#define SVG_H

/*
 * svg.h — Módulo de geração de saída SVG
 *
 * Gera o arquivo SVG do mapa de Bitnópolis com:
 *   - Quadras coloridas com bordas
 *   - Marcações de eventos (morte, mudança, despejo, remoção de quadra)
 *   - Contagem de moradores por face
 *
 * ATENÇÃO: QuadraStyle é definido em geo.h (usado também por qry.h).
 */

#include "geo.h"

/* Contexto SVG (opaco) */
typedef struct SVGCtx SVGCtx;

/*
 * svg_create — abre/cria o contexto SVG.
 *   path_out              : caminho do arquivo .svg de saída
 *   viewbox_w, viewbox_h  : dimensões do canvas (0,0 = calculado automaticamente)
 */
SVGCtx *svg_create(const char *path_out, float viewbox_w, float viewbox_h);

/*
 * svg_free — finaliza e salva o arquivo SVG, libera recursos.
 */
void svg_free(SVGCtx *ctx);

/*
 * svg_draw_quadras — desenha todas as quadras do mapa.
 *   style: estilo corrente; se NULL usa valores padrão.
 */
void svg_draw_quadras(SVGCtx *ctx, GeoCtx *geo, const QuadraStyle *style);

/*
 * svg_draw_quadra — desenha uma única quadra.
 */
void svg_draw_quadra(SVGCtx *ctx, const Quadra *q, const QuadraStyle *style);

/*
 * svg_mark_death — marca morte de morador no local do endereço (cruz vermelha).
 */
void svg_mark_death(SVGCtx *ctx, float x, float y);

/*
 * svg_mark_move — marca mudança de endereço (quadrado vermelho com CPF).
 */
void svg_mark_move(SVGCtx *ctx, float x, float y, const char *cpf);

/*
 * svg_mark_eviction — marca despejo (círculo preto).
 */
void svg_mark_eviction(SVGCtx *ctx, float x, float y);

/*
 * svg_mark_removed_block — marca remoção de quadra (X vermelho na âncora).
 */
void svg_mark_removed_block(SVGCtx *ctx, float ax, float ay);

/*
 * svg_draw_resident_counts — desenha contagem de moradores por face e total.
 *   contagem[0]=N, [1]=S, [2]=L, [3]=O
 */
void svg_draw_resident_counts(SVGCtx *ctx, const Quadra *q,
                              const int contagem[4], int total);

/*
 * svg_parse_cq — atualiza estilo a partir de linha de comando 'cq'.
 *   linha: string "sw cfill cstrk"  (sw pode ter sufixo "px")
 */
void svg_parse_cq(const char *linha, QuadraStyle *style);

#endif /* SVG_H */