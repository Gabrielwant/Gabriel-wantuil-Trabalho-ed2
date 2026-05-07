/*
 * qry.c — Implementação dos comandos de consulta/atualização
 */

#include "qry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- helpers ---------- */

static void print_habitante(FILE *out, const Habitante *h)
{
  fprintf(out, "CPF: %s | Nome: %s %s | Sexo: %c | Nasc: %s | Status: %s\n",
          h->cpf, h->nome, h->sobrenome, h->sexo, h->nasc,
          h->vivo ? "vivo" : "falecido");
}

static void print_endereco(FILE *out, const Endereco *e)
{
  fprintf(out, "Endereco: %s/%c/%d %s\n",
          e->cep, e->face, e->numero, e->compl);
}

/*
 * parse_face — extrai o caractere de face de strings como "L", "Face.L", "face.S", etc.
 */
static char parse_face(const char *s)
{
  if (!s || !s[0])
    return '\0';
  /* procura N, S, L, O em maiúsculo */
  for (int i = 0; s[i]; i++)
  {
    char c = s[i];
    if (c == 'N' || c == 'S' || c == 'L' || c == 'O')
      return c;
  }
  return '\0';
}

/* ---------- API pública ---------- */

void qry_init(QryCtx *qry, GeoCtx *geo, PMCtx *pm,
              SVGCtx *svg, FILE *txt_out)
{
  qry->geo = geo;
  qry->pm = pm;
  qry->svg = svg;
  qry->txt_out = txt_out ? txt_out : stdout;
  /* estilo padrão */
  qry->style.sw = 1.5f;
  strncpy(qry->style.cfill, "#FFA040", sizeof(qry->style.cfill));
  strncpy(qry->style.cstrk, "#884400", sizeof(qry->style.cstrk));
}

/* ---------- rq — remove quadra ---------- */

typedef struct
{
  PMCtx *pm;
  FILE *out;
  const char *cep;
} RqCtx;

static void rq_cb(const Endereco *e, void *ud)
{
  RqCtx *rc = (RqCtx *)ud;
  if (strncmp(e->cep, rc->cep, CEP_MAX) != 0)
    return;
  pm_remover_endereco(rc->pm, e->cpf);
  Habitante h;
  if (pm_buscar_habitante(rc->pm, e->cpf, &h) == 0)
    fprintf(rc->out, "  sem-teto: %s %s %s\n",
            e->cpf, h.nome, h.sobrenome);
}

void qry_cmd_rq(QryCtx *qry, const char *cep)
{
  Quadra q;
  if (geo_busca_quadra(qry->geo, cep, &q) != 0)
  {
    fprintf(qry->txt_out, "rq: quadra %s nao encontrada\n", cep);
    return;
  }

  fprintf(qry->txt_out, "rq %s: moradores despejados:\n", cep);
  RqCtx rc = {qry->pm, qry->txt_out, cep};
  pm_for_each_morador(qry->pm, rq_cb, &rc);

  if (qry->svg)
  {
    float ax, ay;
    geo_anchor(&q, &ax, &ay);
    svg_mark_removed_block(qry->svg, ax, ay);
  }

  geo_remove_quadra(qry->geo, cep);
  fprintf(qry->txt_out, "rq %s: quadra removida\n", cep);
}

/* ---------- pq — conta moradores ---------- */

void qry_cmd_pq(QryCtx *qry, const char *cep)
{
  Quadra q;
  if (geo_busca_quadra(qry->geo, cep, &q) != 0)
  {
    fprintf(qry->txt_out, "pq: quadra %s nao encontrada\n", cep);
    return;
  }

  int cont[4];
  int total = pm_moradores_da_quadra(qry->pm, cep, cont);

  fprintf(qry->txt_out,
          "pq %s: N=%d S=%d L=%d O=%d total=%d\n",
          cep, cont[0], cont[1], cont[2], cont[3], total);

  if (qry->svg)
    svg_draw_resident_counts(qry->svg, &q, cont, total);
}

/* ---------- censo ---------- */

typedef struct
{
  int total_hab;
  int total_mor;
  int homens;
  int mulheres;
  int homens_mor;
  int mulheres_mor;
  PMCtx *pm;
} CensoCtx;

static void censo_hab_cb(const Habitante *h, void *ud)
{
  CensoCtx *cc = (CensoCtx *)ud;
  cc->total_hab++;
  if (h->sexo == 'M')
    cc->homens++;
  else
    cc->mulheres++;
  if (pm_eh_morador(cc->pm, h->cpf))
  {
    cc->total_mor++;
    if (h->sexo == 'M')
      cc->homens_mor++;
    else
      cc->mulheres_mor++;
  }
}

