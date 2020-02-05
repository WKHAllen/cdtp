/*
 * Client types and functions for cdtp
 */

#ifndef CDTP_CLIENT_H
#define CDTP_CLIENT_H

#include "util.h"

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #ifdef CDTP_WINSOCK_INIT
        #undef CDTP_WINSOCK_INIT
    #endif
#else
    #include <sys/socket.h>
#endif

// typedef struct
// {

// } CDTPClient;

#endif /* CDTP_CLIENT_H */
