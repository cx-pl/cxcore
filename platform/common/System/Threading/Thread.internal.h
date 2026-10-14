#ifndef CX_THREAD_INTERNAL_H
#define CX_THREAD_INTERNAL_H

#include <stdint.h>

void cx_thread_worker(void *context);
int cx_platform_thread_start(void *context);
void cx_platform_thread_sleep(uint32_t milliseconds);
void cx_platform_thread_yield(void);

#endif
