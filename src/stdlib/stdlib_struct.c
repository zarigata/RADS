#include "stdlib_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_float(double val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// ---------------------------------------------------------------------------
// Binary data representation
//
// RADS strings are NUL-terminated char* and cannot carry arbitrary binary
// data (any 0x00 byte truncates them). Binary blobs are therefore exchanged
// as arrays of byte ints (0-255), mirroring `list(struct.pack(...))` in
// Python 3. The unpack family also accepts plain strings for text-ish input;
// since such strings contain no NUL bytes, strlen() is an honest length and
// every read below is bounds-checked against it.
// ---------------------------------------------------------------------------

// Format character sizes
static int fmt_size(char fmt) {
    switch (fmt) {
        case 'b': case 'B': case 'c': case '?': return 1;
        case 'h': case 'H': return 2;
        case 'i': case 'I': case 'l': case 'L': case 'f': return 4;
        case 'q': case 'Q': case 'd': return 8;
        default: return 0;
    }
}

// Parse byte-order prefix; returns first format-char index and sets *big_endian.
static int parse_prefix(const char* fmt, int* big_endian) {
    *big_endian = 1; // default: big-endian (network byte order)
    switch (fmt[0]) {
        case '<': *big_endian = 0; return 1;
        case '>': *big_endian = 1; return 1;
        case '!': *big_endian = 1; return 1;
        case '=': *big_endian = 0; return 1; // native = little on most
        case '@': *big_endian = 0; return 1;
        default:  return 0;
    }
}

// Sum of format sizes starting at index fi. Unknown characters contribute 0.
static int fmt_total_size(const char* fmt, int fi) {
    int total = 0;
    for (int i = fi; fmt[i]; i++) total += fmt_size(fmt[i]);
    return total;
}

// Copy a data argument (byte-int array or string) into a fresh byte buffer.
// Returns NULL on type error. Caller frees.
static unsigned char* get_bytes(Value v, size_t* out_len) {
    if (v.type == VAL_ARRAY) {
        Array* a = v.array_val;
        unsigned char* buf = malloc(a->count > 0 ? a->count : 1);
        if (!buf) return NULL;
        for (size_t i = 0; i < a->count; i++) {
            Value item = a->items[i];
            long long b;
            if (item.type == VAL_INT) b = item.int_val;
            else if (item.type == VAL_BOOL) b = item.bool_val ? 1 : 0;
            else if (item.type == VAL_FLOAT) b = (long long)item.float_val;
            else { free(buf); return NULL; }
            buf[i] = (unsigned char)(b & 0xFF);
        }
        *out_len = a->count;
        return buf;
    }
    if (v.type == VAL_STRING) {
        size_t len = strlen(v.string_val);
        unsigned char* buf = malloc(len > 0 ? len : 1);
        if (!buf) return NULL;
        memcpy(buf, v.string_val, len);
        *out_len = len;
        return buf;
    }
    return NULL;
}

