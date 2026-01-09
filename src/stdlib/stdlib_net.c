#define _POSIX_C_SOURCE 200809L
#include "stdlib_net.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

extern uv_loop_t* global_event_loop;
struct Interpreter;

typedef struct TcpHandleCtx {
    uv_tcp_t* handle;
    RadsBufferNode* recv_queue;
    char* id;
    bool is_listener;
    bool owns_handle;
    bool is_http;
    bool data_owner;
    void* data;
    struct Interpreter* interp;
    struct TcpHandleCtx* next;
} TcpHandleCtx;

static TcpHandleCtx* tcp_ctx_head = NULL;
static long next_handle_id = 1;

typedef struct HttpRequest {
    char* method;
    char* path;
    char* query_string;
    char* http_version;
    char** header_names;
    char** header_values;
    int header_count;
    char* body;
    size_t body_length;
} HttpRequest;

typedef struct HttpResponse {
    int status_code;
    char* status_text;
    char** header_names;
    char** header_values;
    int header_count;
    char* body;
    size_t body_length;
} HttpResponse;

typedef struct HttpClientRequest {
    char* method;           // GET, POST, etc.
    char* url;              // Full URL
    char* host;             // Parsed host
    char* path;             // Parsed path with query
    int port;               // Parsed port (default 80)
    char** header_names;
    char** header_values;
    int header_count;
    char* body;
    size_t body_length;
} HttpClientRequest;

typedef struct HttpClientResponse {
    int status_code;
    char* status_text;
    char** header_names;
    char** header_values;
    int header_count;
    char* body;
    size_t body_length;
    bool complete;
} HttpClientResponse;

typedef struct HttpClientContext {
    HttpClientRequest* request;
    HttpClientResponse* response;
    uv_tcp_t* tcp_handle;
    char* buffer;
    size_t buffer_size;
    size_t buffer_used;
    bool headers_complete;
    size_t content_length;
    struct Interpreter* interp;
    bool connect_done;
    int connect_status;
    bool timed_out;
    uv_timer_t* timer_handle;
} HttpClientContext;

typedef struct ConnectionPoolEntry {
    char* host;
    int port;
    uv_tcp_t* handle;
    time_t last_used;
    bool in_use;
    struct ConnectionPoolEntry* next;
} ConnectionPoolEntry;

typedef struct ConnectionPool {
    ConnectionPoolEntry* head;
    int max_connections;
    int idle_timeout;
} ConnectionPool;

typedef struct RouteNode {
    char* path;
    char* method;
    Value handler;
    bool is_static;
    char* static_dir;
    struct RouteNode* next;
} RouteNode;

typedef struct RouteRegistry {
    RouteNode* head;
    int count;
} RouteRegistry;

static Value make_string(const char* s);
static Value make_bool(bool v);
static Value make_null(void);
static Value make_response_tuple(int status, const char* body, const char* ctype);
static Value make_int(long long v);
static Value make_json_response(const char* body);
static Value clone_value(Value v);
// Local array helpers (interpreter internals not exposed here)
static Array* array_create_local(size_t capacity);
static void array_push_local(Array* arr, Value v);
Value native_net_serve(struct Interpreter* interp, int argc, Value* args);
static TcpHandleCtx* register_tcp_ctx(struct Interpreter* interp, uv_tcp_t* handle, const char* prefix, bool is_listener, bool owns_handle, bool is_http, const char* explicit_id);
static void buffer_free(RadsBufferNode* head);
static RadsBufferNode* buffer_push(RadsBufferNode** head, const char* data, size_t len);
static Value buffer_pop(RadsBufferNode** head);
static void enqueue_data(TcpHandleCtx* ctx, const char* data, ssize_t len);
static TcpHandleCtx* find_tcp_ctx(const char* id);
static void unregister_tcp_ctx(TcpHandleCtx* ctx);
static void http_send_response(uv_stream_t* client, HttpResponse* resp);
static HttpResponse* http_response_create(int status_code, const char* status_text);
static void http_response_add_header(HttpResponse* resp, const char* name, const char* value);
static void http_response_set_body(HttpResponse* resp, const char* body, const char* content_type);
static char* http_response_build(HttpResponse* resp, size_t* out_len);
static void http_response_free(HttpResponse* resp);
static HttpRequest* http_request_parse(const char* data, ssize_t len);
static HttpRequest* http_request_create(void);
static void http_request_free(HttpRequest* req);
static const char* http_request_get_header(HttpRequest* req, const char* name);
static void http_handle_request(uv_stream_t* client, const char* data, ssize_t len);
static RouteRegistry* route_registry_create(void);
static void route_registry_free(RouteRegistry* reg);
static bool route_registry_add(RouteRegistry* reg, const char* path, const char* method, Value handler);
static bool route_registry_add_static(RouteRegistry* reg, const char* prefix, const char* dir);
static RouteNode* route_registry_find(RouteRegistry* reg, const char* path, const char* method);
static RouteNode* route_registry_find_static(RouteRegistry* reg, const char* path);
static bool path_has_parent_ref(const char* path);
static const char* guess_mime(const char* path);
static HttpClientRequest* http_client_request_create(const char* method, const char* url);
static void http_client_request_free(HttpClientRequest* req);
static void http_client_request_add_header(HttpClientRequest* req, const char* name, const char* value);
static void http_client_request_set_body(HttpClientRequest* req, const char* body);
static char* http_client_request_build(HttpClientRequest* req, size_t* out_len);
static HttpClientResponse* http_client_response_create(void);
static void http_client_response_free(HttpClientResponse* resp);
static bool http_client_response_parse(HttpClientResponse* resp, const char* data, size_t len);
static const char* http_client_response_get_header(HttpClientResponse* resp, const char* name);
static bool url_parse(const char* url, char** host, int* port, char** path);
static ConnectionPool* connection_pool_create(int max_connections, int idle_timeout);
static void connection_pool_free(ConnectionPool* pool);
static uv_tcp_t* connection_pool_get(ConnectionPool* pool, const char* host, int port);
static void connection_pool_release(ConnectionPool* pool, const char* host, int port, uv_tcp_t* handle);
static void connection_pool_cleanup_idle(ConnectionPool* pool);
static HttpClientResponse* http_client_execute(HttpClientRequest* req);
static void on_http_client_connect(uv_connect_t* req, int status);
static void on_http_client_write(uv_write_t* req, int status);
static void on_http_client_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
static void on_http_client_timeout(uv_timer_t* timer);
static void on_http_client_close(uv_handle_t* handle);
static bool should_keep_alive(HttpClientResponse* resp);

static Value make_string(const char* s) {
    Value v;
    v.type = VAL_STRING;
    v.string_val = strdup(s ? s : "");
    return v;
}

static Value clone_value(Value v) {
    Value out = v;
    if (v.type == VAL_STRING && v.string_val) {
        out.string_val = strdup(v.string_val);
    } else if (v.type == VAL_ARRAY && v.array_val) {
        v.array_val->refcount++;
    }
    return out;
}

static Array* array_create_local(size_t capacity) {
    Array* arr = malloc(sizeof(Array));
    if (!arr) return NULL;
    arr->refcount = 1;
    arr->count = 0;
    arr->capacity = capacity > 0 ? capacity : 4;
    arr->items = calloc(arr->capacity, sizeof(Value));
    return arr;
}

