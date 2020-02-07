#include "util.h"

int CDTP_INIT = CDTP_FALSE;
int CDTP_EXIT = CDTP_FALSE;

int cdtp_init(void)
{
    if (CDTP_INIT != CDTP_TRUE)
    {
        CDTP_INIT = CDTP_TRUE;
        atexit(cdtp_exit);
#ifdef _WIN32
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa);
#else
        return 0;
#endif
    }
    return 0;
}

void cdtp_exit(void)
{
    if (CDTP_EXIT != CDTP_TRUE)
    {
        CDTP_EXIT = CDTP_TRUE;
#ifdef _WIN32
        WSACleanup();
#endif
    }
}
