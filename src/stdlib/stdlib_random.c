#define _POSIX_C_SOURCE 200809L
#include "stdlib_random.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Random Module ──
 * Provides random number generation: ints, floats, choices, shuffles.
 * Uses a fast xorshift64 PRNG seeded from time + PID for good entropy
 * on all platforms including 32-bit single-core systems.
 */

extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_string(const char* val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* ── xorshift64* PRNG ── */
static unsigned long long rads_rand_state = 0;

static void rads_seed(unsigned long long seed) {
    rads_rand_state = seed ? seed : 1; /* state must be nonzero */
}

static unsigned long long rads_xorshift64(void) {
    unsigned long long x = rads_rand_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rads_rand_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

/* Returns double in [0.0, 1.0) */
static double rads_random_double(void) {
    return (double)(rads_xorshift64() >> 11) / (double)(1ULL << 53);
}

/* ── Auto-seed on first call ── */
static void ensure_seeded(void) {
    if (rads_rand_state == 0) {
        unsigned long long seed = (unsigned long long)time(NULL);
        seed ^= ((unsigned long long)getpid() << 32);
        seed ^= (unsigned long long)clock();
        rads_seed(seed);
    }
}

/* ── random.seed(n) ── */
Value native_random_seed(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    ensure_seeded();
    unsigned long long s = 0;
    if (argc >= 1) {
        if (args[0].type == VAL_INT) s = (unsigned long long)args[0].int_val;
        else if (args[0].type == VAL_FLOAT) s = (unsigned long long)args[0].float_val;
    }
    rads_seed(s);
    return make_null();
}

/* ── random.int(min, max) ── */
Value native_random_int(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    ensure_seeded();
    long long lo = 0, hi = 0;
    if (argc >= 2 && args[0].type == VAL_INT && args[1].type == VAL_INT) {
        lo = args[0].int_val;
        hi = args[1].int_val;
    } else if (argc >= 1 && args[0].type == VAL_INT) {
        hi = args[0].int_val;
    }
    if (lo > hi) { long long t = lo; lo = hi; hi = t; }
    unsigned long long range = (unsigned long long)(hi - lo + 1);
    return make_int(lo + (long long)(rads_xorshift64() % range));
}

/* ── random.float() → [0.0, 1.0) ── */
Value native_random_float(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    ensure_seeded();
    return make_float(rads_random_double());
}

/* ── random.range(min, max) → [min, max) ── */
Value native_random_range(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    ensure_seeded();
    double lo = 0.0, hi = 1.0;
    if (argc >= 2 && args[0].type == VAL_FLOAT && args[1].type == VAL_FLOAT) {
        lo = args[0].float_val; hi = args[1].float_val;
    } else if (argc >= 2 && args[0].type == VAL_INT && args[1].type == VAL_INT) {
        lo = (double)args[0].int_val; hi = (double)args[1].int_val;
    } else if (argc >= 1) {
        if (args[0].type == VAL_FLOAT) hi = args[0].float_val;
        else if (args[0].type == VAL_INT) hi = (double)args[0].int_val;
    }
    return make_float(lo + rads_random_double() * (hi - lo));
}

/* ── random.choice(array) ── */
Value native_random_choice(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    ensure_seeded();
    if (argc < 1 || args[0].type != VAL_ARRAY || !args[0].array_val || args[0].array_val->count == 0)
        return make_null();
    size_t idx = (size_t)(rads_xorshift64() % (unsigned long long)args[0].array_val->count);
    return args[0].array_val->items[idx];
}

/* ── random.shuffle(array) → shuffled copy ── */
Value native_random_shuffle(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    ensure_seeded();
    if (argc < 1 || args[0].type != VAL_ARRAY || !args[0].array_val)
        return make_null();

    Array* src = args[0].array_val;
    Array* dst = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) {
        array_push(dst, src->items[i]);
    }

    /* Fisher-Yates shuffle */
    for (size_t i = dst->count - 1; i > 0; i--) {
        size_t j = (size_t)(rads_xorshift64() % (unsigned long long)(i + 1));
        Value tmp = dst->items[i];
        dst->items[i] = dst->items[j];
        dst->items[j] = tmp;
    }

    Value v; v.type = VAL_ARRAY; v.array_val = dst; return v;
}

/* ── random.sample(array, k) → array of k unique elements ── */
Value native_random_sample(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    ensure_seeded();
    if (argc < 2 || args[0].type != VAL_ARRAY || args[1].type != VAL_INT)
        return make_null();

    Array* src = args[0].array_val;
    long long k = args[1].int_val;
    if (k < 0 || (size_t)k > src->count) return make_null();

    /* Copy + Fisher-Yates partial shuffle */
    Array* pool = array_create(src->count);
    for (size_t i = 0; i < src->count; i++) array_push(pool, src->items[i]);

    Array* result = array_create((size_t)k);
    for (long long i = 0; i < k; i++) {
        size_t j = (size_t)(rads_xorshift64() % (unsigned long long)(pool->count - (size_t)i));
        size_t last = pool->count - 1 - (size_t)i;
        array_push(result, pool->items[j]);
        /* swap picked to end */
        Value tmp = pool->items[j];
        pool->items[j] = pool->items[last];
        pool->items[last] = tmp;
    }

    Value v; v.type = VAL_ARRAY; v.array_val = result; return v;
}

/* ── random.bool() ── */
Value native_random_bool(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    ensure_seeded();
    return make_bool(rads_xorshift64() & 1);
}

/* ── random.uuid() → simple UUID v4 string ── */
Value native_random_uuid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    ensure_seeded();
    char buf[37];
    const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        if (i == 12) { buf[i] = '4'; continue; }
        buf[i] = hex[rads_xorshift64() & 0xF];
    }
    /* variant bits */
    buf[16] = hex[0x8 + (rads_xorshift64() & 0x3)];
    buf[8] = buf[13] = buf[18] = buf[23] = '-';
    /* shift chars for dashes */
    memmove(buf + 24, buf + 23, 9);
    buf[23] = '-';
    memmove(buf + 19, buf + 18, 14);
    buf[18] = '-';
    memmove(buf + 14, buf + 13, 19);
    buf[13] = '-';
    buf[36] = '\0';
    return make_string(buf);
}

void stdlib_random_register(void) {
    rads_seed(0); /* will auto-seed on first use */
    register_native("random.seed", native_random_seed);
    register_native("random.int", native_random_int);
    register_native("random.float", native_random_float);
    register_native("random.range", native_random_range);
    register_native("random.choice", native_random_choice);
    register_native("random.shuffle", native_random_shuffle);
    register_native("random.sample", native_random_sample);
    register_native("random.bool", native_random_bool);
    register_native("random.uuid", native_random_uuid);
}
