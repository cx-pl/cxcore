#include "../../../platform.h"
#ifdef CX_PLATFORM_WIN
#include "../../../common/System/Threading/Thread.internal.h"

#include <windows.h>
#include <process.h>


static unsigned __stdcall cx_thread_entry(void *context) {
    cx_thread_worker(context);
    return 0;
}

int cx_platform_thread_start(void *context) {
    uintptr_t handle = _beginthreadex(NULL, 0, cx_thread_entry, context, 0, NULL);
    if (handle == 0) return 0;
    CloseHandle((HANDLE)handle);
    return 1;
}

void cx_platform_thread_sleep(uint32_t milliseconds) {
    Sleep(milliseconds);
}

uint64_t cx_platform_monotonic_time_milliseconds(void) {
    return (uint64_t)GetTickCount64();
}

void cx_platform_thread_yield(void) {
    SwitchToThread();
}



#endif // CX_PLATFORM_WIN
