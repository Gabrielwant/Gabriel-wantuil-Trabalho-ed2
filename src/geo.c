/*
 * geo.c — Implementação do módulo de Quadras
 */

#include "geo.h"
#include "hashfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- serialização da Quadra ---------- */

static void quadra_serialize(const Quadra *q, char *buf, int buflen)
{
  snprintf(buf, buflen, "%s %.4f %.4f %.4f %.4f",
           q->cep, q->x, q->y, q->w, q->h);
}

static int quadra_deserialize(const char *buf, Quadra *q)
{
  return (sscanf(buf, "%31s %f %f %f %f",
                 q->cep, &q->x, &q->y, &q->w, &q->h) == 5)
             ? 0
             : -1;
}

/* ---------- estrutura interna (opaca) ---------- */

struct GeoCtx
{
  HFFile *hf;
  char hf_path[512];
};

/* ---------- API pública ---------- */

GeoCtx *geo_create(const char *path_hf)
{
  GeoCtx *ctx = calloc(1, sizeof(GeoCtx));
  if (!ctx)
    return NULL;
  strncpy(ctx->hf_path, path_hf, sizeof(ctx->hf_path) - 1);
  ctx->hf = hf_create(path_hf, 1);
  if (!ctx->hf)
  {
    free(ctx);
    return NULL;
  }
  return ctx;
}

GeoCtx *geo_open(const char *path_hf)
{
  GeoCtx *ctx = calloc(1, sizeof(GeoCtx));
  if (!ctx)
    return NULL;
  strncpy(ctx->hf_path, path_hf, sizeof(ctx->hf_path) - 1);
  ctx->hf = hf_open(path_hf);
  if (!ctx->hf)
  {
    free(ctx);
    return NULL;
  }
  return ctx;
}

void geo_free(GeoCtx *ctx)
{
  if (!ctx)
    return;
  hf_close(ctx->hf);
  free(ctx);
}

int geo_insert_quadra(GeoCtx *ctx, const Quadra *q)
{
  if (!ctx || !q)
    return -1;
  char buf[HF_MAX_DATA];
  quadra_serialize(q, buf, sizeof(buf));
  return hf_insert(ctx->hf, q->cep, buf);
}

int geo_remove_quadra(GeoCtx *ctx, const char *cep)
{
  if (!ctx || !cep)
    return -1;
  return hf_delete(ctx->hf, cep);
}

int geo_busca_quadra(GeoCtx *ctx, const char *cep, Quadra *q_out)
{
  if (!ctx || !cep || !q_out)
    return -1;
  char buf[HF_MAX_DATA];
  if (hf_search(ctx->hf, cep, buf) != 0)
    return -1;
  return quadra_deserialize(buf, q_out);
}

/*
 * svg_parse_cq_internal — lê "sw cfill cstrk" onde sw pode ter sufixo "px".
 */
static void parse_cq_line(const char *tok_sw, const char *cfill,
                          const char *cstrk, QuadraStyle *s)
{
  /* remove sufixo "px" se presente */
  char sw_buf[16];
  strncpy(sw_buf, tok_sw, sizeof(sw_buf) - 1);
  sw_buf[sizeof(sw_buf) - 1] = '\0';
  char *px = strstr(sw_buf, "px");
  if (px)
    *px = '\0';
  s->sw = (float)atof(sw_buf);
  strncpy(s->cfill, cfill, sizeof(s->cfill) - 1);
  s->cfill[sizeof(s->cfill) - 1] = '\0';
  strncpy(s->cstrk, cstrk, sizeof(s->cstrk) - 1);
  s->cstrk[sizeof(s->cstrk) - 1] = '\0';
}

