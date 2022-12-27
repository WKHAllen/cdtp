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
    TestState *test_state = (TestState *) malloc(sizeof(TestState));

    test_state->server_receive_count_expected = server_receive_count;
    test_state->server_connect_count_expected = server_connect_count;
    test_state->server_disconnect_count_expected = server_disconnect_count;
    test_state->server_receive_count = 0;
    test_state->server_connect_count = 0;
    test_state->server_disconnect_count = 0;
    test_state->server_received_expected = server_received;
    test_state->server_receive_client_ids_expected = server_receive_client_ids;
    test_state->server_connect_client_ids_expected = server_connect_client_ids;
    test_state->server_disconnect_client_ids_expected = server_disconnect_client_ids;
    test_state->server_received = (TestReceivedMessage **) malloc(server_receive_count * sizeof(TestReceivedMessage *));
    test_state->server_receive_client_ids = (size_t *) malloc(server_receive_count * sizeof(size_t));
    test_state->server_connect_client_ids = (size_t *) malloc(server_connect_count * sizeof(size_t));
    test_state->server_disconnect_client_ids = (size_t *) malloc(server_disconnect_count * sizeof(size_t));
    test_state->client_receive_count_expected = client_receive_count;
    test_state->client_disconnected_count_expected = client_disconnected_count;
    test_state->client_receive_count = 0;
    test_state->client_disconnected_count = 0;
    test_state->client_received_expected = client_received;
    test_state->client_received = (TestReceivedMessage **) malloc(client_receive_count * sizeof(TestReceivedMessage *));

    return test_state;
}

void test_state_finish(TestState *test_state)
{
    TEST_ASSERT_EQ(test_state->server_receive_count, test_state->server_receive_count_expected)
    TEST_ASSERT_EQ(test_state->server_connect_count, test_state->server_connect_count_expected)
    TEST_ASSERT_EQ(test_state->server_disconnect_count, test_state->server_disconnect_count_expected)
    TEST_ASSERT_MESSAGES_EQ(test_state->server_received,
                             test_state->server_received_expected,
                             test_state->server_receive_count)
    TEST_ASSERT_ARRAY_EQ(test_state->server_receive_client_ids,
                         test_state->server_receive_client_ids_expected,
                         test_state->server_receive_count)
    TEST_ASSERT_ARRAY_EQ(test_state->server_connect_client_ids,
                         test_state->server_connect_client_ids_expected,
                         test_state->server_connect_count)
    TEST_ASSERT_ARRAY_EQ(test_state->server_disconnect_client_ids,
                         test_state->server_disconnect_client_ids_expected,
                         test_state->server_disconnect_count)
    TEST_ASSERT_EQ(test_state->client_receive_count, test_state->client_receive_count_expected)
    TEST_ASSERT_EQ(test_state->client_disconnected_count, test_state->client_disconnected_count_expected)
    TEST_ASSERT_MESSAGES_EQ(test_state->client_received,
                             test_state->client_received_expected,
                             test_state->client_receive_count)

    for (size_t i = 0; i < test_state->server_receive_count; i++) {
        free(test_state->server_received_expected[i]->data);
        free(test_state->server_received_expected[i]);
        free(test_state->server_received[i]->data);
        free(test_state->server_received[i]);
    }

    free(test_state->server_received);
    free(test_state->server_receive_client_ids);
    free(test_state->server_connect_client_ids);
    free(test_state->server_disconnect_client_ids);

    for (size_t i = 0; i < test_state->client_receive_count; i++) {
        free(test_state->client_received_expected[i]->data);
        free(test_state->client_received_expected[i]);
        free(test_state->client_received[i]->data);
        free(test_state->client_received[i]);
    }

    free(test_state->client_received);
    free(test_state);
}

void test_state_server_received(TestState *test_state, size_t client_id, void *data, size_t data_size)
{
    TEST_ASSERT(test_state->server_receive_count < test_state->server_receive_count_expected)

    test_state->server_received[test_state->server_receive_count] = (TestReceivedMessage *) malloc(sizeof(TestReceivedMessage));
    test_state->server_received[test_state->server_receive_count]->data = data;
    test_state->server_received[test_state->server_receive_count]->data_size = data_size;
    test_state->server_receive_client_ids[test_state->server_receive_count++] = client_id;
}

void test_state_server_connect(TestState *test_state, size_t client_id)
{
    TEST_ASSERT(test_state->server_connect_count < test_state->server_connect_count_expected)

    test_state->server_connect_client_ids[test_state->server_connect_count++] = client_id;
}

void test_state_server_disconnect(TestState *test_state, size_t client_id)
{
    TEST_ASSERT(test_state->server_disconnect_count < test_state->server_disconnect_count_expected)

    test_state->server_disconnect_client_ids[test_state->server_disconnect_count++] = client_id;
}

void test_state_client_received(TestState *test_state, void *data, size_t data_size)
{
    TEST_ASSERT(test_state->client_receive_count < test_state->client_receive_count_expected)

    test_state->client_received[test_state->client_receive_count] = (TestReceivedMessage *) malloc(sizeof(TestReceivedMessage));
    test_state->client_received[test_state->client_receive_count]->data = data;
    test_state->client_received[test_state->client_receive_count]->data_size = data_size;
    test_state->client_receive_count++;
}

void test_state_client_disconnected(TestState *test_state)
{
    TEST_ASSERT(test_state->client_disconnected_count < test_state->client_disconnected_count_expected)

    test_state->client_disconnected_count++;
}