void qry_cmd_censo(QryCtx *qry)
{
  CensoCtx cc;
  memset(&cc, 0, sizeof(cc));
  cc.pm = qry->pm;
  pm_for_each_habitante(qry->pm, censo_hab_cb, &cc);

  int sem_teto = cc.total_hab - cc.total_mor;
  int sem_teto_m = cc.homens - cc.homens_mor;
  int sem_teto_f = cc.mulheres - cc.mulheres_mor;
  float prop_mor = (cc.total_hab > 0) ? 100.0f * cc.total_mor / cc.total_hab : 0.0f;
  float pct_hom = (cc.total_hab > 0) ? 100.0f * cc.homens / cc.total_hab : 0.0f;
  float pct_mul = (cc.total_hab > 0) ? 100.0f * cc.mulheres / cc.total_hab : 0.0f;
  float pct_hmor = (cc.total_mor > 0) ? 100.0f * cc.homens_mor / cc.total_mor : 0.0f;
  float pct_fmor = (cc.total_mor > 0) ? 100.0f * cc.mulheres_mor / cc.total_mor : 0.0f;
  float pct_hst = (sem_teto > 0) ? 100.0f * sem_teto_m / sem_teto : 0.0f;
  float pct_fst = (sem_teto > 0) ? 100.0f * sem_teto_f / sem_teto : 0.0f;

  FILE *out = qry->txt_out;
  fprintf(out, "=== CENSO DE BITNOPOLIS ===\n");
  fprintf(out, "Total de habitantes  : %d\n", cc.total_hab);
  fprintf(out, "Total de moradores   : %d\n", cc.total_mor);
  fprintf(out, "Proporcao mor/hab    : %.1f%%\n", prop_mor);
  fprintf(out, "Homens               : %d (%.1f%%)\n", cc.homens, pct_hom);
  fprintf(out, "Mulheres             : %d (%.1f%%)\n", cc.mulheres, pct_mul);
  fprintf(out, "Moradores homens     : %d (%.1f%% dos moradores)\n", cc.homens_mor, pct_hmor);
  fprintf(out, "Moradores mulheres   : %d (%.1f%% dos moradores)\n", cc.mulheres_mor, pct_fmor);
  fprintf(out, "Sem-teto             : %d\n", sem_teto);
  fprintf(out, "Sem-teto homens      : %.1f%%\n", pct_hst);
  fprintf(out, "Sem-teto mulheres    : %.1f%%\n", pct_fst);
  fprintf(out, "===========================\n");
}

/* ---------- h? — dados do habitante ---------- */

void qry_cmd_h(QryCtx *qry, const char *cpf)
{
  Habitante h;
  if (pm_buscar_habitante(qry->pm, cpf, &h) != 0)
  {
    fprintf(qry->txt_out, "h?: habitante %s nao encontrado\n", cpf);
    return;
  }
  print_habitante(qry->txt_out, &h);
  if (pm_eh_morador(qry->pm, cpf))
  {
    Endereco e;
    pm_buscar_endereco(qry->pm, cpf, &e);
    print_endereco(qry->txt_out, &e);
  }
  else
    fprintf(qry->txt_out, "  (sem-teto)\n");
}

/* ---------- nasc — novo habitante ---------- */

void qry_cmd_nasc(QryCtx *qry, const char *cpf, const char *nome,
                  const char *sobrenome, char sexo, const char *nasc)
{
  Habitante h;
  memset(&h, 0, sizeof(h));
  strncpy(h.cpf, cpf, CPF_MAX - 1);
  strncpy(h.nome, nome, NOME_MAX - 1);
  strncpy(h.sobrenome, sobrenome, NOME_MAX - 1);
  h.sexo = sexo;
  strncpy(h.nasc, nasc, DATA_MAX - 1);
  h.vivo = 1;

  if (pm_inserir_habitante(qry->pm, &h) == 0)
    fprintf(qry->txt_out, "nasc: %s %s %s cadastrado\n", cpf, nome, sobrenome);
  else
    fprintf(qry->txt_out, "nasc: erro ao cadastrar %s\n", cpf);
}

/* ---------- rip — habitante falece ---------- */

void qry_cmd_rip(QryCtx *qry, const char *cpf)
{
  Habitante h;
  if (pm_buscar_habitante(qry->pm, cpf, &h) != 0)
  {
    fprintf(qry->txt_out, "rip: habitante %s nao encontrado\n", cpf);
    return;
  }

  Endereco e;
  int eh_mor = pm_eh_morador(qry->pm, cpf);
  float px = 0, py = 0;
  if (eh_mor)
  {
    pm_buscar_endereco(qry->pm, cpf, &e);
    geo_endereco_para_xy(qry->geo, e.cep, e.face, e.numero, &px, &py);
  }

  fprintf(qry->txt_out, "rip: ");
  print_habitante(qry->txt_out, &h);
  if (eh_mor)
  {
    print_endereco(qry->txt_out, &e);
    if (qry->svg)
      svg_mark_death(qry->svg, px, py);
    pm_remover_endereco(qry->pm, cpf);
  }

  h.vivo = 0;
  pm_inserir_habitante(qry->pm, &h);
}

/* ---------- mud — morador muda ---------- */

