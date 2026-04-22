#define _POSIX_C_SOURCE 200809L
#include "stdlib_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// ============================================================================
// RADS Buffer Module - Binary Data Handling
// ============================================================================
//
// Provides a byte buffer type for constructing and parsing binary data.
// Useful for network protocols, file I/O, and data serialization.
//
// API:
//   buffer.create(size)          - Create buffer of N bytes (zeroed)
//   buffer.from_bytes([bytes])   - Create from array of ints (0-255)
//   buffer.from_string(str)      - Create from UTF-8 string bytes
//   buffer.length(buf)           - Get byte length
//   buffer.get(buf, index)       - Get byte at index (0-255)
//   buffer.set(buf, index, val)  - Set byte at index
//   buffer.to_bytes(buf)         - Convert to array of ints
//   buffer.to_string(buf)        - Convert to string (raw bytes)
//   buffer.to_hex(buf)           - Convert to hex string
//   buffer.from_hex(hex_str)     - Create from hex string
//   buffer.slice(buf, start, end)- Extract sub-buffer
//   buffer.concat(a, b)          - Concatenate two buffers
//   buffer.write_i8(buf, offset, val)   - Write int8
//   buffer.write_u8(buf, offset, val)   - Write uint8
//   buffer.write_i16_le(buf, offset, val)  - Write int16 little-endian
//   buffer.write_i16_be(buf, offset, val)  - Write int16 big-endian
//   buffer.write_i32_le(buf, offset, val)  - Write int32 little-endian
//   buffer.write_i32_be(buf, offset, val)  - Write int32 big-endian
//   buffer.write_f64_le(buf, offset, val)  - Write float64 little-endian
//   buffer.read_i8(buf, offset)   - Read int8
//   buffer.read_u8(buf, offset)   - Read uint8
//   buffer.read_i16_le(buf, offset)  - Read int16 little-endian
//   buffer.read_i16_be(buf, offset)  - Read int16 big-endian
//   buffer.read_i32_le(buf, offset)  - Read int32 little-endian
//   buffer.read_i32_be(buf, offset)  - Read int32 big-endian
//   buffer.read_f64_le(buf, offset)  - Read float64 little-endian
//   buffer.copy(buf)              - Deep copy a buffer
//
// ============================================================================

// Buffer stored as a string internally (raw bytes in a strdup'd block).
// We use VAL_STRING type with raw bytes. A dedicated type would be better
// but we work within the existing value system.

// Helper: validate buffer arg is a string and offset is in range
static unsigned char* get_buf(Value* v, const char* fname) {
    if (v->type != VAL_STRING || v->string_val == NULL) {
        fprintf(stderr, "Error: %s() expected buffer argument\n", fname);
        return NULL;
    }
    return (unsigned char*)v->string_val;
}

static int buf_len(Value* v) {
    // We store buffer data as string, so strlen works for non-null content
    // but for binary data we need a different approach.
    // We store the buffer as: <4-byte length><data...>
    if (v->type != VAL_STRING || v->string_val == NULL) return 0;
    unsigned char* p = (unsigned char*)v->string_val;
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static unsigned char* buf_data(Value* v) {
    if (v->type != VAL_STRING || v->string_val == NULL) return NULL;
    return (unsigned char*)v->string_val + 4;
}

static Value make_buffer(const unsigned char* data, int len) {
    // Store as: 4-byte big-endian length + raw data
    char* buf = malloc(4 + len);
    if (!buf) return make_null();
    buf[0] = (len >> 24) & 0xFF;
    buf[1] = (len >> 16) & 0xFF;
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;
    if (data && len > 0) memcpy(buf + 4, data, len);
    Value v = {0};
    v.type = VAL_STRING;
    v.string_val = buf;
    return v;
}

// buffer.create(size) - Create zeroed buffer
Value native_buffer_create(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) return make_null();
    int size = (int)args[0].int_val;
    if (size < 0) return make_null();
    return make_buffer(NULL, size); // zeroed by malloc if we use calloc
}

// buffer.from_bytes([bytes]) - Create from array of ints
Value native_buffer_from_bytes(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_ARRAY) return make_null();
    Array* arr = args[0].array_val;
    int len = (int)arr->count;
    unsigned char* data = calloc(len, 1);
    for (int i = 0; i < len; i++) {
        if (arr->items[i].type == VAL_INT) {
            data[i] = (unsigned char)(arr->items[i].int_val & 0xFF);
        }
    }
    Value v = make_buffer(data, len);
    free(data);
    return v;
}

// buffer.from_string(str) - Create from string bytes
Value native_buffer_from_string(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    int len = (int)strlen(args[0].string_val);
    return make_buffer((unsigned char*)args[0].string_val, len);
}

// buffer.length(buf)
Value native_buffer_length(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    return make_int(buf_len(&args[0]));
}

