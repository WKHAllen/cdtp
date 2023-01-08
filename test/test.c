#include "../src/cdtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

#ifdef _WIN32
#  define strdup _strdup
#endif

#define STR_SIZE(s) ((strlen(s) + 1) * sizeof(char))

#define TEST_ASSERT(condition) { \
    if (!(condition)) { \
        fprintf( \
            stderr, \
            "test assertion failed (%s:%d): %s\n", \
            __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
}

#define TEST_ASSERT_EQ(a, b) { \
    if (a != b) { \
        fprintf( \
            stderr, \
            "test equality assertion failed (%s:%d): %s != %s, %" PRI_SIZE_T " != %" PRI_SIZE_T "\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_NE(a, b) { \
    if (a == b) { \
        fprintf( \
            stderr, \
            "test inequality assertion failed (%s:%d): %s == %s, %" PRI_SIZE_T " == %" PRI_SIZE_T "\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_INT_EQ(a, b) { \
    if (a != b) { \
        fprintf( \
            stderr, \
            "test equality assertion failed (%s:%d): %s != %s, %d != %d\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_INT_NE(a, b) { \
    if (a == b) { \
        fprintf( \
            stderr, \
            "test inequality assertion failed (%s:%d): %s == %s, %d == %d\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_STR_EQ(a, b) { \
    if (strcmp(a, b) != 0) { \
        fprintf( \
            stderr, \
            "test string equality assertion failed (%s:%d): %s != %s, %s != %s\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_STR_NE(a, b) { \
    if (strcmp(a, b) == 0) { \
        fprintf( \
            stderr, \
            "test string inequality assertion failed (%s:%d): %s == %s, %s == %s\n", \
            __FILE__, __LINE__, #a, #b, a, b); \
        exit(1); \
    } \
}

#define TEST_ASSERT_ARRAY_EQ(a, b, size) { \
    for (size_t i = 0; i < size; i++) { \
        if (a[i] != b[i]) { \
            fprintf( \
                stderr, \
                "test array equality assertion failed (%s:%d): %s != %s at index %" PRI_SIZE_T ", with size = %" PRI_SIZE_T "\n", \
                __FILE__, __LINE__, #a, #b, i, size); \
            exit(1); \
        } \
    } \
}

#define TEST_ASSERT_MEM_EQ(a, b, size) { \
    if (memcmp(a, b, size) != 0) { \
        fprintf( \
            stderr, \
            "test memory equality assertion failed (%s:%d): %s != %s, with size = %" PRI_SIZE_T "\n", \
            __FILE__, __LINE__, #a, #b, size); \
        exit(1); \
    } \
}


#define TEST_ASSERT_ARRAY_MEM_EQ(a, b, size, item_size) { \
    for (size_t i = 0; i < size; i++) { \
        if (memcmp((void *) (a[i]), (void *) (b[i]), item_size) != 0) { \
            fprintf( \
                stderr, \
                "test array memory equality assertion failed (%s:%d): %s != %s at index %" PRI_SIZE_T ", with size = %" PRI_SIZE_T " and item size = %" PRI_SIZE_T "\n", \
                __FILE__, __LINE__, #a, #b, i, size, item_size); \
            exit(1); \
        } \
    } \
}

#define TEST_ASSERT_MESSAGES_EQ(a, b, size) { \
    for (size_t i = 0; i < size; i++) { \
        if (a[i]->data_size != b[i]->data_size) { \
            fprintf( \
                stderr, \
                "test message size equality assertion failed (%s:%d): %" PRI_SIZE_T " != %" PRI_SIZE_T " (%s != %s) at index %" PRI_SIZE_T ", with size = %" PRI_SIZE_T "\n", \
                __FILE__, __LINE__, a[i]->data_size, b[i]->data_size, #a, #b, i, size); \
            exit(1); \
        } \
        if (memcmp((void *) (a[i]->data), (void *) (b[i]->data), a[i]->data_size) != 0) { \
            fprintf( \
                stderr, \
                "test array memory equality assertion failed (%s:%d): %s != %s at index %" PRI_SIZE_T ", with size = %" PRI_SIZE_T "\n", \
                __FILE__, __LINE__, #a, #b, i, size); \
            exit(1); \
        } \
    } \
}

typedef struct _TestReceivedMessage {
    void *data;
    size_t data_size;
} TestReceivedMessage;

TestReceivedMessage *test_received_message(void *data, size_t data_size)
{
    TestReceivedMessage *message = (TestReceivedMessage *) malloc(sizeof(TestReceivedMessage));

    message->data = data;
    message->data_size = data_size;

    return message;
}

TestReceivedMessage *int_message(int data)
{
    int *data_ptr = (int *) malloc(sizeof(int));
    *data_ptr = data;

    return test_received_message(data_ptr, sizeof(int));
}

TestReceivedMessage *size_t_message(size_t data)
{
    size_t *data_ptr = (size_t *) malloc(sizeof(size_t));
    *data_ptr = data;

    return test_received_message(data_ptr, sizeof(size_t));
}

TestReceivedMessage *str_message(char *data)
{
    char *data_ptr = strdup(data);

    return test_received_message(data_ptr, STR_SIZE(data_ptr));
}

typedef struct _TestState {
    size_t server_receive_count_expected;
    size_t server_connect_count_expected;
    size_t server_disconnect_count_expected;
    size_t server_receive_count;
    size_t server_connect_count;
    size_t server_disconnect_count;
    TestReceivedMessage **server_received_expected;
    size_t *server_receive_client_ids_expected;
    size_t *server_connect_client_ids_expected;
    size_t *server_disconnect_client_ids_expected;
    TestReceivedMessage **server_received;
    size_t *server_receive_client_ids;
    size_t *server_connect_client_ids;
    size_t *server_disconnect_client_ids;
    size_t client_receive_count_expected;
    size_t client_disconnected_count_expected;
    size_t client_receive_count;
    size_t client_disconnected_count;
    TestReceivedMessage **client_received_expected;
    TestReceivedMessage **client_received;
} TestState;

TestState *test_state(
    size_t server_receive_count,
    size_t server_connect_count,
    size_t server_disconnect_count,
    TestReceivedMessage **server_received,
    size_t *server_receive_client_ids,
    size_t *server_connect_client_ids,
    size_t *server_disconnect_client_ids,
    size_t client_receive_count,
    size_t client_disconnected_count,
    TestReceivedMessage **client_received
)
{
    TestState *state = (TestState *) malloc(sizeof(TestState));

    state->server_receive_count_expected = server_receive_count;
    state->server_connect_count_expected = server_connect_count;
    state->server_disconnect_count_expected = server_disconnect_count;
    state->server_receive_count = 0;
    state->server_connect_count = 0;
    state->server_disconnect_count = 0;
    state->server_received_expected = server_received;
    state->server_receive_client_ids_expected = server_receive_client_ids;
    state->server_connect_client_ids_expected = server_connect_client_ids;
    state->server_disconnect_client_ids_expected = server_disconnect_client_ids;
    state->server_received = (TestReceivedMessage **) malloc(server_receive_count * sizeof(TestReceivedMessage *));
    state->server_receive_client_ids = (size_t *) malloc(server_receive_count * sizeof(size_t));
    state->server_connect_client_ids = (size_t *) malloc(server_connect_count * sizeof(size_t));
    state->server_disconnect_client_ids = (size_t *) malloc(server_disconnect_count * sizeof(size_t));
    state->client_receive_count_expected = client_receive_count;
    state->client_disconnected_count_expected = client_disconnected_count;
    state->client_receive_count = 0;
    state->client_disconnected_count = 0;
    state->client_received_expected = client_received;
    state->client_received = (TestReceivedMessage **) malloc(client_receive_count * sizeof(TestReceivedMessage *));

    return state;
}

void test_state_finish(TestState *state)
{
    TEST_ASSERT_EQ(state->server_receive_count, state->server_receive_count_expected)
    TEST_ASSERT_EQ(state->server_connect_count, state->server_connect_count_expected)
    TEST_ASSERT_EQ(state->server_disconnect_count, state->server_disconnect_count_expected)
    TEST_ASSERT_MESSAGES_EQ(state->server_received,
                            state->server_received_expected,
                            state->server_receive_count)
    TEST_ASSERT_ARRAY_EQ(state->server_receive_client_ids,
                         state->server_receive_client_ids_expected,
                         state->server_receive_count)
    TEST_ASSERT_ARRAY_EQ(state->server_connect_client_ids,
                         state->server_connect_client_ids_expected,
                         state->server_connect_count)
    TEST_ASSERT_ARRAY_EQ(state->server_disconnect_client_ids,
                         state->server_disconnect_client_ids_expected,
                         state->server_disconnect_count)
    TEST_ASSERT_EQ(state->client_receive_count, state->client_receive_count_expected)
    TEST_ASSERT_EQ(state->client_disconnected_count, state->client_disconnected_count_expected)
    TEST_ASSERT_MESSAGES_EQ(state->client_received,
                            state->client_received_expected,
                            state->client_receive_count)

    for (size_t i = 0; i < state->server_receive_count; i++) {
        free(state->server_received_expected[i]->data);
        free(state->server_received_expected[i]);
        free(state->server_received[i]->data);
        free(state->server_received[i]);
    }

    free(state->server_received);
    free(state->server_receive_client_ids);
    free(state->server_connect_client_ids);
    free(state->server_disconnect_client_ids);

    for (size_t i = 0; i < state->client_receive_count; i++) {
        free(state->client_received_expected[i]->data);
        free(state->client_received_expected[i]);
        free(state->client_received[i]->data);
        free(state->client_received[i]);
    }

    free(state->client_received);
    free(state);
}

void test_state_server_received(TestState *state, size_t client_id, void *data, size_t data_size)
{
    TEST_ASSERT(state->server_receive_count < state->server_receive_count_expected)

    state->server_received[state->server_receive_count] = (TestReceivedMessage *) malloc(sizeof(TestReceivedMessage));
    state->server_received[state->server_receive_count]->data = data;
    state->server_received[state->server_receive_count]->data_size = data_size;
    state->server_receive_client_ids[state->server_receive_count++] = client_id;
}

void test_state_server_connect(TestState *state, size_t client_id)
{
    TEST_ASSERT(state->server_connect_count < state->server_connect_count_expected)

    state->server_connect_client_ids[state->server_connect_count++] = client_id;
}

void test_state_server_disconnect(TestState *state, size_t client_id)
{
    TEST_ASSERT(state->server_disconnect_count < state->server_disconnect_count_expected)

    state->server_disconnect_client_ids[state->server_disconnect_count++] = client_id;
}

void test_state_client_received(TestState *state, void *data, size_t data_size)
{
    TEST_ASSERT(state->client_receive_count < state->client_receive_count_expected)

    state->client_received[state->client_receive_count] = (TestReceivedMessage *) malloc(sizeof(TestReceivedMessage));
    state->client_received[state->client_receive_count]->data = data;
    state->client_received[state->client_receive_count]->data_size = data_size;
    state->client_receive_count++;
}

void test_state_client_disconnected(TestState *state)
{
//    TEST_ASSERT(state->client_disconnected_count < state->client_disconnected_count_expected)

    state->client_disconnected_count++;
}

int rand_int(int min, int max)
{
    return min + (rand() % (max - min));
}

char* rand_bytes(size_t size)
{
    char* bytes = (char*) malloc(size * sizeof(char));

    for (size_t i = 0; i < size; i++) {
        bytes[i] = (char) rand_int(0, 256);
    }

    return bytes;
}

char* voidp_to_str(void* data, size_t data_size)
{
    char* message = (char*) malloc((data_size + 1) * sizeof(char));
    memcpy(message, data, data_size);
    message[data_size] = '\0';
    return message;
}

void server_on_recv(size_t client_id, void *data, size_t data_size, void *arg)
{
    TestState *state = (TestState *) arg;

    test_state_server_received(state, client_id, data, data_size);
}

void server_on_connect(size_t client_id, void *arg)
{
    TestState *state = (TestState *) arg;

    test_state_server_connect(state, client_id);
}

void server_on_disconnect(size_t client_id, void *arg)
{
    TestState *state = (TestState *) arg;

    test_state_server_disconnect(state, client_id);
}

void client_on_recv(void *data, size_t data_size, void *arg)
{
    TestState *state = (TestState *) arg;

    test_state_client_received(state, data, data_size);
}

void client_on_disconnected(void* arg)
{
    TestState *state = (TestState *) arg;

    test_state_client_disconnected(state);
}

void on_err(int cdtp_err, int underlying_err, void* arg)
{
    printf("CDTP error:               %d\n", cdtp_err);
    printf("Underlying error:         %d\n", underlying_err);
    perror("Underlying error message");
    printf("Arg:                      %s\n", (char *) arg);
    exit(EXIT_FAILURE);
}

void check_err(void)
{
    if (cdtp_error()) {
        on_err(cdtp_get_error(), cdtp_get_underlying_error(), NULL);
    }
}

typedef struct _Custom {
    int a;
    size_t b;
} Custom;

#define WAIT_TIME 0.1
#define MAX_CLIENTS 16
#define SERVER_HOST "0.0.0.0"
#define SERVER_PORT 29275
#define CLIENT_HOST "127.0.0.1"
#define CLIENT_PORT 29275
#define EMPTY {0}

/**
 * Test utility functions.
 */
void test_util(void)
{
    // Test message size encoding
    unsigned char expected_msg_size1[] = {0, 0, 0, 0, 0};
    unsigned char expected_msg_size2[] = {0, 0, 0, 0, 1};
    unsigned char expected_msg_size3[] = {0, 0, 0, 0, 255};
    unsigned char expected_msg_size4[] = {0, 0, 0, 1, 0};
    unsigned char expected_msg_size5[] = {0, 0, 0, 1, 1};
    unsigned char expected_msg_size6[] = {1, 1, 1, 1, 1};
    unsigned char expected_msg_size7[] = {1, 2, 3, 4, 5};
    unsigned char expected_msg_size8[] = {11, 7, 5, 3, 2};
    unsigned char expected_msg_size9[] = {255, 255, 255, 255, 255};
    unsigned char *msg_size1 = _cdtp_encode_message_size(0);
    unsigned char *msg_size2 = _cdtp_encode_message_size(1);
    unsigned char *msg_size3 = _cdtp_encode_message_size(255);
    unsigned char *msg_size4 = _cdtp_encode_message_size(256);
    unsigned char *msg_size5 = _cdtp_encode_message_size(257);
    unsigned char *msg_size6 = _cdtp_encode_message_size(4311810305);
    unsigned char *msg_size7 = _cdtp_encode_message_size(4328719365);
    unsigned char *msg_size8 = _cdtp_encode_message_size(47362409218);
    unsigned char *msg_size9 = _cdtp_encode_message_size(1099511627775);
    TEST_ASSERT_ARRAY_EQ(msg_size1, expected_msg_size1, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size2, expected_msg_size2, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size3, expected_msg_size3, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size4, expected_msg_size4, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size5, expected_msg_size5, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size6, expected_msg_size6, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size7, expected_msg_size7, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size8, expected_msg_size8, (size_t) CDTP_LENSIZE)
    TEST_ASSERT_ARRAY_EQ(msg_size9, expected_msg_size9, (size_t) CDTP_LENSIZE)

    // Test message size decoding
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size1), (size_t) 0)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size2), (size_t) 1)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size3), (size_t) 255)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size4), (size_t) 256)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size5), (size_t) 257)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size6), (size_t) 4311810305)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size7), (size_t) 4328719365)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size8), (size_t) 47362409218)
    TEST_ASSERT_EQ(_cdtp_decode_message_size(expected_msg_size9), (size_t) 1099511627775)

    // Clean up
    free(msg_size1);
    free(msg_size2);
    free(msg_size3);
    free(msg_size4);
    free(msg_size5);
    free(msg_size6);
    free(msg_size7);
    free(msg_size8);
    free(msg_size9);
}