static void array_push_local(Array* arr, Value v) {
    if (!arr) return;
    if (arr->count >= arr->capacity) {
        arr->capacity = arr->capacity > 0 ? arr->capacity * 2 : 4;
        arr->items = realloc(arr->items, arr->capacity * sizeof(Value));
    }
    arr->items[arr->count++] = clone_value(v);
}

static HttpRequest* http_request_create(void) {
    HttpRequest* req = calloc(1, sizeof(HttpRequest));
    return req;
}

static void http_request_free(HttpRequest* req) {
    if (!req) return;
    free(req->method);
    free(req->path);
    free(req->query_string);
    free(req->http_version);
    if (req->header_names && req->header_values) {
        for (int i = 0; i < req->header_count; i++) {
            free(req->header_names[i]);
            free(req->header_values[i]);
        }
    }
    free(req->header_names);
    free(req->header_values);
    free(req->body);
    free(req);
}

static const char* http_request_get_header(HttpRequest* req, const char* name) {
    if (!req || !name) return NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (req->header_names[i] && strcasecmp(req->header_names[i], name) == 0) {
            return req->header_values[i];
        }
    }
    return NULL;
}

static HttpRequest* http_request_parse(const char* data, ssize_t len) {
    if (!data || len <= 0) return NULL;
    // Incoming libuv buffers are not null-terminated; operate on a safe copy.
    char* buf = malloc((size_t)len + 1);
    if (!buf) return NULL;
    memcpy(buf, data, (size_t)len);
    buf[len] = '\0';

    HttpRequest* req = http_request_create();
    const char* end = buf + len;
    const char* line_end = strstr(buf, "\r\n");
    if (!line_end) {
        free(buf);
        http_request_free(req);
        return NULL;
    }
    size_t line_len = (size_t)(line_end - buf);
    char* request_line = strndup(buf, line_len);
    char* saveptr;
    char* method = strtok_r(request_line, " ", &saveptr);
    char* path = strtok_r(NULL, " ", &saveptr);
    char* version = strtok_r(NULL, " ", &saveptr);
    if (!method || !path || !version) {
        free(buf);
        free(request_line);
        http_request_free(req);
        return NULL;
    }
    req->method = strdup(method);
    req->http_version = strdup(version);
    char* query = strchr(path, '?');
    if (query) {
        *query = '\0';
        query++;
        req->query_string = strdup(query);
    }
    req->path = strdup(path);
    free(request_line);

    const char* headers_start = line_end + 2;
    const char* headers_end = strstr(headers_start, "\r\n\r\n");
    if (!headers_end) {
        free(buf);
        http_request_free(req);
        return NULL;
    }
    const char* cursor = headers_start;
    while (cursor < headers_end) {
        const char* eol = strstr(cursor, "\r\n");
        if (!eol || eol > headers_end) break;
        size_t hlen = (size_t)(eol - cursor);
        char* header_line = strndup(cursor, hlen);
        char* sep = strchr(header_line, ':');
        if (sep) {
            *sep = '\0';
            char* name = header_line;
            char* value = sep + 1;
            while (*value && isspace((unsigned char)*value)) value++;
            req->header_names = realloc(req->header_names, sizeof(char*) * (req->header_count + 1));
            req->header_values = realloc(req->header_values, sizeof(char*) * (req->header_count + 1));
            req->header_names[req->header_count] = strdup(name);
            req->header_values[req->header_count] = strdup(value);
            req->header_count++;
        }
        free(header_line);
        cursor = eol + 2;
    }

    const char* body_start = headers_end + 4;
    size_t remaining = (size_t)(end - body_start);
    const char* cl = http_request_get_header(req, "Content-Length");
    if (cl) {
        size_t blen = (size_t)strtoul(cl, NULL, 10);
        if (blen <= remaining) {
            req->body_length = blen;
            req->body = malloc(blen + 1);
            memcpy(req->body, body_start, blen);
            req->body[blen] = '\0';
        }
    } else if (remaining > 0) {
        req->body_length = remaining;
        req->body = malloc(remaining + 1);
        memcpy(req->body, body_start, remaining);
        req->body[remaining] = '\0';
    }

    free(buf);
    return req;
}

static HttpResponse* http_response_create(int status_code, const char* status_text) {
    HttpResponse* resp = calloc(1, sizeof(HttpResponse));
    resp->status_code = status_code;
    resp->status_text = strdup(status_text ? status_text : "");
    return resp;
}

static void http_response_add_header(HttpResponse* resp, const char* name, const char* value) {
    if (!resp || !name || !value) return;
    resp->header_names = realloc(resp->header_names, sizeof(char*) * (resp->header_count + 1));
    resp->header_values = realloc(resp->header_values, sizeof(char*) * (resp->header_count + 1));
    resp->header_names[resp->header_count] = strdup(name);
    resp->header_values[resp->header_count] = strdup(value);
    resp->header_count++;
}

static void http_response_set_body(HttpResponse* resp, const char* body, const char* content_type) {
    if (!resp) return;
    free(resp->body);
    resp->body_length = body ? strlen(body) : 0;
    if (resp->body_length > 0) {
        resp->body = strdup(body);
    } else {
        resp->body = NULL;
    }
    if (content_type) {
        http_response_add_header(resp, "Content-Type", content_type);
    }
}

static char* http_response_build(HttpResponse* resp, size_t* out_len) {
    if (!resp) return NULL;
    const char* status_text = resp->status_text ? resp->status_text : "";
    // ensure Content-Length header
    char len_buf[64];
    snprintf(len_buf, sizeof(len_buf), "%zu", resp->body_length);
    bool has_len = false;
    for (int i = 0; i < resp->header_count; i++) {
        if (resp->header_names[i] && strcasecmp(resp->header_names[i], "Content-Length") == 0) {
            has_len = true;
            break;
        }
    }
    if (!has_len) http_response_add_header(resp, "Content-Length", len_buf);
    bool has_server = false;
    for (int i = 0; i < resp->header_count; i++) {
        if (resp->header_names[i] && strcasecmp(resp->header_names[i], "Server") == 0) {
            has_server = true;
            break;
        }
    }
    if (!has_server) http_response_add_header(resp, "Server", "RADS/1.0");
    http_response_add_header(resp, "Connection", "close");

    size_t header_cap = 256 + (size_t)resp->header_count * 64;
    char* header_buf = malloc(header_cap);
    int written = snprintf(header_buf, header_cap, "HTTP/1.1 %d %s\r\n", resp->status_code, status_text);
    for (int i = 0; i < resp->header_count; i++) {
        written += snprintf(header_buf + written, header_cap - (size_t)written, "%s: %s\r\n", resp->header_names[i], resp->header_values[i]);
    }
    written += snprintf(header_buf + written, header_cap - (size_t)written, "\r\n");
    size_t total_len = (size_t)written + resp->body_length;
    char* out = malloc(total_len + 1);
    memcpy(out, header_buf, (size_t)written);
    if (resp->body_length > 0 && resp->body) {
        memcpy(out + written, resp->body, resp->body_length);
    }
    out[total_len] = '\0';
    free(header_buf);
    if (out_len) *out_len = total_len;
    return out;
}

