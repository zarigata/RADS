#ifndef RADS_PLATFORM_H
#define RADS_PLATFORM_H

#ifdef _WIN32
#define RADS_PLATFORM_WINDOWS 1
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#define RADS_PLATFORM_UNIX 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#endif

#endif // RADS_PLATFORM_H