// Wrap a byte buffer as a RADS array of ints (0-255).
static Value bytes_to_array(const unsigned char* buf, int len) {
    Array* arr = array_create(len > 0 ? (size_t)len : 1);
    for (int i = 0; i < len; i++) array_push(arr, make_int((long long)buf[i]));
    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// Parse a single value from bytes according to format.
// Caller guarantees [off, off+fmt_size(fmt)) is within the buffer.
static Value parse_value(const unsigned char* data, char fmt, int big_endian) {
    switch (fmt) {
        case 'b': { // signed byte
            int8_t v = (int8_t)data[0];
            return make_int((long long)v);
        }
        case 'B': { // unsigned byte
            uint8_t v = data[0];
            return make_int((long long)v);
        }
        case 'c': { // char (a 0x00 byte degrades to "" — C-string limitation)
            char buf[2] = { (char)data[0], '\0' };
            return make_string(buf);
        }
        case '?': { // bool
            return make_bool(data[0] != 0);
        }
        case 'h': { // signed short (16-bit)
            int16_t v;
            if (big_endian) {
                v = (int16_t)((data[0] << 8) | data[1]);
            } else {
                v = (int16_t)((data[1] << 8) | data[0]);
            }
            return make_int((long long)v);
        }
        case 'H': { // unsigned short (16-bit)
            uint16_t v;
            if (big_endian) {
                v = (uint16_t)((data[0] << 8) | data[1]);
            } else {
                v = (uint16_t)((data[1] << 8) | data[0]);
            }
            return make_int((long long)v);
        }
        case 'i': case 'l': { // signed int (32-bit)
            int32_t v;
            if (big_endian) {
                v = (int32_t)((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
            } else {
                v = (int32_t)((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
            }
            return make_int((long long)v);
        }
        case 'I': case 'L': { // unsigned int (32-bit)
            uint32_t v;
            if (big_endian) {
                v = (uint32_t)((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
            } else {
                v = (uint32_t)((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
            }
            return make_int((long long)v);
        }
        case 'q': { // signed long long (64-bit)
            int64_t v = 0;
            if (big_endian) {
                for (int i = 0; i < 8; i++) v = (v << 8) | data[i];
            } else {
                for (int i = 7; i >= 0; i--) v = (v << 8) | data[i];
            }
            return make_int((long long)v);
        }
        case 'Q': { // unsigned long long (64-bit)
            uint64_t v = 0;
            if (big_endian) {
                for (int i = 0; i < 8; i++) v = (v << 8) | data[i];
            } else {
                for (int i = 7; i >= 0; i--) v = (v << 8) | data[i];
            }
            return make_int((long long)v);
        }
        case 'f': { // float (32-bit)
            uint32_t v;
            if (big_endian) {
                v = (uint32_t)((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
            } else {
                v = (uint32_t)((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
            }
            float f;
            memcpy(&f, &v, sizeof(f));
            return make_float((double)f);
        }
        case 'd': { // double (64-bit)
            uint64_t v = 0;
            if (big_endian) {
                for (int i = 0; i < 8; i++) v = (v << 8) | data[i];
            } else {
                for (int i = 7; i >= 0; i--) v = (v << 8) | data[i];
            }
            double d;
            memcpy(&d, &v, sizeof(d));
            return make_float(d);
        }
        default:
            return make_null();
    }
}

// Write a single value to bytes according to format.
// Caller guarantees buf has fmt_size(fmt) writable bytes.
static int write_value(unsigned char* buf, char fmt, Value val, int big_endian) {
    int sz = fmt_size(fmt);
    if (sz == 0) return 0;

    switch (fmt) {
        case 'b': {
            int8_t v = (int8_t)(val.type == VAL_INT ? val.int_val : 0);
            buf[0] = (unsigned char)v;
            break;
        }
        case 'B': {
            uint8_t v = (uint8_t)(val.type == VAL_INT ? val.int_val : 0);
            buf[0] = v;
            break;
        }
        case 'c': {
            const char* s = (val.type == VAL_STRING) ? val.string_val : "?";
            buf[0] = (unsigned char)s[0];
            break;
        }
        case '?': {
            buf[0] = (val.type == VAL_BOOL) ? (val.bool_val ? 1 : 0) :
                      (val.type == VAL_INT) ? (val.int_val ? 1 : 0) : 0;
            break;
        }
        case 'h': {
            int16_t v = (int16_t)(val.type == VAL_INT ? val.int_val : 0);
            if (big_endian) { buf[0] = (v >> 8) & 0xFF; buf[1] = v & 0xFF; }
            else { buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF; }
            break;
        }
        case 'H': {
            uint16_t v = (uint16_t)(val.type == VAL_INT ? val.int_val : 0);
            if (big_endian) { buf[0] = (v >> 8) & 0xFF; buf[1] = v & 0xFF; }
            else { buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF; }
            break;
        }
        case 'i': case 'l': {
            int32_t v = (int32_t)(val.type == VAL_INT ? val.int_val : 0);
            if (big_endian) {
                buf[0] = (v >> 24) & 0xFF; buf[1] = (v >> 16) & 0xFF;
                buf[2] = (v >> 8) & 0xFF;  buf[3] = v & 0xFF;
            } else {
                buf[0] = v & 0xFF;          buf[1] = (v >> 8) & 0xFF;
                buf[2] = (v >> 16) & 0xFF;  buf[3] = (v >> 24) & 0xFF;
            }
            break;
        }
        case 'I': case 'L': {
            uint32_t v = (uint32_t)(val.type == VAL_INT ? val.int_val : 0);
            if (big_endian) {
                buf[0] = (v >> 24) & 0xFF; buf[1] = (v >> 16) & 0xFF;
                buf[2] = (v >> 8) & 0xFF;  buf[3] = v & 0xFF;
            } else {
                buf[0] = v & 0xFF;          buf[1] = (v >> 8) & 0xFF;
                buf[2] = (v >> 16) & 0xFF;  buf[3] = (v >> 24) & 0xFF;
            }
            break;
        }
        case 'q': {
            int64_t v = (int64_t)(val.type == VAL_INT ? val.int_val : 0);
            if (big_endian) {
                for (int i = 7; i >= 0; i--) buf[i] = v & 0xFF, v >>= 8;
            } else {
                for (int i = 0; i < 8; i++) buf[i] = v & 0xFF, v >>= 8;
            }
            break;
        }
        case 'Q': {
            uint64_t v = (uint64_t)(val.type == VAL_INT ? val.int_val : 0);
            if (big_endian) {
                for (int i = 7; i >= 0; i--) buf[i] = v & 0xFF, v >>= 8;
            } else {
                for (int i = 0; i < 8; i++) buf[i] = v & 0xFF, v >>= 8;
            }
            break;
        }
        case 'f': {
            float f = (float)(val.type == VAL_FLOAT ? val.float_val :
                              val.type == VAL_INT ? (double)val.int_val : 0.0);
            uint32_t v;
            memcpy(&v, &f, sizeof(v));
            if (big_endian) {
                buf[0] = (v >> 24) & 0xFF; buf[1] = (v >> 16) & 0xFF;
                buf[2] = (v >> 8) & 0xFF;  buf[3] = v & 0xFF;
            } else {
                buf[0] = v & 0xFF;          buf[1] = (v >> 8) & 0xFF;
                buf[2] = (v >> 16) & 0xFF;  buf[3] = (v >> 24) & 0xFF;
            }
            break;
        }
        case 'd': {
            double d = (val.type == VAL_FLOAT ? val.float_val :
                        val.type == VAL_INT ? (double)val.int_val : 0.0);
            uint64_t v;
            memcpy(&v, &d, sizeof(v));
            if (big_endian) {
                for (int i = 7; i >= 0; i--) buf[i] = v & 0xFF, v >>= 8;
            } else {
                for (int i = 0; i < 8; i++) buf[i] = v & 0xFF, v >>= 8;
            }
            break;
        }
    }
    return sz;
}

// struct.pack(fmt, v1, v2, ...) -> array of byte ints (binary data)
Value stdlib_struct_pack(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.pack() requires format string\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    int big_endian;
    int fi = parse_prefix(fmt, &big_endian);
    int total = fmt_total_size(fmt, fi);

    unsigned char* buf = calloc(total > 0 ? (size_t)total : 1, 1);
    if (!buf) return make_null();

    int pos = 0, arg_idx = 1;
    for (int i = fi; fmt[i]; i++) {
        if (fmt_size(fmt[i]) == 0) continue; // unknown char: consumes nothing
        if (arg_idx >= argc) {
            fprintf(stderr, "Error: struct.pack() not enough arguments for format '%s'\n", fmt);
            free(buf);
            return make_null();
        }
        pos += write_value(buf + pos, fmt[i], args[arg_idx++], big_endian);
    }

    Value result = bytes_to_array(buf, total);
    free(buf);
    return result;
}

// struct.unpack(fmt, data) -> array of values
// data: byte-int array (canonical) or string. Bounds-checked.
Value stdlib_struct_unpack(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.unpack() requires format string and data\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    size_t data_len = 0;
    unsigned char* data = get_bytes(args[1], &data_len);
    if (!data) {
        fprintf(stderr, "Error: struct.unpack() data must be a byte array or string\n");
        return make_null();
    }

    int big_endian;
    int fi = parse_prefix(fmt, &big_endian);
    int needed = fmt_total_size(fmt, fi);

    if ((size_t)needed > data_len) {
        fprintf(stderr, "Error: struct.unpack() format '%s' needs %d bytes, got %zu\n",
                fmt, needed, data_len);
        free(data);
        return make_null();
    }

    int fmt_count = 0;
    for (int i = fi; fmt[i]; i++) fmt_count++;

    Array* arr = array_create(fmt_count > 0 ? (size_t)fmt_count : 1);
    int pos = 0;
    for (int i = fi; fmt[i]; i++) {
        int sz = fmt_size(fmt[i]);
        if (sz == 0) continue;
        Value pv = parse_value(data + pos, fmt[i], big_endian);
        array_push(arr, pv);
        value_free(&pv); // array_push cloned it; release our copy
        pos += sz; // invariant: pos <= needed <= data_len
    }
    free(data);

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// struct.calcsize(fmt) -> int
Value stdlib_struct_calcsize(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.calcsize() requires format string\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    int fi = 0;
    if (fmt[0] == '<' || fmt[0] == '>' || fmt[0] == '!' || fmt[0] == '=' || fmt[0] == '@') fi = 1;

    return make_int(fmt_total_size(fmt, fi));
}

// struct.pack_into(fmt, buf, offset, v1, v2, ...) -> new byte array
// buf: byte-int array or string (copied; RADS values are immutable).
// Grows with zero fill when offset+size exceeds the buffer. Bounds-checked.
Value stdlib_struct_pack_into(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_STRING || args[2].type != VAL_INT) {
        fprintf(stderr, "Error: struct.pack_into(fmt, buf, offset, values...) required\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    size_t buf_len = 0;
    unsigned char* buf = get_bytes(args[1], &buf_len);
    if (!buf) {
        fprintf(stderr, "Error: struct.pack_into() buf must be a byte array or string\n");
        return make_null();
    }

    long long offset_ll = args[2].int_val;
    if (offset_ll < 0) {
        fprintf(stderr, "Error: struct.pack_into() offset must be >= 0\n");
        free(buf);
        return make_null();
    }
    size_t offset = (size_t)offset_ll;

    int big_endian;
    int fi = parse_prefix(fmt, &big_endian);
    int pack_size = fmt_total_size(fmt, fi);

    size_t new_len = offset + (size_t)pack_size;
    if (new_len < buf_len) new_len = buf_len;

    unsigned char* new_buf = calloc(new_len > 0 ? new_len : 1, 1);
    if (!new_buf) { free(buf); return make_null(); }
    memcpy(new_buf, buf, buf_len);
    free(buf);

    size_t pos = offset;
    int arg_idx = 3;
    for (int i = fi; fmt[i]; i++) {
        if (fmt_size(fmt[i]) == 0) continue;
        if (arg_idx >= argc) break;
        // invariant: pos + fmt_size <= offset + pack_size <= new_len
        pos += (size_t)write_value(new_buf + pos, fmt[i], args[arg_idx++], big_endian);
    }

    Value result = bytes_to_array(new_buf, (int)new_len);
    free(new_buf);
    return result;
}

// struct.unpack_from(fmt, buf, [offset]) -> array. Bounds-checked.
Value stdlib_struct_unpack_from(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.unpack_from(fmt, buf, [offset]) required\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    size_t data_len = 0;
    unsigned char* data = get_bytes(args[1], &data_len);
    if (!data) {
        fprintf(stderr, "Error: struct.unpack_from() buf must be a byte array or string\n");
        return make_null();
    }

    long long offset_ll = (argc >= 3 && args[2].type == VAL_INT) ? args[2].int_val : 0;
    if (offset_ll < 0) {
        fprintf(stderr, "Error: struct.unpack_from() offset must be >= 0\n");
        free(data);
        return make_null();
    }
    size_t offset = (size_t)offset_ll;

    int big_endian;
    int fi = parse_prefix(fmt, &big_endian);
    int needed = fmt_total_size(fmt, fi);

    if (offset + (size_t)needed > data_len || (size_t)needed > data_len) {
        fprintf(stderr, "Error: struct.unpack_from() format '%s' at offset %zu needs %d bytes, got %zu\n",
                fmt, offset, needed, data_len);
        free(data);
        return make_null();
    }

    int fmt_count = 0;
    for (int i = fi; fmt[i]; i++) fmt_count++;

    Array* arr = array_create(fmt_count > 0 ? (size_t)fmt_count : 1);
    size_t pos = offset;
    for (int i = fi; fmt[i]; i++) {
        int sz = fmt_size(fmt[i]);
        if (sz == 0) continue;
        Value pv = parse_value(data + pos, fmt[i], big_endian);
        array_push(arr, pv);
        value_free(&pv); // array_push cloned it; release our copy
        pos += (size_t)sz; // invariant: pos <= offset + needed <= data_len
    }
    free(data);

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// struct.iter_unpack(fmt, data) -> array of arrays (batched). Bounds-checked.
Value stdlib_struct_iter_unpack(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.iter_unpack(fmt, data) required\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    size_t data_len = 0;
    unsigned char* data = get_bytes(args[1], &data_len);
    if (!data) {
        fprintf(stderr, "Error: struct.iter_unpack() data must be a byte array or string\n");
        return make_null();
    }

    int big_endian;
    int fi = parse_prefix(fmt, &big_endian);

    int record_size = 0;
    int field_count = 0;
    for (int i = fi; fmt[i]; i++) {
        record_size += fmt_size(fmt[i]);
        field_count++;
    }

    if (record_size == 0) { free(data); return make_null(); }

    size_t num_records = data_len / (size_t)record_size; // floor: trailing partial record ignored
    Array* outer = array_create(num_records > 0 ? num_records : 1);

    for (size_t r = 0; r < num_records; r++) {
        Array* inner = array_create((size_t)field_count);
        size_t pos = r * (size_t)record_size;
        for (int i = fi; fmt[i]; i++) {
            int sz = fmt_size(fmt[i]);
            if (sz == 0) continue;
            // invariant: pos + sz <= (r+1)*record_size <= data_len
            Value pv = parse_value(data + pos, fmt[i], big_endian);
            array_push(inner, pv);
            value_free(&pv); // array_push cloned it; release our copy
            pos += (size_t)sz;
        }
        Value inner_val;
        inner_val.type = VAL_ARRAY;
        inner_val.array_val = inner;
        array_push(outer, inner_val);
        value_free(&inner_val); // array_push cloned (refcount++); release ours
    }
    free(data);

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = outer;
    return result;
}

void stdlib_struct_register(void) {
    register_native("struct.pack", stdlib_struct_pack);
    register_native("struct.unpack", stdlib_struct_unpack);
    register_native("struct.calcsize", stdlib_struct_calcsize);
    register_native("struct.pack_into", stdlib_struct_pack_into);
    register_native("struct.unpack_from", stdlib_struct_unpack_from);
    register_native("struct.iter_unpack", stdlib_struct_iter_unpack);
}
