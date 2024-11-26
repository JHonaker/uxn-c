#include "../src/common.h"
#include "../src/uxn.h"
#include "greatest.h"
#include <stdio.h>

SUITE(uxn);

TEST test_push_work() {
  Uxn *uxn = uxn_new(NULL);
  uxn_push_work(uxn, 0x80);

  ASSERT(uxn_peek_work(uxn) == 0x80);

  uxn_delete(uxn);

  PASS();
}

TEST test_pop_work() {
  Uxn *uxn = uxn_new(NULL);
  uxn_push_work(uxn, 0x80);
  uxn_push_work(uxn, 0x81);
  Byte popped = uxn_pop_work(uxn);
  ASSERT(popped == 0x81);
  ASSERT(uxn_peek_work(uxn) == 0x80);

  uxn_delete(uxn);

  PASS();
}

TEST test_push_ret() {
  Uxn *uxn = uxn_new(NULL);
  uxn_push_ret(uxn, 0x80);
  ASSERT(uxn_peek_ret(uxn) == 0x80);

  uxn_delete(uxn);

  PASS();
}

TEST test_pop_ret() {
  Uxn *uxn = uxn_new(NULL);
  uxn_push_ret(uxn, 0x80);
  uxn_push_ret(uxn, 0x81);
  Byte popped = uxn_pop_ret(uxn);
  ASSERT(popped == 0x81);
  ASSERT(uxn_peek_ret(uxn) == 0x80);

  uxn_delete(uxn);

  PASS();
}

SUITE(uxn) {
  RUN_TEST(test_push_work);
  RUN_TEST(test_pop_work);
  RUN_TEST(test_push_ret);
  RUN_TEST(test_pop_ret);
}