static void http_response_free(HttpResponse* resp) {
    if (!resp) return;
    free(resp->status_text);
    if (resp->header_names && resp->header_values) {
        for (int i = 0; i < resp->header_count; i++) {
            free(resp->header_names[i]);
            free(resp->header_values[i]);
        }
    }
    free(resp->header_names);
    free(resp->header_values);
    free(resp->body);
    free(resp);
}

static bool should_keep_alive(HttpClientResponse* resp) {
    const char* conn = http_client_response_get_header(resp, "Connection");
    return conn && strcasecmp(conn, "keep-alive") == 0;
}

static ConnectionPool* global_connection_pool = NULL;

static ConnectionPool* connection_pool_create(int max_connections, int idle_timeout) {
    ConnectionPool* pool = calloc(1, sizeof(ConnectionPool));
    if (!pool) return NULL;
    pool->head = NULL;
    pool->max_connections = max_connections > 0 ? max_connections : 10;
    pool->idle_timeout = idle_timeout > 0 ? idle_timeout : 30;
    return pool;
}

static void connection_pool_free(ConnectionPool* pool) {
    if (!pool) return;
    ConnectionPoolEntry* cur = pool->head;
    while (cur) {
        ConnectionPoolEntry* next = cur->next;
        if (cur->handle) {
            uv_close((uv_handle_t*)cur->handle, on_http_client_close);
        }
        free(cur->host);
        free(cur);
        cur = next;
    }
    free(pool);
}

static void connection_pool_cleanup_idle(ConnectionPool* pool) {
    if (!pool) return;
    time_t now = time(NULL);
    ConnectionPoolEntry** cur = &pool->head;
    while (*cur) {
        ConnectionPoolEntry* entry = *cur;
        if (!entry->in_use && difftime(now, entry->last_used) > pool->idle_timeout) {
            *cur = entry->next;
            if (entry->handle) {
                uv_close((uv_handle_t*)entry->handle, on_http_client_close);
            }
            free(entry->host);
            free(entry);
            continue;
        }
        cur = &entry->next;
    }
}

static uv_tcp_t* connection_pool_get(ConnectionPool* pool, const char* host, int port) {
    if (!pool || !host) return NULL;
    connection_pool_cleanup_idle(pool);
    for (ConnectionPoolEntry* cur = pool->head; cur; cur = cur->next) {
        if (!cur->in_use && cur->port == port && cur->host && strcmp(cur->host, host) == 0) {
            cur->in_use = true;
            return cur->handle;
        }
    }
    return NULL;
}

static void connection_pool_release(ConnectionPool* pool, const char* host, int port, uv_tcp_t* handle) {
    if (!pool || !host || !handle) return;
    connection_pool_cleanup_idle(pool);
    ConnectionPoolEntry* match = NULL;
    int count = 0;
    for (ConnectionPoolEntry* cur = pool->head; cur; cur = cur->next) {
        count++;
        if (!match && cur->port == port && cur->host && strcmp(cur->host, host) == 0 && cur->handle == handle) {
            match = cur;
        }
    }
    if (match) {
        match->in_use = false;
        match->last_used = time(NULL);
        return;
    }
    ConnectionPoolEntry* entry = calloc(1, sizeof(ConnectionPoolEntry));
    entry->host = strdup(host);
    entry->port = port;
    entry->handle = handle;
    entry->last_used = time(NULL);
    entry->in_use = false;
    entry->next = pool->head;
    pool->head = entry;
    count++;
    if (count > pool->max_connections) {
        connection_pool_cleanup_idle(pool);
    }
}

static void on_http_client_close(uv_handle_t* handle) {
    if (!handle) return;
    free(handle);
}

static void on_http_client_connect(uv_connect_t* req, int status) {
    if (!req) return;
    HttpClientContext* ctx = req->data;
    if (ctx) {
        ctx->connect_done = true;
        ctx->connect_status = status;
    }
    free(req);
}

static void on_http_client_write(uv_write_t* req, int status) {
    if (req) {
        if (req->data) free(req->data);
        free(req);
    }
    if (status < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
    }
}

static void on_http_client_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    HttpClientContext* ctx = stream ? stream->data : NULL;
    if (nread > 0 && ctx) {
        if (ctx->buffer_used + (size_t)nread > ctx->buffer_size) {
            size_t new_size = ctx->buffer_size * 2;
            while (new_size < ctx->buffer_used + (size_t)nread) new_size *= 2;
            ctx->buffer = realloc(ctx->buffer, new_size);
            ctx->buffer_size = new_size;
        }
        memcpy(ctx->buffer + ctx->buffer_used, buf->base, (size_t)nread);
        ctx->buffer_used += (size_t)nread;
        if (http_client_response_parse(ctx->response, ctx->buffer, ctx->buffer_used)) {
            uv_read_stop(stream);
        }
    } else if (nread < 0 && ctx) {
        ctx->response->complete = true;
        uv_read_stop(stream);
    }
    if (buf && buf->base) free(buf->base);
}

static void on_http_client_timeout(uv_timer_t* timer) {
    if (!timer) return;
    HttpClientContext* ctx = timer->data;
    if (ctx && ctx->response) {
        ctx->timed_out = true;
        ctx->response->status_code = 0;
        free(ctx->response->status_text);
        ctx->response->status_text = strdup("Request Timeout");
        ctx->response->complete = true;
        if (ctx->tcp_handle) {
            uv_read_stop((uv_stream_t*)ctx->tcp_handle);
            uv_close((uv_handle_t*)ctx->tcp_handle, on_http_client_close);
        }
    }
    uv_timer_stop(timer);
    uv_close((uv_handle_t*)timer, NULL);
}