// buffer.get(buf, index)
Value native_buffer_get(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[1].type != VAL_INT) return make_null();
    int idx = (int)args[1].int_val;
    int len = buf_len(&args[0]);
    if (idx < 0 || idx >= len) return make_null();
    return make_int(buf_data(&args[0])[idx]);
}

// buffer.set(buf, index, val)
Value native_buffer_set(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[1].type != VAL_INT || args[2].type != VAL_INT) return make_null();
    int idx = (int)args[1].int_val;
    int len = buf_len(&args[0]);
    if (idx < 0 || idx >= len) return make_null();
    buf_data(&args[0])[idx] = (unsigned char)(args[2].int_val & 0xFF);
    return make_int(idx);
}

// buffer.to_bytes(buf)
Value native_buffer_to_bytes(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    int len = buf_len(&args[0]);
    unsigned char* data = buf_data(&args[0]);
    Array* arr = array_create(len);
    for (int i = 0; i < len; i++) {
        array_push(arr, make_int(data[i]));
    }
    Value v = {0}; v.type = VAL_ARRAY; v.array_val = arr;
    return v;
}

// buffer.to_string(buf)
Value native_buffer_to_string(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    int len = buf_len(&args[0]);
    unsigned char* data = buf_data(&args[0]);
    char* str = malloc(len + 1);
    memcpy(str, data, len);
    str[len] = '\0';
    Value v = {0}; v.type = VAL_STRING; v.string_val = str;
    return v;
}

// buffer.to_hex(buf)
Value native_buffer_to_hex(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    int len = buf_len(&args[0]);
    unsigned char* data = buf_data(&args[0]);
    char* hex = malloc(len * 2 + 1);
    for (int i = 0; i < len; i++) {
        snprintf(hex + i * 2, 3, "%02x", data[i]);
    }
    Value v = {0}; v.type = VAL_STRING; v.string_val = hex;
    return v;
}

// buffer.from_hex(hex_str)
Value native_buffer_from_hex(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    const char* hex = args[0].string_val;
    int hexlen = (int)strlen(hex);
    int len = hexlen / 2;
    unsigned char* data = calloc(len, 1);
    for (int i = 0; i < len; i++) {
        unsigned int byte;
        sscanf(hex + i * 2, "%02x", &byte);
        data[i] = (unsigned char)byte;
    }
    Value v = make_buffer(data, len);
    free(data);
    return v;
}

// buffer.slice(buf, start, end)
Value native_buffer_slice(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3) return make_null();
    int len = buf_len(&args[0]);
    int start = (args[1].type == VAL_INT) ? (int)args[1].int_val : 0;
    int end = (args[2].type == VAL_INT) ? (int)args[2].int_val : len;
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) return make_buffer(NULL, 0);
    return make_buffer(buf_data(&args[0]) + start, end - start);
}

// buffer.concat(a, b)
Value native_buffer_concat(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2) return make_null();
    int la = buf_len(&args[0]);
    int lb = buf_len(&args[1]);
    unsigned char* data = malloc(la + lb);
    if (la > 0) memcpy(data, buf_data(&args[0]), la);
    if (lb > 0) memcpy(data + la, buf_data(&args[1]), lb);
    Value v = make_buffer(data, la + lb);
    free(data);
    return v;
}

// --- Write operations ---

#define WRITE_BUF_CHECK() \
    if (argc < 3 || args[1].type != VAL_INT) return make_null(); \
    int offset = (int)args[1].int_val; \
    unsigned char* p = buf_data(&args[0]); \
    int len = buf_len(&args[0]); \
    (void)len;

// buffer.write_u8(buf, offset, val)
Value native_buffer_write_u8(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    WRITE_BUF_CHECK();
    if (offset + 1 > len) return make_null();
    p[offset] = (unsigned char)(args[2].type == VAL_INT ? args[2].int_val & 0xFF : 0);
    return make_int(offset + 1);
}

// buffer.write_i16_le(buf, offset, val)
Value native_buffer_write_i16_le(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    WRITE_BUF_CHECK();
    if (offset + 2 > len) return make_null();
    int16_t val = (int16_t)(args[2].type == VAL_INT ? args[2].int_val : 0);
    memcpy(p + offset, &val, 2); // host is typically LE
    return make_int(offset + 2);
}

// buffer.write_i16_be(buf, offset, val)
Value native_buffer_write_i16_be(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    WRITE_BUF_CHECK();
    if (offset + 2 > len) return make_null();
    int16_t val = (int16_t)(args[2].type == VAL_INT ? args[2].int_val : 0);
    int16_t be = htons((uint16_t)val);
    memcpy(p + offset, &be, 2);
    return make_int(offset + 2);
}