int randint(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

char* randbytes(size_t size)
{
    char* bytes = (char*) malloc(size * sizeof(char));

    for (size_t i = 0; i < size; i++) {
        bytes[i] = (char) randint(0, 255);
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
    TestState *test_state = (TestState *) arg;

    test_state_server_received(test_state, client_id, data, data_size);
}

void server_on_connect(size_t client_id, void *arg)
{
    TestState *test_state = (TestState *) arg;

    test_state_server_connect(test_state, client_id);
}

void server_on_disconnect(size_t client_id, void *arg)
{
    TestState *test_state = (TestState *) arg;

    test_state_server_disconnect(test_state, client_id);
}

void client_on_recv(void *data, size_t data_size, void *arg)
{
    TestState *test_state = (TestState *) arg;

    test_state_client_received(test_state, data, data_size);
}

void client_on_disconnected(void* arg)
{
    TestState *test_state = (TestState *) arg;

    test_state_client_disconnected(test_state);
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
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
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
    cdtp_sleep(WAIT_TIME);

    // Stop server
    TEST_ASSERT(cdtp_server_is_serving(s))
    cdtp_server_stop(s);
    cdtp_sleep(WAIT_TIME);

    // Clean up
    test_state_finish(state);
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
    free(server_host);
}

/**
 * Test sending large random messages between server and client.
 */
void test_send_large_messages(void)
{
    // TODO
}

/**
 * Test sending numerous random messages between server and client.
 */
void test_sending_numerous_messages(void)
{
    // TODO
}

/**
 * Test sending and receiving custom types.
 */
void test_sending_custom_types(void)
{
    // TODO
}

/**
 * Test having multiple clients connected.
 */
void test_multiple_clients(void)
{
    // TODO
}

/**
 * Test clients disconnecting from the server.
 */
void test_client_disconnected(void)
{
    // TODO
}

/**
 * Test removing a client from the server.
 */
void test_remove_client(void)
{
    // TODO
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

//int main(void)
//{
//    double wait_time = 0.1;
//
//    // Generate large random messages
//    srand(time(NULL));
//    size_t random_message_to_server_len = randint(32768, 65535);
//    size_t random_message_to_client_len = randint(65536, 82175); // fails on Linux at values >= 82176?
//    char* random_message_to_server = randbytes(random_message_to_server_len);
//    char* random_message_to_client = randbytes(random_message_to_client_len);
//    printf("Large random message sizes: %" PRI_SIZE_T ", %" PRI_SIZE_T "\n", random_message_to_server_len, random_message_to_client_len);
//
//    // Initialize test state
//    TestState* test_state = new_test_state(
//        random_message_to_server_len,
//        random_message_to_client_len,
//        random_message_to_server,
//        random_message_to_client
//    );
//
//    printf("Running tests...\n");
//
//    // Register error event
//    cdtp_on_error(on_err, NULL);
//
//    // Server initialization
//    CDTPServer* server = cdtp_server(
//        16,
//        server_on_recv,
//        server_on_connect,
//        server_on_disconnect,
//        test_state,
//        test_state,
//        test_state
//    );
//
//    // Server start
//    char* host = "127.0.0.1";
//    unsigned short port = CDTP_PORT;
//    cdtp_server_start(server, host, port);
//
//    // Get host and port
//    char* server_host = cdtp_server_get_host(server);
//    unsigned short server_port = cdtp_server_get_port(server);
//    printf("Host: %s\n", server_host);
//    printf("Port: %d\n", server_port);
//
//    // Unregister error event
//    cdtp_on_error_clear();
//
//    // Test that the client does not exist
//    cdtp_server_remove_client(server, 0);
//    assert(cdtp_get_error() == CDTP_CLIENT_DOES_NOT_EXIST);
//
//    // Register error event
//    cdtp_on_error(on_err, NULL);
//
//    cdtp_sleep(wait_time);
//
//    // Client initialization
//    CDTPClient* client = cdtp_client(
//        client_on_recv,
//        client_on_disconnected,
//        test_state,
//        test_state
//    );
//
//    // Client connect
//    cdtp_client_connect(client, host, port);
//    free(server_host);
//
//    // Get host and port
//    char* client_host = cdtp_client_get_host(client);
//    int client_port = cdtp_client_get_port(client);
//    printf("Host: %s\n", client_host);
//    printf("Port: %d\n", client_port);
//    free(client_host);
//
//    cdtp_sleep(wait_time);
//
//    // Client send
//    char* client_message = "Hello, server.";
//    cdtp_client_send(client, client_message, strlen(client_message));
//
//    cdtp_sleep(wait_time);
//
//    // Server send
//    char* server_message = "Hello, client #0.";
//    cdtp_server_send(server, 0, server_message, strlen(server_message));
//
//    cdtp_sleep(wait_time);
//
//    test_state->receiving_random_message = CDTP_TRUE;
//
//    // Client send large message
//    cdtp_client_send(client, random_message_to_server, random_message_to_server_len);
//
//    cdtp_sleep(wait_time);
//
//    // Server send large message
//    cdtp_server_send_all(server, random_message_to_client, random_message_to_client_len);
//
//    cdtp_sleep(wait_time);
//
//    test_state->receiving_random_message = CDTP_FALSE;
//
//    // Client disconnect
//    cdtp_client_disconnect(client);
//
//    cdtp_sleep(wait_time);
//
//    // Server stop
//    cdtp_server_stop(server);
//
//    // Unregister error event
//    cdtp_on_error_clear();
//
//    // Test that server cannot be restarted
//    cdtp_server_start(server, host, port);
//    assert(cdtp_get_error() == CDTP_SERVER_CANNOT_RESTART);
//
//    printf("Successfully passed all tests\n");
//    cleanup_test_state(test_state);
//    return 0;
//}
