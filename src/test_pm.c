/*
 * test_pm.c — Testes unitários do módulo pm
 */

#include "unity.h"
#include "pm.h"
#include <stdio.h>
#include <string.h>

#define HF_P "/tmp/test_pm_pessoas.hf"
#define HF_M "/tmp/test_pm_moradores.hf"

static void cleanup(void)
{
  remove(HF_P);
  remove(HF_M);
}

static PMCtx *mk(void) { return pm_create(HF_P, HF_M); }

void setUp(void) {}
void tearDown(void) {}

static void test_create(void)
{
  cleanup();
  PMCtx *ctx = mk();
  TEST_ASSERT_NOT_NULL(ctx);
  pm_free(ctx);
  cleanup();
}

static void test_inserir_buscar(void)
{
  cleanup();
  PMCtx *ctx = mk();
  TEST_ASSERT_NOT_NULL(ctx);

  Habitante h;
  memset(&h, 0, sizeof(h));
  strncpy(h.cpf, "111.111.111-11", CPF_MAX - 1);
  strncpy(h.nome, "Joao", NOME_MAX - 1);
  strncpy(h.sobrenome, "Silva", NOME_MAX - 1);
  h.sexo = 'M';
  strncpy(h.nasc, "01/01/1990", DATA_MAX - 1);
  h.vivo = 1;

  TEST_ASSERT_EQUAL_INT(0, pm_inserir_habitante(ctx, &h));

  Habitante r;
  TEST_ASSERT_EQUAL_INT(0, pm_buscar_habitante(ctx, "111.111.111-11", &r));
  TEST_ASSERT_EQUAL_STRING("Joao", r.nome);
  TEST_ASSERT_EQUAL_STRING("Silva", r.sobrenome);
  TEST_ASSERT_EQUAL_INT(1, r.vivo);

  pm_free(ctx);
  cleanup();
}

static void test_morador(void)
{
  cleanup();
  PMCtx *ctx = mk();
  TEST_ASSERT_NOT_NULL(ctx);

  Habitante h;
  memset(&h, 0, sizeof(h));
  strncpy(h.cpf, "222.222.222-22", CPF_MAX - 1);
  strncpy(h.nome, "Maria", NOME_MAX - 1);
  strncpy(h.sobrenome, "Santos", NOME_MAX - 1);
  h.sexo = 'F';
  strncpy(h.nasc, "15/06/1985", DATA_MAX - 1);
  h.vivo = 1;
  pm_inserir_habitante(ctx, &h);

  TEST_ASSERT_FALSE(pm_eh_morador(ctx, "222.222.222-22"));

  Endereco e;
  memset(&e, 0, sizeof(e));
  strncpy(e.cpf, "222.222.222-22", CPF_MAX - 1);
  strncpy(e.cep, "cep01", CEP_MAX - 1);
  e.face = 'S';
  e.numero = 45;
  strncpy(e.compl, "apto3", COMPL_MAX - 1);

  TEST_ASSERT_EQUAL_INT(0, pm_registrar_endereco(ctx, &e));
  TEST_ASSERT_TRUE(pm_eh_morador(ctx, "222.222.222-22"));

  Endereco er;
  TEST_ASSERT_EQUAL_INT(0, pm_buscar_endereco(ctx, "222.222.222-22", &er));
  TEST_ASSERT_EQUAL_STRING("cep01", er.cep);
  TEST_ASSERT_EQUAL_INT(45, er.numero);

  pm_free(ctx);
  cleanup();
}

static void test_remover_endereco(void)
{
  cleanup();
  PMCtx *ctx = mk();
  TEST_ASSERT_NOT_NULL(ctx);

  Endereco e;
  memset(&e, 0, sizeof(e));
  strncpy(e.cpf, "333.333.333-33", CPF_MAX - 1);
  strncpy(e.cep, "cep02", CEP_MAX - 1);
  e.face = 'N';
  e.numero = 10;

  pm_registrar_endereco(ctx, &e);
  TEST_ASSERT_TRUE(pm_eh_morador(ctx, "333.333.333-33"));

  pm_remover_endereco(ctx, "333.333.333-33");
  TEST_ASSERT_FALSE(pm_eh_morador(ctx, "333.333.333-33"));

  pm_free(ctx);
  cleanup();
}

static void test_moradores_da_quadra(void)
{
  cleanup();
  PMCtx *ctx = mk();
  TEST_ASSERT_NOT_NULL(ctx);

  /* 3 moradores na mesma quadra */
  const char *cpfs[] = {"aaa", "bbb", "ccc"};
  const char faces[] = {'N', 'N', 'S'};

  for (int i = 0; i < 3; i++)
  {
    Habitante h;
    memset(&h, 0, sizeof(h));
    strncpy(h.cpf, cpfs[i], CPF_MAX - 1);
    h.vivo = 1;
    pm_inserir_habitante(ctx, &h);

    Endereco e;
    memset(&e, 0, sizeof(e));
    strncpy(e.cpf, cpfs[i], CPF_MAX - 1);
    strncpy(e.cep, "cepX", CEP_MAX - 1);
    e.face = faces[i];
    e.numero = 10;
    pm_registrar_endereco(ctx, &e);
  }

  int cont[4];
  int total = pm_moradores_da_quadra(ctx, "cepX", cont);
  TEST_ASSERT_EQUAL_INT(3, total);
  TEST_ASSERT_EQUAL_INT(2, cont[0]); /* N */
  TEST_ASSERT_EQUAL_INT(1, cont[1]); /* S */
  TEST_ASSERT_EQUAL_INT(0, cont[2]); /* L */
  TEST_ASSERT_EQUAL_INT(0, cont[3]); /* O */

  pm_free(ctx);
  cleanup();
}

static void test_parse_pm_file(void)
{
  cleanup();
  PMCtx *ctx = mk();
  TEST_ASSERT_NOT_NULL(ctx);

  FILE *fp = fopen("/tmp/test.pm", "w");
  TEST_ASSERT_NOT_NULL(fp);
  fprintf(fp, "p 000.000.000-00 Carlos Souza M 10/10/2000\n");
  fprintf(fp, "p 001.001.001-01 Ana Lima F 20/03/1995\n");
  fprintf(fp, "m 000.000.000-00 cep15 S 45 ap2\n");
  fclose(fp);

  int n = pm_parse_pm_file(ctx, "/tmp/test.pm");
  TEST_ASSERT_EQUAL_INT(3, n);

  Habitante h;
  TEST_ASSERT_EQUAL_INT(0, pm_buscar_habitante(ctx, "000.000.000-00", &h));
  TEST_ASSERT_EQUAL_STRING("Carlos", h.nome);
  TEST_ASSERT_TRUE(pm_eh_morador(ctx, "000.000.000-00"));
  TEST_ASSERT_FALSE(pm_eh_morador(ctx, "001.001.001-01"));

  remove("/tmp/test.pm");
  pm_free(ctx);
  cleanup();
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_create);
  RUN_TEST(test_inserir_buscar);
  RUN_TEST(test_morador);
  RUN_TEST(test_remover_endereco);
  RUN_TEST(test_moradores_da_quadra);
  RUN_TEST(test_parse_pm_file);
  UNITY_END();
}