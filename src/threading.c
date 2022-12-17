#include "threading.h"

#ifdef _WIN32
DWORD WINAPI _cdtp_event_thread(LPVOID func_info)
#else
void* _cdtp_event_thread(void* func_info)
#endif
{
    CDTPEventFunc* event_func_info = (CDTPEventFunc*)func_info;

    // Determine which function to call
    if (strcmp(event_func_info->name, "on_recv_server") == 0) {
        (*event_func_info->func.func_server_on_recv)(event_func_info->size_t1, event_func_info->voidp1, event_func_info->size_t2, event_func_info->voidp2);
    }
    else if (strcmp(event_func_info->name, "on_connect") == 0) {
        (*event_func_info->func.func_server_on_connect)(event_func_info->size_t1, event_func_info->voidp1);
    }
    else if (strcmp(event_func_info->name, "on_disconnect") == 0) {
        (*event_func_info->func.func_server_on_disconnect)(event_func_info->size_t1, event_func_info->voidp1);
    }
    else if (strcmp(event_func_info->name, "on_recv_client") == 0) {
        (*event_func_info->func.func_client_on_recv)(event_func_info->voidp1, event_func_info->size_t2, event_func_info->voidp2);
    }
    else if (strcmp(event_func_info->name, "on_disconnected") == 0) {
        (*event_func_info->func.func_client_on_disconnected)(event_func_info->voidp1);
    }

    // Free function information memory and return
    free(event_func_info);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

void _cdtp_start_event_thread(CDTPEventFunc* func_info)
{
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, _cdtp_event_thread, func_info, 0, NULL);

    if (thread == NULL) {
        _cdtp_set_error(CDTP_EVENT_THREAD_START_FAILED, GetLastError());
        return;
    }
#else
    pthread_t thread;
    int return_code = pthread_create(&thread, NULL, _cdtp_event_thread, func_info);

    if (return_code != 0) {
        _cdtp_set_error(CDTP_EVENT_THREAD_START_FAILED, return_code);
        return;
    }
#endif
}

void _cdtp_start_thread_on_recv_server(
    ServerOnRecvCallback func,
    size_t client_id,
    void* data,
    size_t data_size,
    void* arg
)
{
    CDTPEventFunc* func_info = (CDTPEventFunc*) malloc(sizeof(CDTPEventFunc));
    func_info->name = "on_recv_server";
    func_info->func.func_server_on_recv = func;
    func_info->size_t1 = client_id;
    func_info->voidp1 = data;
    func_info->size_t2 = data_size;
    func_info->voidp2 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_connect(
    ServerOnConnectCallback func,
    size_t client_id,
    void* arg
)
{
    CDTPEventFunc* func_info = (CDTPEventFunc*) malloc(sizeof(CDTPEventFunc));
    func_info->name = "on_connect";
    func_info->func.func_server_on_connect = func;
    func_info->size_t1 = client_id;
    func_info->voidp1 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_disconnect(
    ServerOnDisconnectCallback func,
    size_t client_id,
    void* arg
)
{
    CDTPEventFunc* func_info = (CDTPEventFunc*) malloc(sizeof(CDTPEventFunc));
    func_info->name = "on_disconnect";
    func_info->func.func_server_on_disconnect = func;
    func_info->size_t1 = client_id;
    func_info->voidp1 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_recv_client(
    ClientOnRecvCallback func,
    void* data,
    size_t data_size,
    void* arg
)
{
    CDTPEventFunc* func_info = (CDTPEventFunc*) malloc(sizeof(CDTPEventFunc));
    func_info->name = "on_recv_client";
    func_info->func.func_client_on_recv = func;
    func_info->voidp1 = data;
    func_info->size_t2 = data_size;
    func_info->voidp2 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_disconnected(
    ClientOnDisconnectedCallback func,
    void* arg
)
{
    CDTPEventFunc* func_info = (CDTPEventFunc*) malloc(sizeof(CDTPEventFunc));
    func_info->name = "on_disconnected";
    func_info->func.func_client_on_disconnected = func;
    func_info->voidp1 = arg;
    _cdtp_start_event_thread(func_info);
}

#ifdef _WIN32
DWORD WINAPI _cdtp_serve_thread(LPVOID func_info)
#else
void* _cdtp_serve_thread(void* func_info)
#endif
{
    CDTPServeFunc* serve_func_info = (CDTPServeFunc*)func_info;

    // Call the function
    (*serve_func_info->func)(serve_func_info->server);

    // Free function information memory and return
    free(serve_func_info);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

#ifdef _WIN32
HANDLE _cdtp_start_serve_thread(
    void (*func)(CDTPServer*),
    CDTPServer* server
)
#else
pthread_t _cdtp_start_serve_thread(
    void (*func)(CDTPServer*),
    CDTPServer* server
)
#endif
{
    // Set function information
    CDTPServeFunc* func_info = (CDTPServeFunc*) malloc(sizeof(CDTPServeFunc));
    func_info->func = func;
    func_info->server = server;

    // Start the thread
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, _cdtp_serve_thread, func_info, 0, NULL);

    if (thread == NULL) {
        _cdtp_set_error(CDTP_SERVE_THREAD_START_FAILED, GetLastError());
        return NULL;
    }
#else
    pthread_t thread;
    int return_code = pthread_create(&thread, NULL, _cdtp_serve_thread, func_info);

    if (return_code != 0) {
        _cdtp_set_error(CDTP_SERVE_THREAD_START_FAILED, return_code);
        return 0;
    }
#endif

    // Return the thread
    return thread;
}

#ifdef _WIN32
DWORD WINAPI _cdtp_handle_thread(LPVOID func_info)
#else
void* _cdtp_handle_thread(void* func_info)
#endif
{
    CDTPHandleFunc* handle_func_info = (CDTPHandleFunc*)func_info;

    // Call the function
    (*handle_func_info->func)(handle_func_info->client);

    // Free function information memory and return
    free(handle_func_info);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

#ifdef _WIN32
HANDLE _cdtp_start_handle_thread(
    void (*func)(CDTPClient*),
    CDTPClient* client
)
#else
pthread_t _cdtp_start_handle_thread(
    void (*func)(CDTPClient*),
    CDTPClient* client
)
#endif
{
    // Set function information
    CDTPHandleFunc* func_info = (CDTPHandleFunc*) malloc(sizeof(CDTPHandleFunc));
    func_info->func = func;
    func_info->client = client;

    // Start the thread
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, _cdtp_handle_thread, func_info, 0, NULL);

    if (thread == NULL) {
        _cdtp_set_error(CDTP_HANDLE_THREAD_START_FAILED, GetLastError());
        return NULL;
    }
#else
    pthread_t thread;
    int return_code = pthread_create(&thread, NULL, _cdtp_handle_thread, func_info);

    if (return_code != 0) {
        _cdtp_set_error(CDTP_HANDLE_THREAD_START_FAILED, return_code);
        return 0;
    }
#endif

    // Return the thread
    return thread;
}
