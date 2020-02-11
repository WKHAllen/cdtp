/*
 * Utility functions and definitions for cdtp
 */

#pragma once
#ifndef CDTP_UTIL_H
#define CDTP_UTIL_H

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <errno.h>
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

// CDTP error codes
#define CDTP_SUCCESS                     0
#define CDTP_SERVER_WINSOCK_INIT_FAILED  1
#define CDTP_SERVER_SOCK_INIT_FAILED     2
#define CDTP_SERVER_SETSOCKOPT_FAILED    3
#define CDTP_SERVER_CANNOT_RESTART       4
#define CDTP_SERVER_NOT_SERVING          5
#define CDTP_SERVER_ALREADY_SERVING      6
#define CDTP_SERVER_ADDRESS_FAILED       7
#define CDTP_SERVER_BIND_FAILED          8
#define CDTP_SERVER_LISTEN_FAILED        9
#define CDTP_SERVER_STOP_FAILED         10
#define CDTP_CLIENT_DOES_NOT_EXIT       11
#define CDTP_CLIENT_REMOVE_FAILED       12

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

// Error code for CDTP
extern int CDTP_ERROR;
// Error code for the layer underneath CDTP
extern int CDTP_ERROR_UNDER;

// Initialize the library
int _cdtp_init(void);

// Exit the library
void _cdtp_exit(void);

// Check if an error has occurred
int cdtp_error(void);

// Get the CDTP error code
int cdtp_get_error(void);

// Get the underlying layer error code
int cdtp_get_underlying_error(void);

// Set the errors
void _cdtp_set_error(int cdtp_err, int underlying_err);

// Set the errors using typical underlying layer error notifying methods
void _cdtp_set_err(int cdtp_err);

#endif // CDTP_UTIL_H
