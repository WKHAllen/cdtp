#include "util.h"

int CDTP_INIT = CDTP_FALSE;
int CDTP_EXIT = CDTP_FALSE;

int CDTP_ERROR = CDTP_SUCCESS;
int CDTP_ERROR_UNDER = 0;

int CDTP_ON_ERROR_REGISTERED = CDTP_FALSE;
void (*CDTP_ON_ERROR)(int, int, void*);
void* CDTP_ON_ERROR_ARG;

int _cdtp_init(void)
{
    if (CDTP_INIT != CDTP_TRUE) {
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
    if (CDTP_EXIT != CDTP_TRUE) {
        CDTP_EXIT = CDTP_TRUE;

#ifdef _WIN32
        WSACleanup();
#endif

    }
}

EXPORT int cdtp_error(void)
{
    if (CDTP_ERROR == CDTP_SUCCESS) {
        return CDTP_FALSE;
    }

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
    if (CDTP_ON_ERROR_REGISTERED != CDTP_TRUE) {
        CDTP_ERROR = cdtp_err;
        CDTP_ERROR_UNDER = underlying_err;
    }
    else {
        (*CDTP_ON_ERROR)(cdtp_err, underlying_err, CDTP_ON_ERROR_ARG);
    }
}

void _cdtp_set_err(int cdtp_err)
{
#ifdef _WIN32
    _cdtp_set_error(cdtp_err, WSAGetLastError());
#else
    _cdtp_set_error(cdtp_err, errno);
#endif
}

EXPORT void cdtp_on_error(
    void (*on_error)(int, int, void*),
    void* arg
)
{
    CDTP_ON_ERROR_REGISTERED = CDTP_TRUE;
    CDTP_ON_ERROR = on_error;
    CDTP_ON_ERROR_ARG = arg;
}

EXPORT void cdtp_on_error_clear(void)
{
    CDTP_ON_ERROR_REGISTERED = CDTP_FALSE;
}

unsigned char* _cdtp_encode_message_size(size_t size)
{
    unsigned char* encoded_size = malloc(CDTP_LENSIZE * sizeof(unsigned char));

    for (int i = CDTP_LENSIZE - 1; i >= 0; i--) {
        encoded_size[i] = size % 256;
        size = size >> 8;
    }

    return encoded_size;
}

size_t _cdtp_decode_message_size(unsigned char* encoded_size)
{
    size_t size = 0;

    for (int i = 0; i < CDTP_LENSIZE; i++) {
        size = size << 8;
        size += encoded_size[i];
    }

    return size;
}

char* _cdtp_construct_message(void* data, size_t data_size)
{
    // data_size should not be greater than 256 ^ CDTP_LENSIZE (or in this case, a tebibyte)
    char* data_str = (char*)data;
    char* message = malloc((CDTP_LENSIZE + data_size) * sizeof(char));
    unsigned char* size = _cdtp_encode_message_size(data_size);

    for (int i = 0; i < CDTP_LENSIZE; i++) {
        message[i] = size[i];
    }

    for (size_t i = 0; i < data_size; i++) {
        message[i + CDTP_LENSIZE] = data_str[i];
    }

    free(size);
    return message;
}

void* _cdtp_deconstruct_message(char* message, size_t* data_size)
{
    // only the first CDTP_LENSIZE bytes of message will be read as the size
    *data_size = _cdtp_decode_message_size((unsigned char*)message);
    char* data = malloc(*data_size * sizeof(char));

    for (size_t i = 0; i < *data_size; i++) {
        data[i] = message[i + CDTP_LENSIZE];
    }

    return (void*)data;
}

EXPORT void cdtp_sleep(double seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    struct timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = ((int)(seconds * 1000) % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}
