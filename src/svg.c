/*
 * svg.c — Implementação do módulo de geração SVG
 */

#include "svg.h"
#include "geo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SVG_MARGIN 40.0f

/* ---------- estrutura interna ---------- */

struct SVGCtx
{
  FILE *fp;
  float vw, vh;
};

/* ---------- mapeamento de coordenadas ---------- */
/* Coordenadas do mapa são usadas diretamente + margem */

static float sx(float x) { return SVG_MARGIN + x; }
static float sy(float y) { return SVG_MARGIN + y; }

/* ---------- API pública ---------- */

SVGCtx *svg_create(const char *path_out, float viewbox_w, float viewbox_h)
{
  SVGCtx *ctx = calloc(1, sizeof(SVGCtx));
  if (!ctx)
    return NULL;

  ctx->fp = fopen(path_out, "w");
  if (!ctx->fp)
  {
    free(ctx);
    return NULL;
  }

  ctx->vw = (viewbox_w > 0) ? viewbox_w : 800.0f;
  ctx->vh = (viewbox_h > 0) ? viewbox_h : 600.0f;

  /* O cabeçalho SVG é escrito por svg_finalize (chamada em svg_free),
   * pois o viewBox só é definitivo após saber o tamanho do mapa.
   * Aqui escrevemos uma versão provisória que será substituída. */
  fprintf(ctx->fp,
          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<svg xmlns=\"http://www.w3.org/2000/svg\""
          " width=\"%.0f\" height=\"%.0f\""
          " viewBox=\"0 0 %.0f %.0f\">\n",
          ctx->vw, ctx->vh, ctx->vw, ctx->vh);

  /* fundo */
  fprintf(ctx->fp,
          "  <rect width=\"%.0f\" height=\"%.0f\" fill=\"#f8f8f4\"/>\n",
          ctx->vw, ctx->vh);

  /* título */
  fprintf(ctx->fp,
          "  <text x=\"%.0f\" y=\"24\" font-family=\"monospace\""
          " font-size=\"14\" fill=\"#333\">Bitnopolis</text>\n",
          SVG_MARGIN);

  return ctx;
}

void svg_free(SVGCtx *ctx)
{
  if (!ctx)
    return;
  fprintf(ctx->fp, "</svg>\n");
  fclose(ctx->fp);
  free(ctx);
}

/* ---------- desenho de quadras ---------- */

static void default_style(QuadraStyle *s)
{
  s->sw = 1.5f;
  strncpy(s->cfill, "#FFA040", sizeof(s->cfill));
  strncpy(s->cstrk, "#884400", sizeof(s->cstrk));
}

void svg_draw_quadra(SVGCtx *ctx, const Quadra *q, const QuadraStyle *style)
{
  if (!ctx || !q)
    return;

  QuadraStyle s;
  if (!style)
    default_style(&s);
  else
    s = *style;

  float x = sx(q->x);
  float y = sy(q->y);
  float w = q->w;
  float h = q->h;

  /* retângulo da quadra */
  fprintf(ctx->fp,
          "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
          " fill=\"%s\" stroke=\"%s\" stroke-width=\"%.2f\"/>\n",
          x, y, w, h, s.cfill, s.cstrk, s.sw);

  /* CEP centralizado */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"9\" fill=\"#222\" text-anchor=\"middle\""
          " dominant-baseline=\"middle\">%s</text>\n",
          x + w / 2.0f, y + h / 2.0f, q->cep);

  /* Labels das faces */
  float mid_x = x + w / 2.0f;
  float mid_y = y + h / 2.0f;

  /* S — face de cima (topo) */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"7\" fill=\"#444\" text-anchor=\"middle\">S</text>\n",
          mid_x, y + 9.0f);
  /* N — face de baixo */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"7\" fill=\"#444\" text-anchor=\"middle\">N</text>\n",
          mid_x, y + h - 2.0f);
  /* O — face esquerda */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"7\" fill=\"#444\" text-anchor=\"middle\">O</text>\n",
          x + 7.0f, mid_y);
  /* L — face direita */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"7\" fill=\"#444\" text-anchor=\"middle\">L</text>\n",
          x + w - 7.0f, mid_y);
}

typedef struct
{
  SVGCtx *ctx;
  const QuadraStyle *style;
} DrawCtx;

static void draw_cb(const Quadra *q, void *ud)
{
  DrawCtx *dc = (DrawCtx *)ud;
  svg_draw_quadra(dc->ctx, q, dc->style);
}

