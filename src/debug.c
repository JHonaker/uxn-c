#include <stdio.h>

#include "common.h"
#include "uxn.h"
#include "debug.h"

void dumpStack(Stack *stack, const char *name) {
  printf("== %s ==\n", name);
  for (int i = 0; i < stack->ptr; i++) {
    printf("%04x: ", i);
    printf("%d", stack->stack[i]);
    printf("\n");
  }
}

void dumpUxn(Uxn *uxn) {
  dumpStack(&uxn->work, "WORK");
  dumpStack(&uxn->ret, "RET");
}