#ifndef __CX_THREAD_INTERNAL_H__
#define __CX_THREAD_INTERNAL_H__

#include <stdint.h>
#include "../../../platform.h"

void cx_thread_worker(void *context);
int cx_platform_thread_start(void *context);
uint64_t cx_platform_monotonic_time_milliseconds(void);
void cx_platform_thread_sleep(uint32_t milliseconds);
void cx_platform_thread_yield(void);

#endif // __CX_THREAD_INTERNAL_H__
