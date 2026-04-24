#define _POSIX_C_SOURCE 200809L
#include "stdlib_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

extern Value make_string(const char* val);
extern Value make_int(long long val);
extern Value make_null(void);
extern Value make_bool(bool val);
extern Array* array_create(size_t capacity);
extern void array_push(Array* arr, Value v);

/* ── Response buffer ── */
typedef struct {
    char* data;
    size_t size;
} HttpBuffer;

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* ud) {
    HttpBuffer* buf = (HttpBuffer*)ud;
    size_t total = size * nmemb;
    char* tmp = realloc(buf->data, buf->size + total + 1);
    if (!tmp) return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

/* ── Response header collector ── */
typedef struct {
    Array* headers;  /* array of "key: value" strings */
} HeaderCollector;

static size_t header_cb(char* ptr, size_t size, size_t nmemb, void* ud) {
    HeaderCollector* hc = (HeaderCollector*)ud;
    size_t total = size * nmemb;
    /* strip trailing \r\n */
    size_t len = total;
    while (len > 0 && (ptr[len-1] == '\r' || ptr[len-1] == '\n')) len--;
    if (len > 0) {
        char* s = malloc(len + 1);
        if (s) {
            memcpy(s, ptr, len);
            s[len] = '\0';
            array_push(hc->headers, make_string(s));
            free(s);
        }
    }
    return total;
}

static int curl_initialized = 0;

static void ensure_curl_init(void) {
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_initialized = 1;
    }
}

/* Helper: extract a single header value from response headers array */
static Value get_header_value(Array* headers, const char* target) {
    size_t tlen = strlen(target);
    for (size_t i = 0; i < headers->count; i++) {
        if (headers->items[i].type == VAL_STRING) {
            const char* h = headers->items[i].string_val;
            /* case-insensitive prefix match up to ':' */
            size_t j = 0;
            while (h[j] && h[j] != ':') j++;
            if (j == tlen && strncasecmp(h, target, tlen) == 0) {
                const char* val = h + j;
                if (*val == ':') val++;
                while (*val == ' ') val++;
                return make_string(val);
            }
        }
    }
    return make_null();
}

/* ────────────────────────────────────────────
   http.get(url [, headers_obj])
   Returns response body as string.
   ──────────────────────────────────────────── */
Value native_http_get(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    ensure_curl_init();

    CURL* curl = curl_easy_init();
    if (!curl) return make_null();

    HttpBuffer buf = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_URL, args[0].string_val);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RADS/0.0.12");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    /* Optional headers via struct instance fields */
    struct curl_slist* slist = NULL;
    /* Note: header passing via struct args[1] not yet supported in simplified version */

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(slist);

    Value result;
    if (res != CURLE_OK) {
        /* Return error info as string */
        char err[256];
        snprintf(err, sizeof(err), "HTTP error: %s", curl_easy_strerror(res));
        result = make_string(err);
    } else {
        result = make_string(buf.data ? buf.data : "");
    }
    free(buf.data);
    curl_easy_cleanup(curl);
    return result;
}

/* ────────────────────────────────────────────
   http.post(url, body [, content_type])
   Returns response body as string.
   ──────────────────────────────────────────── */
Value native_http_post(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_null();
    ensure_curl_init();

    CURL* curl = curl_easy_init();
    if (!curl) return make_null();

    const char* content_type = (argc >= 3 && args[2].type == VAL_STRING)
        ? args[2].string_val : "application/x-www-form-urlencoded";

    HttpBuffer buf = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_URL, args[0].string_val);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args[1].string_val);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RADS/0.0.12");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    struct curl_slist* slist = NULL;
    char ct_header[256];
    snprintf(ct_header, sizeof(ct_header), "Content-Type: %s", content_type);
    slist = curl_slist_append(slist, ct_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(slist);

    Value result;
    if (res != CURLE_OK) {
        char err[256];
        snprintf(err, sizeof(err), "HTTP error: %s", curl_easy_strerror(res));
        result = make_string(err);
    } else {
        result = make_string(buf.data ? buf.data : "");
    }
    free(buf.data);
    curl_easy_cleanup(curl);
    return result;
}

