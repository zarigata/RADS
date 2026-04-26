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

// Parse a single value from bytes according to format
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
        case 'c': { // char
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

// Write a single value to bytes according to format
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

// struct.pack(fmt, v1, v2, ...) -> string of packed bytes
Value stdlib_struct_pack(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.pack() requires format string\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;

    // Determine endianness
    int big_endian = 1; // default: big-endian (network byte order)
    int fi = 0;
    if (fmt[0] == '<') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '>') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '!') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '=') { big_endian = 0; fi = 1; } // native = little on most
    else if (fmt[0] == '@') { big_endian = 0; fi = 1; }

    // Calculate total size
    int total = 0;
    for (int i = fi; fmt[i]; i++) total += fmt_size(fmt[i]);

    unsigned char* buf = malloc(total > 0 ? total : 1);
    int pos = 0, arg_idx = 1;

    for (int i = fi; fmt[i]; i++) {
        if (arg_idx >= argc) {
            fprintf(stderr, "Error: struct.pack() not enough arguments for format '%s'\n", fmt);
            free(buf);
            return make_null();
        }
        pos += write_value(buf + pos, fmt[i], args[arg_idx++], big_endian);
    }

    // Return as string (binary data)
    Value result = make_string((const char*)buf);
    free(buf);
    return result;
}

// struct.unpack(fmt, data) -> array of values
Value stdlib_struct_unpack(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.unpack() requires format string and data\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    const unsigned char* data = (const unsigned char*)args[1].string_val;
    size_t data_len = strlen((const char*)data); // Note: binary data may have nulls

    // Use the actual allocated size hint from the string
    // For binary safety, we calculate expected size from format
    int big_endian = 1;
    int fi = 0;
    if (fmt[0] == '<') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '>') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '!') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '=') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '@') { big_endian = 0; fi = 1; }

    int fmt_count = 0;
    for (int i = fi; fmt[i]; i++) fmt_count++;

    Array* arr = array_create(fmt_count > 0 ? fmt_count : 1);
    int pos = 0;

    for (int i = fi; fmt[i]; i++) {
        int sz = fmt_size(fmt[i]);
        if (sz == 0) continue;
        array_push(arr, parse_value(data + pos, fmt[i], big_endian));
        pos += sz;
    }

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

    int total = 0;
    for (int i = fi; fmt[i]; i++) total += fmt_size(fmt[i]);

    return make_int(total);
}

// struct.pack_into(fmt, buf, offset, v1, v2, ...) -> string
// Since RADS strings are immutable, returns a new string
Value stdlib_struct_pack_into(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    // Simplified: same as pack but with offset support
    // struct.pack_into(fmt, buf, offset, values...)
    if (argc < 3 || args[0].type != VAL_STRING || args[1].type != VAL_STRING || args[2].type != VAL_INT) {
        fprintf(stderr, "Error: struct.pack_into(fmt, buf, offset, values...) required\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    const char* buf = args[1].string_val;
    int offset = (int)args[2].int_val;
    size_t buf_len = strlen(buf);

    int big_endian = 1;
    int fi = 0;
    if (fmt[0] == '<') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '>') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '!') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '=') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '@') { big_endian = 0; fi = 1; }

    int pack_size = 0;
    for (int i = fi; fmt[i]; i++) pack_size += fmt_size(fmt[i]);

    size_t new_len = offset + pack_size;
    if (new_len < buf_len) new_len = buf_len;

    char* new_buf = malloc(new_len + 1);
    memcpy(new_buf, buf, buf_len);
    if (new_len > buf_len) memset(new_buf + buf_len, 0, new_len - buf_len);
    new_buf[new_len] = '\0';

    int pos = offset, arg_idx = 3;
    for (int i = fi; fmt[i]; i++) {
        if (arg_idx >= argc) break;
        pos += write_value((unsigned char*)new_buf + pos, fmt[i], args[arg_idx++], big_endian);
    }

    Value result = make_string(new_buf);
    free(new_buf);
    return result;
}

// struct.unpack_from(fmt, buf, offset) -> array
Value stdlib_struct_unpack_from(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.unpack_from(fmt, buf, [offset]) required\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    const unsigned char* data = (const unsigned char*)args[1].string_val;
    int offset = (argc >= 3 && args[2].type == VAL_INT) ? (int)args[2].int_val : 0;

    int big_endian = 1;
    int fi = 0;
    if (fmt[0] == '<') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '>') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '!') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '=') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '@') { big_endian = 0; fi = 1; }

    int fmt_count = 0;
    for (int i = fi; fmt[i]; i++) fmt_count++;

    Array* arr = array_create(fmt_count > 0 ? fmt_count : 1);
    int pos = offset;

    for (int i = fi; fmt[i]; i++) {
        int sz = fmt_size(fmt[i]);
        if (sz == 0) continue;
        array_push(arr, parse_value(data + pos, fmt[i], big_endian));
        pos += sz;
    }

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

// struct.iter_unpack(fmt, data) -> array of arrays (batched)
Value stdlib_struct_iter_unpack(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "Error: struct.iter_unpack(fmt, data) required\n");
        return make_null();
    }

    const char* fmt = args[0].string_val;
    const unsigned char* data = (const unsigned char*)args[1].string_val;
    size_t data_len = strlen((const char*)data);

    int big_endian = 1;
    int fi = 0;
    if (fmt[0] == '<') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '>') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '!') { big_endian = 1; fi = 1; }
    else if (fmt[0] == '=') { big_endian = 0; fi = 1; }
    else if (fmt[0] == '@') { big_endian = 0; fi = 1; }

    int record_size = 0;
    int field_count = 0;
    for (int i = fi; fmt[i]; i++) {
        record_size += fmt_size(fmt[i]);
        field_count++;
    }

    if (record_size == 0) return make_null();

    int num_records = data_len / record_size;
    Array* outer = array_create(num_records > 0 ? num_records : 1);

    for (int r = 0; r < num_records; r++) {
        Array* inner = array_create(field_count);
        int pos = r * record_size;
        for (int i = fi; fmt[i]; i++) {
            int sz = fmt_size(fmt[i]);
            if (sz == 0) continue;
            array_push(inner, parse_value(data + pos, fmt[i], big_endian));
            pos += sz;
        }
        Value inner_val;
        inner_val.type = VAL_ARRAY;
        inner_val.array_val = inner;
        array_push(outer, inner_val);
    }

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
