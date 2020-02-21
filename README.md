# C Data Transfer Protocol

CDTP is a cross platform networking library written in C. It is based on [dtplib](https://github.com/WKHAllen/dtplib), a similar project written in Python.

## Compatibility

The library is written and tested on Windows and Linux. It is expected to work on macOS and related platforms as well.

## Checking for errors

Errors can occur in libraries CDTP depends on. Because of this, CDTP provides several ways of checking if errors have occurred.

### cdtp_error

```c
int cdtp_error(void);
```

Check if an error has occurred. Returns `CDTP_FALSE` (0) or `CDTP_TRUE` (1). If a function has been registered using `cdtp_on_error`, this error will not be set. When this function is called, the set error will be cleared.

### cdtp_get_error

```c
int cdtp_get_error(void);
```

Get the CDTP error code. CDTP error codes are defined in [`util.h`](/src/util.h). If a function has been registered using `cdtp_on_error`, this error will not be set.

### cdtp_get_underlying_error

```c
int cdtp_get_underlying_error(void);
```

Get the underlying error code. Errors that show up here will be related to sockets, threads, etc. Some CDTP errors (i.e. `CDTP_SERVER_CANNOT_RESTART` and `CDTP_SERVER_ALREADY_SERVING`) will not set this error value.

### cdtp_on_error

```c
void cdtp_on_error(void (*on_error)(int, int, void *), void *arg);
```

Register a function to be called if an error occurs. The function takes two arguments. The first argument is a function take three arguments: an integer representing the CDTP error, an integer representing the underlying error, and a void pointer. The second argument is a void pointer to a variable that will be passed to the function provided as the first argument. When a function has been registered, errors will not be set, and the `cdtp_get_error` function will only return `CDTP_SUCCESS`.

### cdtp_on_error_clear

```c
void cdtp_on_error_clear(void);
```

Unregister a function set by `cdtp_on_error`.

### cdtp_sleep

```c
void cdtp_sleep(double seconds);
```

While not necessarily a function that has to do with checking errors, this can be useful for avoiding them. It can sometimes be necessary to wait between network operations, such as connect, send, etc. This function provides a cross platform way to do this.

## Server

CDTP provides the `CDTPServer` type, which represents a network server. The library also contains a variety of functions to manipulate the server.

### Creating a server

First, you'll want to create a server.

#### cdtp_server

```c
CDTPServer *cdtp_server(size_t max_clients,
                        void (*on_recv      )(int, void *, size_t, void *),
                        void (*on_connect   )(int, void *),
                        void (*on_disconnect)(int, void *),
                        void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg,
                        int blocking, int event_blocking);
```

Create a CDTPServer object.

`max_clients` is the maximum number of clients the server can handle at once.

`on_recv` is a function that will be called when a data packet is received from a client. It should take four arguments: an integer corresponding to the ID of the client who sent the packet, a void pointer to the data that was sent, a size_t value indicating the size of the data, and a void pointer.

`on_connect` is a function that will be called when a client connects. It should take two arguments: an integer corresponding to the ID of the client who connected, and a void pointer.

`on_disconnect` is a function that will be called when a client connects. It should take two arguments: an integer corresponding to the ID of the client who disconnected, and a void pointer.

`on_recv_arg` is a void pointer to the variable that will be passed to the `on_recv` function.

`on_connect_arg` is a void pointer to the variable that will be passed to the `on_connect` function.

`on_disconnect_arg` is a void pointer to the variable that will be passed to the `on_disconnect` function.

`blocking` is a boolean flag (i.e. `CDTP_FALSE` (0) or `CDTP_TRUE` (1)) that indicates whether the `cdtp_server_start` function will block. If set to false, the server will serve in a thread.

`event_blocking` is a boolean flag (i.e. `CDTP_FALSE` (0) or `CDTP_TRUE` (1)) that indicates whether the event functions (i.e. `on_recv`, `on_connect`, `on_disconnect`) will block. If set to false, the event functions will be called using threads.

#### cdtp_server_default

```c
CDTPServer *cdtp_server_default(size_t max_clients,
                                void (*on_recv      )(int, void *, size_t, void *),
                                void (*on_connect   )(int, void *),
                                void (*on_disconnect)(int, void *),
                                void *on_recv_arg, void *on_connect_arg, void *on_disconnect_arg);
```

Create a CDTPServer object. This function is exactly like `cdtp_server` except that the `blocking` and `event_blocking` arguments default to `CDTP_FALSE` (0).

### Starting the server

After creating a server, the next step is starting it. To do this, you again have several options.

#### cdtp_server_start

```c
void cdtp_server_start(CDTPServer *server, char *host, int port);
```

Start a CDTP server, given a host IP address and integer port. `host` can be either an IPv4 or IPv6 address. `port` must be an unused port between 1 and 65535.

#### cdtp_server_start_host

```c
void cdtp_server_start_host(CDTPServer *server, ULONG     host, int port);
void cdtp_server_start_host(CDTPServer *server, in_addr_t host, int port);
```

Start a CDTP server. `host` is an unsigned long (i.e. `INADDR_ANY`) on Windows, and an in_addr_t structure elsewhere. `port` is an integer representing an unused port.

#### cdtp_server_start_default_host

```c
void cdtp_server_start_default_host(CDTPServer *server, int port);
```

Start a CDTP server, specifying the port. The host will be set to `INADDR_ANY`.

#### cdtp_server_start_default_port

```c
void cdtp_server_start_default_port(CDTPServer *server, char *host);
```

Start a CDTP server, specifying the host as a string. The port will default to `CDTP_PORT` (29275).

#### cdtp_server_start_host_default_port

```c
void cdtp_server_start_host_default_port(CDTPServer *server, ULONG     host);
void cdtp_server_start_host_default_port(CDTPServer *server, in_addr_t host);
```

Start a CDTP server. `host` is an unsigned long (i.e. `INADDR_ANY`) on Windows, and an in_addr_t structure elsewhere. The port will default to `CDTP_PORT` (29275).

#### cdt_server_start_default

```c
void cdtp_server_start_default(CDTPServer *server);
```

Start a CDTP server using the default options. The host will be `INADDR_ANY` and port will be `CDTP_PORT` (29275).

### Stopping the server

When one needs a server to stop serving, one can use the following function to shut it down.

#### cdtp_server_stop

```c
void cdtp_server_stop(CDTPServer *server);
```

Stop a CDTP server.

### Sending data

CDTP provides two functions for sending data to clients.

#### cdtp_server_send

```c
void cdtp_server_send(CDTPServer *server, int client_id, void *data, size_t data_size);
```

Send data to a client, providing the client's ID, a void pointer to the data, and the size of the data.

#### cdtp_server_send_all

```c
void cdtp_server_send_all(CDTPServer *server, void *data, size_t data_size);
```

Send data to all clients, providing a void pointer to the data and the size of the data.

### Other server functions

A few other server functions are made available for various purposes.

#### cdtp_server_serving

```c
int cdtp_server_serving(CDTPServer *server);
```

Check if the server is serving. Returns `CDTP_FALSE` (0) or `CDTP_TRUE` (1).

#### cdtp_server_addr

```c
struct sockaddr_in cdtp_server_addr(CDTPServer *server);
```

Get the server address as a sockaddr_in structure.

#### cdtp_server_host

```c
char *cdtp_server_host(CDTPServer *server);
```

Get the server IP address as a string. `free()` will need to be called on the string after it has been used.

#### cdtp_server_port

```c
int cdtp_server_port(CDTPServer *server);
```

Get the port the server is running on.

#### cdtp_server_remove_client

```c
void cdtp_server_remove_client(CDTPServer *server, int client_id);
```

Disconnect a client, providing the client's ID.

## Client

Documentation on client types and functions will be provided once development on them is complete.
