/*
 * main.c — Programa TED: Sistema de Informações Geográficas de Bitnópolis
 *
 * Uso:
 *   ted -f cidade.geo -o ./saida [-e ./entrada] [-pm pessoas.pm] [-q consultas.qry]
 *
 * Parâmetros:
 *   -e  diretório-base de entrada (BED), padrão: "."
 *   -f  arquivo .geo com o mapa da cidade (obrigatório)
 *   -o  diretório-base de saída (BSD) (obrigatório)
 *   -q  arquivo .qry com consultas
 *   -pm arquivo .pm com pessoas e moradores
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geo.h"
#include "pm.h"
#include "svg.h"
#include "qry.h"

/* ---------- utilitários de caminho ---------- */

static void build_path(char *out, int outsz,
                       const char *dir, const char *file)
{
  if (dir && dir[0])
    snprintf(out, outsz, "%s/%s", dir, file);
  else
  {
    strncpy(out, file, outsz - 1);
    out[outsz - 1] = '\0';
  }
}

/* extrai nome-base sem extensão: "c1.geo" → "c1" */
static void basename_noext(const char *filepath, char *out, int outsz)
{
  /* encontra último '/' ou '\\' */
  const char *base = filepath;
  const char *p;
  for (p = filepath; *p; p++)
    if (*p == '/' || *p == '\\')
      base = p + 1;

  strncpy(out, base, outsz - 1);
  out[outsz - 1] = '\0';

  /* remove extensão */
  char *dot = strrchr(out, '.');
  if (dot)
    *dot = '\0';
}

/* ---------- leitura de parâmetros ---------- */

typedef struct
{
  char bed[512];
  char bsd[512];
  char geo[256];
  char pm[256];
  char qry[256];
} Params;

static void params_init(Params *p)
{
  strncpy(p->bed, ".", sizeof(p->bed));
  p->bsd[0] = '\0';
  p->geo[0] = '\0';
  p->pm[0] = '\0';
  p->qry[0] = '\0';
}

static int params_parse(Params *p, int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-e") == 0 && i + 1 < argc)
      strncpy(p->bed, argv[++i], sizeof(p->bed) - 1);
    else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
      strncpy(p->geo, argv[++i], sizeof(p->geo) - 1);
    else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
      strncpy(p->bsd, argv[++i], sizeof(p->bsd) - 1);
    else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc)
      strncpy(p->qry, argv[++i], sizeof(p->qry) - 1);
    else if (strcmp(argv[i], "-pm") == 0 && i + 1 < argc)
      strncpy(p->pm, argv[++i], sizeof(p->pm) - 1);
    else
    {
      fprintf(stderr, "Parametro desconhecido: %s\n", argv[i]);
      return -1;
    }
  }
  if (p->geo[0] == '\0')
  {
    fprintf(stderr, "Erro: -f <arquivo.geo> e obrigatorio\n");
    return -1;
  }
  if (p->bsd[0] == '\0')
  {
    fprintf(stderr, "Erro: -o <diretorio-saida> e obrigatorio\n");
    return -1;
  }
  return 0;
}

/* ---------- main ---------- */

