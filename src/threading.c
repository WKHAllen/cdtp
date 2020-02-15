#include "threading.h"

struct CDTPEventFunc
{
    char *name;
    union func {
        void (*func_int_voidp_voidp)(int, void *, void *); // on_recv (server)
        void (*func_int_voidp      )(int, void *);         // on_connect, on_disconnect (server)
        void (*func_voidp_voidp    )(void *, void *);      // on_recv (client)
        void (*func_voidp          )(void *);              // on_disconnected (client)
    } func;
    int int1;
    void *voidp1;
    void *voidp2;
};

struct CDTPServeFunc
{
    void (*func)(CDTPServer *);
    CDTPServer *server;
};

#ifdef _WIN32
DWORD WINAPI _cdtp_event_thread(LPVOID func_info)
#else
void *_cdtp_event_thread(void *func_info)
#endif
{
    CDTPEventFunc *event_func_info = (CDTPEventFunc *)func_info;

    // Find out which function to call
    if (strcmp(event_func_info->name, "on_recv_server") == 0)
        (*event_func_info->func.func_int_voidp_voidp)(event_func_info->int1, event_func_info->voidp1, event_func_info->voidp2);
    else if (strcmp(event_func_info->name, "on_connect") == 0)
        (*event_func_info->func.func_int_voidp)(event_func_info->int1, event_func_info->voidp1);
    else if (strcmp(event_func_info->name, "on_disconnect") == 0)
        (*event_func_info->func.func_int_voidp)(event_func_info->int1, event_func_info->voidp1);
    else if (strcmp(event_func_info->name, "on_recv_client") == 0)
        (*event_func_info->func.func_voidp_voidp)(event_func_info->voidp1, event_func_info->voidp2);
    else if (strcmp(event_func_info->name, "on_disconnected") == 0)
        (*event_func_info->func.func_voidp)(event_func_info->voidp1);

    // Free function information memory and return
    free(event_func_info);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

void _cdtp_start_event_thread(CDTPEventFunc *func_info)
{
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, _cdtp_event_thread, func_info, 0, NULL);
    if (thread == NULL)
    {
        _cdtp_set_error(CDTP_THREAD_START_FAILED, GetLastError());
        return;
    }
#else
    pthread_t thread;
    int return_code = pthread_create(&thread, NULL, _cdtp_event_thread, func_info);
    if (return_code != 0)
    {
        _cdtp_set_error(CDTP_THREAD_START_FAILED, return_code);
        return;
    }
#endif
}

void _cdtp_start_thread_on_recv_server(void (*func)(int, void *, void *), int client_id, void *data, void *arg)
{
    CDTPEventFunc *func_info = malloc(sizeof(*func_info));
    func_info->name = "on_recv_server";
    func_info->func.func_int_voidp_voidp = func;
    func_info->int1 = client_id;
    func_info->voidp1 = data;
    func_info->voidp2 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_connect(void (*func)(int, void *), int client_id, void *arg)
{
    CDTPEventFunc *func_info = malloc(sizeof(*func_info));
    func_info->name = "on_connect";
    func_info->func.func_int_voidp = func;
    func_info->int1 = client_id;
    func_info->voidp1 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_disconnect(void (*func)(int, void *), int client_id, void *arg)
{
    CDTPEventFunc *func_info = malloc(sizeof(*func_info));
    func_info->name = "on_disconnect";
    func_info->func.func_int_voidp = func;
    func_info->int1 = client_id;
    func_info->voidp1 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_recv_client(void (*func)(void *, void *), void *data, void *arg)
{
    CDTPEventFunc *func_info = malloc(sizeof(*func_info));
    func_info->name = "on_recv_client";
    func_info->func.func_voidp_voidp = func;
    func_info->voidp1 = data;
    func_info->voidp2 = arg;
    _cdtp_start_event_thread(func_info);
}

void _cdtp_start_thread_on_disconnected(void (*func)(void *), void *arg)
{
    CDTPEventFunc *func_info = malloc(sizeof(*func_info));
    func_info->name = "on_disconnected";
    func_info->func.func_voidp = func;
    func_info->voidp1 = arg;
    _cdtp_start_event_thread(func_info);
}

#ifdef _WIN32
DWORD WINAPI _cdtp_serve_thread(LPVOID func_info)
#else
void *_cdtp_serve_thread(void *func_info)
#endif
{
    CDTPServeFunc *serve_func_info = (CDTPServeFunc *)func_info;

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
HANDLE _cdtp_start_serve_thread(void (*func)(CDTPServer *), CDTPServer *server)
#else
pthread_t _cdtp_start_serve_thread(void (*func)(CDTPServer *), CDTPServer *server)
#endif
{
    // Set function information
    CDTPServeFunc *func_info = malloc(sizeof(*func_info));
    func_info->func = func;
    func_info->server = server;

    // Start the thread
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, _cdtp_serve_thread, func_info, 0, NULL);
    if (thread == NULL)
    {
        _cdtp_set_error(CDTP_THREAD_START_FAILED, GetLastError());
        return NULL;
    }
#else
    pthread_t thread;
    int return_code = pthread_create(&thread, NULL, _cdtp_serve_thread, func_info);
    if (return_code != 0)
    {
        _cdtp_set_error(CDTP_THREAD_START_FAILED, return_code);
        return 0;
    }
#endif

    // Return the thread
    return thread;
}
