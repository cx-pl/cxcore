#define _GNU_SOURCE

#include "../platform.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

static _Atomic cx_platform_thread_id cx_next_thread_id = 1;
static _Thread_local cx_platform_thread_id cx_local_thread_id;

cx_platform_thread_id cx_platform_current_thread_id(void) {
    if (cx_local_thread_id == 0) {
        cx_local_thread_id = atomic_fetch_add_explicit(
            &cx_next_thread_id, 1, memory_order_relaxed);
    }
    return cx_local_thread_id;
}

cx_platform_thread_id cx_platform_owner_thread_load(
    const cx_platform_atomic_thread_id *owner) {
    return atomic_load_explicit(owner, memory_order_acquire);
}

cx_platform_thread_id cx_platform_owner_thread_claim(
    cx_platform_atomic_thread_id *owner,
    cx_platform_thread_id current_thread) {
    cx_platform_thread_id expected = 0;
    atomic_compare_exchange_strong_explicit(
        owner, &expected, current_thread,
        memory_order_acq_rel, memory_order_acquire);
    return expected;
}

int cx_platform_current_stack_bounds(uintptr_t *low, uintptr_t *high) {
    pthread_attr_t attributes;
    void *stack_address;
    size_t stack_size;
    int result;

    if (pthread_getattr_np(pthread_self(), &attributes) != 0) {
        return 0;
    }
    result = pthread_attr_getstack(&attributes, &stack_address, &stack_size);
    pthread_attr_destroy(&attributes);
    if (result != 0 || stack_size > UINTPTR_MAX - (uintptr_t)stack_address) {
        return 0;
    }
    *low = (uintptr_t)stack_address;
    *high = *low + stack_size;
    return 1;
}