/**
 * Test crypto functions.
 */
void test_crypto(void)
{
    // TODO
}

/**
 * Test server creation and serving.
 */
void test_server_serve(void)
{
    // Initialize test state
    TestReceivedMessage *server_received[] = EMPTY;
    size_t receive_clients[] = EMPTY;
    size_t connect_clients[] = EMPTY;
    size_t disconnect_clients[] = EMPTY;
    TestReceivedMessage *client_received[] = EMPTY;
    TestState *state = test_state(0, 0, 0,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  0, 0,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    TEST_ASSERT(!cdtp_server_is_serving(s))

    // Start server
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    TEST_ASSERT(cdtp_server_is_serving(s))
    cdtp_sleep(WAIT_TIME);

    // Check server address info
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);

    // Stop server
    TEST_ASSERT(cdtp_server_is_serving(s))
    cdtp_server_stop(s);
    TEST_ASSERT(!cdtp_server_is_serving(s))
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    free(server_host);
}

/**
 * Test getting server and client addresses.
 */
void test_addresses(void)
{
    // Initialize test state
    TestReceivedMessage *server_received[] = EMPTY;
    size_t receive_clients[] = EMPTY;
    size_t connect_clients[] = {0};
    size_t disconnect_clients[] = {0};
    TestReceivedMessage *client_received[] = EMPTY;
    TestState *state = test_state(0, 1, 1,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  0, 0,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    TEST_ASSERT(!cdtp_server_is_serving(s))
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    TEST_ASSERT(cdtp_server_is_serving(s))
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client
    CDTPClient *c = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    TEST_ASSERT(!cdtp_client_is_connected(c))
    cdtp_client_connect(c, CLIENT_HOST, CLIENT_PORT);
    TEST_ASSERT(cdtp_client_is_connected(c))
    char *client_host = cdtp_client_get_host(c);
    unsigned short client_port = cdtp_client_get_port(c);
    printf("Client address: %s:%d\n", client_host, client_port);
    cdtp_sleep(WAIT_TIME);

    // Check addresses
    char *ss_host = cdtp_server_get_host(s);
    unsigned short ss_port = cdtp_server_get_port(s);
    char *sc_host = cdtp_client_get_server_host(c);
    unsigned short sc_port = cdtp_client_get_server_port(c);
    char *cc_host = cdtp_client_get_host(c);
    unsigned short cc_port = cdtp_client_get_port(c);
    char *cs_host = cdtp_server_get_client_host(s, 0);
    unsigned short cs_port = cdtp_server_get_client_port(s, 0);
    printf("Server (according to server): %s:%d\n", ss_host, ss_port);
    printf("Server (according to client): %s:%d\n", sc_host, sc_port);
    printf("Client (according to client): %s:%d\n", cc_host, cc_port);
    printf("Client (according to server): %s:%d\n", cs_host, cs_port);
    TEST_ASSERT_STR_EQ(ss_host, "0.0.0.0")
    TEST_ASSERT_STR_EQ(sc_host, "127.0.0.1")
    TEST_ASSERT_INT_EQ(ss_port, sc_port)
    TEST_ASSERT_STR_EQ(cc_host, cs_host)
    TEST_ASSERT_INT_EQ(cc_port, cs_port)

    // Disconnect client
    TEST_ASSERT(cdtp_client_is_connected(c))
    cdtp_client_disconnect(c);
    TEST_ASSERT(!cdtp_client_is_connected(c))
    cdtp_sleep(WAIT_TIME);

    // Stop server
    TEST_ASSERT(cdtp_server_is_serving(s))
    cdtp_server_stop(s);
    TEST_ASSERT(!cdtp_server_is_serving(s))
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c);
    free(server_host);
    free(client_host);
    free(ss_host);
    free(sc_host);
    free(cc_host);
    free(cs_host);
}

