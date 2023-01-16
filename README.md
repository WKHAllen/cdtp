# Data Transfer Protocol for C

Modern cross-platform networking interfaces for C.

## Data Transfer Protocol

The Data Transfer Protocol (DTP) is a larger project to make ergonomic network programming available in any language.
See the full project [here](https://wkhallen.com/dtp/).

## Creating a server

A server can be built using the `CDTPServer` implementation:

```c
#include "cdtp.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Create a server that receives strings and returns the length of each string
void server_on_recv(CDTPServer *server, size_t client_id, void *data, size_t data_size, void *arg)
{
    // Send back the length of the string
    char *str_data = (char *) data;
    size_t str_len = strlen(str_data) + 1;
    assert(str_len == data_size);
    cdtp_server_send(server, client_id, &str_len, sizeof(size_t));
    // `free` always needs to be called on received data
    free(data);
}

void server_on_connect(CDTPServer *server, size_t client_id, void *arg)
{
    printf("Client with ID %zu connected\n", client_id);
}

void server_on_disconnect(CDTPServer *server, size_t client_id, void *arg)
{
    printf("Client with ID %zu disconnected\n", client_id);
}

int main(void)
{
    // Start the server
    CDTPServer *server = cdtp_server(server_on_recv, server_on_connect, server_on_disconnect, NULL, NULL, NULL);
    cdtp_server_start(server, "127.0.0.1", 29275);

    // Stop the server
    cdtp_sleep(1.0);
    cdtp_server_stop(server);
    cdtp_server_free(server);

    return 0;
}
```

## Creating a client

A client can be built using the `CDTPClient` implementation:

```c
#include "cdtp.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Create a client that sends a message to the server and receives the length of the message
void client_on_recv(CDTPClient *client, void *data, size_t data_size, void *arg)
{
    // Validate the response
    size_t *str_len = (size_t *) data;
    char *message = (char *) arg;
    printf("Received response from server: %zu\n", *str_len);
    assert(*str_len == strlen(message) + 1);
    // `free` always needs to be called on received data
    free(data);
}

void client_on_disconnected(CDTPClient *client, void *arg)
{
    printf("Unexpectedly disconnected from server\n");
}

int main(void) {
    // Connect to the server
    char *message = "Hello, server!";
    CDTPClient *client = cdtp_client(client_on_recv, client_on_disconnected, message, NULL);
    cdtp_client_connect(client, "127.0.0.1", 29275);

    // Send a message to the server
    cdtp_client_send(client, message, strlen(message) + 1);

    // Disconnect from the server
    cdtp_sleep(1.0);
    cdtp_client_disconnect(client);
    cdtp_client_free(client);

    return 0;
}
```

## Memory management

All data received is allocated on the heap. To prevent memory leaks, those who use the library must call `free(...)` on
these pieces of data at some point. The same goes for strings returned from the following functions:

- `cdtp_server_get_host(...)`
- `cdtp_server_get_client_host(...)`
- `cdtp_client_get_host(...)`
- `cdtp_client_get_server_host(...)`

## Serialization

Unlike [the C++ implementation](https://github.com/WKHAllen/cppdtp), the protocol cannot serialize types for you. All
send and receive operations deal with void pointers (`void *`) and their sizes (`size_t`).

## Compilation

The protocol has a few dependencies that must be included when compiling:

### Compiling on Windows

- Link Winsock (`-lWs2_32`)
- Link and provide headers for OpenSSL 3.0

### Compiling on other platforms

- Link pthread (`-lpthread`)
- Link and provide headers for OpenSSL 3.0

For more information on the compilation process, see the [Makefile](Makefile).

## Security

Information security comes included. Every message sent over a network interface is encrypted with AES-256. Key
exchanges are performed using a 2048-bit RSA key-pair.
