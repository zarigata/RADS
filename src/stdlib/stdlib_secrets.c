#define _POSIX_C_SOURCE 200809L
#include "stdlib_secrets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ── secrets Module ──
 * Cryptographically secure random number generation.
 * Uses OS-provided CSPRNG (/dev/urandom on POSIX, CryptGenRandom on Windows).
 * Suitable for tokens, passwords, and cryptographic keys.
 * Lightweight — no external crypto library dependency.
 */

extern Value make_int(long long val);
extern Value make_string(const char* val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* ── OS entropy source ── */
static int get_secure_bytes(unsigned char* buf, size_t len) {
#ifdef _WIN32
    /* Windows: use rand_s or CryptGenRandom */
    for (size_t i = 0; i < len; i++) {
        unsigned int val;
        if (rand_s(&val) != 0) return -1;
        buf[i] = (unsigned char)(val & 0xFF);
    }
    return 0;
#else
    /* POSIX: /dev/urandom (non-blocking, cryptographically secure on modern kernels) */
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t n = fread(buf, 1, len, f);
    fclose(f);
    return (n == len) ? 0 : -1;
#endif
}

/* ── secrets.token_hex(nbytes=32) → hex string ── */
Value native_secrets_token_hex(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    size_t nbytes = 32;
    if (argc >= 1 && args[0].type == VAL_INT) {
        nbytes = (size_t)args[0].int_val;
        if (nbytes > 1024) nbytes = 1024; /* cap for safety */
    }

    unsigned char* buf = (unsigned char*)malloc(nbytes);
    if (!buf) return make_null();
    if (get_secure_bytes(buf, nbytes) != 0) {
        free(buf);
        return make_null();
    }

    char* hex = (char*)malloc(nbytes * 2 + 1);
    if (!hex) { free(buf); return make_null(); }
    static const char hextable[] = "0123456789abcdef";
    for (size_t i = 0; i < nbytes; i++) {
        hex[i * 2]     = hextable[(buf[i] >> 4) & 0xF];
        hex[i * 2 + 1] = hextable[buf[i] & 0xF];
    }
    hex[nbytes * 2] = '\0';
    free(buf);

    Value v = make_string(hex);
    free(hex);
    return v;
}

/* ── secrets.token_bytes(nbytes=32) → array of ints ── */
Value native_secrets_token_bytes(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    size_t nbytes = 32;
    if (argc >= 1 && args[0].type == VAL_INT) {
        nbytes = (size_t)args[0].int_val;
        if (nbytes > 1024) nbytes = 1024;
    }

    unsigned char* buf = (unsigned char*)malloc(nbytes);
    if (!buf) return make_null();
    if (get_secure_bytes(buf, nbytes) != 0) {
        free(buf);
        return make_null();
    }

    Array* arr = array_create(nbytes);
    for (size_t i = 0; i < nbytes; i++) {
        array_push(arr, make_int((long long)buf[i]));
    }
    free(buf);

    Value v; v.type = VAL_ARRAY; v.array_val = arr; return v;
}

/* ── secrets.token_urlsafe(nbytes=32) → URL-safe base64 string ── */
Value native_secrets_token_urlsafe(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    size_t nbytes = 32;
    if (argc >= 1 && args[0].type == VAL_INT) {
        nbytes = (size_t)args[0].int_val;
        if (nbytes > 1024) nbytes = 1024;
    }

    unsigned char* buf = (unsigned char*)malloc(nbytes);
    if (!buf) return make_null();
    if (get_secure_bytes(buf, nbytes) != 0) {
        free(buf);
        return make_null();
    }

    /* base64url encode */
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    size_t outlen = 4 * ((nbytes + 2) / 3);
    char* out = (char*)malloc(outlen + 1);
    if (!out) { free(buf); return make_null(); }

    size_t j = 0;
    for (size_t i = 0; i < nbytes; i += 3) {
        uint32_t n = (uint32_t)buf[i] << 16;
        if (i + 1 < nbytes) n |= (uint32_t)buf[i + 1] << 8;
        if (i + 2 < nbytes) n |= (uint32_t)buf[i + 2];

        out[j++] = b64[(n >> 18) & 0x3F];
        out[j++] = b64[(n >> 12) & 0x3F];
        if (i + 1 < nbytes) out[j++] = b64[(n >> 6) & 0x3F];
        if (i + 2 < nbytes) out[j++] = b64[n & 0x3F];
    }
    out[j] = '\0';
    free(buf);

    Value v = make_string(out);
    free(out);
    return v;
}

/* ── secrets.randbelow(n) → int in [0, n) ── */
Value native_secrets_randbelow(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT || args[0].int_val <= 0)
        return make_null();

    long long n = args[0].int_val;
    /* Rejection sampling for uniformity */
    unsigned long long limit = (ULLONG_MAX / (unsigned long long)n) * (unsigned long long)n;
    unsigned char buf[8];
    unsigned long long val;
    do {
        if (get_secure_bytes(buf, 8) != 0) return make_null();
        val = 0;
        for (int i = 0; i < 8; i++) val = (val << 8) | buf[i];
    } while (val >= limit);

    return make_int((long long)(val % (unsigned long long)n));
}