int geo_parse_geo_file(GeoCtx *ctx, const char *path_geo,
                       QuadraStyle *style_out)
{
  if (!ctx || !path_geo)
    return -1;

  FILE *fp = fopen(path_geo, "r");
  if (!fp)
    return -1;

  /* estilo padrão */
  QuadraStyle cur_style;
  cur_style.sw = 1.5f;
  strncpy(cur_style.cfill, "#FFA040", sizeof(cur_style.cfill));
  strncpy(cur_style.cstrk, "#884400", sizeof(cur_style.cstrk));

  char line[512];
  int count = 0;

  while (fgets(line, sizeof(line), fp))
  {
    char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0')
      continue;

    char cmd[16];
    if (sscanf(p, "%15s", cmd) != 1)
      continue;

    if (strcmp(cmd, "q") == 0)
    {
      Quadra q;
      if (sscanf(p, "%*s %31s %f %f %f %f",
                 q.cep, &q.x, &q.y, &q.w, &q.h) == 5)
      {
        geo_insert_quadra(ctx, &q);
        count++;
      }
    }
    else if (strcmp(cmd, "cq") == 0)
    {
      /* "cq sw cfill cstrk" — sw pode ter sufixo "px" */
      char sw_tok[16], cfill[32], cstrk[32];
      if (sscanf(p, "%*s %15s %31s %31s", sw_tok, cfill, cstrk) == 3)
        parse_cq_line(sw_tok, cfill, cstrk, &cur_style);
    }
  }

  fclose(fp);

  if (style_out)
    *style_out = cur_style;

  return count;
}

/*
 * geo_anchor — canto SUDESTE da quadra.
 * ax = q.x + q.w (borda leste)
 * ay = q.y       (borda sul = topo no canvas SVG)
 */
void geo_anchor(const Quadra *q, float *ax, float *ay)
{
  if (!q || !ax || !ay)
    return;
  *ax = q->x + q->w;
  *ay = q->y;
}

int geo_endereco_para_xy(GeoCtx *ctx, const char *cep, char face,
                         int numero, float *px, float *py)
{
  Quadra q;
  if (geo_busca_quadra(ctx, cep, &q) != 0)
    return -1;

  float ax, ay;
  geo_anchor(&q, &ax, &ay);

  switch (face)
  {
  case FACE_S:
    *px = ax - (float)numero;
    *py = q.y;
    break;
  case FACE_N:
    *px = ax - (float)numero;
    *py = q.y + q.h;
    break;
  case FACE_L:
    *px = q.x + q.w;
    *py = ay + (float)numero;
    break;
  case FACE_O:
    *px = q.x;
    *py = ay + (float)numero;
    break;
  default:
    return -1;
  }
  return 0;
}

/* ---------- bounding box ---------- */

typedef struct
{
  float xmax;
  float ymax;
} BBoxCtx;

static void bbox_cb(const Quadra *q, void *ud)
{
  BBoxCtx *bb = (BBoxCtx *)ud;
  float x2 = q->x + q->w;
  float y2 = q->y + q->h;
  if (x2 > bb->xmax)
    bb->xmax = x2;
  if (y2 > bb->ymax)
    bb->ymax = y2;
}

void geo_bbox(GeoCtx *ctx, float *xmax, float *ymax)
{
  BBoxCtx bb = {0.0f, 0.0f};
  if (ctx)
    geo_for_each(ctx, bbox_cb, &bb);
  if (xmax)
    *xmax = bb.xmax;
  if (ymax)
    *ymax = bb.ymax;
}

void geo_dump(GeoCtx *ctx, const char *path_hfd)
{
  if (!ctx)
    return;
  hf_dump(ctx->hf, path_hfd);
}

/* ---------- iteração ---------- */

typedef struct
{
  void (*cb)(const Quadra *, void *);
  void *ud;
} GeoIterCtx;

static void geo_iter_cb(const char *key, const char *data, void *ud)
{
  (void)key;
  GeoIterCtx *ic = (GeoIterCtx *)ud;
  Quadra q;
  if (quadra_deserialize(data, &q) == 0)
    ic->cb(&q, ic->ud);
}

void geo_for_each(GeoCtx *ctx, void (*cb)(const Quadra *, void *), void *ud)
{
  if (!ctx || !cb)
    return;
  GeoIterCtx ic = {cb, ud};
  hf_for_each(ctx->hf, geo_iter_cb, &ic);
}