static HttpClientResponse* http_client_execute(HttpClientRequest* req) {
    if (!req) return NULL;
    if (!global_event_loop) interpreter_init_event_loop();
    if (!global_connection_pool) global_connection_pool = connection_pool_create(10, 30);
    connection_pool_cleanup_idle(global_connection_pool);

    uv_tcp_t* handle = connection_pool_get(global_connection_pool, req->host, req->port);
    bool reused = handle != NULL;
    if (!handle) {
        handle = malloc(sizeof(uv_tcp_t));
        uv_tcp_init(global_event_loop, handle);
        struct sockaddr_in dest;
        if (uv_ip4_addr(req->host, req->port, &dest) != 0) {
            free(handle);
            return NULL;
        }
        HttpClientContext tmp_ctx = {0};
        uv_connect_t* connect_req = malloc(sizeof(uv_connect_t));
        connect_req->data = &tmp_ctx;
        tmp_ctx.connect_done = false;
        tmp_ctx.connect_status = 0;
        uv_tcp_connect(connect_req, handle, (const struct sockaddr*)&dest, on_http_client_connect);
        while (!tmp_ctx.connect_done) {
            uv_run(global_event_loop, UV_RUN_ONCE);
        }
        if (tmp_ctx.connect_status < 0) {
            uv_close((uv_handle_t*)handle, on_http_client_close);
            return NULL;
        }
    }

    size_t req_len = 0;
    char* req_data = http_client_request_build(req, &req_len);
    if (!req_data) {
        if (!reused) uv_close((uv_handle_t*)handle, on_http_client_close);
        return NULL;
    }

    HttpClientContext* ctx = calloc(1, sizeof(HttpClientContext));
    ctx->request = req;
    ctx->response = http_client_response_create();
    ctx->tcp_handle = handle;
    ctx->buffer_size = 8192;
    ctx->buffer_used = 0;
    ctx->buffer = malloc(ctx->buffer_size);
    ctx->headers_complete = false;
    ctx->content_length = 0;
    ctx->interp = NULL;
    ctx->connect_done = true;
    ctx->connect_status = 0;
    ctx->timed_out = false;
    ctx->timer_handle = malloc(sizeof(uv_timer_t));
    ctx->timer_handle->data = ctx;

    handle->data = ctx;

    uv_write_t* write_req = malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init(req_data, (unsigned int)req_len);
    write_req->data = req_data;
    uv_write(write_req, (uv_stream_t*)handle, &buf, 1, on_http_client_write);

    uv_read_start((uv_stream_t*)handle, alloc_buffer, on_http_client_read);

    uv_timer_init(global_event_loop, ctx->timer_handle);
    uv_timer_start(ctx->timer_handle, on_http_client_timeout, 30000, 0);

    while (!ctx->response->complete && !ctx->timed_out) {
        uv_run(global_event_loop, UV_RUN_ONCE);
    }

    if (ctx->timer_handle) {
        uv_timer_stop(ctx->timer_handle);
        uv_close((uv_handle_t*)ctx->timer_handle, NULL);
    }

    HttpClientResponse* resp = ctx->response;
    ctx->response = NULL;

    if (!ctx->timed_out && (reused || should_keep_alive(resp))) {
        connection_pool_release(global_connection_pool, req->host, req->port, handle);
    } else {
        uv_close((uv_handle_t*)handle, on_http_client_close);
    }

    free(ctx->buffer);
    free(ctx);
    return resp;
}

static RouteRegistry* route_registry_create(void) {
    RouteRegistry* reg = calloc(1, sizeof(RouteRegistry));
    return reg;
}

static void route_registry_free(RouteRegistry* reg) {
    if (!reg) return;
    RouteNode* cur = reg->head;
    while (cur) {
        RouteNode* next = cur->next;
        free(cur->path);
        free(cur->method);
        free(cur->static_dir);
        value_free(&cur->handler);
        free(cur);
        cur = next;
    }
    free(reg);
}

static bool route_registry_add(RouteRegistry* reg, const char* path, const char* method, Value handler) {
    if (!reg || !path) return false;
    RouteNode* node = calloc(1, sizeof(RouteNode));
    node->path = strdup(path);
    node->method = method ? strdup(method) : NULL;
    node->handler = handler;
    node->next = reg->head;
    reg->head = node;
    reg->count++;
#ifdef RADS_DEBUG_NET
    fprintf(stderr, "[NET] Route registered: %s %s\n", method ? method : "*", path);
#endif
    return true;
}

static bool route_registry_add_static(RouteRegistry* reg, const char* prefix, const char* dir) {
    if (!reg || !prefix || !dir) return false;
    RouteNode* node = calloc(1, sizeof(RouteNode));
    node->path = strdup(prefix);
    node->method = NULL;
    node->is_static = true;
    node->static_dir = strdup(dir);
    node->next = reg->head;
    reg->head = node;
    reg->count++;
#ifdef RADS_DEBUG_NET
    fprintf(stderr, "[NET] Static route registered: %s -> %s\n", prefix, dir);
#endif
    return true;
}

static RouteNode* route_registry_find(RouteRegistry* reg, const char* path, const char* method) {
    if (!reg || !path) return NULL;
    RouteNode* wildcard = NULL;
    for (RouteNode* cur = reg->head; cur; cur = cur->next) {
        if (cur->method && method && strcasecmp(cur->method, method) == 0 && strcmp(cur->path, path) == 0) {
            return cur;
        }
        if (!cur->method && strcmp(cur->path, path) == 0) {
            wildcard = cur;
        }
    }
    return wildcard;
}

static RouteNode* route_registry_find_static(RouteRegistry* reg, const char* path) {
    if (!reg || !path) return NULL;
    for (RouteNode* cur = reg->head; cur; cur = cur->next) {
        if (cur->is_static && strncmp(path, cur->path, strlen(cur->path)) == 0) {
            return cur;
        }
    }
    return NULL;
}

static bool path_has_parent_ref(const char* path) {
    if (!path) return true;
    return strstr(path, "..") != NULL;
}

static const char* guess_mime(const char* path) {
    if (!path) return "application/octet-stream";
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    ext++;
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) return "text/html";
    if (strcasecmp(ext, "css") == 0) return "text/css";
    if (strcasecmp(ext, "js") == 0) return "application/javascript";
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcasecmp(ext, "txt") == 0) return "text/plain";
    return "application/octet-stream";
}

// Parse URL format: http://host:port/path or https://host:port/path
// Default port: 80 for http, 443 for https
static bool url_parse(const char* url, char** host, int* port, char** path) {
    if (!url || !host || !port || !path) return false;
    *host = NULL;
    *path = NULL;
    *port = 80;

    const char* prefix_http = "http://";
    const char* prefix_https = "https://";
    const char* cursor = NULL;
    if (strncasecmp(url, prefix_http, strlen(prefix_http)) == 0) {
        cursor = url + strlen(prefix_http);
        *port = 80;
    } else if (strncasecmp(url, prefix_https, strlen(prefix_https)) == 0) {
        cursor = url + strlen(prefix_https);
        *port = 443;
    } else {
        return false;
    }

    const char* host_start = cursor;
    const char* slash = strchr(host_start, '/');
    const char* colon = strchr(host_start, ':');

    if (colon && slash && colon > slash) {
        colon = NULL; // colon belongs to path/query
    }

    const char* host_end = slash ? slash : url + strlen(url);
    if (colon && (!slash || colon < slash)) {
        host_end = colon;
    }

    if (host_end <= host_start) return false;

    size_t host_len = (size_t)(host_end - host_start);
    *host = strndup(host_start, host_len);
    if (!*host) return false;

    if (colon && (!slash || colon < slash)) {
        const char* port_str = colon + 1;
        char* endptr = NULL;
        long p = strtol(port_str, &endptr, 10);
        if (endptr == port_str || p <= 0 || p > 65535) {
            free(*host);
            *host = NULL;
            return false;
        }
        *port = (int)p;
    }

    if (slash) {
        *path = strdup(slash);
    } else {
        *path = strdup("/");
    }
    if (!*path) {
        free(*host);
        *host = NULL;
        return false;
    }
    return true;
}

static RadsBufferNode* buffer_push(RadsBufferNode** head, const char* data, size_t len) {
    RadsBufferNode* node = malloc(sizeof(RadsBufferNode));
    node->data = malloc(len + 1);
    memcpy(node->data, data, len);
    node->data[len] = '\0';
    node->len = len;
    node->next = NULL;
    if (!*head) {
        *head = node;
    } else {
        RadsBufferNode* cur = *head;
        while (cur->next) cur = cur->next;
        cur->next = node;
    }
    return node;
}