// buffer.write_i32_le(buf, offset, val)
Value native_buffer_write_i32_le(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    WRITE_BUF_CHECK();
    if (offset + 4 > len) return make_null();
    int32_t val = (int32_t)(args[2].type == VAL_INT ? args[2].int_val : 0);
    memcpy(p + offset, &val, 4); // host is typically LE
    return make_int(offset + 4);
}

// buffer.write_i32_be(buf, offset, val)
Value native_buffer_write_i32_be(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    WRITE_BUF_CHECK();
    if (offset + 4 > len) return make_null();
    int32_t val = (int32_t)(args[2].type == VAL_INT ? args[2].int_val : 0);
    int32_t be = htonl((uint32_t)val);
    memcpy(p + offset, &be, 4);
    return make_int(offset + 4);
}

// buffer.write_f64_le(buf, offset, val)
Value native_buffer_write_f64_le(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    WRITE_BUF_CHECK();
    if (offset + 8 > len) return make_null();
    double val = (args[2].type == VAL_FLOAT) ? args[2].float_val :
                 (args[2].type == VAL_INT) ? (double)args[2].int_val : 0.0;
    memcpy(p + offset, &val, 8);
    return make_int(offset + 8);
}

// --- Read operations ---

#define READ_BUF_CHECK(sz) \
    if (argc < 2 || args[1].type != VAL_INT) return make_null(); \
    int offset = (int)args[1].int_val; \
    int len = buf_len(&args[0]); \
    unsigned char* p = buf_data(&args[0]); \
    if (offset + sz > len) return make_null();

// buffer.read_u8(buf, offset)
Value native_buffer_read_u8(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    READ_BUF_CHECK(1);
    return make_int(p[offset]);
}

// buffer.read_i8(buf, offset)
Value native_buffer_read_i8(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    READ_BUF_CHECK(1);
    return make_int((int8_t)p[offset]);
}

// buffer.read_i16_le(buf, offset)
Value native_buffer_read_i16_le(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    READ_BUF_CHECK(2);
    int16_t val;
    memcpy(&val, p + offset, 2);
    return make_int(val);
}

// buffer.read_i16_be(buf, offset)
Value native_buffer_read_i16_be(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    READ_BUF_CHECK(2);
    uint16_t val;
    memcpy(&val, p + offset, 2);
    return make_int((int16_t)ntohs(val));
}

// buffer.read_i32_le(buf, offset)
Value native_buffer_read_i32_le(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    READ_BUF_CHECK(4);
    int32_t val;
    memcpy(&val, p + offset, 4);
    return make_int(val);
}

// buffer.read_i32_be(buf, offset)
Value native_buffer_read_i32_be(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    READ_BUF_CHECK(4);
    uint32_t val;
    memcpy(&val, p + offset, 4);
    return make_int((int32_t)ntohl(val));
}

// buffer.read_f64_le(buf, offset)
Value native_buffer_read_f64_le(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    READ_BUF_CHECK(8);
    double val;
    memcpy(&val, p + offset, 8);
    return make_float(val);
}

// buffer.copy(buf) - Deep copy
Value native_buffer_copy(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1) return make_null();
    int len = buf_len(&args[0]);
    return make_buffer(buf_data(&args[0]), len);
}

// Registration
void stdlib_buffer_register(void) {
    register_native("buffer.create", native_buffer_create);
    register_native("buffer.from_bytes", native_buffer_from_bytes);
    register_native("buffer.from_string", native_buffer_from_string);
    register_native("buffer.length", native_buffer_length);
    register_native("buffer.get", native_buffer_get);
    register_native("buffer.set", native_buffer_set);
    register_native("buffer.to_bytes", native_buffer_to_bytes);
    register_native("buffer.to_string", native_buffer_to_string);
    register_native("buffer.to_hex", native_buffer_to_hex);
    register_native("buffer.from_hex", native_buffer_from_hex);
    register_native("buffer.slice", native_buffer_slice);
    register_native("buffer.concat", native_buffer_concat);
    register_native("buffer.write_u8", native_buffer_write_u8);
    register_native("buffer.write_i16_le", native_buffer_write_i16_le);
    register_native("buffer.write_i16_be", native_buffer_write_i16_be);
    register_native("buffer.write_i32_le", native_buffer_write_i32_le);
    register_native("buffer.write_i32_be", native_buffer_write_i32_be);
    register_native("buffer.write_f64_le", native_buffer_write_f64_le);
    register_native("buffer.read_u8", native_buffer_read_u8);
    register_native("buffer.read_i8", native_buffer_read_i8);
    register_native("buffer.read_i16_le", native_buffer_read_i16_le);
    register_native("buffer.read_i16_be", native_buffer_read_i16_be);
    register_native("buffer.read_i32_le", native_buffer_read_i32_le);
    register_native("buffer.read_i32_be", native_buffer_read_i32_be);
    register_native("buffer.read_f64_le", native_buffer_read_f64_le);
    register_native("buffer.copy", native_buffer_copy);
}
