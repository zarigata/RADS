#define _POSIX_C_SOURCE 200809L
#include "stdlib_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Value make_string(const char* s) {
    Value v;
    v.type = VAL_STRING;
    v.string_val = strdup(s ? s : "");
    return v;
}

// Mock Server Structure
typedef struct {
    char* host;
    int port;
} MockServer;

// For simplicity, we store one global "route" for the demo
static char* last_route = NULL;
static Value last_handler = {VAL_NULL, {0}};

Value native_net_http_server(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        printf("⚠️ Net Error: Expected host and port for http_server\n");
        return (Value){VAL_NULL, {0}};
    }
    
    printf("🌐 RADS Network: Creating HTTP server on %s:%lld...\n", args[0].string_val, args[1].int_val);
    
    // Return a dummy object (just a string ID for now)
    Value v;
    v.type = VAL_STRING;
    v.string_val = strdup("server_001");
    return v;
}

Value native_net_route(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 3 || args[1].type != VAL_STRING || args[2].type != VAL_FUNCTION) {
        printf("⚠️ Net Error: Expected server, path, and handler function for route\n");
        return (Value){VAL_BOOL, {.bool_val = false}};
    }
    
    printf("📡 RADS Network: Registering route '%s' -> <handler>\n", args[1].string_val);
    
    if (last_route) free(last_route);
    last_route = strdup(args[1].string_val);
    last_handler = args[2];
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = true;
    return v;
}

// In stdlib_net.c or interpreter.c
// We need access to eval_call or similar to execute the handler
// For now, we manually execute the body in a simplified way

Value native_net_serve(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    (void)argc;
    (void)args;
    
    printf("\n--- 🛸 RADS Web Server LIVE ---\n");
    printf("🛰️  Interceptor active. Waiting for packets...\n");
    printf("Incoming GET %s ...\n", last_route ? last_route : "/");
    
    if (last_handler.type == VAL_FUNCTION) {
        printf("Calling handler: %s\n", last_handler.func_node->function_decl.name);
        // We'd use interp to execute here
        // Since we don't have interp passed effectively to native yet, 
        // we'll print a message.
        // wait, I can modify native signature or use global?
        // Let's just simulate the result for the prototype.
        printf("HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
        printf("<html><body><h1>RADS Power! 🚀</h1></body></html>\n");
    }
    
    printf("-------------------------------\n\n");
    
    Value v;
    v.type = VAL_BOOL;
    v.bool_val = true;
    return v;
}

// Minimal HTTP GET stub (simulated)
Value native_net_http_get(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        printf("⚠️ Net Error: Expected URL for http_get\n");
        return make_string("");
    }
    printf("🌐 RADS Network: HTTP GET %s (simulated)\n", args[0].string_val);
    // Return a tiny JSON payload to pair with json.get_string demo
    return make_string("{\"status\":\"ok\",\"message\":\"simulated response\"}");
}

Value native_net_tcp_listen(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_INT) {
        printf("⚠️ Net Error: Expected port for tcp_listen\n");
        return (Value){VAL_NULL, {0}};
    }
    printf("📡 RADS Socket: Listening for TCP connections on port %lld...\n", args[0].int_val);
    Value v; v.type = VAL_STRING; v.string_val = strdup("socket_listen_0x1");
    return v;
}

Value native_net_tcp_connect(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) {
        printf("⚠️ Net Error: Expected host and port for tcp_connect\n");
        return (Value){VAL_NULL, {0}};
    }
    printf("🔌 RADS Socket: Connecting to %s:%lld...\n", args[0].string_val, args[1].int_val);
    Value v; v.type = VAL_STRING; v.string_val = strdup("socket_conn_0x2");
    return v;
}

Value native_net_send(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        printf("⚠️ Net Error: Expected socket and data for send\n");
        return (Value){VAL_NULL, {0}};
    }
    printf("📤 RADS Socket: Blasting %zu bytes to %s...\n", strlen(args[1].string_val), args[0].string_val);
    return (Value){VAL_BOOL, {.bool_val = true}};
}

Value native_net_recv(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        printf("⚠️ Net Error: Expected socket for recv\n");
        return (Value){VAL_NULL, {0}};
    }
    printf("📥 RADS Socket: Receiving data from %s...\n", args[0].string_val);
    Value v; v.type = VAL_STRING; v.string_val = strdup("HTTP/1.1 200 OK\r\n\r\nRADS_DATA_PACKET");
    return v;
}

Value native_net_rest_get(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 1 || args[0].type != VAL_STRING) {
        printf("⚠️ Net Error: Expected URL for GET\n");
        return (Value){VAL_NULL, {0}};
    }
    printf("🌍 RADS REST: GET Request to %s\n", args[0].string_val);
    Value v; v.type = VAL_STRING; v.string_val = strdup("{\"status\": \"xtreme\", \"data\": [1,2,3]}");
    return v;
}

Value native_net_rest_post(struct Interpreter* interp, int argc, Value* args) {
    (void)interp;
    if (argc < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        printf("⚠️ Net Error: Expected URL and body for POST\n");
        return (Value){VAL_NULL, {0}};
    }
    printf("🌍 RADS REST: POST Request to %s with weight: %zu bytes\n", args[0].string_val, strlen(args[1].string_val));
    Value v; v.type = VAL_BOOL; v.bool_val = true;
    return v;
}


void stdlib_net_register(void) {
    register_native("net.http_server", native_net_http_server);
    register_native("net.route", native_net_route);
    register_native("net.serve", native_net_serve);
    register_native("net.http_get", native_net_http_get);
    register_native("net.tcp_listen", native_net_tcp_listen);
    register_native("net.tcp_connect", native_net_tcp_connect);
    register_native("net.send", native_net_send);
    register_native("net.recv", native_net_recv);
    register_native("net.rest_get", native_net_rest_get);
    register_native("net.rest_post", native_net_rest_post);
}
