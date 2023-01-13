/*
 * Utility functions and definitions for cdtp.
 */

#pragma once
#ifndef CDTP_UTIL_H
#define CDTP_UTIL_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
#  include <WinSock2.h>
#  include <Windows.h>
#  include <WS2tcpip.h>
#else
#  include <unistd.h>
#  include <sys/socket.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <errno.h>
#  include <time.h>
#  include <limits.h>
#  include <stdint.h>
#endif

// Export functions
#ifdef _WIN32
#  define CDTP_EXPORT __declspec(dllexport)
#else
#  define CDTP_EXPORT __attribute__((visibility("default")))
#endif

// Test export functions
#ifdef CDTP_TEST
#  define CDTP_TEST_EXPORT CDTP_EXPORT
#else
#  define CDTP_TEST_EXPORT
#endif

// Boolean values
#define CDTP_FALSE 0
#define CDTP_TRUE  1

// CDTP error codes
#define CDTP_SUCCESS                     0
#define CDTP_WINSOCK_INIT_FAILED         1
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
#define CDTP_EVENT_THREAD_START_FAILED  15
#define CDTP_SERVE_THREAD_START_FAILED  16
#define CDTP_HANDLE_THREAD_START_FAILED 17
#define CDTP_SELECT_FAILED              18
#define CDTP_SOCKET_ACCEPT_FAILED       19
#define CDTP_SERVER_RECV_FAILED         20
#define CDTP_CLIENT_SOCK_INIT_FAILED    23
#define CDTP_CLIENT_CANNOT_RECONNECT    24
#define CDTP_CLIENT_ALREADY_CONNECTED   25
#define CDTP_CLIENT_ADDRESS_FAILED      26
#define CDTP_CLIENT_CONNECT_FAILED      27
#define CDTP_CLIENT_DISCONNECT_FAILED   28
#define CDTP_HANDLE_THREAD_NOT_CLOSING  29
#define CDTP_CLIENT_NOT_CONNECTED       30
#define CDTP_CLIENT_SEND_FAILED         31
#define CDTP_CLIENT_RECV_FAILED         32

// Global address family to use
#ifndef CDTP_ADDRESS_FAMILY
#  define CDTP_ADDRESS_FAMILY AF_INET
#endif

// INET and INET6 address string length
#define CDTP_INET_ADDRSTRLEN  22
#define CDTP_INET6_ADDRSTRLEN 65

// Global address string length
#if (CDTP_ADDRESS_FAMILY == AF_INET)
#  define CDTP_ADDRSTRLEN CDTP_INET_ADDRSTRLEN
#elif (CDTP_ADDRESS_FAMILY == AF_INET6)
#  define CDTP_ADDRSTRLEN CDTP_INET6_ADDRSTRLEN
#endif

// Default CDTP port
#ifndef CDTP_PORT
#  define CDTP_PORT 29275
#endif

// Default CDTP server listen backlog
#ifndef CDTP_SERVER_LISTEN_BACKLOG
#  define CDTP_SERVER_LISTEN_BACKLOG 8
#endif

// Length of the size portion of each message
#define CDTP_LENSIZE 5

// Amount of time to sleep between socket reads
#define CDTP_SLEEP_TIME 0.001

// Determine if a blocking error has occurred
// This is necessary because -Wlogical-op causes a compile-time error on machines where EAGAIN and EWOULDBLOCK are equal
#ifndef _WIN32
#  if EAGAIN == EWOULDBLOCK
#    define CDTP_EAGAIN_OR_WOULDBLOCK(e) (e == EAGAIN)
#  else
#    define CDTP_EAGAIN_OR_WOULDBLOCK(e) (e == EAGAIN || e == EWOULDBLOCK)
#  endif
#endif

// Keep track of whether the library has been initialized and exited
extern int CDTP_INIT;
extern int CDTP_EXIT;

// Error code for CDTP
extern int CDTP_ERROR;
// Error code for the layer underneath CDTP
extern int CDTP_ERROR_UNDER;

// Registered function to handle errors
extern int CDTP_ON_ERROR_REGISTERED;
extern void (*CDTP_ON_ERROR)(int, int, void*);
extern void* CDTP_ON_ERROR_ARG;

// Initialize the library
int _cdtp_init(void);

// Exit the library
void _cdtp_exit(void);

/*
 * Check if an error has occurred
 */
CDTP_EXPORT int cdtp_error(void);

/*
 * Get the CDTP error code
 */
CDTP_EXPORT int cdtp_get_error(void);

/*
 * Get the underlying layer error code
 */
CDTP_EXPORT int cdtp_get_underlying_error(void);

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
CDTP_EXPORT void cdtp_on_error(
    void (*on_error)(int, int, void*),
    void* arg
);

/*
 * Unregister the on_error function
 */
CDTP_EXPORT void cdtp_on_error_clear(void);

// Encode the size of a message
CDTP_TEST_EXPORT unsigned char* _cdtp_encode_message_size(size_t dec);

// Decode the size of a message
CDTP_TEST_EXPORT size_t _cdtp_decode_message_size(unsigned char* ascii);

// Construct a message
char* _cdtp_construct_message(void* data, size_t data_size);

// Deconstruct a message
void* _cdtp_deconstruct_message(char* message, size_t* data_size);

/*
 * Cross-platform wait function.
 * Useful for waiting short amounts of time between connecting to a server, sending data, disconnecting, etc.
 */
CDTP_EXPORT void cdtp_sleep(double seconds);

#ifdef _WIN32
// Convert a string to a wide character type.
wchar_t *_str_to_wchar(const char *str);

// Convert a wide character string to a string.
char *_wchar_to_str(const wchar_t *wchar);
#endif

#endif // CDTP_UTIL_H
