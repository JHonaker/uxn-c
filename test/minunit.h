#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>
#include <stdlib.h>

#define log_err(message) printf("\tError: %s\n", message)

#define mu_suite_start() char *message = NULL

#define mu_assert(test, message) \
  do { \
    if (!(test)) { \
      log_err(message); \
      return message; \
    } \
  } while (0)

#define mu_run_test(test) \
  do { \
    printf("Running test: %s\n", #test); \
    message = test(); \
    tests_run++; \
    if (message) return message; \
  } while (0)

#define RUN_TESTS(name) \
  int main(int argc, char *argv[]) { \
    tests_run = 0; \
    argc = 1; \
    printf("---- RUNNING: %s ----\n", argv[0]); \
    char *result = name(); \
    if (result != 0) { \
      printf("Failed: %s\n", result); \
    } else { \
      printf("All tests passed\n"); \
    } \
    printf("Tests run: %d\n", tests_run); \
    exit(result == 0 ? EXIT_SUCCESS : EXIT_FAILURE); \
  }

int tests_run;

#endif // MINUNIT_H