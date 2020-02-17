#include "client.h"

EXPORT CDTPClient *cdtp_client(void (*on_recv        )(void *, size_t, void *),
                        void (*on_disconnected)(void *),
                        void *on_recv_arg, void *on_disconnected_arg,
                        int blocking, int event_blocking)
{
    // TODO: implement this function
}

EXPORT CDTPClient *cdtp_client_default(void (*on_recv        )(void *, size_t, void *),
                                void (*on_disconnected)(void *),
                                void *on_recv_arg, void *on_disconnected_arg)
{
    // TODO: implement this function
}

EXPORT void cdtp_client_connect(CDTPClient *client, char *host, int port)
{
    // TODO: implement this function
}

#ifdef _WIN32
EXPORT void cdtp_client_connect_host(CDTPClient *client, ULONG host, int port)
#else
EXPORT void cdtp_client_connect_host(CDTPClient *client, in_addr_t host, int port)
#endif
{
    // TODO: implement this function
}

EXPORT void cdtp_client_connect_default_host(CDTPClient *client, int port)
{
    // TODO: implement this function
}

EXPORT void cdtp_client_connect_default_port(CDTPClient *client, char *host)
{
    // TODO: implement this function
}

#ifdef _WIN32
EXPORT void cdtp_client_connect_host_default_port(CDTPClient *client, ULONG host)
#else
EXPORT void cdtp_client_connect_host_default_port(CDTPClient *client, in_addr_t host)
#endif
{
    // TODO: implement this function
}

EXPORT void cdtp_client_connect_default(CDTPClient *client)
{
    // TODO: implement this function
}

EXPORT void cdtp_client_disconnect(CDTPClient *client)
{
    // TODO: implement this function
}

EXPORT int cdtp_client_connected(CDTPClient *client)
{
    // TODO: implement this function
}

EXPORT struct sockaddr_in cdtp_client_addr(CDTPClient *client)
{
    // TODO: implement this function
}

EXPORT char *cdtp_client_host(CDTPClient *client)
{
    // TODO: implement this function
}

EXPORT int cdtp_client_port(CDTPClient *client)
{
    // TODO: implement this function
}

EXPORT void cdtp_client_send(CDTPClient *client, void *data, size_t data_len)
{
    // TODO: implement this function
}
