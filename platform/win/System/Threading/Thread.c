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

void cx_platform_thread_yield(void) {
    SwitchToThread();
}
