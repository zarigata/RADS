#ifndef RADS_STDLIB_NET_H
#define RADS_STDLIB_NET_H

#include "../core/interpreter.h"
#include "../core/platform.h"
#include <uv.h>

typedef struct RadsTcpServer {
    uv_tcp_t handle;
    uv_loop_t* loop;
    char* host;
    int port;
    void* data;
} RadsTcpServer;

typedef struct RadsTcpClient {
    uv_tcp_t handle;
    uv_loop_t* loop;
    char* host;
    int port;
} RadsTcpClient;

typedef struct RadsBufferNode {
    char* data;
    size_t len;
    struct RadsBufferNode* next;
} RadsBufferNode;

// Net Module registration
void stdlib_net_register(void);

// Native functions
Value native_net_http_server(struct Interpreter* interp, int argc, Value* args);
Value native_net_route(struct Interpreter* interp, int argc, Value* args);
Value native_net_static(struct Interpreter* interp, int argc, Value* args);
Value native_net_serve(struct Interpreter* interp, int argc, Value* args);

// Advanced Socket Primitives
Value native_net_tcp_listen(struct Interpreter* interp, int argc, Value* args);
Value native_net_tcp_connect(struct Interpreter* interp, int argc, Value* args);
Value native_net_send(struct Interpreter* interp, int argc, Value* args);
Value native_net_recv(struct Interpreter* interp, int argc, Value* args);

// High-level REST stubs
Value native_net_rest_get(struct Interpreter* interp, int argc, Value* args);
Value native_net_rest_post(struct Interpreter* interp, int argc, Value* args);
Value native_net_http_get(struct Interpreter* interp, int argc, Value* args);

// Callbacks
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void on_new_connection(uv_stream_t* server, int status);
void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
void on_write(uv_write_t* req, int status);
void on_connect(uv_connect_t* req, int status);
void on_close(uv_handle_t* handle);

#endif // RADS_STDLIB_NET_H
