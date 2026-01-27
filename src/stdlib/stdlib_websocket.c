#include "stdlib_websocket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define WS_MAX_CLIENTS 128
#define WS_BUFFER_SIZE 8192
#define WS_HANDSHAKE_TIMEOUT 5000

static int ws_handshake(int client_fd, const char* request);
static int ws_send_frame(int fd, const uint8_t* data, size_t length, WSMessageType type);
static int ws_decode_frame(int fd, WSMessage* message);

static uint64_t ntoh64(uint64_t value) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result = (result << 8) | ((value >> (i * 8)) & 0xFF);
    }
    return result;
}

static uint64_t hton64(uint64_t value) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= ((value >> (i * 8)) & 0xFF) << ((7 - i) * 8);
    }
    return result;
}

WebSocket* websocket_server_new(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return NULL;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, WS_MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        return NULL;
    }

    WebSocket* ws = (WebSocket*)calloc(1, sizeof(WebSocket));
    ws->fd = server_fd;
    ws->url = NULL;
    ws->is_server = true;
    ws->connected = false;
    ws->client_count = 0;
    ws->rx_len = 0;

    return ws;
}

WebSocket* websocket_client_new(const char* url) {
    WebSocket* ws = (WebSocket*)calloc(1, sizeof(WebSocket));
    ws->fd = -1;
    ws->url = strdup(url);
    ws->is_server = false;
    ws->connected = false;
    ws->rx_len = 0;

    return ws;
}

void websocket_free(WebSocket* ws) {
    if (!ws) return;

    if (ws->url) free(ws->url);

    if (ws->is_server && ws->fd >= 0) {
        close(ws->fd);
    }

    if (!ws->is_server && ws->fd >= 0) {
        close(ws->fd);
    }

    for (int i = 0; i < ws->client_count; i++) {
        if (ws->clients[i]) {
            websocket_close(ws->clients[i]);
            free(ws->clients[i]);
        }
    }

    free(ws);
}

int ws_handshake(int client_fd, const char* request) {
    const char* upgrade = strstr(request, "Upgrade: websocket");
    if (!upgrade) return -1;

    const char* key_start = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_start) return -1;

    key_start += 19;
    const char* key_end = strstr(key_start, "\r\n");
    if (!key_end) return -1;

    char key[128];
    int key_len = key_end - key_start;
    if (key_len > 127) key_len = 127;
    memcpy(key, key_start, key_len);
    key[key_len] = '\0';

    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        key);

    return send(client_fd, response, strlen(response), 0) > 0 ? 0 : -1;
}

static int ws_send_frame(int fd, const uint8_t* data, size_t length, WSMessageType type) {
    uint8_t frame[WS_BUFFER_SIZE + 14];
    int frame_len = 0;

    frame[0] = 0x80;

    switch (type) {
        case WS_MESSAGE_TEXT: frame[0] |= 0x01; break;
        case WS_MESSAGE_BINARY: frame[0] |= 0x02; break;
        case WS_MESSAGE_CLOSE: frame[0] |= 0x08; break;
        case WS_MESSAGE_PING: frame[0] |= 0x09; break;
        case WS_MESSAGE_PONG: frame[0] |= 0x0A; break;
    }

    if (length < 126) {
        frame[1] = (uint8_t)length;
        frame_len = 2;
    } else if (length < 65536) {
        frame[1] = 126;
        frame[2] = (length >> 8) & 0xFF;
        frame[3] = length & 0xFF;
        frame_len = 4;
    } else {
        frame[1] = 127;
        uint64_t len = hton64((uint64_t)length);
        memcpy(&frame[2], &len, 8);
        frame_len = 10;
    }

    memcpy(&frame[frame_len], data, length);
    frame_len += length;

    return send(fd, (char*)frame, frame_len, 0);
}

