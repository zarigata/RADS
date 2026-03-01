#include "stdlib_base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_bool(bool val);
extern Value make_null(void);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

// Base64 encoding table
static const char base64_table[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 decoding table (initialized on first use)
static int base64_decode_table[256];
static bool decode_table_initialized = false;

static void init_decode_table(void) {
    if (decode_table_initialized) return;
    
    memset(base64_decode_table, -1, sizeof(base64_decode_table));
    
    for (int i = 0; i < 64; i++) {
        base64_decode_table[(unsigned char)base64_table[i]] = i;
    }
    
    base64_decode_table['='] = 0; // Padding character
    
    decode_table_initialized = true;
}

// base64.encode(str) - Encode string to base64
Value stdlib_base64_encode(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: base64.encode() requires a string argument\n");
        return make_null();
    }
    
    const unsigned char* data = (const unsigned char*)args[0].string_val;
    size_t input_len = strlen((const char*)data);
    
    if (input_len == 0) {
        return make_string("");
    }
    
    // Calculate output size: 4 chars for every 3 bytes, plus padding
    size_t output_len = ((input_len + 2) / 3) * 4;
    
    char* encoded = malloc(output_len + 1);
    size_t j = 0;
    
    for (size_t i = 0; i < input_len; i += 3) {
        unsigned int octet_a = i < input_len ? data[i] : 0;
        unsigned int octet_b = i + 1 < input_len ? data[i + 1] : 0;
        unsigned int octet_c = i + 2 < input_len ? data[i + 2] : 0;
        
        unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        
        encoded[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded[j++] = base64_table[(triple >> 6) & 0x3F];
        encoded[j++] = base64_table[triple & 0x3F];
    }
    
    // Add padding
    int mod = input_len % 3;
    if (mod == 1) {
        encoded[output_len - 1] = '=';
        encoded[output_len - 2] = '=';
    } else if (mod == 2) {
        encoded[output_len - 1] = '=';
    }
    
    encoded[output_len] = '\0';
    
    Value result;
    result.type = VAL_STRING;
    result.string_val = encoded;
    return result;
}

// base64.decode(str) - Decode base64 to string
Value stdlib_base64_decode(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: base64.decode() requires a string argument\n");
        return make_null();
    }
    
    init_decode_table();
    
    const char* encoded = args[0].string_val;
    size_t input_len = strlen(encoded);
    
    if (input_len == 0) {
        return make_string("");
    }
    
    // Validate input
    if (input_len % 4 != 0) {
        fprintf(stderr, "Error: base64.decode() invalid input length\n");
        return make_null();
    }
    
    // Calculate output size
    size_t output_len = (input_len / 4) * 3;
    
    // Adjust for padding
    if (input_len >= 1 && encoded[input_len - 1] == '=') output_len--;
    if (input_len >= 2 && encoded[input_len - 2] == '=') output_len--;
    
    unsigned char* decoded = malloc(output_len + 1);
    size_t j = 0;
    
    for (size_t i = 0; i < input_len; i += 4) {
        int sextet_a = base64_decode_table[(unsigned char)encoded[i]];
        int sextet_b = base64_decode_table[(unsigned char)encoded[i + 1]];
        int sextet_c = base64_decode_table[(unsigned char)encoded[i + 2]];
        int sextet_d = base64_decode_table[(unsigned char)encoded[i + 3]];
        
        if (sextet_a == -1 || sextet_b == -1 || sextet_c == -1 || sextet_d == -1) {
            fprintf(stderr, "Error: base64.decode() invalid character in input\n");
            free(decoded);
            return make_null();
        }
        
        unsigned int triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;
        
        if (j < output_len) decoded[j++] = (triple >> 16) & 0xFF;
        if (j < output_len) decoded[j++] = (triple >> 8) & 0xFF;
        if (j < output_len) decoded[j++] = triple & 0xFF;
    }
    
    decoded[output_len] = '\0';
    
    Value result;
    result.type = VAL_STRING;
    result.string_val = (char*)decoded;
    return result;
}