/* ────────────────────────────────────────────
   http.put(url, body [, content_type])
   ──────────────────────────────────────────── */
Value native_http_put(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING)
        return make_null();
    ensure_curl_init();

    CURL* curl = curl_easy_init();
    if (!curl) return make_null();

    const char* content_type = (argc >= 3 && args[2].type == VAL_STRING)
        ? args[2].string_val : "application/octet-stream";

    HttpBuffer buf = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_URL, args[0].string_val);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args[1].string_val);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RADS/0.0.12");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    struct curl_slist* slist = NULL;
    char ct_header[256];
    snprintf(ct_header, sizeof(ct_header), "Content-Type: %s", content_type);
    slist = curl_slist_append(slist, ct_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(slist);

    Value result;
    if (res != CURLE_OK) {
        char err[256];
        snprintf(err, sizeof(err), "HTTP error: %s", curl_easy_strerror(res));
        result = make_string(err);
    } else {
        result = make_string(buf.data ? buf.data : "");
    }
    free(buf.data);
    curl_easy_cleanup(curl);
    return result;
}

/* ────────────────────────────────────────────
   http.delete(url)
   ──────────────────────────────────────────── */
Value native_http_delete(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    ensure_curl_init();

    CURL* curl = curl_easy_init();
    if (!curl) return make_null();

    HttpBuffer buf = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_URL, args[0].string_val);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RADS/0.0.12");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);

    Value result;
    if (res != CURLE_OK) {
        char err[256];
        snprintf(err, sizeof(err), "HTTP error: %s", curl_easy_strerror(res));
        result = make_string(err);
    } else {
        result = make_string(buf.data ? buf.data : "");
    }
    free(buf.data);
    curl_easy_cleanup(curl);
    return result;
}

/* ────────────────────────────────────────────
   http.head(url) — returns headers as array of strings
   ──────────────────────────────────────────── */
Value native_http_head(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    ensure_curl_init();

    CURL* curl = curl_easy_init();
    if (!curl) return make_null();

    Array* headers = array_create(32);
    HeaderCollector hc = { headers };

    curl_easy_setopt(curl, CURLOPT_URL, args[0].string_val);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &hc);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RADS/0.0.12");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    Value result;
    result.type = VAL_ARRAY;
    result.array_val = headers;
    return result;
}

/* ────────────────────────────────────────────
   http.status_code(url) — returns HTTP status as int
   ──────────────────────────────────────────── */
Value native_http_status_code(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_int(-1);
    ensure_curl_init();

    CURL* curl = curl_easy_init();
    if (!curl) return make_int(-1);

    curl_easy_setopt(curl, CURLOPT_URL, args[0].string_val);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RADS/0.0.12");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);
    long code = -1;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    }
    curl_easy_cleanup(curl);
    return make_int((long long)code);
}

/* ────────────────────────────────────────────
   http.url_encode(str) / http.url_decode(str)
   ──────────────────────────────────────────── */
Value native_http_url_encode(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    ensure_curl_init();
    char* encoded = curl_easy_escape(NULL, args[0].string_val, 0);
    if (!encoded) return make_null();
    Value result = make_string(encoded);
    curl_free(encoded);
    return result;
}

Value native_http_url_decode(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) return make_null();
    ensure_curl_init();
    int outlen = 0;
    char* decoded = curl_easy_unescape(NULL, args[0].string_val, 0, &outlen);
    if (!decoded) return make_null();
    Value result = make_string(decoded);
    curl_free(decoded);
    return result;
}

/* ── Register ── */
void stdlib_http_register(void) {
    register_native("http.get",         native_http_get);
    register_native("http.post",        native_http_post);
    register_native("http.put",         native_http_put);
    register_native("http.delete",      native_http_delete);
    register_native("http.head",        native_http_head);
    register_native("http.status_code", native_http_status_code);
    register_native("http.url_encode",  native_http_url_encode);
    register_native("http.url_decode",  native_http_url_decode);
}