static int ws_decode_frame(int fd, WSMessage* message) {
    uint8_t header[14];
    ssize_t len = recv(fd, (char*)header, 2, 0);

    if (len <= 0) return -1;

    bool fin = (header[0] & 0x80) != 0;
    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;

    int header_len = 2;

    if (payload_len == 126) {
        len = recv(fd, (char*)&header[2], 2, 0);
        if (len != 2) return -1;
        payload_len = (header[2] << 8) | header[3];
        header_len = 4;
    } else if (payload_len == 127) {
        len = recv(fd, (char*)&header[2], 8, 0);
        if (len != 8) return -1;
        payload_len = ntoh64(*(uint64_t*)&header[2]);
        header_len = 10;
    }

    uint8_t mask[4];
    if (masked) {
        len = recv(fd, (char*)mask, 4, 0);
        if (len != 4) return -1;
        header_len += 4;
    }

    if (payload_len > WS_BUFFER_SIZE) {
        payload_len = WS_BUFFER_SIZE;
    }

    uint8_t* payload = (uint8_t*)malloc(payload_len + 1);
    len = recv(fd, (char*)payload, payload_len, 0);

    if (masked) {
        for (uint64_t i = 0; i < payload_len; i++) {
            payload[i] ^= mask[i % 4];
        }
    }

    payload[payload_len] = '\0';

    message->data = payload;
    message->length = payload_len;

    switch (opcode) {
        case 0x01: message->type = WS_MESSAGE_TEXT; break;
        case 0x02: message->type = WS_MESSAGE_BINARY; break;
        case 0x08: message->type = WS_MESSAGE_CLOSE; break;
        case 0x09: message->type = WS_MESSAGE_PING; break;
        case 0x0A: message->type = WS_MESSAGE_PONG; break;
        default: message->type = WS_MESSAGE_TEXT; break;
    }

    return 0;
}