/**
 * Test sending messages between server and client.
 */
void test_send_receive(void)
{
    // Initialize test state
    char *server_message = "Hello, server!";
    char *client_message = "Hello, client #0!";
    TestReceivedMessage *server_received[] = {
        str_message(server_message)
    };
    size_t receive_clients[] = {0};
    size_t connect_clients[] = {0};
    size_t disconnect_clients[] = {0};
    TestReceivedMessage *client_received[] = {
        str_message(client_message)
    };
    TestState *state = test_state(1, 1, 1,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  1, 0,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client
    CDTPClient *c = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    cdtp_client_connect(c, CLIENT_HOST, CLIENT_PORT);
    cdtp_sleep(WAIT_TIME);

    // Send messages
    cdtp_client_send(c, server_message, STR_SIZE(server_message));
    cdtp_server_send(s, 0, client_message, STR_SIZE(client_message));
    cdtp_sleep(WAIT_TIME);

    // Disconnect client
    cdtp_client_disconnect(c);
    cdtp_sleep(WAIT_TIME);

    // Stop server
    cdtp_server_stop(s);
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c);
    free(server_host);
}

/**
 * Test sending large random messages between server and client.
 */
void test_send_large_messages(void)
{
    // Initialize test state
    size_t large_server_message_len = (size_t) rand_int(256, 512);
    char *large_server_message = rand_bytes(large_server_message_len);
    size_t large_client_message_len = (size_t) rand_int(128, 256);
    char *large_client_message = rand_bytes(large_client_message_len);
    TestReceivedMessage *server_received[] = {
            test_received_message((void *) large_server_message, large_server_message_len)
    };
    size_t receive_clients[] = {0};
    size_t connect_clients[] = {0};
    size_t disconnect_clients[] = {0};
    TestReceivedMessage *client_received[] = {
            test_received_message((void *) large_client_message, large_client_message_len)
    };
    TestState *state = test_state(1, 1, 1,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  1, 0,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client
    CDTPClient *c = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    cdtp_client_connect(c, CLIENT_HOST, CLIENT_PORT);
    cdtp_sleep(WAIT_TIME);

    // Send messages
    cdtp_client_send(c, large_server_message, large_server_message_len);
    cdtp_server_send(s, 0, large_client_message, large_client_message_len);
    cdtp_sleep(WAIT_TIME);

    // Disconnect client
    cdtp_client_disconnect(c);
    cdtp_sleep(WAIT_TIME);

    // Stop server
    cdtp_server_stop(s);
    cdtp_sleep(WAIT_TIME);

    // Log message sizes
    printf("Server message sizes: %" PRI_SIZE_T ", %" PRI_SIZE_T "\n", state->server_received[0]->data_size, large_server_message_len);
    printf("Client message sizes: %" PRI_SIZE_T ", %" PRI_SIZE_T "\n", state->client_received[0]->data_size, large_client_message_len);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c);
    free(server_host);
}