static HttpClientRequest* http_client_request_create(const char* method, const char* url) {
    if (!method || !url) return NULL;
    HttpClientRequest* req = calloc(1, sizeof(HttpClientRequest));
    if (!req) return NULL;
    req->method = strdup(method);
    req->url = strdup(url);
    if (!url_parse(url, &req->host, &req->port, &req->path)) {
        http_client_request_free(req);
        return NULL;
    }
    req->header_names = NULL;
    req->header_values = NULL;
    req->header_count = 0;
    req->body = NULL;
    req->body_length = 0;
    return req;
}

static void http_client_request_add_header(HttpClientRequest* req, const char* name, const char* value) {
    if (!req || !name || !value) return;
    req->header_names = realloc(req->header_names, sizeof(char*) * (req->header_count + 1));
    req->header_values = realloc(req->header_values, sizeof(char*) * (req->header_count + 1));
    req->header_names[req->header_count] = strdup(name);
    req->header_values[req->header_count] = strdup(value);
    req->header_count++;
}

static void http_client_request_set_body(HttpClientRequest* req, const char* body) {
    if (!req) return;
    free(req->body);
    req->body = body ? strdup(body) : NULL;
    req->body_length = body ? strlen(body) : 0;
    if (req->body_length > 0) {
        char len_buf[64];
        snprintf(len_buf, sizeof(len_buf), "%zu", req->body_length);
        http_client_request_add_header(req, "Content-Length", len_buf);
    }
}

static char* http_client_request_build(HttpClientRequest* req, size_t* out_len) {
    if (!req || !req->method || !req->host || !req->path) return NULL;
    bool has_host = false;
    bool has_connection = false;
    bool has_user_agent = false;
    bool has_content_length = false;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->header_names[i], "Host") == 0) has_host = true;
        if (strcasecmp(req->header_names[i], "Connection") == 0) has_connection = true;
        if (strcasecmp(req->header_names[i], "User-Agent") == 0) has_user_agent = true;
        if (strcasecmp(req->header_names[i], "Content-Length") == 0) has_content_length = true;
    }

    size_t header_cap = 256 + (size_t)req->header_count * 64 + (req->body_length > 0 ? req->body_length : 0);
    char* header_buf = malloc(header_cap);
    if (!header_buf) return NULL;
    int written = snprintf(header_buf, header_cap, "%s %s HTTP/1.1\r\n", req->method, req->path);
    if (!has_host) {
        written += snprintf(header_buf + written, header_cap - (size_t)written, "Host: %s:%d\r\n", req->host, req->port);
    }
    if (!has_user_agent) {
        written += snprintf(header_buf + written, header_cap - (size_t)written, "User-Agent: RADS/1.0\r\n");
    }
    if (!has_connection) {
        written += snprintf(header_buf + written, header_cap - (size_t)written, "Connection: close\r\n");
    }
    if (req->body_length > 0 && !has_content_length) {
        written += snprintf(header_buf + written, header_cap - (size_t)written, "Content-Length: %zu\r\n", req->body_length);
    }

    for (int i = 0; i < req->header_count; i++) {
        written += snprintf(header_buf + written, header_cap - (size_t)written, "%s: %s\r\n", req->header_names[i], req->header_values[i]);
    }
    written += snprintf(header_buf + written, header_cap - (size_t)written, "\r\n");

    size_t total_len = (size_t)written + req->body_length;
    char* out = malloc(total_len + 1);
    if (!out) {
        free(header_buf);
        return NULL;
    }
    memcpy(out, header_buf, (size_t)written);
    if (req->body_length > 0 && req->body) {
        memcpy(out + written, req->body, req->body_length);
    }
    out[total_len] = '\0';
    free(header_buf);
    if (out_len) *out_len = total_len;
    return out;
}

static void http_client_request_free(HttpClientRequest* req) {
    if (!req) return;
    free(req->method);
    free(req->url);
    free(req->host);
    free(req->path);
    if (req->header_names && req->header_values) {
        for (int i = 0; i < req->header_count; i++) {
            free(req->header_names[i]);
            free(req->header_values[i]);
        }
    }
    free(req->header_names);
    free(req->header_values);
    free(req->body);
    free(req);
}

static Value buffer_pop(RadsBufferNode** head) {
    if (!head || !*head) return make_null();
    RadsBufferNode* node = *head;
    *head = node->next;
    Value v = make_string(node->data);
    free(node->data);
    free(node);
    return v;
}

static HttpClientResponse* http_client_response_create(void) {
    HttpClientResponse* resp = calloc(1, sizeof(HttpClientResponse));
    if (!resp) return NULL;
    resp->status_code = 0;
    resp->status_text = NULL;
    resp->header_names = NULL;
    resp->header_values = NULL;
    resp->header_count = 0;
    resp->body = NULL;
    resp->body_length = 0;
    resp->complete = false;
    return resp;
}

static const char* http_client_response_get_header(HttpClientResponse* resp, const char* name) {
    if (!resp || !name) return NULL;
    for (int i = 0; i < resp->header_count; i++) {
        if (resp->header_names[i] && strcasecmp(resp->header_names[i], name) == 0) {
            return resp->header_values[i];
        }
    }
    return NULL;
}

static bool http_client_response_parse(HttpClientResponse* resp, const char* data, size_t len) {
    if (!resp || !data || len == 0) return false;
    const char* end = data + len;
    const char* line_end = strstr(data, "\r\n");
    if (!line_end) return false;
    size_t status_len = (size_t)(line_end - data);
    char* status_line = strndup(data, status_len);
    char* saveptr;
    char* http_version = strtok_r(status_line, " ", &saveptr);
    char* status_code_str = strtok_r(NULL, " ", &saveptr);
    char* status_text = strtok_r(NULL, "", &saveptr);
    if (!http_version || !status_code_str) {
        free(status_line);
        return false;
    }
    resp->status_code = atoi(status_code_str);
    free(resp->status_text);
    resp->status_text = status_text ? strdup(status_text) : strdup("");
    free(status_line);

    const char* headers_start = line_end + 2;
    const char* headers_end = strstr(headers_start, "\r\n\r\n");
    if (!headers_end) return false;
    const char* cursor = headers_start;
    while (cursor < headers_end) {
        const char* eol = strstr(cursor, "\r\n");
        if (!eol || eol > headers_end) break;
        size_t hlen = (size_t)(eol - cursor);
        char* header_line = strndup(cursor, hlen);
        char* sep = strchr(header_line, ':');
        if (sep) {
            *sep = '\0';
            char* name = header_line;
            char* value = sep + 1;
            while (*value && isspace((unsigned char)*value)) value++;
            resp->header_names = realloc(resp->header_names, sizeof(char*) * (resp->header_count + 1));
            resp->header_values = realloc(resp->header_values, sizeof(char*) * (resp->header_count + 1));
            resp->header_names[resp->header_count] = strdup(name);
            resp->header_values[resp->header_count] = strdup(value);
            resp->header_count++;
        }
        free(header_line);
        cursor = eol + 2;
    }

    const char* body_start = headers_end + 4;
    size_t remaining = (size_t)(end - body_start);
    const char* cl = http_client_response_get_header(resp, "Content-Length");
    if (cl) {
        size_t blen = (size_t)strtoul(cl, NULL, 10);
        if (blen <= remaining) {
            resp->body_length = blen;
            free(resp->body);
            resp->body = malloc(blen + 1);
            memcpy(resp->body, body_start, blen);
            resp->body[blen] = '\0';
            resp->complete = true;
            return true;
        }
    } else if (remaining > 0) {
        resp->body_length = remaining;
        free(resp->body);
        resp->body = malloc(remaining + 1);
        memcpy(resp->body, body_start, remaining);
        resp->body[remaining] = '\0';
        resp->complete = true;
        return true;
    }
    return false;
}

