#ifndef GEO_H
#define GEO_H

/*
 * geo.h — Módulo de Quadras (blocos da cidade)
 *
 * Gerencia o mapa da cidade: quadras georreferenciadas por CEP,
 * inserção/remoção via hashfile dinâmico, cálculo de endereço.
 *
 * Sistema de coordenadas: origem (0,0) no canto superior esquerdo,
 * eixo Y cresce para baixo (padrão SVG).
 *
 * No mapa de Bitnópolis o eixo Norte aponta para baixo (y crescente),
 * de modo que as faces ficam:
 *
 *      (x,y) ──── face S ──── (x+w, y)
 *        |                       |
 *      face O               face L
 *        |                       |
 *      (x,y+h) ── face N ── (x+w, y+h)
 *
 * Ponto de ancoragem: canto SUDESTE = face S ∩ face L = (x+w, y).
 * Número de uma casa = distância da frente até a projeção da âncora.
 */

#include "hashfile.h"

#define CEP_MAX 32 /* tamanho máximo do CEP */

/* Faces da quadra */
#define FACE_N 'N' /* face inferior  (y = q.y + q.h)  */
#define FACE_S 'S' /* face superior  (y = q.y)         */
#define FACE_L 'L' /* face direita   (x = q.x + q.w)  */
#define FACE_O 'O' /* face esquerda  (x = q.x)         */

/* Representação de uma quadra */
typedef struct
{
  char cep[CEP_MAX]; /* identificador da quadra              */
  float x, y;        /* posição do canto noroeste (top-left) */
  float w, h;        /* largura e altura                     */
} Quadra;

/*
 * Estilo de desenho das quadras — atualizado pelo comando 'cq'.
 * Definido aqui pois é compartilhado entre geo, svg e qry.
 */
typedef struct
{
  float sw;       /* stroke-width da borda               */
  char cfill[32]; /* cor de preenchimento (ex: "#FFA500") */
  char cstrk[32]; /* cor da borda                         */
} QuadraStyle;

/* Contexto do módulo geo (opaco — struct completa em geo.c) */
typedef struct GeoCtx GeoCtx;

/*
 * geo_create — inicializa o contexto e cria o hashfile de quadras.
 *   path_hf: caminho do arquivo .hf a criar.
 * Retorna NULL em falha.
 */
GeoCtx *geo_create(const char *path_hf);

/*
 * geo_open — abre contexto sobre hashfile existente.
 */
GeoCtx *geo_open(const char *path_hf);

/*
 * geo_free — libera recursos do contexto.
 */
void geo_free(GeoCtx *ctx);

/*
 * geo_insert_quadra — insere quadra no mapa.
 * Retorna 0 em sucesso, -1 em falha.
 */
int geo_insert_quadra(GeoCtx *ctx, const Quadra *q);

/*
 * geo_remove_quadra — remove quadra pelo CEP.
 * Retorna 0 em sucesso, -1 se não encontrada.
 */
int geo_remove_quadra(GeoCtx *ctx, const char *cep);

/*
 * geo_busca_quadra — busca quadra pelo CEP.
 * Preenche q_out se encontrada.
 * Retorna 0 em sucesso, -1 se não encontrada.
 */
int geo_busca_quadra(GeoCtx *ctx, const char *cep, Quadra *q_out);

/*
 * geo_parse_geo_file — lê arquivo .geo e insere quadras.
 * Interpreta comandos 'q' (quadra) e 'cq' (estilo).
 * Preenche *style_out com o último estilo lido (se não NULL).
 * Retorna número de quadras inseridas, -1 em falha de abertura.
 */
int geo_parse_geo_file(GeoCtx *ctx, const char *path_geo,
                       QuadraStyle *style_out);

/*
 * geo_anchor — retorna o ponto de ancoragem (canto sudeste) de uma quadra.
 *   ax = q.x + q.w  (borda leste / direita)
 *   ay = q.y        (borda sul   / superior no mapa SVG)
 */
void geo_anchor(const Quadra *q, float *ax, float *ay);

/*
 * geo_endereco_para_xy — converte CEP/face/numero em coordenada (x,y).
 * Preenche px, py com o ponto no mapa.
 * Retorna 0 em sucesso, -1 se quadra não encontrada ou face inválida.
 */
int geo_endereco_para_xy(GeoCtx *ctx, const char *cep, char face,
                         int numero, float *px, float *py);

/*
 * geo_bbox — calcula bounding box de todas as quadras.
 * Preenche xmax, ymax com os maiores valores encontrados.
 */
void geo_bbox(GeoCtx *ctx, float *xmax, float *ymax);

/*
 * geo_dump — gera arquivo .hfd das quadras.
 */
void geo_dump(GeoCtx *ctx, const char *path_hfd);

/*
 * geo_for_each — itera sobre todas as quadras.
 */
void geo_for_each(GeoCtx *ctx,
                  void (*cb)(const Quadra *q, void *ud),
                  void *ud);

#endif /* GEO_H */