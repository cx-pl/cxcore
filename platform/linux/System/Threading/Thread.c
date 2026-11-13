#define _POSIX_C_SOURCE 200809L

#include "../../../platform.h"
#ifdef CX_PLATFORM_LINUX
#include "../../../common/System/Threading/Thread.internal.h"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

static void *cx_thread_entry(void *context) {
    cx_thread_worker(context);
    return NULL;
}

int cx_platform_thread_start(void *context) {
    pthread_t thread;
    pthread_attr_t attributes;
    int started = 0;
    if (pthread_attr_init(&attributes) == 0) {
        if (pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED) == 0) {
            started = pthread_create(&thread, &attributes, cx_thread_entry, context) == 0;
        }
        pthread_attr_destroy(&attributes);
    }
    return started;
}

void cx_platform_thread_sleep(uint32_t milliseconds) {
    struct timespec duration = {
        (time_t)(milliseconds / 1000), (long)(milliseconds % 1000) * 1000000L
    };
    while (nanosleep(&duration, &duration) != 0 && errno == EINTR) { }
}

uint64_t cx_platform_monotonic_time_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }
    return (uint64_t)now.tv_sec * 1000u + (uint64_t)now.tv_nsec / 1000000u;
}

void cx_platform_thread_yield(void) {
    sched_yield();
}



#endif // CX_PLATFORM_LINUX
