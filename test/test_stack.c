#include "minunit.h"
#include "../src/common.h"
#include "../src/stack.h"

mu_suite_start();

static char *test_push() {
  Stack stack;
  Stack_init(&stack);
  Stack_push(&stack, 0x80);
  mu_assert(Stack_peek(&stack) == 0x80, "error, stack[0] != 0x80");
  return 0;
}

static char *test_pop() {
  Stack stack;
  Stack_init(&stack);
  Stack_push(&stack, 0x80);
  Stack_push(&stack, 0x81);
  Byte popped = Stack_pop(&stack);
  mu_assert(popped == 0x81, "error, stack[0] != 0x81");
  mu_assert(Stack_peek(&stack) == 0x80, "error, stack[0] != 0x80");
  return 0;
}

static char *all_tests() {
  mu_run_test(test_push);
  mu_run_test(test_pop);
  return 0;
}

RUN_TESTS(all_tests);