/**
 * Test sending numerous random messages between server and client.
 */
void test_sending_numerous_messages(void)
{
    // Initialize test state
    size_t num_server_messages = (size_t) rand_int(64, 128);
    int *server_messages = (int *) malloc(num_server_messages * sizeof(int));
    for (size_t i = 0; i < num_server_messages; i++) server_messages[i] = rand();
    size_t num_client_messages = (size_t) rand_int(128, 256);
    int *client_messages = (int *) malloc(num_client_messages * sizeof(int));
    for (size_t i = 0; i < num_client_messages; i++) client_messages[i] = rand();
    TestReceivedMessage **server_received = (TestReceivedMessage **) malloc(num_server_messages * sizeof(TestReceivedMessage *));
    for (size_t i = 0; i < num_server_messages; i++) server_received[i] = int_message(server_messages[i]);
    size_t *receive_clients = (size_t *) malloc(num_server_messages * sizeof(size_t));
    for (size_t i = 0; i < num_server_messages; i++) receive_clients[i] = 0;
    size_t connect_clients[] = {0};
    size_t disconnect_clients[] = {0};
    TestReceivedMessage **client_received = (TestReceivedMessage **) malloc(num_client_messages * sizeof(TestReceivedMessage *));
    for (size_t i = 0; i < num_client_messages; i++) client_received[i] = int_message(client_messages[i]);
    TestState *state = test_state(num_server_messages, 1, 1,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  num_client_messages, 0,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client
    CDTPClient *c = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    cdtp_client_connect(c, CLIENT_HOST, CLIENT_PORT);
    cdtp_sleep(WAIT_TIME);

    // Send messages
    for (size_t i = 0; i < num_server_messages; i++) {
        cdtp_client_send(c, &(server_messages[i]), sizeof(int));
        cdtp_sleep(0.01);
    }
    for (size_t i = 0; i < num_client_messages; i++) {
        cdtp_server_send_all(s, &(client_messages[i]), sizeof(int));
        cdtp_sleep(0.01);
    }
    cdtp_sleep(5);

    // Disconnect client
    cdtp_client_disconnect(c);
    cdtp_sleep(WAIT_TIME);

    // Stop server
    cdtp_server_stop(s);
    cdtp_sleep(WAIT_TIME);

    // Log number of messages
    printf("Number of server messages: %" PRI_SIZE_T ", %" PRI_SIZE_T "\n", state->server_receive_count, num_server_messages);
    printf("Number of client messages: %" PRI_SIZE_T ", %" PRI_SIZE_T "\n", state->client_receive_count, num_client_messages);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c);
    free(server_messages);
    free(client_messages);
    free(server_received);
    free(client_received);
    free(receive_clients);
    free(server_host);
}

