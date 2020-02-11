#include "util.h"

int CDTP_INIT = CDTP_FALSE;
int CDTP_EXIT = CDTP_FALSE;

int CDTP_ERROR       = CDTP_SUCCESS;
int CDTP_ERROR_UNDER = 0;

int CDTP_ON_ERROR_REGISTERED = CDTP_FALSE;
void (*CDTP_ON_ERROR)(int, int, void *);
void *CDTP_ON_ERROR_ARG;

int _cdtp_init(void)
{
    if (CDTP_INIT != CDTP_TRUE)
    {
        CDTP_INIT = CDTP_TRUE;
        atexit(_cdtp_exit);
#ifdef _WIN32
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa);
#else
        return 0;
#endif
    }
    return 0;
}

void _cdtp_exit(void)
{
    if (CDTP_EXIT != CDTP_TRUE)
    {
        CDTP_EXIT = CDTP_TRUE;
#ifdef _WIN32
        WSACleanup();
#endif
    }
}

EXPORT int cdtp_error(void)
{
    if (CDTP_ERROR == CDTP_SUCCESS)
        return CDTP_FALSE;
    return CDTP_TRUE;
}

EXPORT int cdtp_get_error(void)
{
    int err = CDTP_ERROR;
    CDTP_ERROR = CDTP_SUCCESS;
    return err;
}

EXPORT int cdtp_get_underlying_error(void)
{
    int err = CDTP_ERROR_UNDER;
    CDTP_ERROR_UNDER = 0;
    return err;
}

void _cdtp_set_error(int cdtp_err, int underlying_err)
{
    if (CDTP_ON_ERROR_REGISTERED != CDTP_TRUE)
    {
        CDTP_ERROR = cdtp_err;
        CDTP_ERROR_UNDER = underlying_err;
    }
    else
        (*CDTP_ON_ERROR)(cdtp_err, underlying_err, CDTP_ON_ERROR_ARG);
}

void _cdtp_set_err(int cdtp_err)
{
#ifdef _WIN32
    _cdtp_set_error(cdtp_err, WSAGetLastError());
#else
    _cdtp_set_error(cdtp_err, errno);
#endif
}

EXPORT void cdtp_on_error(void (*on_error)(int, int, void *), void *arg)
{
    CDTP_ON_ERROR_REGISTERED = CDTP_TRUE;
    CDTP_ON_ERROR = on_error;
    CDTP_ON_ERROR_ARG = arg;
}

EXPORT void cdtp_on_error_nothing(void)
{
    CDTP_ON_ERROR_REGISTERED = CDTP_FALSE;
}
