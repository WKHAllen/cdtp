/*
 * Client types and functions for cdtp
 */

#pragma once
#ifndef CDTP_CLIENT_H
#define CDTP_CLIENT_H

#include "defs.h"
#include "util.h"
#include "threading.h"

/*
 * Client creation/initialization
 * 
 * on_recv:             pointer to a function that will be called when a packet is received
 * on_connect:          pointer to a function that will be called when the client is disconnected
 * on_recv_arg:         a value that will be passed to the on_recv function
 * on_disconnected_arg: a value that will be passed to the on_disconnected function
 * blocking:            whether or not the cdtp_client_connect function and related functions will block
 * event_blocking:      whether or not on_recv, on_connect, and on_disconnect will block
 */
CDTPClient *cdtp_client(void (*on_recv        )(void *, size_t, void *),
                        void (*on_disconnected)(void *),
                        void *on_recv_arg, void *on_disconnected_arg,
                        int blocking, int event_blocking);

/*
 * Client creation/initialization
 * 
 * on_recv:             pointer to a function that will be called when a packet is received
 * on_connect:          pointer to a function that will be called when the client is disconnected
 * on_recv_arg:         a value that will be passed to the on_recv function
 * on_disconnected_arg: a value that will be passed to the on_disconnected function
 * 
 * blocking and event_blocking are set to false
 */
CDTPClient *cdtp_client_default(void (*on_recv        )(void *, size_t, void *),
                                void (*on_disconnected)(void *),
                                void *on_recv_arg, void *on_disconnected_arg);

/*
 * Connect to a server
 * 
 * client: the client object
 * host:   the host as a string
 * port:   the port as an integer
 */
void cdtp_client_connect(CDTPClient *client, char *host, int port);

/*
 * Connect to a server with a non-string host
 * 
 * client: the client object
 * host:   the host in the socket-library-provided format
 * port:   the port as an integer
 */
#ifdef _WIN32
void cdtp_client_connect_host(CDTPClient *client, ULONG host, int port);
#else
void cdtp_client_connect_host(CDTPClient *client, in_addr_t host, int port);
#endif

/*
 * Connect to a server with the default host
 * 
 * client: the client object
 * port:   the port as an integer
 * 
 * host is set to INADDR_ANY
 */
void cdtp_client_connect_default_host(CDTPClient *client, int port);

/*
 * Connect to a server with the default port
 * 
 * client: the client object
 * host:   the host as a string
 * 
 * port is set to CDTP_PORT
 */
void cdtp_client_connect_default_port(CDTPClient *client, char *host);

/*
 * Connect to a server with a non-string host and the default port
 * 
 * client: the client object
 * host:   the host in the socket-library-provided format
 * 
 * port is set to CDTP_PORT
 */
#ifdef _WIN32
void cdtp_client_connect_host_default_port(CDTPClient *client, ULONG host);
#else
void cdtp_client_connect_host_default_port(CDTPClient *client, in_addr_t host);
#endif

/*
 * Connect to a server with the default host and the default port
 * 
 * client: the client object
 * 
 * host is set to INADDR_ANY, port is set to CDTP_PORT
 */
void cdtp_client_connect_default(CDTPClient *client);

/*
 * Disconnect from the server and free up memory
 * 
 * client: the client object
 */
void cdtp_client_disconnect(CDTPClient *client);

/*
 * Check if the client is connected to a server
 * 
 * client: the client object
 */
int cdtp_client_connected(CDTPClient *client);

/*
 * Get the client address
 * 
 * client: the client object
 */
struct sockaddr_in cdtp_client_addr(CDTPClient *client);

/*
 * Get the client ip address
 * 
 * client: the client object
 * 
 * The returned value's memory will need to be freed after use
 */
char *cdtp_client_host(CDTPClient *client);

/*
 * Get the client port
 * 
 * client: the client object
 */
int cdtp_client_port(CDTPClient *client);

/*
 * Send data to the server
 * 
 * client:    the client object
 * data:      the data to send
 * data_size: the size of the data
 */
void cdtp_client_send(CDTPClient *client, void *data, size_t data_size);

// Call the handle function
void _cdtp_client_call_handle(CDTPClient *client);

// Client handle function
void _cdtp_client_handle(CDTPClient *client);

// Call the on_recv function
void _cdtp_client_call_on_recv(CDTPClient *client, void *data, size_t data_size);

// Call the on_connect function
void _cdtp_client_call_on_disconnected(CDTPClient *client);

#endif // CDTP_CLIENT_H