/**
 * Test sending and receiving custom types.
 */
void test_sending_custom_types(void)
{
    // Initialize test state
    Custom *custom_server_message = (Custom *) malloc(sizeof(Custom));
    custom_server_message->a = -123;
    custom_server_message->b = 789;
    Custom *custom_client_message = (Custom *) malloc(sizeof(Custom));
    custom_client_message->a = -234;
    custom_client_message->b = 678;
    TestReceivedMessage *server_received[] = {
            test_received_message(custom_server_message, sizeof(Custom))
    };
    size_t receive_clients[] = {0};
    size_t connect_clients[] = {0};
    size_t disconnect_clients[] = {0};
    TestReceivedMessage *client_received[] = {
            test_received_message(custom_client_message, sizeof(Custom))
    };
    TestState *state = test_state(1, 1, 1,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  1, 0,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client
    CDTPClient *c = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    cdtp_client_connect(c, CLIENT_HOST, CLIENT_PORT);
    cdtp_sleep(WAIT_TIME);

    // Send messages
    cdtp_client_send(c, custom_server_message, sizeof(Custom));
    cdtp_server_send(s, 0, custom_client_message, sizeof(Custom));
    cdtp_sleep(WAIT_TIME);

    // Disconnect client
    cdtp_client_disconnect(c);
    cdtp_sleep(WAIT_TIME);

    // Stop server
    cdtp_server_stop(s);
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c);
    free(server_host);
}