static void http_client_response_free(HttpClientResponse* resp) {
    if (!resp) return;
    free(resp->status_text);
    if (resp->header_names && resp->header_values) {
        for (int i = 0; i < resp->header_count; i++) {
            free(resp->header_names[i]);
            free(resp->header_values[i]);
        }
    }
    free(resp->header_names);
    free(resp->header_values);
    free(resp->body);
    free(resp);
}

static TcpHandleCtx* register_tcp_ctx(struct Interpreter* interp, uv_tcp_t* handle, const char* prefix, bool is_listener, bool owns_handle, bool is_http, const char* explicit_id) {
    TcpHandleCtx* ctx = calloc(1, sizeof(TcpHandleCtx));
    ctx->handle = handle;
    ctx->recv_queue = NULL;
    ctx->is_listener = is_listener;
    ctx->owns_handle = owns_handle;
    ctx->is_http = is_http;
    ctx->data_owner = false;
    ctx->data = NULL;
    ctx->interp = interp;
    if (explicit_id) {
        ctx->id = strdup(explicit_id);
    } else {
        char id_buf[64];
        snprintf(id_buf, sizeof(id_buf), "%s_%ld", prefix ? prefix : "tcp", next_handle_id++);
        ctx->id = strdup(id_buf);
    }
    ctx->next = tcp_ctx_head;
    tcp_ctx_head = ctx;
    handle->data = ctx;
    if (is_listener && is_http) {
        ctx->data = route_registry_create();
        ctx->data_owner = true;
    }
    return ctx;
}

static void unregister_tcp_ctx(TcpHandleCtx* ctx) {
    if (!ctx) return;
    TcpHandleCtx** cur = &tcp_ctx_head;
    while (*cur && *cur != ctx) {
        cur = &(*cur)->next;
    }
    if (*cur == ctx) {
        *cur = ctx->next;
    }
    buffer_free(ctx->recv_queue);
    if (ctx->data_owner && ctx->data) {
        route_registry_free((RouteRegistry*)ctx->data);
    }
    free(ctx->id);
    if (ctx->owns_handle) {
        free(ctx->handle);
    }
    free(ctx);
}

static TcpHandleCtx* find_tcp_ctx(const char* id) {
    if (!id) return NULL;
    for (TcpHandleCtx* cur = tcp_ctx_head; cur; cur = cur->next) {
        if (cur->id && strcmp(cur->id, id) == 0) return cur;
    }
    return NULL;
}

static void enqueue_data(TcpHandleCtx* ctx, const char* data, ssize_t len) {
    if (!ctx || len <= 0 || !data) return;
    buffer_push(&ctx->recv_queue, data, (size_t)len);
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)handle;
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error: %s\n", uv_strerror(status));
        return;
    }
    fprintf(stderr, "[NET] on_new_connection status=%d\n", status);
    TcpHandleCtx* server_ctx = server ? server->data : NULL;
    uv_tcp_t* client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, client);
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        TcpHandleCtx* ctx = register_tcp_ctx(server_ctx ? server_ctx->interp : NULL, client, "tcp_conn", false, true, server_ctx && server_ctx->is_http, NULL);
        if (server_ctx && server_ctx->is_http && server_ctx->data) {
            ctx->data = server_ctx->data;
            ctx->data_owner = false;
        }
        // Expose new client handle to interpreter via the listener's queue
        if (server_ctx) {
            enqueue_data(server_ctx, ctx->id, (ssize_t)strlen(ctx->id));
        }
        uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);
    } else {
        uv_close((uv_handle_t*)client, on_close);
    }
}

void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    TcpHandleCtx* ctx = client ? client->data : NULL;
    if (nread > 0) {
        enqueue_data(ctx, buf->base, nread);
        if (ctx && ctx->is_http) {
            http_handle_request(client, buf->base, nread);
        }
    } else if (nread < 0) {
        uv_close((uv_handle_t*)client, on_close);
    }
    if (buf->base) free(buf->base);
}

void on_write(uv_write_t* req, int status) {
    if (req->data) free(req->data);
    free(req);
    if (status < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
    }
}

void on_connect(uv_connect_t* req, int status) {
    if (status < 0) {
        fprintf(stderr, "Connect error: %s\n", uv_strerror(status));
    } else {
        uv_read_start(req->handle, alloc_buffer, on_read);
    }
    free(req);
}

void on_close(uv_handle_t* handle) {
    TcpHandleCtx* ctx = handle ? handle->data : NULL;
    unregister_tcp_ctx(ctx);
}

static void http_send_response(uv_stream_t* client, HttpResponse* resp) {
    if (!client || !resp) return;
    size_t resp_len = 0;
    char* serialized = http_response_build(resp, &resp_len);
    if (!serialized) return;
    uv_write_t* req = malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init(serialized, (unsigned int)resp_len);
    req->data = serialized;
    uv_write(req, client, &buf, 1, on_write);
    uv_close((uv_handle_t*)client, on_close);
}

