#ifndef STDLIB_TEST_H
#define STDLIB_TEST_H

#include "../core/interpreter.h"

void stdlib_test_register(void);
bool stdlib_test_failed(void); /* true once any test has failed (run 23) */

#endif // STDLIB_TEST_H