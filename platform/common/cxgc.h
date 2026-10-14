#ifndef CX_GC_INTERNAL_H
#define CX_GC_INTERNAL_H

#include "cx.h"

// Internal runtime registries share the heap mutex. Callers must not allocate
// or call other managed entry points while holding it.
void cx_gc_runtime_lock(void);
void cx_gc_runtime_unlock(void);
void cx_gc_runtime_wait(void);
void cx_gc_runtime_wake(void);
void cx_gc_runtime_detach(void);
cx_ulong cx_gc_runtime_thread_id(void);
void cx_gc_runtime_mutex_acquired(void);
void cx_gc_runtime_mutex_released(void);
cx_ptr *cx_exception_thread_root(void);
const struct cx_exception *cx_exception_thread_state(void);
void cx_exception_restore(const struct cx_exception *state);
cx_bool cx_gc_resume_native_callback(void);
void cx_gc_park_native_callback(void);
void cx_gc_native_lock(void);
void cx_gc_native_unlock(void);
void cx_gc_native_wake(void);
cx_bool cx_exception_thread_detach(void);

#endif