int main(int argc, char *argv[])
{
  Params p;
  params_init(&p);

  if (params_parse(&p, argc, argv) != 0)
  {
    fprintf(stderr,
            "Uso: %s -f cidade.geo -o ./saida [-e ./entrada]"
            " [-pm pessoas.pm] [-q consultas.qry]\n",
            argv[0]);
    return 1;
  }

  /* ---------- deriva nome-base para nomear saídas ---------- */

  char base[256];
  basename_noext(p.geo, base, sizeof(base));

  /* ---------- caminhos ---------- */

  char path_geo[768], path_pm_in[768];
  char path_hf_quadras[768], path_hf_pessoas[768], path_hf_moradores[768];
  char path_hfd_quadras[768], path_hfd_pessoas[768], path_hfd_moradores[768];
  char path_svg[768], path_txt[768], path_qry[768];

  build_path(path_geo, sizeof(path_geo), p.bed, p.geo);

  /* hashfiles e dumps nomeados com o base do .geo */
  {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s-quadras.hf", base);
    build_path(path_hf_quadras, sizeof(path_hf_quadras), p.bsd, tmp);
    snprintf(tmp, sizeof(tmp), "%s-quadras.hfd", base);
    build_path(path_hfd_quadras, sizeof(path_hfd_quadras), p.bsd, tmp);

    snprintf(tmp, sizeof(tmp), "%s-pessoas.hf", base);
    build_path(path_hf_pessoas, sizeof(path_hf_pessoas), p.bsd, tmp);
    snprintf(tmp, sizeof(tmp), "%s-pessoas.hfd", base);
    build_path(path_hfd_pessoas, sizeof(path_hfd_pessoas), p.bsd, tmp);

    snprintf(tmp, sizeof(tmp), "%s-moradores.hf", base);
    build_path(path_hf_moradores, sizeof(path_hf_moradores), p.bsd, tmp);
    snprintf(tmp, sizeof(tmp), "%s-moradores.hfd", base);
    build_path(path_hfd_moradores, sizeof(path_hfd_moradores), p.bsd, tmp);

    snprintf(tmp, sizeof(tmp), "%s.svg", base);
    build_path(path_svg, sizeof(path_svg), p.bsd, tmp);
    snprintf(tmp, sizeof(tmp), "%s.txt", base);
    build_path(path_txt, sizeof(path_txt), p.bsd, tmp);
  }

  /* ---------- inicializa módulos ---------- */

  GeoCtx *geo = geo_create(path_hf_quadras);
  if (!geo)
  {
    fprintf(stderr, "Erro ao criar hashfile de quadras\n");
    return 1;
  }

  PMCtx *pm = pm_create(path_hf_pessoas, path_hf_moradores);
  if (!pm)
  {
    fprintf(stderr, "Erro ao criar hashfiles de pessoas/moradores\n");
    geo_free(geo);
    return 1;
  }

  /* ---------- lê a cidade (.geo) — captura estilo ---------- */

  QuadraStyle geo_style;
  int n_quadras = geo_parse_geo_file(geo, path_geo, &geo_style);
  if (n_quadras < 0)
  {
    fprintf(stderr, "Erro ao abrir arquivo .geo: %s\n", path_geo);
    pm_free(pm);
    geo_free(geo);
    return 1;
  }
  fprintf(stdout, "Carregadas %d quadras de %s\n", n_quadras, path_geo);

  /* ---------- lê pessoas e moradores (.pm) ---------- */

  if (p.pm[0])
  {
    build_path(path_pm_in, sizeof(path_pm_in), p.bed, p.pm);
    int n_pm = pm_parse_pm_file(pm, path_pm_in);
    if (n_pm >= 0)
      fprintf(stdout, "Carregados %d registros de %s\n", n_pm, path_pm_in);
    else
      fprintf(stderr, "Aviso: nao foi possivel abrir %s\n", path_pm_in);
  }

  /* ---------- calcula viewBox pelo bounding box real ---------- */

  float xmax = 0, ymax = 0;
  geo_bbox(geo, &xmax, &ymax);
  float vw = xmax + 2 * 40.0f + 20.0f; /* margem dos dois lados + folga */
  float vh = ymax + 2 * 40.0f + 20.0f;

  /* ---------- cria saída SVG e TXT ---------- */

  SVGCtx *svg = svg_create(path_svg, vw, vh);
  if (!svg)
    fprintf(stderr, "Aviso: nao foi possivel criar SVG em %s\n", path_svg);

  FILE *txt_out = fopen(path_txt, "w");
  if (!txt_out)
  {
    fprintf(stderr, "Aviso: nao foi possivel criar relatorio em %s\n", path_txt);
    txt_out = stdout;
  }

  /* desenha quadras com o estilo lido do .geo */
  if (svg)
    svg_draw_quadras(svg, geo, &geo_style);

  /* ---------- executa consultas (.qry) ---------- */

  if (p.qry[0])
  {
    build_path(path_qry, sizeof(path_qry), p.bed, p.qry);
    QryCtx qry;
    qry_init(&qry, geo, pm, svg, txt_out);
    /* estilo inicial = o que veio do .geo */
    qry.style = geo_style;

    int n_cmds = qry_execute_file(&qry, path_qry);
    if (n_cmds >= 0)
      fprintf(stdout, "Executados %d comandos de %s\n", n_cmds, path_qry);
    else
      fprintf(stderr, "Aviso: nao foi possivel abrir %s\n", path_qry);
  }

  /* ---------- finaliza saídas ---------- */

  if (svg)
    svg_free(svg);
  if (txt_out && txt_out != stdout)
    fclose(txt_out);

  /* ---------- gera dumps .hfd ---------- */

  geo_dump(geo, path_hfd_quadras);
  pm_dump(pm, path_hfd_pessoas, path_hfd_moradores);

  fprintf(stdout, "Dumps gerados em %s, %s, %s\n",
          path_hfd_quadras, path_hfd_pessoas, path_hfd_moradores);

  /* ---------- libera recursos ---------- */

  pm_free(pm);
  geo_free(geo);

  return 0;
}