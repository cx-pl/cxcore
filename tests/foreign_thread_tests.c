#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
typedef HANDLE test_thread;
#else
#include <pthread.h>
typedef pthread_t test_thread;
#endif
#include "../src/cxcore.h"

static struct {
    cx_ptr retained;
    struct cx_gc_root_handle *root;
    int failed;
} context;

static void foreign_worker(void) {
    cx_bool attached = cx_gc_enter_thread();
    cx_ulong id = CX_ID_6(cxcore, System, Threading, Thread, CurrentId, __const_get)();
    cx_bool nested = cx_gc_enter_thread();
    if (!attached || nested) context.failed = 1;
    context.retained = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_uint));
    *(cx_uint *)context.retained = 0xabc123;
    context.root = cx_gc_add_root(&context.retained, (cx_uint)sizeof(context.retained));
    cx_gc_leave_thread(nested);
    if (CX_ID_6(cxcore, System, Threading, Thread, CurrentId, __const_get)() != id) context.failed = 2;
    cx_gc_leave_thread(attached);
}
#ifdef _WIN32
static DWORD WINAPI worker_entry(LPVOID argument) { (void)argument; foreign_worker(); return 0; }
static void join_worker(void *argument) {
    HANDLE *thread = (HANDLE *)argument;
    if (WaitForSingleObject(*thread, INFINITE) != WAIT_OBJECT_0) ExitProcess(9);
    CloseHandle(*thread);
}
#else
static void *worker_entry(void *argument) { (void)argument; foreign_worker(); return NULL; }
static void join_worker(void *argument) {
    if (pthread_join(*(pthread_t *)argument, NULL) != 0) _Exit(9);
}
#endif

int main(int argc, char **argv) {
    test_thread thread;
    cx_gc_check_thread();
    if (argc == 2 && strcmp(argv[1], "invalid-root") == 0) {
        cx_gc_remove_root(NULL);
        return 20;
    }
    if (argc == 2 && strcmp(argv[1], "detach-handler") == 0) {
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        cx_gc_detach_thread();
        return 21;
    }
    if (argc != 1) return 22;
    if (cx_gc_enter_thread()) return 1; // Existing CX callers retain their attachment.
#ifdef _WIN32
    thread = CreateThread(NULL, 0, worker_entry, NULL, 0, NULL);
    if (thread == NULL) return 2;
#else
    if (pthread_create(&thread, NULL, worker_entry, NULL) != 0) return 2;
#endif
    cx_gc_run_blocking(join_worker, &thread);
    cx_gc_collect(); // The worker stack and TLS no longer exist.
    if (context.failed || *(cx_uint *)context.retained != 0xabc123) return 3;
    cx_gc_remove_root(context.root);
    context.root = NULL;
    context.retained = NULL;
    cx_gc_collect();
    cx_gc_leave_thread(CX_FALSE);
    cx_gc_detach_thread();
    return 0;
}
