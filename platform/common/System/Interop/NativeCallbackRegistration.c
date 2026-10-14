#include "cxcore.h"
#include "cxgc.h"
#include "../../../platform.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct CX_ID_4(cxcore, System, Interop, NativeCallbackRegistration) cx_registration;
struct cx_registration_state {
    void *callback;
    size_t callback_size;
    char *signature;
    cx_ptr owner;
    cx_ptr failure;
    struct cx_gc_root_handle *callback_root;
    struct cx_gc_root_handle *owner_root;
    struct cx_gc_root_handle *failure_root;
    cx_uint in_flight;
    cx_bool closing;
    cx_bool closed;
    cx_bool unregistering;
    cx_ulong unregister_owner;
    struct cx_registration_state *next;
};
static struct cx_registration_state *cx_registration_states;
static CX_THREAD_LOCAL struct cx_native_registration_scope *cx_registration_current;
CX_STRING_DEF(cx_registration_invalid, "Invalid callback registration, signature, or self unregistration");

static CX_NORETURN void cx_registration_error(void) {
    struct CX_ID_3(cxcore, System, Exception) *error = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*error));
    CX_ID_4(cxcore, System, Exception, __constructor)(error, &cx_registration_invalid);
    cx_exception_throw(error, __FILE__, __LINE__);
}
static struct cx_registration_state *cx_registration_find(void *context) {
    struct cx_registration_state *state;
    for (state = cx_registration_states; state != NULL; state = state->next)
        if (state == context) return state;
    return NULL;
}

