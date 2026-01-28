#ifndef RADS_WEBSOCKET_H
#define RADS_WEBSOCKET_H

#include "../core/interpreter.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    WS_MESSAGE_TEXT,
    WS_MESSAGE_BINARY,
    WS_MESSAGE_PING,
    WS_MESSAGE_PONG,
    WS_MESSAGE_CLOSE
} WSMessageType;

typedef struct {
    WSMessageType type;
    uint8_t* data;
    size_t length;
} WSMessage;

typedef struct WebSocket WebSocket;

struct WebSocket {
    int fd;
    char* url;
    bool is_server;
    bool connected;

    void (*on_message)(struct WebSocket* ws, WSMessage* message, void* user_data);
    void (*on_connect)(struct WebSocket* ws, void* user_data);
    void (*on_close)(struct WebSocket* ws, void* user_data);
    void* user_data;

    char rx_buffer[8192];
    int rx_len;

    struct WebSocket* clients[128];
    int client_count;
};

typedef void (*WSMessageHandler)(struct WebSocket* ws, WSMessage* message, void* user_data);
typedef void (*WSConnectHandler)(struct WebSocket* ws, void* user_data);
typedef void (*WSCloseHandler)(struct WebSocket* ws, void* user_data);

struct WebSocket* websocket_server_new(int port);
struct WebSocket* websocket_client_new(const char* url);
void websocket_free(struct WebSocket* ws);
int websocket_listen(struct WebSocket* ws);
int websocket_connect(struct WebSocket* ws);
void websocket_send(struct WebSocket* ws, const char* message, size_t length);
void websocket_send_text(struct WebSocket* ws, const char* text);
void websocket_send_binary(struct WebSocket* ws, const uint8_t* data, size_t length);
void websocket_on_message(struct WebSocket* ws, WSMessageHandler handler, void* user_data);
void websocket_on_connect(struct WebSocket* ws, WSConnectHandler handler, void* user_data);
void websocket_on_close(struct WebSocket* ws, WSCloseHandler handler, void* user_data);
void websocket_close(struct WebSocket* ws);
void stdlib_websocket_register(void);
void websocket_broadcast(struct WebSocket* server, const char* message, size_t length, struct WebSocket* exclude);
int websocket_poll(struct WebSocket* ws, int timeout_ms);
const char* websocket_get_remote_address(struct WebSocket* ws);
int websocket_get_port(struct WebSocket* ws);

#endif