// base64.encode_bytes(arr) - Encode byte array to base64
Value stdlib_base64_encode_bytes(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_ARRAY) {
        fprintf(stderr, "Error: base64.encode_bytes() requires an array argument\n");
        return make_null();
    }
    
    Array* arr = args[0].array_val;
    if (arr->count == 0) {
        return make_string("");
    }

    // Convert array to byte buffer
    unsigned char* data = malloc(arr->count);
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->items[i].type == VAL_INT) {
            data[i] = (unsigned char)(arr->items[i].int_val & 0xFF);
        } else {
            data[i] = 0;
        }
    }

    // Calculate output size
    size_t input_len = arr->count;
    size_t output_len = ((input_len + 2) / 3) * 4;

    char* encoded = malloc(output_len + 1);
    size_t j = 0;

    for (size_t i = 0; i < input_len; i += 3) {
        unsigned int octet_a = i < input_len ? data[i] : 0;
        unsigned int octet_b = i + 1 < input_len ? data[i + 1] : 0;
        unsigned int octet_c = i + 2 < input_len ? data[i + 2] : 0;
        
        unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        
        encoded[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded[j++] = base64_table[(triple >> 6) & 0x3F];
        encoded[j++] = base64_table[triple & 0x3F];
    }
    
    // Add padding
    int mod = input_len % 3;
    if (mod == 1) {
        encoded[output_len - 1] = '=';
        encoded[output_len - 2] = '=';
    } else if (mod == 2) {
        encoded[output_len - 1] = '=';
    }
    
    encoded[output_len] = '\0';
    
    free(data);
    
    Value result;
    result.type = VAL_STRING;
    result.string_val = encoded;
    return result;
}

// base64.decode_bytes(str) - Decode base64 to byte array
Value stdlib_base64_decode_bytes(Interpreter* interp, int argc, Value* args) {
    (void)interp;
    
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "Error: base64.decode_bytes() requires a string argument\n");
        return make_null();
    }
    
    init_decode_table();
    
    const char* encoded = args[0].string_val;
    size_t input_len = strlen(encoded);
    
    if (input_len == 0) {
        Array* arr = array_create(0);
        Value result;
        result.type = VAL_ARRAY;
        result.array_val = arr;
        return result;
    }
    
    // Validate input
    if (input_len % 4 != 0) {
        fprintf(stderr, "Error: base64.decode_bytes() invalid input length\n");
        return make_null();
    }
    
    // Calculate output size
    size_t output_len = (input_len / 4) * 3;
    
    // Adjust for padding
    if (input_len >= 1 && encoded[input_len - 1] == '=') output_len--;
    if (input_len >= 2 && encoded[input_len - 2] == '=') output_len--;
    
    unsigned char* decoded = malloc(output_len);
    size_t j = 0;
    
    for (size_t i = 0; i < input_len; i += 4) {
        int sextet_a = base64_decode_table[(unsigned char)encoded[i]];
        int sextet_b = base64_decode_table[(unsigned char)encoded[i + 1]];
        int sextet_c = base64_decode_table[(unsigned char)encoded[i + 2]];
        int sextet_d = base64_decode_table[(unsigned char)encoded[i + 3]];
        
        if (sextet_a == -1 || sextet_b == -1 || sextet_c == -1 || sextet_d == -1) {
            fprintf(stderr, "Error: base64.decode_bytes() invalid character in input\n");
            free(decoded);
            return make_null();
        }
        
        unsigned int triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;
        
        if (j < output_len) decoded[j++] = (triple >> 16) & 0xFF;
        if (j < output_len) decoded[j++] = (triple >> 8) & 0xFF;
        if (j < output_len) decoded[j++] = triple & 0xFF;
    }
    
    // Create array
    Array* arr = array_create(output_len);
    for (size_t i = 0; i < output_len; i++) {
        Value byte;
        byte.type = VAL_INT;
        byte.int_val = decoded[i];
        array_push(arr, byte);
    }
    
    free(decoded);
    
    Value result;
    result.type = VAL_ARRAY;
    result.array_val = arr;
    return result;
}

void stdlib_base64_register(void) {
    register_native("base64.encode", stdlib_base64_encode);
    register_native("base64.decode", stdlib_base64_decode);
    register_native("base64.encode_bytes", stdlib_base64_encode_bytes);
    register_native("base64.decode_bytes", stdlib_base64_decode_bytes);
}