static void http_handle_request(uv_stream_t* client, const char* data, ssize_t len) {
    if (len <= 0 || !data) {
        fprintf(stderr, "[NET] http_handle_request early exit len=%zd data=%p\n", (size_t)len, (void*)data);
        return;
    }
    TcpHandleCtx* ctx = client ? client->data : NULL;
    if (!ctx) {
        fprintf(stderr, "[NET] http_handle_request missing ctx\n");
        return;
    }
    RouteRegistry* reg = ctx ? (RouteRegistry*)ctx->data : NULL;
    if (!reg) {
        fprintf(stderr, "[NET] http_handle_request missing route registry\n");
    }
    HttpRequest* req = http_request_parse(data, len);
    HttpResponse* resp = NULL;
    if (!req) {
        resp = http_response_create(400, "Bad Request");
        http_response_set_body(resp, "Bad Request", "text/plain");
        http_send_response(client, resp);
        http_request_free(req);
        http_response_free(resp);
        return;
    }
    if (!reg) {
        resp = http_response_create(500, "Internal Server Error");
        http_response_set_body(resp, "No route registry", "text/plain");
        http_send_response(client, resp);
        http_request_free(req);
        http_response_free(resp);
        return;
    }

    // Static file handling
    RouteNode* static_route = route_registry_find_static(reg, req->path);
    if (static_route) {
        const char* prefix = static_route->path;
        const char* remainder = req->path + strlen(prefix);
        if (*remainder == '/') remainder++;
        if (path_has_parent_ref(remainder)) {
            resp = http_response_create(403, "Forbidden");
            http_response_set_body(resp, "Forbidden", "text/plain");
            http_send_response(client, resp);
            http_request_free(req);
            http_response_free(resp);
            return;
        }
        char fullpath[PATH_MAX];
        if (snprintf(fullpath, sizeof(fullpath), "%s/%s", static_route->static_dir ? static_route->static_dir : ".", remainder) >= (int)sizeof(fullpath)) {
            resp = http_response_create(414, "URI Too Long");
            http_response_set_body(resp, "Path too long", "text/plain");
            http_send_response(client, resp);
            http_request_free(req);
            http_response_free(resp);
            return;
        }
        struct stat st;
        if (stat(fullpath, &st) != 0 || !S_ISREG(st.st_mode)) {
            resp = http_response_create(404, "Not Found");
            http_response_set_body(resp, "Not Found", "text/plain");
            http_send_response(client, resp);
            http_request_free(req);
            http_response_free(resp);
            return;
        }
        FILE* f = fopen(fullpath, "rb");
        if (!f) {
            resp = http_response_create(500, "Internal Server Error");
            http_response_set_body(resp, "Failed to read file", "text/plain");
            http_send_response(client, resp);
            http_request_free(req);
            http_response_free(resp);
            return;
        }
        char* buf = malloc((size_t)st.st_size + 1);
        if (!buf) {
            fclose(f);
            resp = http_response_create(500, "Internal Server Error");
            http_response_set_body(resp, "OOM", "text/plain");
            http_send_response(client, resp);
            http_request_free(req);
            http_response_free(resp);
            return;
        }
        size_t readn = fread(buf, 1, (size_t)st.st_size, f);
        fclose(f);
        buf[readn] = '\0';
        resp = http_response_create(200, "OK");
        http_response_set_body(resp, buf, guess_mime(fullpath));
        free(buf);
        http_send_response(client, resp);
        http_request_free(req);
        http_response_free(resp);
        return;
    }

    RouteNode* route = route_registry_find(reg, req->path, req->method);
    if (!route) {
        resp = http_response_create(404, "Not Found");
        http_response_set_body(resp, "Not Found", "text/plain");
        http_send_response(client, resp);
        http_request_free(req);
        http_response_free(resp);
        return;
    }

    Value args[4];
    args[0] = make_string(req->path);
    args[1] = make_string(req->method ? req->method : "");
    args[2] = req->body ? make_string(req->body) : make_null();
    args[3] = req->query_string ? make_string(req->query_string) : make_null();
    if (route->handler.type != VAL_FUNCTION) {
        fprintf(stderr, "[NET] handler not function\n");
        resp = http_response_create(500, "Internal Server Error");
        http_response_set_body(resp, "Handler invalid", "text/plain");
        http_send_response(client, resp);
        http_request_free(req);
        http_response_free(resp);
        return;
    }
    fprintf(stderr, "[NET] executing handler path=%s method=%s\n", req->path, req->method ? req->method : "");
    Value resp_val = interpreter_execute_callback(route->handler, 4, args);
    for (int i = 0; i < 4; i++) value_free(&args[i]);

    int status = 200;
    const char* status_text = "OK";
    const char* content_type = "text/plain";
    char* body = NULL;

    if (resp_val.type == VAL_ARRAY && resp_val.array_val && resp_val.array_val->count >= 2) {
        Value code = resp_val.array_val->items[0];
        Value body_val = resp_val.array_val->items[1];
        Value ctype_val = resp_val.array_val->count >= 3 ? resp_val.array_val->items[2] : make_null();
        if (code.type == VAL_INT) {
            status = (int)code.int_val;
        }
        if (body_val.type == VAL_STRING && body_val.string_val) {
            body = strdup(body_val.string_val);
        }
        if (ctype_val.type == VAL_STRING && ctype_val.string_val) {
            content_type = ctype_val.string_val;
        }
    } else if (resp_val.type == VAL_STRING && resp_val.string_val) {
        body = strdup(resp_val.string_val);
    } else {
        status = 500;
        status_text = "Internal Server Error";
        body = strdup("Internal Error");
    }

    resp = http_response_create(status, status_text);
    http_response_set_body(resp, body ? body : "", content_type);
    http_send_response(client, resp);

    free(body);
    value_free(&resp_val);

    http_request_free(req);
    http_response_free(resp);
}
static Value make_bool(bool v) {
    Value val;
    val.type = VAL_BOOL;
    val.bool_val = v;
    return val;
}

static Value make_null(void) {
    Value v;
    v.type = VAL_NULL;
    return v;
}

static Value make_int(long long v) {
    Value val;
    val.type = VAL_INT;
    val.int_val = v;
    return val;
}

static Value make_response_tuple(int status, const char* body, const char* ctype) {
    Array* arr = array_create_local(3);
    Value code = make_int(status);
    Value bodyv = make_string(body ? body : "");
    Value ctypev = make_string(ctype ? ctype : "text/plain");
    array_push_local(arr, code);
    array_push_local(arr, bodyv);
    array_push_local(arr, ctypev);
    value_free(&code);
    value_free(&bodyv);
    value_free(&ctypev);
    Value out;
    out.type = VAL_ARRAY;
    out.array_val = arr;
    return out;
}

static Value make_json_response(const char* body) {
    return make_response_tuple(200, body ? body : "", "application/json");
}

static void buffer_free(RadsBufferNode* head) {
    while (head) {
        RadsBufferNode* next = head->next;
        free(head->data);
        free(head);
        head = next;
    }
}

static RadsTcpServer* global_server = NULL;
static RadsTcpClient* global_client = NULL;

static bool next_listen_is_http = false;

Value native_net_http_server(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        fprintf(stderr, " Net Error: Expected host and port for http_server\n");
        return make_null();
    }
    fprintf(stderr, "[NET] http_server host=%s port=%lld\n", args[0].string_val, args[1].int_val);
    next_listen_is_http = true;
    return native_net_tcp_listen(interp, argc - 1, &args[1]);
}

// Register a dynamic route; optional 4th arg is method.
Value native_net_route(struct Interpreter* interp, int argc, Value* args) {
    if (argc < 3 || args[1].type != VAL_STRING || args[2].type != VAL_FUNCTION) {
        fprintf(stderr, "⚠️ Net Error: Expected server, path, and handler function for route\n");
        return make_bool(false);
    }
    TcpHandleCtx* ctx = find_tcp_ctx(args[0].string_val);
    if (!ctx || !ctx->is_listener || !ctx->is_http) {
        fprintf(stderr, "⚠️ Net Error: Unknown or non-http server handle\n");
        return make_bool(false);
    }
    if (!ctx->data) {
        ctx->data = route_registry_create();
        ctx->data_owner = true;
    }
    RouteRegistry* reg = (RouteRegistry*)ctx->data;
    const char* method = NULL;
    if (argc >= 4 && args[3].type == VAL_STRING) {
        method = args[3].string_val;
    }
    Value handler = clone_value(args[2]);
    bool ok = route_registry_add(reg, args[1].string_val, method, handler);
    fprintf(stderr, "[NET] route added path=%s method=%s ok=%d\n", args[1].string_val, method ? method : "*", ok);
    return make_bool(ok);
}

