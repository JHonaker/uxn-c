#include "greatest.h"
#include "../src/common.h"
#include "../src/stack.h"

SUITE(stack);

TEST test_push() {
  Stack* stack = Stack_new();
  Stack_init(stack);
  Stack_push(stack, 0x80);
  ASSERT(Stack_peek(stack) == 0x80);

  Stack_delete(stack);
  
  PASS();
}

TEST test_pop() {
  Stack* stack = Stack_new();
  Stack_init(stack);
  Stack_push(stack, 0x80);
  Stack_push(stack, 0x81);
  Byte popped = Stack_pop(stack);
  ASSERT(popped == 0x81);
  ASSERT(Stack_peek(stack) == 0x80);

  Stack_delete(stack);
  PASS();
}

SUITE(stack) {
  RUN_TEST(test_push);
  RUN_TEST(test_pop);
}