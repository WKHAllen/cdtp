/*
 * Utility functions and definitions for cdtp
 */

#ifndef CDTP_UTIL_H
#define CDTP_UTIL_H

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

// Export functions
#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

// Boolean values
#define CDTP_FALSE 0
#define CDTP_TRUE  1

// Global address family to use
#ifndef CDTP_ADDRESS_FAMILY
    #define CDTP_ADDRESS_FAMILY AF_INET
#endif

// INET and INET6 address string length
#define CDTP_INET_ADDRSTRLEN  22
#define CDTP_INET6_ADDRSTRLEN 65

// Global address string length
#if (CDTP_ADDRESS_FAMILY == AF_INET)
    #define CDTP_ADDRSTRLEN CDTP_INET_ADDRSTRLEN
#elif (CDTP_ADDRESS_FAMILY == AF_INET6)
    #define CDTP_ADDRSTRLEN CDTP_INET6_ADDRSTRLEN
#endif

// Default CDTP port
#ifndef CDTP_PORT
    #define CDTP_PORT 29275
#endif

// Keep track of whether or not the library has been initialized and exited
extern int CDTP_INIT;
extern int CDTP_EXIT;

// Initialize the library
int cdtp_init(void);

// Exit the library
void cdtp_exit(void);

#endif // CDTP_UTIL_H
