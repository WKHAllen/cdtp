/*
 * Utility functions and definitions for cdtp
 */

#ifndef CDTP_UTIL_H
#define CDTP_UTIL_H

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

#define CDTP_FALSE 0
#define CDTP_TRUE  1

#endif /* CDTP_UTIL_H */
