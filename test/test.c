#include "../src/cdtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#ifdef _WIN32
#  ifdef _WIN64
#    define PRI_SIZE_T PRIu64
#  else
#    define PRI_SIZE_T PRIu32
#  endif
#else
#  define PRI_SIZE_T "zu"
#endif

typedef struct _TestState {
    int receiving_random_message;
    size_t random_message_to_server_len;
    size_t random_message_to_client_len;
    char* random_message_to_server;
    char* random_message_to_client;
} TestState;

TestState* new_test_state(
    size_t random_message_to_server_len,
    size_t random_message_to_client_len,
    char* random_message_to_server,
    char* random_message_to_client
)
{
    TestState* test_state = malloc(sizeof(TestState));
    test_state->receiving_random_message = CDTP_FALSE;
    test_state->random_message_to_server_len = random_message_to_server_len;
    test_state->random_message_to_client_len = random_message_to_client_len;
    test_state->random_message_to_server = random_message_to_server;
    test_state->random_message_to_client = random_message_to_client;
    return test_state;
}

void cleanup_test_state(TestState* test_state)
{
    free(test_state->random_message_to_server);
    free(test_state->random_message_to_client);
    free(test_state);
}

int randint(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

char* randbytes(size_t size)
{
    char* bytes = malloc(size * sizeof(char));

    for (size_t i = 0; i < size; i++) {
        bytes[i] = (char)randint(0, 255);
    }

    return bytes;
}

char* voidp_to_str(void* data, size_t data_size)
{
    char* message = malloc((data_size + 1) * sizeof(char));
    memcpy(message, data, data_size);
    message[data_size] = '\0';
    return message;
}

void server_on_recv(size_t client_id, void* data, size_t data_size, void* arg)
{
    TestState* test_state = (TestState*)arg;

    if (test_state->receiving_random_message != CDTP_TRUE) {
        char* message = voidp_to_str(data, data_size);
        printf("Received data from client #%" PRI_SIZE_T ": %s (size %" PRI_SIZE_T ")\n", client_id, message, data_size);
        printf("Arg: %s\n", (char*)arg);
        free(message);
    }
    else {
        printf("Received large random message from client (size %" PRI_SIZE_T ", %" PRI_SIZE_T ")\n", data_size, test_state->random_message_to_server_len);
        assert(data_size == test_state->random_message_to_server_len);
        assert(memcmp(data, test_state->random_message_to_server, data_size) == 0);
    }

    free(data);
}

void server_on_connect(size_t client_id, void* arg)
{
    printf("Client #%" PRI_SIZE_T " connected\n", client_id);
    printf("Arg: %s\n", (char*)arg);
}

void server_on_disconnect(size_t client_id, void* arg)
{
    printf("Client #%" PRI_SIZE_T " disconnected\n", client_id);
    printf("Arg: %s\n", (char*)arg);
}

void client_on_recv(void* data, size_t data_size, void* arg)
{
    TestState* test_state = (TestState*)arg;

    if (test_state->receiving_random_message != CDTP_TRUE) {
        char* message = voidp_to_str(data, data_size);
        printf("Received data from server: %s (size %" PRI_SIZE_T ")\n", message, data_size);
        printf("Arg: %s\n", (char*)arg);
        free(message);
    }
    else {
        printf("Received large random message from server (size %" PRI_SIZE_T ", %" PRI_SIZE_T ")\n", data_size, test_state->random_message_to_client_len);
        assert(data_size == test_state->random_message_to_client_len);
        assert(memcmp(data, test_state->random_message_to_client, data_size) == 0);
    }

    free(data);
}

void client_on_disconnected(void* arg)
{
    printf("Unexpectedly disconnected from server\n");
    printf("Arg: %s\n", (char*)arg);
}

void on_err(int cdtp_err, int underlying_err, void* arg)
{
    printf("CDTP error:       %d\n", cdtp_err);
    printf("Underlying error: %d\n", underlying_err);
    printf("Arg: %s\n", (char*)arg);
    exit(EXIT_FAILURE);
}

void check_err(void)
{
    if (cdtp_error()) {
        on_err(cdtp_get_error(), cdtp_get_underlying_error(), NULL);
    }
}

int main(void)
{
    double wait_time = 0.1;

    // Generate large random messages
    srand(time(NULL));
    size_t random_message_to_server_len = randint(32768, 65535);
    size_t random_message_to_client_len = randint(65536, 82175); // fails on Linux at values >= 82176?
    char* random_message_to_server = randbytes(random_message_to_server_len);
    char* random_message_to_client = randbytes(random_message_to_client_len);
    printf("Large random message sizes: %" PRI_SIZE_T ", %" PRI_SIZE_T "\n", random_message_to_server_len, random_message_to_client_len);

    // Initialize test state
    TestState* test_state = new_test_state(
        random_message_to_server_len,
        random_message_to_client_len,
        random_message_to_server,
        random_message_to_client
    );

    printf("Running tests...\n");

    // Register error event
    cdtp_on_error(on_err, NULL);

    // Server initialization
    CDTPServer* server = cdtp_server(
        16,
        server_on_recv,
        server_on_connect,
        server_on_disconnect,
        test_state,
        test_state,
        test_state
    );

    // Server start
    char* host = "127.0.0.1";
    unsigned short port = CDTP_PORT;
    cdtp_server_start(server, host, port);

    // Get host and port
    char* server_host = cdtp_server_host(server);
    unsigned short server_port = cdtp_server_port(server);
    printf("Host: %s\n", server_host);
    printf("Port: %d\n", server_port);

    // Unregister error event
    cdtp_on_error_clear();

    // Test that the client does not exist
    cdtp_server_remove_client(server, 0);
    assert(cdtp_get_error() == CDTP_CLIENT_DOES_NOT_EXIST);

    // Register error event
    cdtp_on_error(on_err, NULL);

    cdtp_sleep(wait_time);

    // Client initialization
    CDTPClient* client = cdtp_client(
        client_on_recv,
        client_on_disconnected,
        test_state,
        test_state
    );

    // Client connect
    cdtp_client_connect(client, host, port);
    free(server_host);

    // Get host and port
    char* client_host = cdtp_client_host(client);
    int client_port = cdtp_client_port(client);
    printf("Host: %s\n", client_host);
    printf("Port: %d\n", client_port);
    free(client_host);

    cdtp_sleep(wait_time);

    // Client send
    char* client_message = "Hello, server.";
    cdtp_client_send(client, client_message, strlen(client_message));

    cdtp_sleep(wait_time);

    // Server send
    char* server_message = "Hello, client #0.";
    cdtp_server_send(server, 0, server_message, strlen(server_message));

    cdtp_sleep(wait_time);

    test_state->receiving_random_message = CDTP_TRUE;

    // Client send large message
    cdtp_client_send(client, random_message_to_server, random_message_to_server_len);

    cdtp_sleep(wait_time);

    // Server send large message
    cdtp_server_send_all(server, random_message_to_client, random_message_to_client_len);

    cdtp_sleep(wait_time);

    test_state->receiving_random_message = CDTP_FALSE;

    // Client disconnect
    cdtp_client_disconnect(client);

    cdtp_sleep(wait_time);

    // Server stop
    cdtp_server_stop(server);

    // Unregister error event
    cdtp_on_error_clear();

    // Test that server cannot be restarted
    cdtp_server_start(server, host, port);
    assert(cdtp_get_error() == CDTP_SERVER_CANNOT_RESTART);

    printf("Successfully passed all tests\n");
    cleanup_test_state(test_state);
    return 0;
}
