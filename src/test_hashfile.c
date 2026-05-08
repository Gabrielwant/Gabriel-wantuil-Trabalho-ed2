/*
 * test_hashfile.c — Testes unitários do módulo hashfile
 */

#include "unity.h"
#include "hashfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_HF "/tmp/test_hf_unit.hf"

static void cleanup(void) { remove(TEST_HF); }

void setUp(void) {}
void tearDown(void) {}

/* ---- criação e abertura ---- */

static void test_create_open(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);
  hf_close(hf);

  hf = hf_open(TEST_HF);
  TEST_ASSERT_NOT_NULL(hf);
  hf_close(hf);
  cleanup();
}

/* ---- inserção e busca simples ---- */

static void test_insert_search(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  TEST_ASSERT_EQUAL_INT(0, hf_insert(hf, "cep01", "dado_da_quadra_01"));

  char out[HF_MAX_DATA];
  TEST_ASSERT_EQUAL_INT(0, hf_search(hf, "cep01", out));
  TEST_ASSERT_EQUAL_STRING("dado_da_quadra_01", out);

  hf_close(hf);
  cleanup();
}

/* ---- busca de chave inexistente ---- */

static void test_search_not_found(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  char out[HF_MAX_DATA];
  TEST_ASSERT_EQUAL_INT(-1, hf_search(hf, "naoexiste", out));

  hf_close(hf);
  cleanup();
}

/* ---- remoção ---- */

static void test_delete(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  hf_insert(hf, "cpf123", "dado");
  TEST_ASSERT_EQUAL_INT(0, hf_delete(hf, "cpf123"));

  char out[HF_MAX_DATA];
  TEST_ASSERT_EQUAL_INT(-1, hf_search(hf, "cpf123", out));

  hf_close(hf);
  cleanup();
}

/* ---- atualização (chave duplicada) ---- */

static void test_update(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  hf_insert(hf, "k1", "valor_antigo");
  hf_insert(hf, "k1", "valor_novo");

  char out[HF_MAX_DATA];
  hf_search(hf, "k1", out);
  TEST_ASSERT_EQUAL_STRING("valor_novo", out);

  hf_close(hf);
  cleanup();
}

/* ---- split automático (muitos registros) ---- */

static void test_many_inserts_split(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  char key[HF_MAX_KEY], val[HF_MAX_DATA], out[HF_MAX_DATA];
  for (int i = 0; i < 32; i++)
  {
    snprintf(key, sizeof(key), "chave%03d", i);
    snprintf(val, sizeof(val), "valor_%d", i);
    TEST_ASSERT_EQUAL_INT(0, hf_insert(hf, key, val));
  }

  /* verifica todas as chaves */
  for (int i = 0; i < 32; i++)
  {
    snprintf(key, sizeof(key), "chave%03d", i);
    snprintf(val, sizeof(val), "valor_%d", i);
    TEST_ASSERT_EQUAL_INT(0, hf_search(hf, key, out));
    TEST_ASSERT_EQUAL_STRING(val, out);
  }

  hf_close(hf);
  cleanup();
}

/* ---- persistência entre sessões ---- */

static void test_persistence(void)
{
  cleanup();

  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);
  hf_insert(hf, "persistente", "valor_persistido");
  hf_close(hf);

  hf = hf_open(TEST_HF);
  TEST_ASSERT_NOT_NULL(hf);

  char out[HF_MAX_DATA];
  TEST_ASSERT_EQUAL_INT(0, hf_search(hf, "persistente", out));
  TEST_ASSERT_EQUAL_STRING("valor_persistido", out);

  hf_close(hf);
  cleanup();
}

/* ---- persistência com múltiplos registros (testa reconstrução do diretório) ---- */

static void test_persistence_many(void)
{
  cleanup();

  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  char key[HF_MAX_KEY], val[HF_MAX_DATA];
  for (int i = 0; i < 20; i++)
  {
    snprintf(key, sizeof(key), "pers%03d", i);
    snprintf(val, sizeof(val), "dado_%d", i);
    hf_insert(hf, key, val);
  }
  hf_close(hf);

  hf = hf_open(TEST_HF);
  TEST_ASSERT_NOT_NULL(hf);

  char out[HF_MAX_DATA];
  for (int i = 0; i < 20; i++)
  {
    snprintf(key, sizeof(key), "pers%03d", i);
    snprintf(val, sizeof(val), "dado_%d", i);
    TEST_ASSERT_EQUAL_INT(0, hf_search(hf, key, out));
    TEST_ASSERT_EQUAL_STRING(val, out);
  }

  hf_close(hf);
  cleanup();
}

/* ---- dump .hfd ---- */

static void test_dump(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  hf_insert(hf, "a", "dado_a");
  hf_insert(hf, "b", "dado_b");

  hf_dump(hf, "/tmp/test_dump.hfd");
  hf_close(hf);

  FILE *f = fopen("/tmp/test_dump.hfd", "r");
  TEST_ASSERT_NOT_NULL(f);
  fclose(f);
  remove("/tmp/test_dump.hfd");
  cleanup();
}

/* ---- for_each ---- */

typedef struct
{
  int count;
} IterCount;

static void count_cb(const char *key, const char *data, void *ud)
{
  (void)key;
  (void)data;
  ((IterCount *)ud)->count++;
}

static void test_for_each(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  hf_insert(hf, "x1", "d1");
  hf_insert(hf, "x2", "d2");
  hf_insert(hf, "x3", "d3");

  IterCount ic = {0};
  hf_for_each(hf, count_cb, &ic);
  TEST_ASSERT_EQUAL_INT(3, ic.count);

  hf_close(hf);
  cleanup();
}

/* ---- remoção de chave inexistente ---- */

static void test_delete_not_found(void)
{
  cleanup();
  HFFile *hf = hf_create(TEST_HF, 1);
  TEST_ASSERT_NOT_NULL(hf);

  TEST_ASSERT_EQUAL_INT(-1, hf_delete(hf, "naoexiste"));

  hf_close(hf);
  cleanup();
}

/* ---- main ---- */

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_create_open);
  RUN_TEST(test_insert_search);
  RUN_TEST(test_search_not_found);
  RUN_TEST(test_delete);
  RUN_TEST(test_update);
  RUN_TEST(test_many_inserts_split);
  RUN_TEST(test_persistence);
  RUN_TEST(test_persistence_many);
  RUN_TEST(test_dump);
  RUN_TEST(test_for_each);
  RUN_TEST(test_delete_not_found);
  UNITY_END();
}