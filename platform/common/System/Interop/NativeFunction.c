#include "cxcore.h"
#include "cxgc.h"

static cx_ptr cx_callback_exception;
static const struct cx_gc_root_region cx_callback_exception_root[] = {
    { &cx_callback_exception, (cx_uint)sizeof(cx_callback_exception) }
};

void cx_interop_callback_failed(cx_ptr exception) {
    cx_gc_register_static_roots(cx_callback_exception_root, 1);
    (void)cx_gc_store_reference(&cx_callback_exception, exception);
}

struct cxcore_System_Exception *cxcore_System_Interop_NativeCallback_GetLastException(void) {
    cx_ptr exception;
    cx_gc_check_thread();
    cx_gc_runtime_lock(); exception = cx_callback_exception; cx_gc_runtime_unlock();
    return exception;
}
void cxcore_System_Interop_NativeCallback_ClearLastException(void) {
    cx_gc_check_thread();
    cx_gc_runtime_lock();
    (void)cx_gc_store_reference_locked(&cx_callback_exception, NULL);
    cx_gc_runtime_unlock();
}
