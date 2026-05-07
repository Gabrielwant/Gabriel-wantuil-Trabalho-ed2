/*
 * test_geo.c — Testes unitários do módulo geo
 */

#include "unity.h"
#include "geo.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define TEST_HF "/tmp/test_geo_unit.hf"
#define EPSILON 0.01f

static void cleanup(void) { remove(TEST_HF); }

static int feq(float a, float b)
{
  return (a - b < EPSILON) && (b - a < EPSILON);
}

void setUp(void) {}
void tearDown(void) {}

static void test_create_open(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);
  geo_free(ctx);
  cleanup();
}

static void test_insert_busca(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);

  Quadra q;
  strncpy(q.cep, "cep01", CEP_MAX - 1);
  q.x = 10.0f;
  q.y = 20.0f;
  q.w = 100.0f;
  q.h = 80.0f;

  TEST_ASSERT_EQUAL_INT(0, geo_insert_quadra(ctx, &q));

  Quadra r;
  TEST_ASSERT_EQUAL_INT(0, geo_busca_quadra(ctx, "cep01", &r));
  TEST_ASSERT_EQUAL_STRING("cep01", r.cep);
  TEST_ASSERT_TRUE(feq(r.x, 10.0f));
  TEST_ASSERT_TRUE(feq(r.w, 100.0f));

  geo_free(ctx);
  cleanup();
}

static void test_remove(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);

  Quadra q;
  strncpy(q.cep, "cep02", CEP_MAX - 1);
  q.x = 0;
  q.y = 0;
  q.w = 50;
  q.h = 50;
  geo_insert_quadra(ctx, &q);

  TEST_ASSERT_EQUAL_INT(0, geo_remove_quadra(ctx, "cep02"));
  Quadra r;
  TEST_ASSERT_EQUAL_INT(-1, geo_busca_quadra(ctx, "cep02", &r));

  geo_free(ctx);
  cleanup();
}

/*
 * Testa a âncora:
 *   âncora = canto SUDESTE = (x+w, y)
 *   Para q = {x=10, y=20, w=100, h=80}:
 *     ax = 10 + 100 = 110
 *     ay = 20        (borda sul = face superior no canvas SVG)
 */
static void test_anchor(void)
{
  Quadra q;
  q.x = 10.0f;
  q.y = 20.0f;
  q.w = 100.0f;
  q.h = 80.0f;
  float ax, ay;
  geo_anchor(&q, &ax, &ay);
  TEST_ASSERT_TRUE(feq(ax, 110.0f));
  TEST_ASSERT_TRUE(feq(ay, 20.0f)); /* ay = y, não y+h */
}

/*
 * Face S (superior, y = q.y):
 *   px = ax - numero = (x+w) - numero = 100 - 30 = 70
 *   py = q.y = 0
 */
static void test_endereco_face_S(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);

  Quadra q;
  strncpy(q.cep, "cep03", CEP_MAX - 1);
  q.x = 0;
  q.y = 0;
  q.w = 100;
  q.h = 80;
  geo_insert_quadra(ctx, &q);

  float px, py;
  TEST_ASSERT_EQUAL_INT(0, geo_endereco_para_xy(ctx, "cep03", 'S', 30, &px, &py));
  TEST_ASSERT_TRUE(feq(px, 70.0f));
  TEST_ASSERT_TRUE(feq(py, 0.0f));

  geo_free(ctx);
  cleanup();
}

/*
 * Face N (inferior, y = q.y + q.h):
 *   px = ax - numero = 100 - 30 = 70
 *   py = q.y + q.h = 0 + 80 = 80
 */
static void test_endereco_face_N(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);

  Quadra q;
  strncpy(q.cep, "cep05", CEP_MAX - 1);
  q.x = 0;
  q.y = 0;
  q.w = 100;
  q.h = 80;
  geo_insert_quadra(ctx, &q);

  float px, py;
  TEST_ASSERT_EQUAL_INT(0, geo_endereco_para_xy(ctx, "cep05", 'N', 30, &px, &py));
  TEST_ASSERT_TRUE(feq(px, 70.0f));
  TEST_ASSERT_TRUE(feq(py, 80.0f));

  geo_free(ctx);
  cleanup();
}

/*
 * Face L (direita, x = q.x + q.w):
 *   px = q.x + q.w = 100
 *   py = ay + numero = q.y + numero = 0 + 20 = 20
 */
static void test_endereco_face_L(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);

  Quadra q;
  strncpy(q.cep, "cep04", CEP_MAX - 1);
  q.x = 0;
  q.y = 0;
  q.w = 100;
  q.h = 80;
  geo_insert_quadra(ctx, &q);

  float px, py;
  TEST_ASSERT_EQUAL_INT(0, geo_endereco_para_xy(ctx, "cep04", 'L', 20, &px, &py));
  TEST_ASSERT_TRUE(feq(px, 100.0f));
  TEST_ASSERT_TRUE(feq(py, 20.0f));

  geo_free(ctx);
  cleanup();
}

/*
 * Face O (esquerda, x = q.x):
 *   px = q.x = 0
 *   py = ay + numero = 0 + 20 = 20
 */
static void test_endereco_face_O(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);

  Quadra q;
  strncpy(q.cep, "cep06", CEP_MAX - 1);
  q.x = 0;
  q.y = 0;
  q.w = 100;
  q.h = 80;
  geo_insert_quadra(ctx, &q);

  float px, py;
  TEST_ASSERT_EQUAL_INT(0, geo_endereco_para_xy(ctx, "cep06", 'O', 20, &px, &py));
  TEST_ASSERT_TRUE(feq(px, 0.0f));
  TEST_ASSERT_TRUE(feq(py, 20.0f));

  geo_free(ctx);
  cleanup();
}

static void test_parse_geo_file(void)
{
  cleanup();
  GeoCtx *ctx = geo_create(TEST_HF);
  TEST_ASSERT_NOT_NULL(ctx);

  FILE *fp = fopen("/tmp/test_cidade.geo", "w");
  TEST_ASSERT_NOT_NULL(fp);
  fprintf(fp, "# comentario\n");
  fprintf(fp, "cq 1.5 #FFA040 #884400\n");
  fprintf(fp, "q cepA 0 0 100 80\n");
  fprintf(fp, "q cepB 120 0 100 80\n");
  fclose(fp);

  int n = geo_parse_geo_file(ctx, "/tmp/test_cidade.geo", NULL);
  TEST_ASSERT_EQUAL_INT(2, n);

  Quadra r;
  TEST_ASSERT_EQUAL_INT(0, geo_busca_quadra(ctx, "cepA", &r));
  TEST_ASSERT_EQUAL_INT(0, geo_busca_quadra(ctx, "cepB", &r));

  remove("/tmp/test_cidade.geo");
  geo_free(ctx);
  cleanup();
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_create_open);
  RUN_TEST(test_insert_busca);
  RUN_TEST(test_remove);
  RUN_TEST(test_anchor);
  RUN_TEST(test_endereco_face_S);
  RUN_TEST(test_endereco_face_N);
  RUN_TEST(test_endereco_face_L);
  RUN_TEST(test_endereco_face_O);
  RUN_TEST(test_parse_geo_file);
  UNITY_END();
}