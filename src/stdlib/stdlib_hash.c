#define _POSIX_C_SOURCE 200809L
#include "stdlib_hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* ── FNV-1a 32-bit ── */
static unsigned int fnv1a_32(const char* data, size_t len) {
    unsigned int h = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)data[i];
        h *= 16777619u;
    }
    return h;
}

/* ── FNV-1a 64-bit ── */
static unsigned long long fnv1a_64(const char* data, size_t len) {
    unsigned long long h = 14695981039346656037ull;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)data[i];
        h *= 1099511628211ull;
    }
    return h;
}

/* ── DJB2 ── */
static unsigned long djb2(const char* data, size_t len) {
    unsigned long h = 5381;
    for (size_t i = 0; i < len; i++) {
        h = ((h << 5) + h) + (unsigned char)data[i];
    }
    return h;
}

/* ── CRC32 (table-based) ── */
static unsigned int crc32_table[256];
static int crc32_table_init = 0;

static void crc32_init_table(void) {
    for (unsigned int i = 0; i < 256; i++) {
        unsigned int c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1) c = 0xEDB88320u ^ (c >> 1);
            else       c >>= 1;
        }
        crc32_table[i] = c;
    }
    crc32_table_init = 1;
}

static unsigned int compute_crc32(const char* data, size_t len) {
    if (!crc32_table_init) crc32_init_table();
    unsigned int crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ (unsigned char)data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

/* ── Adler-32 ── */
static unsigned int compute_adler32(const char* data, size_t len) {
    unsigned int a = 1, b = 0;
    const unsigned int MOD = 65521;
    for (size_t i = 0; i < len; i++) {
        a = (a + (unsigned char)data[i]) % MOD;
        b = (b + a) % MOD;
    }
    return (b << 16) | a;
}

/* ── Simple hash based on xorshift (not cryptographic) ── */
static unsigned long long xorshift_hash(const char* data, size_t len) {
    unsigned long long h = 0;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned long long)(unsigned char)data[i];
        h ^= h << 13;
        h ^= h >> 7;
        h ^= h << 17;
    }
    return h;
}

/* ── Public API ── */

Value native_hash_fnv1a(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char buf[32];
    snprintf(buf, sizeof(buf), "%08x", fnv1a_32(args[0].string_val, strlen(args[0].string_val)));
    return make_string(buf);
}

Value native_hash_fnv1a64(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", fnv1a_64(args[0].string_val, strlen(args[0].string_val)));
    return make_string(buf);
}

Value native_hash_djb2(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char buf[32];
    snprintf(buf, sizeof(buf), "%08lx", djb2(args[0].string_val, strlen(args[0].string_val)));
    return make_string(buf);
}

Value native_hash_crc32(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char buf[32];
    snprintf(buf, sizeof(buf), "%08x", compute_crc32(args[0].string_val, strlen(args[0].string_val)));
    return make_string(buf);
}

Value native_hash_adler32(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char buf[32];
    snprintf(buf, sizeof(buf), "%08x", compute_adler32(args[0].string_val, strlen(args[0].string_val)));
    return make_string(buf);
}

Value native_hash_xorshift(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", xorshift_hash(args[0].string_val, strlen(args[0].string_val)));
    return make_string(buf);
}

/* ── Utility: hash an integer bucket index for hash tables ── */
Value native_hash_bucket(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        return make_int(0);
    }
    unsigned int h = fnv1a_32(args[0].string_val, strlen(args[0].string_val));
    long long buckets = args[1].int_val;
    if (buckets <= 0) buckets = 1;
    return make_int((long long)(h % (unsigned int)buckets));
}

void stdlib_hash_register(void) {
    register_native("hash.fnv1a",     native_hash_fnv1a);
    register_native("hash.fnv1a64",   native_hash_fnv1a64);
    register_native("hash.djb2",      native_hash_djb2);
    register_native("hash.crc32",     native_hash_crc32);
    register_native("hash.adler32",   native_hash_adler32);
    register_native("hash.xorshift",  native_hash_xorshift);
    register_native("hash.bucket",    native_hash_bucket);
}
