#include "cxcore.h"
#include "cxgc.h"
#include "../../../platform.h"
#include "../Threading/Threading.internal.h"
#include "../../cxarena.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct cxcore_System_Interop_NativeCallbackRegistration cx_registration;
struct cx_registration_state {
    void *callback;
    size_t callback_size;
    char *signature;
    cx_ptr owner;
    cx_ptr failure;
    struct cx_arena *logical_arena;
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
    cx_threading_raise(&cx_registration_invalid);
    abort();
}
static struct cx_registration_state *cx_registration_find(void *context) {
    struct cx_registration_state *state;
    for (state = cx_registration_states; state != NULL; state = state->next)
        if (state == context) return state;
    return NULL;
}

static void cx_registration_discard_unpublished(
    struct cx_registration_state *state) {
    if (state == NULL) return;
    if (state->callback_root != NULL) cx_gc_remove_root(state->callback_root);
    if (state->owner_root != NULL) cx_gc_remove_root(state->owner_root);
    if (state->failure_root != NULL) cx_gc_remove_root(state->failure_root);
    free(state->callback);
    free(state->signature);
    if (state->logical_arena != NULL) {
        cx_arena_release_logical(state->logical_arena);
    }
    free(state);
}

void cxcore_System_Interop_NativeCallbackRegistration___constructor(cx_registration *self) {
    cxcore_System_Object___constructor(&self->__base);
    CX_INIT_VTABLE(self, cxcore_System_Interop_NativeCallbackRegistration);
    (void)cx_gc_store_native_pointer(&self->_state, NULL);
    (void)cx_gc_store_reference((cx_ptr *)&self->_lastException, NULL);
}
cx_registration *cx_interop_registration_create(const void *callback, size_t size, const char *signature, cx_ptr owner) {
    struct cx_registration_state *state;
    cx_registration *result;
    cx_action callback_value;
    struct cx_arena *logical_arena;
    volatile cx_ptr owner_keepalive = owner;
    cx_gc_check_thread();
    if (!cx_arena_scheduler_use_allowed() || callback == NULL ||
        size != sizeof(callback_value) || size > UINT_MAX || signature == NULL) {
        cx_registration_error();
    }
    memcpy(&callback_value, callback, sizeof(callback_value));
    logical_arena = cx_arena_capture_logical_context();
    if (logical_arena != NULL) {
        int references_allowed;
        cx_gc_runtime_lock();
        references_allowed =
            cx_arena_retained_reference_source_allowed_locked(
                logical_arena, (cx_ptr)callback_value.__environment) &&
            cx_arena_retained_reference_source_allowed_locked(
                logical_arena, callback_value.__target) &&
            cx_arena_retained_reference_source_allowed_locked(
                logical_arena, (cx_ptr)callback_value.__invocation_list) &&
            cx_arena_retained_reference_source_allowed_locked(logical_arena, owner);
        cx_gc_runtime_unlock();
        if (!references_allowed) {
            cx_arena_release_logical(logical_arena);
            cx_registration_error();
        }
    } else if (!cx_gc_is_durable_reference((cx_ptr)callback_value.__environment) ||
               !cx_gc_is_durable_reference(callback_value.__target) ||
               !cx_gc_is_durable_reference((cx_ptr)callback_value.__invocation_list) ||
               !cx_gc_is_durable_reference(owner)) {
        cx_registration_error();
    }
    state = cx_interop_context_alloc(sizeof(*state)); memset(state, 0, sizeof(*state));
    state->logical_arena = logical_arena;
    state->callback = cx_interop_context_alloc(size); memcpy(state->callback, callback, size);
    state->callback_size = size;
    state->signature = cx_interop_context_alloc(strlen(signature) + 1); memcpy(state->signature, signature, strlen(signature) + 1);
    state->callback_root = cx_gc_add_root(state->callback, (cx_uint)size);
    state->owner_root = cx_gc_add_root(&state->owner, (cx_uint)sizeof(state->owner));
    state->failure_root = cx_gc_add_root(&state->failure, (cx_uint)sizeof(state->failure));
    if (logical_arena != NULL) {
        const cx_ptr callback_references[] = {
            (cx_ptr)callback_value.__environment,
            callback_value.__target,
            (cx_ptr)callback_value.__invocation_list
        };
        cx_action *retained_callback = (cx_action *)state->callback;
        cx_ptr *callback_destinations[] = {
            (cx_ptr *)&retained_callback->__environment,
            &retained_callback->__target,
            (cx_ptr *)&retained_callback->__invocation_list
        };
        cx_ptr *owner_destination = &state->owner;
        if (!cx_gc_store_references_retained_arena(callback_destinations,
                callback_references,
                (cx_uint)(sizeof(callback_references) / sizeof(callback_references[0])),
                logical_arena) ||
            !cx_gc_store_references_retained_arena(&owner_destination, &owner, 1u,
                logical_arena)) {
            cx_registration_discard_unpublished(state);
            cx_registration_error();
        }
    } else {
        (void)cx_gc_store_reference(&state->owner, owner);
    }
    (void)owner_keepalive;
    cx_gc_runtime_lock(); state->next = cx_registration_states; cx_registration_states = state; cx_gc_runtime_unlock();
    {
        struct cx_arena_durable_scope durable_scope;
        cx_arena_enter_durable(&durable_scope);
        result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
        cxcore_System_Interop_NativeCallbackRegistration___constructor(result);
        cx_arena_leave_durable(&durable_scope);
    }
    (void)cx_gc_store_native_pointer(&result->_state, state);
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
    scope->logical_arena = state->logical_arena;
    scope->arena_scope = NULL;
    cx_gc_native_unlock(); return CX_TRUE;
}
cx_bool cx_interop_registration_enter_logical(
    struct cx_native_registration_scope *scope,
    const volatile char *callback_stack_boundary) {
    struct cx_arena_scope *arena_scope;
    if (scope == NULL || scope->state == NULL || scope->logical_arena == NULL) {
        return CX_FALSE;
    }
    if (callback_stack_boundary == NULL || scope->arena_scope != NULL) {
        cx_registration_error();
    }
    arena_scope = calloc(1u, sizeof(*arena_scope));
    if (arena_scope == NULL) cx_registration_error();
    scope->arena_scope = arena_scope;
    cx_arena_enter_logical(scope->logical_arena, arena_scope);
    cx_arena_allow_callback_stack_borrows(arena_scope, callback_stack_boundary);
    return CX_TRUE;
}
void cx_interop_registration_leave_logical(
    struct cx_native_registration_scope *scope) {
    if (scope != NULL && scope->logical_arena != NULL) {
        struct cx_arena_scope *arena_scope = scope->arena_scope;
        if (arena_scope == NULL) cx_registration_error();
        cx_arena_leave(arena_scope);
        scope->arena_scope = NULL;
        free(arena_scope);
    }
}
void cx_interop_registration_release(struct cx_native_registration_scope *scope) {
    struct cx_registration_state *state = scope->state;
    if (scope->arena_scope != NULL) cx_registration_error();
    cx_gc_native_lock();
    cx_registration_current = scope->previous;
    scope->logical_arena = NULL;
    --state->in_flight; cx_gc_native_wake(); cx_gc_native_unlock();
}
const void *cx_interop_registration_payload(void *context) {
    return ((struct cx_registration_state *)context)->callback;
}
void cx_interop_registration_failed(void *context, cx_ptr error) {
    struct cx_registration_state *state = context;
    if (state->logical_arena != NULL) {
        error = cx_exception_copy_durable(error);
    }
    (void)cx_gc_store_reference(&state->failure, error);
    cx_interop_callback_failed(error);
}
cx_ptr cxcore_System_Interop_NativeCallbackRegistration_Context___const_get(const cx_registration *self) {
    if (self == NULL || self->_state == NULL) cx_registration_error();
    return self->_state;
}
struct cxcore_System_Exception *cxcore_System_Interop_NativeCallbackRegistration_LastException___const_get(const cx_registration *self) {
    struct cx_registration_state *state;
    cx_ptr failure;
    cx_gc_runtime_lock(); state = self == NULL ? NULL : cx_registration_find(self->_state);
    failure = state == NULL || state->closed ? (self == NULL ? NULL : self->_lastException) : state->failure;
    cx_gc_runtime_unlock(); return failure;
}
void cxcore_System_Interop_NativeCallbackRegistration_Unregister(cx_registration *self, cx_action unregister) {
    struct cx_registration_state *volatile state;
    struct cx_native_registration_scope *active;
    struct cx_gc_root_handle *callback_root, *owner_root, *failure_root;
    struct cx_arena *logical_arena;
    cx_ptr failure;
    void *payload;
    cx_registration *volatile keepalive = self;
    struct cx_exception_frame frame;
    cx_gc_runtime_lock(); state = self == NULL ? NULL : cx_registration_find(self->_state);
    if (state == NULL || (unregister.__function == NULL && unregister.__invocation_list == NULL)) {
        cx_gc_runtime_unlock(); cx_registration_error();
    }
    if (state->closed) { cx_gc_runtime_unlock(); return; }
    if (state->logical_arena != NULL && cx_arena_current_is(state->logical_arena)) {
        cx_gc_runtime_unlock(); cx_registration_error();
    }
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
    failure = state->failure;
    callback_root = state->callback_root; owner_root = state->owner_root; failure_root = state->failure_root;
    payload = state->callback;
    logical_arena = state->logical_arena;
    state->callback = NULL;
    (void)cx_gc_store_reference_locked((cx_ptr *)&state->owner, NULL);
    (void)cx_gc_store_reference_locked((cx_ptr *)&state->failure, NULL);
    state->closed = CX_TRUE;
    state->logical_arena = NULL;
    state->unregistering = CX_FALSE; state->unregister_owner = 0; cx_gc_runtime_wake();
    cx_gc_runtime_unlock();
    (void)cx_gc_store_reference((cx_ptr *)&self->_lastException, failure);
    cx_gc_remove_root(callback_root); cx_gc_remove_root(owner_root); cx_gc_remove_root(failure_root);
    if (logical_arena != NULL) cx_arena_release_logical(logical_arena);
    free(payload); free(state->signature); state->signature = NULL;
    (void)keepalive;
    // Keep the small context token until exit. Late native callbacks see closed,
    // and addresses cannot be recycled into an unrelated registration (no ABA).
}