void svg_draw_quadras(SVGCtx *ctx, GeoCtx *geo, const QuadraStyle *style)
{
  if (!ctx || !geo)
    return;
  DrawCtx dc = {ctx, style};
  geo_for_each(geo, draw_cb, &dc);
}

/* ---------- marcações de eventos ---------- */

void svg_mark_death(SVGCtx *ctx, float x, float y)
{
  if (!ctx)
    return;
  float px = sx(x), py = sy(y);
  float r = 5.0f;
  fprintf(ctx->fp,
          "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
          " stroke=\"red\" stroke-width=\"2\"/>\n",
          px - r, py - r, px + r, py + r);
  fprintf(ctx->fp,
          "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
          " stroke=\"red\" stroke-width=\"2\"/>\n",
          px + r, py - r, px - r, py + r);
}

void svg_mark_move(SVGCtx *ctx, float x, float y, const char *cpf)
{
  if (!ctx)
    return;
  float px = sx(x), py = sy(y);
  float sz = 14.0f;
  fprintf(ctx->fp,
          "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\""
          " fill=\"none\" stroke=\"red\" stroke-width=\"1.5\"/>\n",
          px - sz / 2.0f, py - sz / 2.0f, sz, sz);
  if (cpf)
  {
    fprintf(ctx->fp,
            "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
            " font-size=\"4\" fill=\"red\" text-anchor=\"middle\""
            " dominant-baseline=\"middle\">%s</text>\n",
            px, py, cpf);
  }
}

void svg_mark_eviction(SVGCtx *ctx, float x, float y)
{
  if (!ctx)
    return;
  float px = sx(x), py = sy(y);
  fprintf(ctx->fp,
          "  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"5\""
          " fill=\"black\" opacity=\"0.7\"/>\n",
          px, py);
}

void svg_mark_removed_block(SVGCtx *ctx, float ax, float ay)
{
  if (!ctx)
    return;
  float px = sx(ax), py = sy(ay);
  float r = 7.0f;
  fprintf(ctx->fp,
          "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
          " stroke=\"red\" stroke-width=\"2.5\"/>\n",
          px - r, py - r, px + r, py + r);
  fprintf(ctx->fp,
          "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\""
          " stroke=\"red\" stroke-width=\"2.5\"/>\n",
          px + r, py - r, px - r, py + r);
}

void svg_draw_resident_counts(SVGCtx *ctx, const Quadra *q,
                              const int contagem[4], int total)
{
  if (!ctx || !q)
    return;

  float x = sx(q->x);
  float y = sy(q->y);
  float w = q->w;
  float h = q->h;

  /* S — acima do retângulo (face topo) */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"9\" fill=\"#00008B\" text-anchor=\"middle\">%d</text>\n",
          x + w / 2.0f, y - 4.0f, contagem[1]);
  /* N — abaixo */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"9\" fill=\"#00008B\" text-anchor=\"middle\">%d</text>\n",
          x + w / 2.0f, y + h + 12.0f, contagem[0]);
  /* O — à esquerda */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"9\" fill=\"#00008B\" text-anchor=\"end\">%d</text>\n",
          x - 4.0f, y + h / 2.0f + 3.0f, contagem[3]);
  /* L — à direita */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"9\" fill=\"#00008B\" text-anchor=\"start\">%d</text>\n",
          x + w + 4.0f, y + h / 2.0f + 3.0f, contagem[2]);
  /* total — no centro */
  fprintf(ctx->fp,
          "  <text x=\"%.2f\" y=\"%.2f\" font-family=\"monospace\""
          " font-size=\"11\" font-weight=\"bold\" fill=\"#8B0000\""
          " text-anchor=\"middle\" dominant-baseline=\"middle\">%d</text>\n",
          x + w / 2.0f, y + h / 2.0f + 9.0f, total);
}

/* ---------- parse do comando cq ---------- */

void svg_parse_cq(const char *linha, QuadraStyle *style)
{
  if (!linha || !style)
    return;
  char sw_tok[16], cfill[32], cstrk[32];
  if (sscanf(linha, "%15s %31s %31s", sw_tok, cfill, cstrk) == 3)
  {
    /* remove sufixo "px" se presente */
    char *px_ptr = strstr(sw_tok, "px");
    if (px_ptr)
      *px_ptr = '\0';
    style->sw = (float)atof(sw_tok);
    strncpy(style->cfill, cfill, sizeof(style->cfill) - 1);
    style->cfill[sizeof(style->cfill) - 1] = '\0';
    strncpy(style->cstrk, cstrk, sizeof(style->cstrk) - 1);
    style->cstrk[sizeof(style->cstrk) - 1] = '\0';
  }
}