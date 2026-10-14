#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
typedef HANDLE test_thread;
typedef volatile LONG test_flag;
static void signal_flag(test_flag *flag) { InterlockedExchange(flag, 1); }
static int read_flag(test_flag *flag) { return InterlockedCompareExchange(flag, 0, 0) != 0; }
static void native_yield(void) { Sleep(0); }
#else
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
typedef pthread_t test_thread;
typedef _Atomic int test_flag;
static void signal_flag(test_flag *flag) { atomic_store_explicit(flag, 1, memory_order_release); }
static int read_flag(test_flag *flag) { return atomic_load_explicit(flag, memory_order_acquire); }
static void native_yield(void) { sched_yield(); }
#endif
#include "../src/cxcore.h"
CX_API cx_uint cx_runtime_once_test_waiter_count(struct cx_runtime_once *once);

static struct cx_runtime_once initialized;
static struct cx_runtime_once failed;
static test_flag worker_waiting;
static test_flag worker_has_exception;
static test_flag release_exception;
static int initialized_value;
static int worker_failed;

static void wait_for_release(void *context) {
    (void)context;
    signal_flag(&worker_has_exception);
    while (!read_flag(&release_exception)) native_yield();
}

#ifdef _WIN32
__declspec(noinline)
#else
__attribute__((noinline))
#endif
static void throw_managed_value(void) {
    cx_uint *value = (cx_uint *)CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_uint));
    *value = 0xabcdef;
    cx_exception_throw(value, __FILE__, __LINE__);
}

static void worker_body(void) {
    struct cx_exception_frame frame;
    cx_gc_attach_thread();
    signal_flag(&worker_waiting);
    if (cx_runtime_once_begin(&initialized) || initialized_value != 42) {
        worker_failed = 1;
    }
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        throw_managed_value();
        abort();
    }
    cx_exception_pop(&frame);
    // No exception value has been read into the caller's stack yet: only TLS
    // retains it across this parked interval and the parent's collection.
    cx_gc_run_blocking(wait_for_release, NULL);
    if (*(cx_uint *)cx_exception_current() != 0xabcdef) worker_failed = 2;
    if (!cx_runtime_once_begin(&failed)) worker_failed = 3;
    cx_runtime_once_fail(&failed, cx_exception_current());
    cx_exception_clear();
    cx_gc_detach_thread();
}

#ifdef _WIN32
static DWORD WINAPI worker_entry(LPVOID context) { (void)context; worker_body(); return 0; }
static void join_worker(void *context) {
    HANDLE *thread = (HANDLE *)context;
    if (WaitForSingleObject(*thread, INFINITE) != WAIT_OBJECT_0) ExitProcess(9);
    CloseHandle(*thread);
}
#else
static void *worker_entry(void *context) { (void)context; worker_body(); return NULL; }
static void join_worker(void *context) {
    if (pthread_join(*(pthread_t *)context, NULL) != 0) _Exit(9);
}
#endif

int main(void) {
    test_thread thread;
    struct cx_exception_frame frame;
    if (!cx_runtime_once_begin(&initialized) || cx_runtime_once_begin(&initialized)) return 1;
#ifdef _WIN32
    thread = CreateThread(NULL, 0, worker_entry, NULL, 0, NULL);
    if (thread == NULL) return 2;
#else
    if (pthread_create(&thread, NULL, worker_entry, NULL) != 0) return 2;
#endif
    while (!read_flag(&worker_waiting)) { cx_gc_safepoint(); native_yield(); }
    while (cx_runtime_once_test_waiter_count(&initialized) != 1) native_yield();
    // The initializer must be able to collect while another thread waits on it.
    (void)CX_ID_4(cxcore, System, Memory, Alloc)(32);
    cx_gc_collect();
    initialized_value = 42;
    cx_runtime_once_complete(&initialized);
    while (!read_flag(&worker_has_exception)) { cx_gc_safepoint(); native_yield(); }
    cx_gc_collect();
    signal_flag(&release_exception);
    cx_gc_run_blocking(join_worker, &thread);
    if (worker_failed != 0) return 3;
    cx_gc_collect(); // The failed gate, rather than the departed worker's TLS, roots the value.
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        (void)cx_runtime_once_begin(&failed);
        return 4;
    }
    cx_exception_pop(&frame);
    if (*(cx_uint *)cx_exception_current() != 0xabcdef) return 5;
    cx_exception_clear();
    cx_gc_detach_thread();
    return 0;
}
