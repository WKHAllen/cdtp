/**
 * CDTP utilities.
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

// Export functions.
#ifdef _WIN32
#  define CDTP_EXPORT __declspec(dllexport)
#else
#  define CDTP_EXPORT __attribute__((visibility("default")))
#endif

// Test export functions.
#ifdef CDTP_TEST
#  define CDTP_TEST_EXPORT CDTP_EXPORT
#else
#  define CDTP_TEST_EXPORT
#endif

// Boolean values.
#define CDTP_FALSE 0
#define CDTP_TRUE  1

// CDTP error codes.
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
#define CDTP_SOCKET_ACCEPT_FAILED       18
#define CDTP_SERVER_RECV_FAILED         19
#define CDTP_CLIENT_SOCK_INIT_FAILED    20
#define CDTP_CLIENT_CANNOT_RECONNECT    21
#define CDTP_CLIENT_ALREADY_CONNECTED   22
#define CDTP_CLIENT_ADDRESS_FAILED      23
#define CDTP_CLIENT_CONNECT_FAILED      24
#define CDTP_CLIENT_DISCONNECT_FAILED   25
#define CDTP_HANDLE_THREAD_NOT_CLOSING  26
#define CDTP_CLIENT_NOT_CONNECTED       27
#define CDTP_CLIENT_SEND_FAILED         28
#define CDTP_CLIENT_RECV_FAILED         29
#define CDTP_OPENSSL_ERROR              30
#define CDTP_SERVER_KEY_EXCHANGE_FAILED 31
#define CDTP_CLIENT_KEY_EXCHANGE_FAILED 32

// Global address family to use.
#ifndef CDTP_ADDRESS_FAMILY
#  define CDTP_ADDRESS_FAMILY AF_INET
#endif

// INET address string length.
#define CDTP_INET_ADDRSTRLEN  22
// INET6 address string length.
#define CDTP_INET6_ADDRSTRLEN 65

// Global address string length.
#if (CDTP_ADDRESS_FAMILY == AF_INET)
#  define CDTP_ADDRSTRLEN CDTP_INET_ADDRSTRLEN
#elif (CDTP_ADDRESS_FAMILY == AF_INET6)
#  define CDTP_ADDRSTRLEN CDTP_INET6_ADDRSTRLEN
#endif

// Default CDTP port.
#ifndef CDTP_PORT
#  define CDTP_PORT 29275
#endif

// Default CDTP server listen backlog.
#ifndef CDTP_SERVER_LISTEN_BACKLOG
#  define CDTP_SERVER_LISTEN_BACKLOG 8
#endif

// Length of the size portion of each message.
#define CDTP_LENSIZE 5

// Amount of time to sleep between socket reads.
#define CDTP_SLEEP_TIME 0.001

// Determine if a blocking error has occurred.
// This is necessary because -Wlogical-op causes a compile-time error on machines where EAGAIN and EWOULDBLOCK are equal.
#ifndef _WIN32
#  if EAGAIN == EWOULDBLOCK
#    define CDTP_EAGAIN_OR_WOULDBLOCK(e) (e == EAGAIN)
#  else
#    define CDTP_EAGAIN_OR_WOULDBLOCK(e) (e == EAGAIN || e == EWOULDBLOCK)
#  endif
#endif

// Track whether the library has been initialized.
extern int CDTP_INIT;
// Track whether the library has exited.
extern int CDTP_EXIT;

// Error code for CDTP.
extern int CDTP_ERROR;
// Error code for the layer underneath CDTP.
extern int CDTP_ERROR_UNDER;

// Whether an error function has been registered.
extern int CDTP_ON_ERROR_REGISTERED;
// Registered function to handle errors.
extern void (*CDTP_ON_ERROR)(int, int, void *);
// Pointer to a value to pass to the registered error function.
extern void *CDTP_ON_ERROR_ARG;

/**
 * Initialize the library.
 *
 * @return The success status.
 */
int _cdtp_init(void);

/**
 * Cleanup on exit.
 */
void _cdtp_exit(void);

/**
 * Check if an error has occurred.
 *
 * @return If an error code is set.
 */
CDTP_EXPORT int cdtp_error(void);

/**
 * Get the CDTP error code.
 *
 * @return The CDTP error code.
 */
CDTP_EXPORT int cdtp_get_error(void);

/**
 * Get the underlying error code.
 *
 * @return The underlying error code. This is usually the value of `errno` or the value returned from `WSAGetLastError()`.
 */
CDTP_EXPORT int cdtp_get_underlying_error(void);

/**
 * Set the CDTP error and underlying error.
 *
 * @param cdtp_err The CDTP error code.
 * @param underlying_err The underlying error code.
 */
void _cdtp_set_error(int cdtp_err, int underlying_err);

/**
 * Set the CDTP error, setting the underlying error code from `errno` or `WSAGetLastError()`.
 *
 * @param cdtp_err The CDTP error code.
 */
void _cdtp_set_err(int cdtp_err);

/**
 * Register a function to be called when an error occurs.
 *
 * @param on_error A pointer to a function that will be called when an error is set.
 * @param arg A value that will be passed to the `on_error` event function.
 *
 * The `on_error` function should take three parameters:
 *   - an `int` representing the CDTP error code
 *   - an `int` representing the underlying error code
 *   - a `void *` containing the `arg` parameter
 */
CDTP_EXPORT void cdtp_on_error(
    void (*on_error)(int, int, void *),
    void *arg
);

/**
 * Unregister the `on_error` function.
 */
CDTP_EXPORT void cdtp_on_error_clear(void);

/**
 * Encode the size portion of a message.
 *
 * @param size The message size.
 * @return The message size encoded in bytes.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
CDTP_TEST_EXPORT unsigned char *_cdtp_encode_message_size(size_t size);

/**
 * Decode the size portion of a message.
 *
 * @param encoded_size The message size encoded in bytes.
 * @return The size of the message.
 */
CDTP_TEST_EXPORT size_t _cdtp_decode_message_size(unsigned char *encoded_size);

/**
 * Construct a message. The message size will always be `data_size + CDTP_LENSIZE` bytes.
 *
 * @param data The message data.
 * @param data_size The size of the message, in bytes.
 * @return The constructed message.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
char *_cdtp_construct_message(void *data, size_t data_size);

/**
 * Deconstruct a message.
 *
 * @param message The message to be deconstructed.
 * @param data_size The value to write the message size to.
 * @return The deconstructed message.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
void *_cdtp_deconstruct_message(char *message, size_t *data_size);

/**
 * Sleep for a number of seconds.
 *
 * @param seconds The number of seconds to sleep.
 */
CDTP_EXPORT void cdtp_sleep(double seconds);

#ifdef _WIN32
/**
 * Convert a string to a wide character type.
 *
 * @param str The string.
 * @return The wide character string.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
wchar_t *_str_to_wchar(const char *str);

/**
 * Convert a wide character string to a string.
 *
 * @param wchar The wide character string.
 * @return The string.
 *
 * Note that the returned value is allocated on the heap, and `free` will need to be called on it.
 */
char *_wchar_to_str(const wchar_t *wchar);
#endif

#endif // CDTP_UTIL_H