void qry_cmd_mud(QryCtx *qry, const char *cpf, const char *cep,
                 char face, int num, const char *compl)
{
  if (!pm_eh_morador(qry->pm, cpf))
  {
    fprintf(qry->txt_out, "mud: %s nao eh morador\n", cpf);
    return;
  }

  float px = 0, py = 0;
  geo_endereco_para_xy(qry->geo, cep, face, num, &px, &py);

  Endereco e;
  memset(&e, 0, sizeof(e));
  strncpy(e.cpf, cpf, CPF_MAX - 1);
  strncpy(e.cep, cep, CEP_MAX - 1);
  e.face = face;
  e.numero = num;
  if (compl)
    strncpy(e.compl, compl, COMPL_MAX - 1);

  pm_registrar_endereco(qry->pm, &e);

  fprintf(qry->txt_out, "mud: %s -> %s/%c/%d %s\n",
          cpf, cep, face, num, compl ? compl : "");

  if (qry->svg)
    svg_mark_move(qry->svg, px, py, cpf);
}

/* ---------- dspj — despejo ---------- */

void qry_cmd_dspj(QryCtx *qry, const char *cpf)
{
  Habitante h;
  if (pm_buscar_habitante(qry->pm, cpf, &h) != 0)
  {
    fprintf(qry->txt_out, "dspj: habitante %s nao encontrado\n", cpf);
    return;
  }
  if (!pm_eh_morador(qry->pm, cpf))
  {
    fprintf(qry->txt_out, "dspj: %s nao eh morador\n", cpf);
    return;
  }

  Endereco e;
  pm_buscar_endereco(qry->pm, cpf, &e);
  float px = 0, py = 0;
  geo_endereco_para_xy(qry->geo, e.cep, e.face, e.numero, &px, &py);

  fprintf(qry->txt_out, "dspj: ");
  print_habitante(qry->txt_out, &h);
  print_endereco(qry->txt_out, &e);

  pm_remover_endereco(qry->pm, cpf);

  if (qry->svg)
    svg_mark_eviction(qry->svg, px, py);
}

/* ---------- execução do arquivo .qry ---------- */

int qry_execute_file(QryCtx *qry, const char *path_qry)
{
  if (!qry || !path_qry)
    return -1;

  FILE *fp = fopen(path_qry, "r");
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

    char cmd[16];
    if (sscanf(p, "%15s", cmd) != 1)
      continue;

    if (strcmp(cmd, "rq") == 0)
    {
      char cep[CEP_MAX];
      if (sscanf(p, "%*s %31s", cep) == 1)
      {
        qry_cmd_rq(qry, cep);
        count++;
      }
    }
    else if (strcmp(cmd, "pq") == 0)
    {
      char cep[CEP_MAX];
      if (sscanf(p, "%*s %31s", cep) == 1)
      {
        qry_cmd_pq(qry, cep);
        count++;
      }
    }
    else if (strcmp(cmd, "censo") == 0)
    {
      qry_cmd_censo(qry);
      count++;
    }
    else if (strcmp(cmd, "h?") == 0)
    {
      char cpf[CPF_MAX];
      if (sscanf(p, "%*s %15s", cpf) == 1)
      {
        qry_cmd_h(qry, cpf);
        count++;
      }
    }
    else if (strcmp(cmd, "nasc") == 0)
    {
      char cpf[CPF_MAX], nome[NOME_MAX], sob[NOME_MAX], nasc[DATA_MAX];
      char sexo_str[4];
      if (sscanf(p, "%*s %15s %63s %63s %3s %11s",
                 cpf, nome, sob, sexo_str, nasc) == 5)
      {
        qry_cmd_nasc(qry, cpf, nome, sob, sexo_str[0], nasc);
        count++;
      }
    }
    else if (strcmp(cmd, "rip") == 0)
    {
      char cpf[CPF_MAX];
      if (sscanf(p, "%*s %15s", cpf) == 1)
      {
        qry_cmd_rip(qry, cpf);
        count++;
      }
    }
    else if (strcmp(cmd, "mud") == 0)
    {
      char cpf[CPF_MAX], cep[CEP_MAX], face_s[16], compl[COMPL_MAX];
      int num;
      compl[0] = '\0';
      if (sscanf(p, "%*s %15s %31s %15s %d %31s",
                 cpf, cep, face_s, &num, compl) >= 4)
      {
        char face = parse_face(face_s); /* aceita "L", "Face.L", etc. */
        if (face)
        {
          qry_cmd_mud(qry, cpf, cep, face, num, compl);
          count++;
        }
        else
          fprintf(qry->txt_out, "mud: face invalida '%s'\n", face_s);
      }
    }
    else if (strcmp(cmd, "dspj") == 0)
    {
      char cpf[CPF_MAX];
      if (sscanf(p, "%*s %15s", cpf) == 1)
      {
        qry_cmd_dspj(qry, cpf);
        count++;
      }
    }
    else if (strcmp(cmd, "cq") == 0)
    {
      /* atualiza estilo das quadras para SVG */
      svg_parse_cq(p + 3, &qry->style);
    }
  }

  fclose(fp);
  return count;
}