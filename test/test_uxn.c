#include <stdio.h>
#include "../src/common.h"
#include "../src/uxn.h"
#include "minunit.h"

mu_suite_start();

static char *test_push_work() {
  Uxn uxn;
  initUxn(&uxn);
  uxn_push_work(&uxn, 0x80);
  mu_assert(uxn_peek_work(&uxn) == 0x80, "error, work.stack[0] != 0x80");
  return 0;
}

static char *test_pop_work() {
  Uxn uxn;
  initUxn(&uxn);
  uxn_push_work(&uxn, 0x80);
  uxn_push_work(&uxn, 0x81);
  Byte popped = uxn_pop_work(&uxn);
  mu_assert(popped == 0x81, "error, work.stack[0] != 0x81");
  mu_assert(uxn_peek_work(&uxn) == 0x80, "error, work.stack[0] != 0x80");
  return 0;
}

static char *test_push_ret() {
  Uxn uxn;
  initUxn(&uxn);
  uxn_push_ret(&uxn, 0x80);
  mu_assert(uxn_peek_ret(&uxn) == 0x80, "error, ret.stack[0] != 0x80");
  return 0;
}

static char *test_pop_ret() {
  Uxn uxn;
  initUxn(&uxn);
  uxn_push_ret(&uxn, 0x80);
  uxn_push_ret(&uxn, 0x81);
  Byte popped = uxn_pop_ret(&uxn);
  mu_assert(popped == 0x81, "error, ret.stack[0] != 0x81");
  mu_assert(uxn_peek_ret(&uxn) == 0x80, "error, ret.stack[0] != 0x80");
  return 0;
}

static char *all_tests() {
  mu_run_test(test_push_work);
  mu_run_test(test_pop_work);
  mu_run_test(test_push_ret);
  mu_run_test(test_pop_ret);
  return 0;
}

RUN_TESTS(all_tests);