int websocket_connect(WebSocket* ws) {
    if (!ws || ws->is_server) return -1;

    struct sockaddr_in server_addr;
    const char* host = ws->url + 7;
    int port = 80;

    const char* slash = strchr(host, '/');
    if (slash) {
        port = atoi(slash + 1);
        const char* colon = strchr(host, ':');
        if (colon) {
            port = atoi(colon + 1);
        }
    }

    ws->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ws->fd < 0) {
        perror("socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    const char* colon = strchr(host, ':');
    if (colon) {
        const char* slash2 = strchr(colon, '/');
        int host_len = slash2 ? slash2 - host - 7 : strlen(host) - 7;
        char hostname[256];
        memcpy(hostname, host + 7, host_len);
        hostname[host_len] = '\0';

        struct hostent* he = gethostbyname(hostname);
        if (!he) {
            fprintf(stderr, "Failed to resolve host: %s\n", hostname);
            close(ws->fd);
            return -1;
        }

        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    } else {
        server_addr.sin_addr.s_addr = inet_addr(host);
    }

    if (connect(ws->fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(ws->fd);
        ws->fd = -1;
        return -1;
    }

    char handshake[1024];
    snprintf(handshake, sizeof(handshake),
        "GET / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        ws->url);

    if (send(ws->fd, handshake, strlen(handshake), 0) <= 0) {
        close(ws->fd);
        ws->fd = -1;
        return -1;
    }

    char response[1024];
    ssize_t len = recv(ws->fd, response, sizeof(response) - 1, 0);
    if (len <= 0 || !strstr(response, "101 Switching Protocols")) {
        close(ws->fd);
        ws->fd = -1;
        return -1;
    }

    ws->connected = true;
    return 0;
}

int websocket_listen(WebSocket* ws) {
    if (!ws || !ws->is_server) return -1;

    printf("WebSocket server listening on port %d\n", ws->fd);

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    while (ws->fd >= 0) {
        FD_ZERO(&readfds);
        FD_SET(ws->fd, &readfds);

        int max_fd = ws->fd;
        for (int i = 0; i < ws->client_count; i++) {
            if (ws->clients[i] && ws->clients[i]->fd >= 0) {
                FD_SET(ws->clients[i]->fd, &readfds);
                if (ws->clients[i]->fd > max_fd) {
                    max_fd = ws->clients[i]->fd;
                }
            }
        }

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0) {
            perror("select");
            break;
        }

        if (activity == 0) continue;

        if (FD_ISSET(ws->fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(ws->fd, (struct sockaddr*)&client_addr, &client_len);

            if (client_fd >= 0) {
                char* ip = inet_ntoa(client_addr.sin_addr);
                printf("Client connected: %s\n", ip);

                char request[1024];
                ssize_t len = recv(client_fd, request, sizeof(request) - 1, 0);
                request[len] = '\0';

                if (ws_handshake(client_fd, request) == 0) {
                    if (ws->client_count < WS_MAX_CLIENTS) {
                        WebSocket* client = (WebSocket*)calloc(1, sizeof(WebSocket));
                        client->fd = client_fd;
                        client->is_server = false;
                        client->connected = true;
                        client->rx_len = 0;
                        client->user_data = ws->user_data;

                        if (ws->on_connect) {
                            ws->on_connect(client, ws->user_data);
                        }

                        ws->clients[ws->client_count++] = client;
                    } else {
                        close(client_fd);
                    }
                } else {
                    close(client_fd);
                }
            }
        }

        for (int i = 0; i < ws->client_count; i++) {
            WebSocket* client = ws->clients[i];
            if (!client || client->fd < 0) continue;

            if (FD_ISSET(client->fd, &readfds)) {
                WSMessage message;
                if (ws_decode_frame(client->fd, &message) >= 0) {
                    if (message.type == WS_MESSAGE_CLOSE) {
                        printf("Client disconnected\n");
                        websocket_close(client);
                        continue;
                    }

                    if (message.type == WS_MESSAGE_PING) {
                        ws_send_frame(client->fd, message.data, message.length, WS_MESSAGE_PONG);
                    }

                    if (ws->on_message && (message.type == WS_MESSAGE_TEXT || message.type == WS_MESSAGE_BINARY)) {
                        ws->on_message(client, &message, ws->user_data);
                    }

                    free(message.data);
                }
            }
        }
    }

    return 0;
}

void websocket_send(WebSocket* ws, const char* message, size_t length) {
    if (!ws || ws->fd < 0) return;

    WSMessageType type = WS_MESSAGE_TEXT;
    ws_send_frame(ws->fd, (const uint8_t*)message, length, type);
}

void websocket_send_text(WebSocket* ws, const char* text) {
    websocket_send(ws, text, strlen(text));
}

void websocket_send_binary(WebSocket* ws, const uint8_t* data, size_t length) {
    if (!ws || ws->fd < 0) return;

    ws_send_frame(ws->fd, data, length, WS_MESSAGE_BINARY);
}

void websocket_broadcast(WebSocket* server, const char* message, size_t length, WebSocket* exclude) {
    if (!server || !server->is_server) return;

    for (int i = 0; i < server->client_count; i++) {
        if (server->clients[i] && server->clients[i] != exclude && server->clients[i]->fd >= 0) {
            websocket_send(server->clients[i], message, length);
        }
    }
}

void websocket_on_message(WebSocket* ws, WSMessageHandler handler, void* user_data) {
    if (!ws) return;
    ws->on_message = handler;
    ws->user_data = user_data;
}

void websocket_on_connect(WebSocket* ws, WSConnectHandler handler, void* user_data) {
    if (!ws) return;
    ws->on_connect = handler;
    ws->user_data = user_data;
}

void websocket_on_close(WebSocket* ws, WSCloseHandler handler, void* user_data) {
    if (!ws) return;
    ws->on_close = handler;
    ws->user_data = user_data;
}

void websocket_close(WebSocket* ws) {
    if (!ws || ws->fd < 0) return;

    ws_send_frame(ws->fd, NULL, 0, WS_MESSAGE_CLOSE);
    close(ws->fd);
    ws->fd = -1;
    ws->connected = false;

    if (ws->on_close) {
        ws->on_close(ws, ws->user_data);
    }
}

int websocket_poll(WebSocket* ws, int timeout_ms) {
    if (!ws || ws->fd < 0) return -1;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(ws->fd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    return select(ws->fd + 1, &readfds, NULL, NULL, &timeout);
}

const char* websocket_get_remote_address(WebSocket* ws) {
    static char addr_str[64];
    if (!ws || ws->fd < 0) return "unknown";

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getpeername(ws->fd, (struct sockaddr*)&addr, &len);

    snprintf(addr_str, sizeof(addr_str), "%s:%d",
        inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    return addr_str;
}

int websocket_get_port(WebSocket* ws) {
    if (!ws || ws->fd < 0) return -1;

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(ws->fd, (struct sockaddr*)&addr, &len);

    return ntohs(addr.sin_port);
}