/**
 * Test having multiple clients connected.
 */
void test_multiple_clients(void)
{
    // Initialize test state
    char *message_from_client1 = "Hello from client #1!";
    char *message_from_client2 = "Goodbye from client #2!";
    size_t message_from_server = 29275;
    size_t message_to_client1 = 123;
    size_t message_to_client2 = 789;
    TestReceivedMessage *server_received[] = {
            str_message(message_from_client1),
            str_message(message_from_client2)
    };
    size_t receive_clients[] = {0, 1};
    size_t connect_clients[] = {0, 1};
    size_t disconnect_clients[] = {0, 1};
    TestReceivedMessage *client_received[] = {
            size_t_message(message_from_server),
            size_t_message(message_from_server),
            size_t_message(message_to_client1),
            size_t_message(message_to_client2)
    };
    TestState *state = test_state(2, 2, 2,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  4, 0,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client 1
    CDTPClient *c1 = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    cdtp_client_connect(c1, CLIENT_HOST, CLIENT_PORT);
    cdtp_sleep(WAIT_TIME);

    // Check client 1 address info
    char *cc1_host = cdtp_client_get_host(c1);
    unsigned short cc1_port = cdtp_client_get_port(c1);
    char *cs1_host = cdtp_server_get_client_host(s, 0);
    unsigned short cs1_port = cdtp_server_get_client_port(s, 0);
    printf("Client #1 (according to client #1): %s:%d\n", cc1_host, cc1_port);
    printf("Client #1 (according to server):    %s:%d\n", cs1_host, cs1_port);
    TEST_ASSERT_STR_EQ(cc1_host, cs1_host)
    TEST_ASSERT_INT_EQ(cc1_port, cs1_port)

    // Create client 2
    CDTPClient *c2 = cdtp_client(client_on_recv, client_on_disconnected,
                                 state, state);
    cdtp_client_connect(c2, CLIENT_HOST, CLIENT_PORT);
    cdtp_sleep(WAIT_TIME);

    // Check client 2 address info
    char *cc2_host = cdtp_client_get_host(c2);
    unsigned short cc2_port = cdtp_client_get_port(c2);
    char *cs2_host = cdtp_server_get_client_host(s, 1);
    unsigned short cs2_port = cdtp_server_get_client_port(s, 1);
    printf("Client #2 (according to client #2): %s:%d\n", cc2_host, cc2_port);
    printf("Client #2 (according to server):    %s:%d\n", cs2_host, cs2_port);
    TEST_ASSERT_STR_EQ(cc2_host, cs2_host)
    TEST_ASSERT_INT_EQ(cc2_port, cs2_port)

    // Send message from client 1
    cdtp_client_send(c1, message_from_client1, STR_SIZE(message_from_client1));
    cdtp_sleep(WAIT_TIME);

    // Send message from client 2
    cdtp_client_send(c2, message_from_client2, STR_SIZE(message_from_client2));
    cdtp_sleep(WAIT_TIME);

    // Send message to all clients
    cdtp_server_send_all(s, &message_from_server, sizeof(size_t));
    cdtp_sleep(WAIT_TIME);

    // Send message to client 1
    cdtp_server_send(s, 0, &message_to_client1, sizeof(size_t));
    cdtp_sleep(WAIT_TIME);

    // Send message to client 2
    cdtp_server_send(s, 1, &message_to_client2, sizeof(size_t));
    cdtp_sleep(WAIT_TIME);

    // Disconnect client 1
    cdtp_client_disconnect(c1);
    cdtp_sleep(WAIT_TIME);

    // Disconnect client 2
    cdtp_client_disconnect(c2);
    cdtp_sleep(WAIT_TIME);

    // Stop server
    cdtp_server_stop(s);
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c1);
    cdtp_client_free(c2);
    free(server_host);
    free(cc1_host);
    free(cs1_host);
    free(cc2_host);
    free(cs2_host);
}

