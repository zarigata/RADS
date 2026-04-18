#define _POSIX_C_SOURCE 200809L
#include "stdlib_uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern Value make_string(const char* val);
extern Value make_null(void);

/* Simple xorshift128 PRNG seeded from time + address entropy */
static unsigned long long s[2] = {0, 0};
static int seeded = 0;

static void seed_prng(void) {
    if (seeded) return;
    s[0] = (unsigned long long)time(NULL) ^ ((unsigned long long)&seeded >> 4);
    s[1] = (unsigned long long)clock() ^ ((unsigned long long)&s[0] >> 4);
    /* Ensure non-zero */
    if (s[0] == 0) s[0] = 0xdeadbeefcafebabeull;
    if (s[1] == 0) s[1] = 0x0123456789abcdefull;
    seeded = 1;
}

static unsigned long long xorshift128plus(void) {
    unsigned long long x = s[0];
    unsigned long long y = s[1];
    s[0] = y;
    x ^= x << 23;
    s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return s[1] + y;
}

static unsigned int rand_bits(int bits) {
    return (unsigned int)(xorshift128plus() >> (64 - bits));
}

/* Generate a version 4 UUID (random) */
Value native_uuid_v4(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    seed_prng();

    unsigned int time_low       = rand_bits(32);
    unsigned int time_mid       = rand_bits(16);
    unsigned int time_hi_ver    = (4 << 12) | (rand_bits(12) & 0x0FFF); /* version 4 */
    unsigned int clock_seq      = (2 << 14) | (rand_bits(14) & 0x3FFF); /* variant 2 */
    unsigned int node_hi        = rand_bits(16);
    unsigned int node_lo        = rand_bits(32);

    char buf[37];
    snprintf(buf, sizeof(buf),
        "%08x-%04x-%04x-%04x-%04x%08x",
        time_low, time_mid, time_hi_ver, clock_seq, node_hi, node_lo);
    return make_string(buf);
}

/* Generate a short ID: 8 hex chars from FNV-1a of seed + counter */
static unsigned long long id_counter = 0;

Value native_uuid_short(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    seed_prng();

    unsigned long long combined = xorshift128plus() ^ (++id_counter);
    /* Mix further */
    combined ^= combined >> 33;
    combined *= 0xff51afd7ed558ccdull;
    combined ^= combined >> 33;
    combined *= 0xc4ceb9fe1a85ec53ull;
    combined ^= combined >> 33;

    char buf[17];
    snprintf(buf, sizeof(buf), "%016llx", combined);
    /* Return first 8 chars */
    buf[8] = '\0';
    return make_string(buf);
}

/* Generate a nanoid-style string (default 21 chars from 64-char alphabet) */
Value native_uuid_nanoid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    seed_prng();

    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
    int len = 21;
    if (argc >= 1 && args[0].type == VAL_INT && args[0].int_val > 0 && args[0].int_val <= 256) {
        len = (int)args[0].int_val;
    }

    char* buf = malloc(len + 1);
    if (!buf) return make_null();

    for (int i = 0; i < len; i++) {
        buf[i] = alphabet[xorshift128plus() % 64];
    }
    buf[len] = '\0';

    Value v;
    v.type = VAL_STRING;
    v.string_val = buf;
    return v;
}

/* Check if a string is a valid UUID format */
Value native_uuid_is_valid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        Value v; v.type = VAL_BOOL; v.bool_val = false; return v;
    }
    const char* s = args[0].string_val;
    if (strlen(s) != 36) {
        Value v; v.type = VAL_BOOL; v.bool_val = false; return v;
    }
    /* Format: 8-4-4-4-12 hex digits with dashes */
    const char* p = s;
    static const int groups[] = {8, 4, 4, 4, 12};
    for (int g = 0; g < 5; g++) {
        for (int i = 0; i < groups[g]; i++) {
            char c = *p++;
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                Value v; v.type = VAL_BOOL; v.bool_val = false; return v;
            }
        }
        if (g < 4) {
            if (*p++ != '-') {
                Value v; v.type = VAL_BOOL; v.bool_val = false; return v;
            }
        }
    }
    Value v; v.type = VAL_BOOL; v.bool_val = true; return v;
}

void stdlib_uuid_register(void) {
    register_native("uuid.v4",       native_uuid_v4);
    register_native("uuid.short",    native_uuid_short);
    register_native("uuid.nanoid",   native_uuid_nanoid);
    register_native("uuid.is_valid", native_uuid_is_valid);
}
