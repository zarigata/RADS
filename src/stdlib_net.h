#ifndef RADS_STDLIB_NET_H
#define RADS_STDLIB_NET_H

#include "interpreter.h"

// Net Module registration
void stdlib_net_register(void);

// Native functions
Value native_net_http_server(struct Interpreter* interp, int argc, Value* args);
Value native_net_route(struct Interpreter* interp, int argc, Value* args);
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

#endif // RADS_STDLIB_NET_H
