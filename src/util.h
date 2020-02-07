/*
 * Utility functions and definitions for cdtp
 */

#ifndef CDTP_UTIL_H
#define CDTP_UTIL_H

#include <stdlib.h>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
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

// Keep track of whether or not the library has been initialized and exited
extern int CDTP_INIT;
extern int CDTP_EXIT;

// Initialize the library
int cdtp_init(void);

// Exit the library
void cdtp_exit(void);

#endif // CDTP_UTIL_H