void CX_ID_5(cxcore, System, Interop, NativeCallbackRegistration, __constructor)(cx_registration *self) {
    CX_ID_4(cxcore, System, Object, __constructor)(&self->__base);
    CX_INIT_VTABLE(self, CX_ID_4(cxcore, System, Interop, NativeCallbackRegistration));
    self->_state = NULL; self->_lastException = NULL;
}
cx_registration *cx_interop_registration_create(const void *callback, size_t size, const char *signature, cx_ptr owner) {
    struct cx_registration_state *state;
    cx_registration *result;
    volatile cx_ptr owner_keepalive = owner;
    cx_gc_check_thread();
    if (callback == NULL || size == 0 || size > UINT_MAX || signature == NULL) cx_registration_error();
    state = cx_interop_context_alloc(sizeof(*state)); memset(state, 0, sizeof(*state));
    state->callback = cx_interop_context_alloc(size); memcpy(state->callback, callback, size);
    state->callback_size = size;
    state->signature = cx_interop_context_alloc(strlen(signature) + 1); memcpy(state->signature, signature, strlen(signature) + 1);
    state->owner = owner;
    state->callback_root = cx_gc_add_root(state->callback, (cx_uint)size);
    state->owner_root = cx_gc_add_root(&state->owner, (cx_uint)sizeof(state->owner));
    state->failure_root = cx_gc_add_root(&state->failure, (cx_uint)sizeof(state->failure));
    (void)owner_keepalive;
    cx_gc_runtime_lock(); state->next = cx_registration_states; cx_registration_states = state; cx_gc_runtime_unlock();
    result = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*result));
    CX_ID_5(cxcore, System, Interop, NativeCallbackRegistration, __constructor)(result);
    result->_state = state;
    return result;
}
void cx_interop_registration_require(const cx_registration *registration, const char *signature) {
    struct cx_registration_state *state;
    cx_bool valid;
    cx_gc_runtime_lock();
    state = registration == NULL ? NULL : cx_registration_find(registration->_state);
    valid = state != NULL && !state->closing && !state->closed && strcmp(state->signature, signature) == 0;
    cx_gc_runtime_unlock();
    if (!valid) cx_registration_error();
}
cx_bool cx_interop_registration_acquire(void *context, struct cx_native_registration_scope *scope) {
    struct cx_registration_state *state;
    cx_gc_native_lock(); state = cx_registration_find(context);
    if (state == NULL || state->closing || state->closed || state->in_flight == UINT_MAX) {
        cx_gc_native_unlock(); return CX_FALSE;
    }
    ++state->in_flight;
    scope->state = state; scope->previous = cx_registration_current; cx_registration_current = scope;
    cx_gc_native_unlock(); return CX_TRUE;
}
void cx_interop_registration_release(struct cx_native_registration_scope *scope) {
    struct cx_registration_state *state = scope->state;
    cx_gc_native_lock();
    cx_registration_current = scope->previous;
    --state->in_flight; cx_gc_native_wake(); cx_gc_native_unlock();
}
const void *cx_interop_registration_payload(void *context) {
    return ((struct cx_registration_state *)context)->callback;
}
void cx_interop_registration_failed(void *context, cx_ptr error) {
    struct cx_registration_state *state = context;
    cx_gc_runtime_lock(); state->failure = error; cx_gc_runtime_unlock();
    cx_interop_callback_failed(error);
}
cx_ptr CX_ID_6(cxcore, System, Interop, NativeCallbackRegistration, Context, __const_get)(const cx_registration *self) {
    if (self == NULL || self->_state == NULL) cx_registration_error();
    return self->_state;
}
struct CX_ID_3(cxcore, System, Exception) *CX_ID_6(cxcore, System, Interop, NativeCallbackRegistration, LastException, __const_get)(const cx_registration *self) {
    struct cx_registration_state *state;
    cx_ptr failure;
    cx_gc_runtime_lock(); state = self == NULL ? NULL : cx_registration_find(self->_state);
    failure = state == NULL || state->closed ? (self == NULL ? NULL : self->_lastException) : state->failure;
    cx_gc_runtime_unlock(); return failure;
}
void CX_ID_5(cxcore, System, Interop, NativeCallbackRegistration, Unregister)(cx_registration *self, cx_action unregister) {
    struct cx_registration_state *state;
    struct cx_native_registration_scope *active;
    struct cx_gc_root_handle *callback_root, *owner_root, *failure_root;
    void *payload;
    cx_registration *volatile keepalive = self;
    struct cx_exception_frame frame;
    cx_gc_runtime_lock(); state = self == NULL ? NULL : cx_registration_find(self->_state);
    if (state == NULL || (unregister.__function == NULL && unregister.__invocation_list == NULL)) {
        cx_gc_runtime_unlock(); cx_registration_error();
    }
    if (state->closed) { cx_gc_runtime_unlock(); return; }
    for (active = cx_registration_current; active != NULL; active = active->previous)
        if (active->state == state) { cx_gc_runtime_unlock(); cx_registration_error(); }
    while (state->unregistering && !state->closed) {
        if (state->unregister_owner == cx_gc_runtime_thread_id()) { cx_gc_runtime_unlock(); cx_registration_error(); }
        cx_gc_runtime_wait();
    }
    if (state->closed) { cx_gc_runtime_unlock(); return; }
    state->closing = CX_TRUE; state->unregistering = CX_TRUE;
    state->unregister_owner = cx_gc_runtime_thread_id(); cx_gc_runtime_unlock();
    // If the action throws, the registration stays closing and rooted. The
    // caller can retry; no unregistration success or buffer lifetime is assumed.
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        if (unregister.__invocation_list != NULL) {
            struct CX_FN_136FFEE706DF35B2_invocation *item;
            for (item = unregister.__invocation_list; item != NULL; item = item->next) item->value.__function(item->value.__environment);
        } else unregister.__function(unregister.__environment);
        cx_exception_pop(&frame);
    } else {
        cx_exception_pop(&frame);
        cx_gc_runtime_lock(); state->unregistering = CX_FALSE; state->unregister_owner = 0;
        cx_gc_runtime_wake(); cx_gc_runtime_unlock();
        cx_exception_rethrow();
    }
    cx_gc_runtime_lock();
    while (state->in_flight != 0) cx_gc_runtime_wait();
    self->_lastException = state->failure;
    callback_root = state->callback_root; owner_root = state->owner_root; failure_root = state->failure_root;
    payload = state->callback;
    state->callback = NULL; state->owner = NULL; state->failure = NULL; state->closed = CX_TRUE;
    state->unregistering = CX_FALSE; state->unregister_owner = 0; cx_gc_runtime_wake();
    cx_gc_runtime_unlock();
    cx_gc_remove_root(callback_root); cx_gc_remove_root(owner_root); cx_gc_remove_root(failure_root);
    free(payload); free(state->signature); state->signature = NULL;
    (void)keepalive;
    // Keep the small context token until exit. Late native callbacks see closed,
    // and addresses cannot be recycled into an unrelated registration (no ABA).
}
