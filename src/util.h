/*
 * Utility functions and definitions for cdtp
 */

#pragma once
#ifndef CDTP_UTIL_H
#define CDTP_UTIL_H

// Includes
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
    #include <Windows.h>
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
#define CDTP_SERVE_THREAD_NOT_CLOSING   11
#define CDTP_CLIENT_DOES_NOT_EXIST      12
#define CDTP_CLIENT_REMOVE_FAILED       13
#define CDTP_SERVER_SEND_FAILED         14
#define CDTP_THREAD_START_FAILED        15
#define CDTP_SELECT_FAILED              16
#define CDTP_SOCKET_ACCEPT_FAILED       17
#define CDTP_STATUS_SEND_FAILED         18
#define CDTP_SERVER_FULL                19

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

// Length of the size portion of each message
#define CDTP_LENSIZE 5

// Keep track of whether or not the library has been initialized and exited
extern int CDTP_INIT;
extern int CDTP_EXIT;

// Error code for CDTP
extern int CDTP_ERROR;
// Error code for the layer underneath CDTP
extern int CDTP_ERROR_UNDER;

// Registered function to handle errors
extern int CDTP_ON_ERROR_REGISTERED;
extern void (*CDTP_ON_ERROR)(int, int, void *);
extern void *CDTP_ON_ERROR_ARG;

// Initialize the library
int _cdtp_init(void);

// Exit the library
void _cdtp_exit(void);

/*
 * Check if an error has occurred
 */
int cdtp_error(void);

/*
 * Get the CDTP error code
 */
int cdtp_get_error(void);

/*
 * Get the underlying layer error code
 */
int cdtp_get_underlying_error(void);

// Set the errors
void _cdtp_set_error(int cdtp_err, int underlying_err);

// Set the errors using typical underlying layer error notifying methods
void _cdtp_set_err(int cdtp_err);

/*
 * Register a function to run when an error occurs
 * 
 * on_error: pointer to a function that will be called when an error occurs
 * arg:      a value that will be passed to the on_error function
 */
void cdtp_on_error(void (*on_error)(int, int, void *), void *arg);

/*
 * Unregister the on_error function
 */
void cdtp_on_error_clear(void);

// Convert decimal to ascii
char *_cdtp_dec_to_ascii(size_t dec);

// Convert ascii to decimal
size_t _cdtp_ascii_to_dec(char *ascii);

// Construct a message
char *_cdtp_construct_message(void *data, size_t data_size);

// Deconstruct a message
void *_cdtp_deconstruct_message(char *message, size_t *data_size);

#endif // CDTP_UTIL_H