// Register a static prefix to directory mapping.
Value native_net_static(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[0].type != VAL_STRING || args[1].type != VAL_STRING || args[2].type != VAL_STRING) {
        fprintf(stderr, "⚠️ Net Error: Expected server, prefix, and directory for static\n");
        return make_bool(false);
    }
    TcpHandleCtx* ctx = find_tcp_ctx(args[0].string_val);
    if (!ctx || !ctx->is_listener || !ctx->is_http) {
        fprintf(stderr, "⚠️ Net Error: Unknown or non-http server handle\n");
        return make_bool(false);
    }
    if (!ctx->data) {
        ctx->data = route_registry_create();
        ctx->data_owner = true;
    }
    RouteRegistry* reg = (RouteRegistry*)ctx->data;
    bool ok = route_registry_add_static(reg, args[1].string_val, args[2].string_val);
    fprintf(stderr, "[NET] static added prefix=%s dir=%s ok=%d\n", args[1].string_val, args[2].string_val, ok);
    return make_bool(ok);
}

Value native_net_json_response(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        return make_response_tuple(500, "Invalid JSON body", "text/plain");
    }
    return make_response_tuple(200, args[0].string_val, "application/json");
}

// Run the event loop (server serve)
Value native_net_serve(struct Interpreter* interp, int argc, Value* args) {
    (void)args;
    if (!interp) return make_bool(false);
    fprintf(stderr, "[NET] serve entering event loop\n");
    interpreter_run_event_loop();
    fprintf(stderr, "[NET] serve exit\n");
    return make_bool(true);
}

Value native_net_http_get(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "⚠️ Net Error: Expected URL for http_get\n");
        return make_string("");
    }
    return make_string("{\"status\":\"ok\",\"message\":\"simulated response\"}");
}

Value native_net_tcp_listen(struct Interpreter* interp, int argc, Value* args) {
    if (!global_event_loop) interpreter_init_event_loop();
    if (argc < 1 || args[0].type != VAL_INT) {
        fprintf(stderr, "⚠️ Net Error: Expected port for tcp_listen\n");
        return make_null();
    }
    int port = (int)args[0].int_val;
    RadsTcpServer* server = calloc(1, sizeof(RadsTcpServer));
    server->loop = global_event_loop;
    server->port = port;
    server->host = strdup("0.0.0.0");

    int r = uv_tcp_init(server->loop, &server->handle);
    if (r != 0) {
        fprintf(stderr, "uv_tcp_init error: %s\n", uv_strerror(r));
        free(server->host);
        free(server);
        return make_null();
    }
    struct sockaddr_in addr;
    uv_ip4_addr(server->host, port, &addr);
    r = uv_tcp_bind(&server->handle, (const struct sockaddr*)&addr, 0);
    if (r != 0) {
        fprintf(stderr, "uv_tcp_bind error: %s\n", uv_strerror(r));
        uv_close((uv_handle_t*)&server->handle, NULL);
        free(server->host);
        free(server);
        return make_null();
    }
    r = uv_listen((uv_stream_t*)&server->handle, 128, on_new_connection);
    if (r != 0) {
        fprintf(stderr, "uv_listen error: %s\n", uv_strerror(r));
        uv_close((uv_handle_t*)&server->handle, NULL);
        free(server->host);
        free(server);
        return make_null();
    }
    TcpHandleCtx* ctx = register_tcp_ctx(interp, &server->handle, "tcp_listener", true, false, next_listen_is_http, NULL);
    next_listen_is_http = false;
    global_server = server;
    return make_string(ctx->id);
}

Value native_net_tcp_connect(struct Interpreter* interp, int argc, Value* args) {
    if (!global_event_loop) interpreter_init_event_loop();
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        fprintf(stderr, "⚠️ Net Error: Expected host and port for tcp_connect\n");
        return make_null();
    }
    const char* host = args[0].string_val;
    int port = (int)args[1].int_val;
    RadsTcpClient* client = calloc(1, sizeof(RadsTcpClient));
    client->loop = global_event_loop;
    client->host = strdup(host);
    client->port = port;

    int r = uv_tcp_init(client->loop, &client->handle);
    if (r != 0) {
        fprintf(stderr, "uv_tcp_init error: %s\n", uv_strerror(r));
        free(client->host);
        free(client);
        return make_null();
    }
    TcpHandleCtx* ctx = register_tcp_ctx(interp, &client->handle, "tcp_client", false, false, false, NULL);

    struct sockaddr_in dest;
    uv_ip4_addr(host, port, &dest);
    uv_connect_t* connect_req = malloc(sizeof(uv_connect_t));
    connect_req->data = client;
    r = uv_tcp_connect(connect_req, &client->handle, (const struct sockaddr*)&dest, on_connect);
    if (r != 0) {
        fprintf(stderr, "uv_tcp_connect error: %s\n", uv_strerror(r));
        uv_close((uv_handle_t*)&client->handle, NULL);
        free(connect_req);
        free(client->host);
        free(client);
        return make_null();
    }

    global_client = client;
    return make_string(ctx->id);
}

Value native_net_send(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "⚠️ Net Error: Expected socket and data for send\n");
        return make_null();
    }
    TcpHandleCtx* ctx = find_tcp_ctx(args[0].string_val);
    if (!ctx) {
        fprintf(stderr, "⚠️ Net Error: Unknown handle\n");
        return make_bool(false);
    }
    uv_write_t* req = malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init(strdup(args[1].string_val), (unsigned int)strlen(args[1].string_val));
    req->data = buf.base;
    int r = uv_write(req, (uv_stream_t*)ctx->handle, &buf, 1, on_write);
    if (r != 0) {
        fprintf(stderr, "uv_write error: %s\n", uv_strerror(r));
        free(buf.base);
        free(req);
        return make_bool(false);
    }
    return make_bool(true);
}

Value native_net_recv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "⚠️ Net Error: Expected socket for recv\n");
        return make_null();
    }
    TcpHandleCtx* ctx = find_tcp_ctx(args[0].string_val);
    if (!ctx) {
        fprintf(stderr, "⚠️ Net Error: Unknown handle\n");
        return make_null();
    }
    return buffer_pop(&ctx->recv_queue);
}

Value native_net_rest_get(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        fprintf(stderr, "⚠️ Net Error: Expected URL for GET\n");
        return make_null();
    }
    return make_string("{\"status\":\"xtreme\",\"data\":[1,2,3]}");
}

Value native_net_rest_post(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        fprintf(stderr, "⚠️ Net Error: Expected URL and body for POST\n");
        return make_null();
    }
    return make_bool(true);
}


void stdlib_net_register(void) {
    register_native("net.http_server", native_net_http_server);
    register_native("net.route", native_net_route);
    register_native("net.static", native_net_static);
    register_native("net.json_response", native_net_json_response);
    register_native("net.serve", native_net_serve);
    register_native("net.http_get", native_net_http_get);
    register_native("net.tcp_listen", native_net_tcp_listen);
    register_native("net.tcp_connect", native_net_tcp_connect);
    register_native("net.send", native_net_send);
    register_native("net.recv", native_net_recv);
    register_native("net.rest_get", native_net_rest_get);
    register_native("net.rest_post", native_net_rest_post);
}