/**
 * Test clients disconnecting from the server.
 */
void test_client_disconnected(void)
{
    // Initialize test state
    TestReceivedMessage *server_received[] = EMPTY;
    size_t receive_clients[] = EMPTY;
    size_t connect_clients[] = {0};
    size_t disconnect_clients[] = EMPTY;
    TestReceivedMessage *client_received[] = EMPTY;
    TestState *state = test_state(0, 1, 0,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  0, 1,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    TEST_ASSERT(!cdtp_server_is_serving(s))
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    TEST_ASSERT(cdtp_server_is_serving(s))
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client
    CDTPClient *c = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    TEST_ASSERT(!cdtp_client_is_connected(c))
    cdtp_client_connect(c, CLIENT_HOST, CLIENT_PORT);
    TEST_ASSERT(cdtp_client_is_connected(c))
    cdtp_sleep(WAIT_TIME);

    // Stop server
    TEST_ASSERT(cdtp_server_is_serving(s))
    TEST_ASSERT(cdtp_client_is_connected(c))
    cdtp_server_stop(s);
    TEST_ASSERT(!cdtp_server_is_serving(s))
    cdtp_sleep(WAIT_TIME);
    TEST_ASSERT(!cdtp_client_is_connected(c))
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c);
    free(server_host);
}

