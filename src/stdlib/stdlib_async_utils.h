#ifndef RADS_STDLIB_ASYNC_UTILS_H
#define RADS_STDLIB_ASYNC_UTILS_H

#include "../core/interpreter.h"

// Async Utils Module registration
void stdlib_async_utils_register(void);

// Native functions
Value native_async_sleep(struct Interpreter* interp, int argc, Value* args);
Value native_async_timeout(struct Interpreter* interp, int argc, Value* args);
Value native_async_interval(struct Interpreter* interp, int argc, Value* args);
Value native_async_set_timeout(struct Interpreter* interp, int argc, Value* args);
Value native_async_clear_timeout(struct Interpreter* interp, int argc, Value* args);
Value native_async_set_interval(struct Interpreter* interp, int argc, Value* args);
Value native_async_clear_interval(struct Interpreter* interp, int argc, Value* args);
Value native_async_promisify(struct Interpreter* interp, int argc, Value* args);
Value native_async_await(struct Interpreter* interp, int argc, Value* args);
Value native_async_all(struct Interpreter* interp, int argc, Value* args);
Value native_async_race(struct Interpreter* interp, int argc, Value* args);
Value native_async_any(struct Interpreter* interp, int argc, Value* args);
Value native_async_each(struct Interpreter* interp, int argc, Value* args);
Value native_async_map(struct Interpreter* interp, int argc, Value* args);
Value native_async_filter(struct Interpreter* interp, int argc, Value* args);
Value native_async_reduce(struct Interpreter* interp, int argc, Value* args);
Value native_async_queue(struct Interpreter* interp, int argc, Value* args);
Value native_async_debounce(struct Interpreter* interp, int argc, Value* args);
Value native_async_throttle(struct Interpreter* interp, int argc, Value* args);

#endif