#include "../platform.h"

#include <windows.h>

cx_platform_thread_id cx_platform_current_thread_id(void) {
    return (cx_platform_thread_id)GetCurrentThreadId();
}

cx_platform_thread_id cx_platform_owner_thread_load(
    const cx_platform_atomic_thread_id *owner) {
    return (cx_platform_thread_id)InterlockedCompareExchange(
        (volatile LONG *)owner, 0, 0);
}

cx_platform_thread_id cx_platform_owner_thread_claim(
    cx_platform_atomic_thread_id *owner,
    cx_platform_thread_id current_thread) {
    return (cx_platform_thread_id)InterlockedCompareExchange(
        owner, (LONG)current_thread, 0);
}

int cx_platform_current_stack_bounds(uintptr_t *low, uintptr_t *high) {
    ULONG_PTR stack_low;
    ULONG_PTR stack_high;
    GetCurrentThreadStackLimits(&stack_low, &stack_high);
    *low = (uintptr_t)stack_low;
    *high = (uintptr_t)stack_high;
    return 1;
}