/**
 * Test removing a client from the server.
 */
void test_remove_client(void)
{
    // Initialize test state
    TestReceivedMessage *server_received[] = EMPTY;
    size_t receive_clients[] = EMPTY;
    size_t connect_clients[] = {0};
    size_t disconnect_clients[] = EMPTY;
    TestReceivedMessage *client_received[] = EMPTY;
    TestState *state = test_state(0, 1, 0,
                                  server_received, receive_clients, connect_clients, disconnect_clients,
                                  0, 1,
                                  client_received);

    // Create server
    CDTPServer *s = cdtp_server(MAX_CLIENTS,
                                server_on_recv, server_on_connect, server_on_disconnect,
                                state, state, state);
    TEST_ASSERT(!cdtp_server_is_serving(s))
    cdtp_server_start(s, SERVER_HOST, SERVER_PORT);
    TEST_ASSERT(cdtp_server_is_serving(s))
    char *server_host = cdtp_server_get_host(s);
    unsigned short server_port = cdtp_server_get_port(s);
    printf("Server address: %s:%d\n", server_host, server_port);
    cdtp_sleep(WAIT_TIME);

    // Create client
    CDTPClient *c = cdtp_client(client_on_recv, client_on_disconnected,
                                state, state);
    TEST_ASSERT(!cdtp_client_is_connected(c))
    cdtp_client_connect(c, CLIENT_HOST, CLIENT_PORT);
    TEST_ASSERT(cdtp_client_is_connected(c))
    cdtp_sleep(WAIT_TIME);

    // Disconnect the client
    TEST_ASSERT(cdtp_client_is_connected(c))
    cdtp_server_remove_client(s, 0);
    cdtp_sleep(WAIT_TIME);
    TEST_ASSERT(!cdtp_client_is_connected(c))

    // Stop server
    TEST_ASSERT(cdtp_server_is_serving(s))
    cdtp_server_stop(s);
    TEST_ASSERT(!cdtp_server_is_serving(s))
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
    cdtp_server_free(s);
    cdtp_client_free(c);
    free(server_host);
}

int main(void)
{
    printf("Beginning tests\n");

    // Initialize the random number generator
    srand(time(NULL));

    // Register error event
    cdtp_on_error(on_err, NULL);

    // Run tests
    printf("\nTesting utilities...\n");
    test_util();
    printf("\nTesting crypto...\n");
    test_crypto();
    printf("\nTesting server creation and serving...\n");
    test_server_serve();
    printf("\nTesting addresses...\n");
    test_addresses();
    printf("\nTesting send and receive...\n");
    test_send_receive();
    printf("\nTesting sending large messages...\n");
    test_send_large_messages();
    printf("\nTesting sending numerous messages...\n");
    test_sending_numerous_messages();
    printf("\nTesting sending custom types...\n");
    test_sending_custom_types();
    printf("\nTesting with multiple clients...\n");
    test_multiple_clients();
    printf("\nTesting disconnecting clients...\n");
    test_client_disconnected();
    printf("\nTesting removing clients...\n");
    test_remove_client();

    // Done
    printf("\nCompleted tests\n");
}