/* ── secrets.randbits(k) → int with k random bits ── */
Value native_secrets_randbits(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT || args[0].int_val <= 0 || args[0].int_val > 64)
        return make_null();

    int k = (int)args[0].int_val;
    int nbytes = (k + 7) / 8;
    unsigned char buf[8] = {0};
    if (get_secure_bytes(buf, nbytes) != 0) return make_null();

    unsigned long long val = 0;
    for (int i = 0; i < nbytes; i++) val = (val << 8) | buf[i];
    /* Mask to exact bit count */
    if (k < 64) val &= ((1ULL << k) - 1);

    return make_int((long long)val);
}

/* ── secrets.choice(array) → secure random element ── */
Value native_secrets_choice(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY || !args[0].array_val || args[0].array_val->count == 0)
        return make_null();

    /* Use secure randbelow for index */
    size_t count = args[0].array_val->count;
    unsigned char buf[8];
    unsigned long long limit = (ULLONG_MAX / (unsigned long long)count) * (unsigned long long)count;
    unsigned long long val;
    do {
        if (get_secure_bytes(buf, 8) != 0) return make_null();
        val = 0;
        for (int i = 0; i < 8; i++) val = (val << 8) | buf[i];
    } while (val >= limit);

    size_t idx = (size_t)(val % (unsigned long long)count);
    return args[0].array_val->items[idx];
}

/* ── secrets.password(length=16, charset="alphanumeric") ── */
Value native_secrets_password(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    size_t length = 16;
    if (argc >= 1 && args[0].type == VAL_INT) {
        length = (size_t)args[0].int_val;
        if (length > 256) length = 256;
    }

    const char* charset;
    const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const char full[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}|;:,.<>?";
    const char digits_only[] = "0123456789";

    if (argc >= 2 && args[1].type == VAL_STRING) {
        if (strcmp(args[1].string_val, "full") == 0) charset = full;
        else if (strcmp(args[1].string_val, "digits") == 0) charset = digits_only;
        else charset = alpha;
    } else {
        charset = alpha;
    }

    size_t cslen = strlen(charset);
    unsigned char* rng = (unsigned char*)malloc(length);
    if (!rng) return make_null();
    if (get_secure_bytes(rng, length) != 0) {
        free(rng);
        return make_null();
    }

    char* result = (char*)malloc(length + 1);
    if (!result) { free(rng); return make_null(); }
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[rng[i] % cslen];
    }
    result[length] = '\0';
    free(rng);

    Value v = make_string(result);
    free(result);
    return v;
}

/* ── secrets.uuid() → cryptographically secure UUID v4 ── */
Value native_secrets_uuid(struct Interpreter* interp, int argc, Value* args) {
    (void)interp; (void)argc; (void)args;
    unsigned char buf[16];
    if (get_secure_bytes(buf, 16) != 0) return make_null();

    /* Set version 4 and variant bits */
    buf[6] = (buf[6] & 0x0F) | 0x40;  /* version 4 */
    buf[8] = (buf[8] & 0x3F) | 0x80;  /* variant 1 */

    static const char hextable[] = "0123456789abcdef";
    char out[37];
    for (int i = 0; i < 16; i++) {
        out[i * 2]     = hextable[(buf[i] >> 4) & 0xF];
        out[i * 2 + 1] = hextable[buf[i] & 0xF];
    }
    out[8] = '-'; out[13] = '-'; out[18] = '-'; out[23] = '-';
    out[36] = '\0';

    return make_string(out);
}

void stdlib_secrets_register(void) {
    register_native("secrets.token_hex", native_secrets_token_hex);
    register_native("secrets.token_bytes", native_secrets_token_bytes);
    register_native("secrets.token_urlsafe", native_secrets_token_urlsafe);
    register_native("secrets.randbelow", native_secrets_randbelow);
    register_native("secrets.randbits", native_secrets_randbits);
    register_native("secrets.choice", native_secrets_choice);
    register_native("secrets.password", native_secrets_password);
    register_native("secrets.uuid", native_secrets_uuid);
}
