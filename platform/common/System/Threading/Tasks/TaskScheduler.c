#include "../Threading.internal.h"
#include "../Thread.internal.h"
#include "TaskScheduler.internal.h"
#include "../../../cxarena.h"

#include <stdlib.h>
#include <string.h>

#define CX_TASK_SCHEDULER_REUSABLE_IDLE UINT64_MAX
#define CX_TASK_SCHEDULER_REUSABLE_QUEUED (UINT64_MAX - 1u)
#define CX_TASK_SCHEDULER_REUSABLE_DISPATCHING (UINT64_MAX - 2u)
#define CX_TASK_SCHEDULER_REUSABLE_EVENT_WAIT (UINT64_MAX - 3u)

enum cx_task_scheduler_payload_kind {
    CX_TASK_SCHEDULER_PAYLOAD_NONE,
    CX_TASK_SCHEDULER_PAYLOAD_COMPLETION,
    CX_TASK_SCHEDULER_PAYLOAD_TIMER,
    CX_TASK_SCHEDULER_PAYLOAD_WAIT,
};

struct cx_task_scheduler_item {
    cx_action continuation;
    cx_action shutdown_continuation;
    struct cx_arena *logical_arena;
    enum cx_task_scheduler_payload_kind payload_kind;
    union {
        struct {
            void (*function)(void *environment);
            void *environment;
        } completion;
        uint64_t due_time_milliseconds;
        struct {
            cx_ptr context;
            cx_scheduler_wait_function function;
        } wait;
    } payload;
    uint64_t registration_id;
    struct cx_task_scheduler_item *next;
};

static struct cx_task_scheduler_item *cx_task_scheduler_head;
static struct cx_task_scheduler_item *cx_task_scheduler_tail;
static struct cx_task_scheduler_item *cx_task_scheduler_timer_head;
static struct cx_task_scheduler_item *cx_task_scheduler_wait_head;
static uint64_t cx_task_scheduler_next_wait_registration_id = 1;
static cx_bool cx_task_scheduler_roots_registered;
static cx_platform_atomic_thread_id cx_task_scheduler_owner;
static cx_ptr cx_task_scheduler_run_until_fault_owner;
static cx_bool cx_task_scheduler_dispatching;
static cx_bool cx_task_scheduler_shutting_down;
static cx_bool cx_task_scheduler_shutdown_complete;

static uint64_t cx_task_scheduler_allocate_wait_registration_id(void) {
    uint64_t registration_id = cx_task_scheduler_next_wait_registration_id;
    if (registration_id == 0 ||
        registration_id >= CX_TASK_SCHEDULER_REUSABLE_DISPATCHING) {
        registration_id = 1;
    }
    cx_task_scheduler_next_wait_registration_id = registration_id + 1u;
    if (cx_task_scheduler_next_wait_registration_id == 0 ||
        cx_task_scheduler_next_wait_registration_id >=
            CX_TASK_SCHEDULER_REUSABLE_DISPATCHING) {
        cx_task_scheduler_next_wait_registration_id = 1;
    }
    return registration_id;
}
#ifdef CXCORE_TESTING
static size_t cx_task_scheduler_test_post_allocation_count;

CX_API void cx_task_scheduler_test_post_allocation_count_reset(void) {
    cx_task_scheduler_test_post_allocation_count = 0;
}

CX_API size_t cx_task_scheduler_test_post_allocation_count_get(void) {
    return cx_task_scheduler_test_post_allocation_count;
}

CX_API size_t cx_task_scheduler_test_item_size(void) {
    return sizeof(struct cx_task_scheduler_item);
}

#endif

static const struct cx_gc_root_region cx_task_scheduler_roots[] = {
    {&cx_task_scheduler_head, (cx_uint)sizeof(cx_task_scheduler_head)},
    {&cx_task_scheduler_timer_head, (cx_uint)sizeof(cx_task_scheduler_timer_head)},
    {&cx_task_scheduler_wait_head, (cx_uint)sizeof(cx_task_scheduler_wait_head)},
};

CX_STRING_DEF(cx_task_scheduler_null, "TaskScheduler.Post requires a non-null continuation");
CX_STRING_DEF(cx_task_scheduler_continuation_parameter, "continuation");
CX_STRING_DEF(cx_task_scheduler_event_wait_failed, "TaskScheduler failed while waiting for external events");
CX_STRING_DEF(cx_task_scheduler_wrong_thread, "TaskScheduler may only be accessed from its owner thread");
CX_STRING_DEF(cx_task_scheduler_reentrant, "TaskScheduler.Run and RunOne cannot be called from a scheduler callback");
CX_STRING_DEF(cx_task_scheduler_no_completion_path, "RunUntil task has no completion path");
CX_STRING_DEF(cx_task_scheduler_shutdown, "TaskScheduler cannot accept work after shutdown completes");
CX_STRING_DEF(cx_task_scheduler_reusable_item_busy, "TaskScheduler reusable continuation item is already queued or dispatching");
CX_STRING_DEF(cx_task_scheduler_scoped_async, "TaskScheduler cannot retain work from a synchronous allocation arena");

static void cx_task_scheduler_require_durable_context(void) {
    if (!cx_arena_scheduler_use_allowed()) {
        cx_threading_raise(&cx_task_scheduler_scoped_async);
    }
}

static void cx_task_scheduler_store_action(
    cx_action *destination, cx_action value) {
    destination->__function = value.__function;
    cx_gc_store_reference(
        (cx_ptr *)&destination->__environment, (cx_ptr)value.__environment);
    cx_gc_store_reference(
        (cx_ptr *)&destination->__target, value.__target);
    destination->__method = value.__method;
    cx_gc_store_reference(
        (cx_ptr *)&destination->__invocation_list,
        (cx_ptr)value.__invocation_list);
}

static cx_bool cx_task_scheduler_store_action_for_arena(
    cx_action *destination,
    cx_action value,
    struct cx_arena *logical_arena) {
    if (logical_arena == NULL) {
        cx_task_scheduler_store_action(destination, value);
        return CX_TRUE;
    }
    {
        cx_ptr *destinations[3] = {
            (cx_ptr *)&destination->__environment,
            (cx_ptr *)&destination->__target,
            (cx_ptr *)&destination->__invocation_list,
        };
        cx_ptr values[3] = {
            (cx_ptr)value.__environment,
            value.__target,
            (cx_ptr)value.__invocation_list,
        };
        if (!cx_gc_store_references_retained_arena(
                destinations, values, 3u, logical_arena)) {
            return CX_FALSE;
        }
        destination->__function = value.__function;
        destination->__method = value.__method;
    }
    return CX_TRUE;
}

static cx_bool cx_task_scheduler_store_event_callbacks(
    struct cx_task_scheduler_item *item,
    cx_ptr context,
    cx_scheduler_wait_function wait,
    cx_action continuation,
    cx_action shutdown_continuation,
    struct cx_arena *logical_arena) {
    // Provider contexts may be inspected while the scheduler is blocked on a
    // different thread; only the managed callback itself is lease-scoped.
    if (!cx_gc_is_durable_reference(context)) return CX_FALSE;
    if (logical_arena == NULL) {
        cx_task_scheduler_store_action(&item->continuation, continuation);
        cx_task_scheduler_store_action(
            &item->shutdown_continuation, shutdown_continuation);
        (void)cx_gc_store_reference(
            (cx_ptr *)&item->payload.wait.context, context);
    } else {
        cx_ptr *destinations[7] = {
            (cx_ptr *)&item->continuation.__environment,
            (cx_ptr *)&item->continuation.__target,
            (cx_ptr *)&item->continuation.__invocation_list,
            (cx_ptr *)&item->shutdown_continuation.__environment,
            (cx_ptr *)&item->shutdown_continuation.__target,
            (cx_ptr *)&item->shutdown_continuation.__invocation_list,
            (cx_ptr *)&item->payload.wait.context,
        };
        cx_ptr values[7] = {
            (cx_ptr)continuation.__environment,
            continuation.__target,
            (cx_ptr)continuation.__invocation_list,
            (cx_ptr)shutdown_continuation.__environment,
            shutdown_continuation.__target,
            (cx_ptr)shutdown_continuation.__invocation_list,
            context,
        };
        if (!cx_gc_store_references_retained_arena(
                destinations, values, 7u, logical_arena)) {
            return CX_FALSE;
        }
        item->continuation.__function = continuation.__function;
        item->continuation.__method = continuation.__method;
        item->shutdown_continuation.__function =
            shutdown_continuation.__function;
        item->shutdown_continuation.__method = shutdown_continuation.__method;
    }
    item->payload.wait.function = wait;
    return CX_TRUE;
}

static cx_bool cx_task_scheduler_store_action_with_context(
    struct cx_task_scheduler_item *item,
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    struct cx_arena *logical_arena) {
    if (logical_arena == NULL) {
        cx_task_scheduler_store_action(&item->continuation, continuation);
        (void)cx_gc_store_reference(
            (cx_ptr *)&item->payload.completion.environment,
            (cx_ptr)environment);
    } else {
        cx_ptr *destinations[4] = {
            (cx_ptr *)&item->continuation.__environment,
            (cx_ptr *)&item->continuation.__target,
            (cx_ptr *)&item->continuation.__invocation_list,
            (cx_ptr *)&item->payload.completion.environment,
        };
        cx_ptr values[4] = {
            (cx_ptr)continuation.__environment,
            continuation.__target,
            (cx_ptr)continuation.__invocation_list,
            (cx_ptr)environment,
        };
        if (!cx_gc_store_references_retained_arena(
                destinations, values, 4u, logical_arena)) {
            return CX_FALSE;
        }
        item->continuation.__function = continuation.__function;
        item->continuation.__method = continuation.__method;
    }
    item->payload.completion.function = completion;
    return CX_TRUE;
}

static void cx_task_scheduler_store_reference(cx_ptr *destination, cx_ptr value) {
    (void)cx_gc_store_reference(destination, value);
}

static void cx_task_scheduler_clear_payload(
    struct cx_task_scheduler_item *item) {
    if (item->payload_kind == CX_TASK_SCHEDULER_PAYLOAD_WAIT) {
        cx_task_scheduler_store_reference(
            &item->payload.wait.context, NULL);
        item->payload.wait.function = NULL;
    } else if (item->payload_kind == CX_TASK_SCHEDULER_PAYLOAD_COMPLETION) {
        cx_task_scheduler_store_reference(
            (cx_ptr *)&item->payload.completion.environment, NULL);
        item->payload.completion.function = NULL;
    } else if (item->payload_kind == CX_TASK_SCHEDULER_PAYLOAD_TIMER) {
        item->payload.due_time_milliseconds = 0;
    }
    item->payload_kind = CX_TASK_SCHEDULER_PAYLOAD_NONE;
}

static struct cx_task_scheduler_item *cx_task_scheduler_allocate_item(void) {
    struct cx_arena_durable_scope durable_scope;
    struct cx_task_scheduler_item *item;
    cx_arena_enter_durable(&durable_scope);
    item = (struct cx_task_scheduler_item *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*item));
    cx_arena_leave_durable(&durable_scope);
    return item;
}

static void cx_task_scheduler_capture_logical_arena(
    struct cx_task_scheduler_item *item) {
    (void)cx_gc_store_native_pointer(
        (cx_ptr *)&item->logical_arena,
        (cx_ptr)cx_arena_capture_logical_context());
}

static void cx_task_scheduler_release_logical_arena(
    struct cx_task_scheduler_item *item) {
    struct cx_arena *arena = item->logical_arena;
    (void)cx_gc_store_native_pointer((cx_ptr *)&item->logical_arena, NULL);
    if (arena != NULL) cx_arena_release_logical(arena);
}

static void cx_task_scheduler_require_owner(void) {
    cx_platform_thread_id current = cx_platform_current_thread_id();
    cx_platform_owner_thread_claim(&cx_task_scheduler_owner, current);
    if (cx_platform_owner_thread_load(&cx_task_scheduler_owner) != current) {
        cx_threading_raise(&cx_task_scheduler_wrong_thread);
    }
}

CX_API cx_bool cx_task_scheduler_fault_is_deferred(void *owner) {
    return owner != NULL && owner == cx_task_scheduler_run_until_fault_owner;
}

static void cx_task_scheduler_queue_ready(struct cx_task_scheduler_item *item) {
    cx_task_scheduler_store_reference((cx_ptr *)&item->next, NULL);
    if (item->registration_id == CX_TASK_SCHEDULER_REUSABLE_IDLE) {
        item->registration_id = CX_TASK_SCHEDULER_REUSABLE_QUEUED;
    }
    if (cx_task_scheduler_tail == NULL) {
        cx_task_scheduler_store_reference(
            (cx_ptr *)&cx_task_scheduler_head, (cx_ptr)item);
    } else {
        cx_task_scheduler_store_reference(
            (cx_ptr *)&cx_task_scheduler_tail->next, (cx_ptr)item);
    }
    cx_task_scheduler_store_reference(
        (cx_ptr *)&cx_task_scheduler_tail, (cx_ptr)item);
}

static void cx_task_scheduler_finish_dispatch(
    struct cx_task_scheduler_item *item) {
    cx_task_scheduler_store_action(&item->continuation, (cx_action){0});
    cx_task_scheduler_store_action(
        &item->shutdown_continuation, (cx_action){0});
    cx_task_scheduler_clear_payload(item);
    if (item->registration_id == CX_TASK_SCHEDULER_REUSABLE_DISPATCHING) {
        item->registration_id = CX_TASK_SCHEDULER_REUSABLE_IDLE;
    }
    cx_task_scheduler_release_logical_arena(item);
}

static void cx_task_scheduler_post_ready(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    struct cx_arena *logical_arena,
    cx_bool capture_current_context) {
    struct cx_task_scheduler_item *item;
    cx_gc_check_thread();
    cx_task_scheduler_require_durable_context();
    cx_task_scheduler_require_owner();
    if (cx_task_scheduler_shutdown_complete) {
        cx_threading_raise(&cx_task_scheduler_shutdown);
    }
    if (continuation.__function == NULL) {
        cx_threading_raise_argument_null(
            &cx_task_scheduler_null, &cx_task_scheduler_continuation_parameter);
    }
    if (!cx_task_scheduler_roots_registered) {
        cx_gc_register_static_roots(
            cx_task_scheduler_roots,
            (cx_uint)(sizeof(cx_task_scheduler_roots) /
                sizeof(cx_task_scheduler_roots[0])));
        cx_task_scheduler_roots_registered = CX_TRUE;
    }
    item = cx_task_scheduler_allocate_item();
#ifdef CXCORE_TESTING
    ++cx_task_scheduler_test_post_allocation_count;
#endif
    (void)cx_gc_store_native_pointer(
        (cx_ptr *)&item->logical_arena,
        (cx_ptr)(capture_current_context
            ? cx_arena_capture_logical_context()
            : logical_arena));
    item->payload_kind = completion == NULL
        ? CX_TASK_SCHEDULER_PAYLOAD_NONE
        : CX_TASK_SCHEDULER_PAYLOAD_COMPLETION;
    if (!cx_task_scheduler_store_action_with_context(
            item, continuation, completion, environment,
            item->logical_arena)) {
        struct cx_arena *failed_arena = item->logical_arena;
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&item->logical_arena, NULL);
        if (failed_arena != NULL) cx_arena_release_logical(failed_arena);
        cx_threading_raise(&cx_task_scheduler_scoped_async);
    }
    cx_task_scheduler_store_action(&item->shutdown_continuation, (cx_action){0});
    item->registration_id = 0;
    cx_task_scheduler_queue_ready(item);
}

void cxcore_System_Threading_Tasks_TaskScheduler_Post(
    cx_action continuation) {
    cx_task_scheduler_post_ready(
        continuation, NULL, NULL, NULL, CX_TRUE);
}

CX_API void cx_task_scheduler_post_with_completion(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment) {
    if (completion == NULL) {
        cx_threading_raise(&cx_task_scheduler_null);
    }
    cx_task_scheduler_post_ready(
        continuation, completion, environment, NULL, CX_TRUE);
}

CX_API void cx_task_scheduler_post_with_logical_context(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    struct cx_arena *logical_arena) {
    if (completion == NULL && environment != NULL) {
        cx_threading_raise(&cx_task_scheduler_null);
    }
    cx_task_scheduler_post_ready(
        continuation, completion, environment, logical_arena, CX_FALSE);
}

static void cx_task_scheduler_post_reusable(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    cx_ptr *cached_item,
    struct cx_arena *logical_arena,
    cx_bool capture_current_context) {
    struct cx_task_scheduler_item *item;
    cx_gc_check_thread();
    cx_task_scheduler_require_durable_context();
    cx_task_scheduler_require_owner();
    if (continuation.__function == NULL || completion == NULL || cached_item == NULL) {
        cx_threading_raise(&cx_task_scheduler_null);
    }
    if (cx_task_scheduler_shutdown_complete) {
        cx_threading_raise(&cx_task_scheduler_shutdown);
    }
    if (!cx_task_scheduler_roots_registered) {
        cx_gc_register_static_roots(
            cx_task_scheduler_roots,
            (cx_uint)(sizeof(cx_task_scheduler_roots) /
                sizeof(cx_task_scheduler_roots[0])));
        cx_task_scheduler_roots_registered = CX_TRUE;
    }
    if (*cached_item == NULL) {
        item = cx_task_scheduler_allocate_item();
#ifdef CXCORE_TESTING
        ++cx_task_scheduler_test_post_allocation_count;
#endif
        item->registration_id = CX_TASK_SCHEDULER_REUSABLE_IDLE;
        cx_task_scheduler_store_reference(cached_item, (cx_ptr)item);
    } else {
        item = (struct cx_task_scheduler_item *)*cached_item;
        if (item->registration_id != CX_TASK_SCHEDULER_REUSABLE_IDLE) {
            cx_threading_raise(&cx_task_scheduler_reusable_item_busy);
        }
    }
    if (capture_current_context) {
        logical_arena = cx_arena_capture_logical_context();
    }
    (void)cx_gc_store_native_pointer((cx_ptr *)&item->logical_arena, NULL);
    if (!cx_task_scheduler_store_action_with_context(
            item, continuation, completion, environment,
            logical_arena)) {
        if (capture_current_context && logical_arena != NULL) {
            cx_arena_release_logical(logical_arena);
        }
        cx_threading_raise(&cx_task_scheduler_scoped_async);
    }
    // The caller transfers its retained lease only after the callback and
    // completion references have passed the exact-arena store checks.
    (void)cx_gc_store_native_pointer(
        (cx_ptr *)&item->logical_arena, (cx_ptr)logical_arena);
    item->payload_kind = CX_TASK_SCHEDULER_PAYLOAD_COMPLETION;
    cx_task_scheduler_store_action(&item->shutdown_continuation, (cx_action){0});
    cx_task_scheduler_store_reference((cx_ptr *)&item->next, NULL);
    cx_task_scheduler_queue_ready(item);
}

CX_API void cx_task_scheduler_post_reusable_with_completion(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    cx_ptr *cached_item) {
    cx_task_scheduler_post_reusable(
        continuation, completion, environment, cached_item, NULL, CX_TRUE);
}

CX_API void cx_task_scheduler_post_reusable_with_logical_context(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    cx_ptr *cached_item,
    struct cx_arena *logical_arena) {
    cx_task_scheduler_post_reusable(
        continuation, completion, environment, cached_item,
        logical_arena, CX_FALSE);
}

void cxcore_System_Threading_Tasks_TaskScheduler_PostAfter(
    uint32_t milliseconds,
    cx_action continuation) {
    struct cx_task_scheduler_item *item;
    struct cx_task_scheduler_item **cursor;
    uint64_t now;
    cx_gc_check_thread();
    cx_task_scheduler_require_durable_context();
    cx_task_scheduler_require_owner();
    if (cx_task_scheduler_shutdown_complete) {
        cx_threading_raise(&cx_task_scheduler_shutdown);
    }
    if (continuation.__function == NULL) {
        cx_threading_raise(&cx_task_scheduler_null);
    }
    if (milliseconds == 0) {
        cxcore_System_Threading_Tasks_TaskScheduler_Post(
            continuation);
        return;
    }
    if (cx_task_scheduler_shutting_down) {
        cxcore_System_Threading_Tasks_TaskScheduler_Post(
            continuation);
        return;
    }
    if (!cx_task_scheduler_roots_registered) {
        cx_gc_register_static_roots(
            cx_task_scheduler_roots,
            (cx_uint)(sizeof(cx_task_scheduler_roots) /
                sizeof(cx_task_scheduler_roots[0])));
        cx_task_scheduler_roots_registered = CX_TRUE;
    }
    now = cx_platform_monotonic_time_milliseconds();
    item = cx_task_scheduler_allocate_item();
    cx_task_scheduler_capture_logical_arena(item);
    item->payload_kind = CX_TASK_SCHEDULER_PAYLOAD_TIMER;
    if (!cx_task_scheduler_store_action_for_arena(
            &item->continuation, continuation, item->logical_arena)) {
        struct cx_arena *failed_arena = item->logical_arena;
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&item->logical_arena, NULL);
        if (failed_arena != NULL) cx_arena_release_logical(failed_arena);
        cx_threading_raise(&cx_task_scheduler_scoped_async);
    }
    cx_task_scheduler_store_action(&item->shutdown_continuation, (cx_action){0});
    item->payload.completion.function = NULL;
    cx_task_scheduler_store_reference(
        (cx_ptr *)&item->payload.completion.environment, NULL);
    item->payload.due_time_milliseconds = now > UINT64_MAX - milliseconds
        ? UINT64_MAX
        : now + milliseconds;
    item->registration_id = 0;
    cx_task_scheduler_store_reference((cx_ptr *)&item->next, NULL);
    cursor = &cx_task_scheduler_timer_head;
    while (*cursor != NULL &&
        (*cursor)->payload.due_time_milliseconds <=
            item->payload.due_time_milliseconds) {
        cursor = &(*cursor)->next;
    }
    cx_task_scheduler_store_reference((cx_ptr *)&item->next, (cx_ptr)*cursor);
    cx_task_scheduler_store_reference((cx_ptr *)cursor, (cx_ptr)item);
}

CX_API uint64_t cx_task_scheduler_post_on_event(
    cx_ptr context,
    cx_scheduler_wait_function wait,
    cx_action continuation,
    cx_action shutdown_continuation) {
    struct cx_task_scheduler_item *item;
    cx_gc_check_thread();
    cx_task_scheduler_require_durable_context();
    cx_task_scheduler_require_owner();
    if (continuation.__function == NULL || wait == NULL) {
        cx_threading_raise(&cx_task_scheduler_null);
    }
    if (cx_task_scheduler_shutdown_complete) {
        cx_threading_raise(&cx_task_scheduler_shutdown);
    }
    if (cx_task_scheduler_shutting_down) {
        if (shutdown_continuation.__function != NULL) {
            cx_task_scheduler_post_ready(
                shutdown_continuation, NULL, NULL, NULL, CX_TRUE);
        }
        return 0;
    }
    if (!cx_task_scheduler_roots_registered) {
        cx_gc_register_static_roots(
            cx_task_scheduler_roots,
            (cx_uint)(sizeof(cx_task_scheduler_roots) /
                sizeof(cx_task_scheduler_roots[0])));
        cx_task_scheduler_roots_registered = CX_TRUE;
    }
    item = cx_task_scheduler_allocate_item();
    cx_task_scheduler_clear_payload(item);
    cx_task_scheduler_capture_logical_arena(item);
    item->payload_kind = CX_TASK_SCHEDULER_PAYLOAD_WAIT;
    if (!cx_task_scheduler_store_event_callbacks(
            item, context, wait, continuation, shutdown_continuation,
            item->logical_arena)) {
        struct cx_arena *failed_arena = item->logical_arena;
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&item->logical_arena, NULL);
        if (failed_arena != NULL) cx_arena_release_logical(failed_arena);
        cx_threading_raise(&cx_task_scheduler_scoped_async);
    }
    item->registration_id = cx_task_scheduler_allocate_wait_registration_id();
    cx_task_scheduler_store_reference(
        (cx_ptr *)&item->next, (cx_ptr)cx_task_scheduler_wait_head);
    cx_task_scheduler_store_reference(
        (cx_ptr *)&cx_task_scheduler_wait_head, (cx_ptr)item);
    return item->registration_id;
}

CX_API void cx_task_scheduler_post_on_event_reusable(
    cx_ptr context,
    cx_scheduler_wait_function wait,
    cx_action continuation,
    cx_action shutdown_continuation,
    cx_ptr *cached_item) {
    struct cx_task_scheduler_item *item;
    cx_gc_check_thread();
    cx_task_scheduler_require_durable_context();
    cx_task_scheduler_require_owner();
    if (continuation.__function == NULL || wait == NULL || cached_item == NULL) {
        cx_threading_raise(&cx_task_scheduler_null);
    }
    if (cx_task_scheduler_shutdown_complete) {
        cx_threading_raise(&cx_task_scheduler_shutdown);
    }
    if (*cached_item == NULL) {
        item = cx_task_scheduler_allocate_item();
        cx_task_scheduler_store_reference(cached_item, (cx_ptr)item);
    } else {
        item = (struct cx_task_scheduler_item *)*cached_item;
        if (item->registration_id != CX_TASK_SCHEDULER_REUSABLE_IDLE) {
            cx_threading_raise(&cx_task_scheduler_reusable_item_busy);
        }
    }
    cx_task_scheduler_clear_payload(item);
    item->registration_id = CX_TASK_SCHEDULER_REUSABLE_EVENT_WAIT;
    item->payload_kind = CX_TASK_SCHEDULER_PAYLOAD_WAIT;
    cx_task_scheduler_store_reference((cx_ptr *)&item->next, NULL);
    if (cx_task_scheduler_shutting_down) {
        cx_task_scheduler_capture_logical_arena(item);
        if (!cx_task_scheduler_store_action_for_arena(
                &item->continuation, shutdown_continuation,
                item->logical_arena)) {
            struct cx_arena *failed_arena = item->logical_arena;
            (void)cx_gc_store_native_pointer(
                (cx_ptr *)&item->logical_arena, NULL);
            if (failed_arena != NULL) cx_arena_release_logical(failed_arena);
            cx_threading_raise(&cx_task_scheduler_scoped_async);
        }
        cx_task_scheduler_store_action(&item->shutdown_continuation, (cx_action){0});
        item->registration_id = CX_TASK_SCHEDULER_REUSABLE_QUEUED;
        cx_task_scheduler_queue_ready(item);
        return;
    }
    cx_task_scheduler_capture_logical_arena(item);
    if (!cx_task_scheduler_store_event_callbacks(
            item, context, wait, continuation, shutdown_continuation,
            item->logical_arena)) {
        struct cx_arena *failed_arena = item->logical_arena;
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&item->logical_arena, NULL);
        if (failed_arena != NULL) cx_arena_release_logical(failed_arena);
        cx_threading_raise(&cx_task_scheduler_scoped_async);
    }
    cx_task_scheduler_store_reference(
        (cx_ptr *)&item->next, (cx_ptr)cx_task_scheduler_wait_head);
    cx_task_scheduler_store_reference(
        (cx_ptr *)&cx_task_scheduler_wait_head, (cx_ptr)item);
}

CX_API int cx_task_scheduler_cancel_reusable_event_wait(cx_ptr cached_item) {
    struct cx_task_scheduler_item *item =
        (struct cx_task_scheduler_item *)cached_item;
    struct cx_task_scheduler_item **heads[] = {
        &cx_task_scheduler_wait_head,
        &cx_task_scheduler_head,
    };
    cx_gc_check_thread();
    cx_task_scheduler_require_owner();
    if (item == NULL) {
        return 0;
    }
    for (size_t list = 0; list < sizeof(heads) / sizeof(heads[0]); ++list) {
        struct cx_task_scheduler_item **cursor = heads[list];
        struct cx_task_scheduler_item *previous = NULL;
        while (*cursor != NULL) {
            if (*cursor == item) {
                if (item->registration_id != CX_TASK_SCHEDULER_REUSABLE_EVENT_WAIT &&
                    item->registration_id != CX_TASK_SCHEDULER_REUSABLE_QUEUED) {
                    return 0;
                }
                cx_task_scheduler_store_reference(
                    (cx_ptr *)cursor, (cx_ptr)item->next);
                if (list == 1 && cx_task_scheduler_tail == item) {
                    cx_task_scheduler_tail = previous;
                }
                cx_task_scheduler_store_action(
                    &item->continuation, (cx_action){0});
                cx_task_scheduler_store_action(
                    &item->shutdown_continuation, (cx_action){0});
                cx_task_scheduler_clear_payload(item);
                cx_task_scheduler_release_logical_arena(item);
                item->registration_id = CX_TASK_SCHEDULER_REUSABLE_IDLE;
                cx_task_scheduler_store_reference(
                    (cx_ptr *)&item->next, NULL);
                return 1;
            }
            previous = *cursor;
            cursor = &(*cursor)->next;
        }
    }
    return 0;
}

static int cx_task_scheduler_remove_registration(
    struct cx_task_scheduler_item **head,
    struct cx_task_scheduler_item **tail,
    uint64_t registration_id) {
    struct cx_task_scheduler_item **cursor = head;
    struct cx_task_scheduler_item *previous = NULL;
    while (*cursor != NULL) {
        if ((*cursor)->registration_id == registration_id) {
            struct cx_task_scheduler_item *removed = *cursor;
            cx_task_scheduler_store_reference(
                (cx_ptr *)cursor, (cx_ptr)removed->next);
            if (tail != NULL && *tail == removed) {
                *tail = previous;
            }
            cx_task_scheduler_store_reference(
                (cx_ptr *)&removed->next, NULL);
            cx_task_scheduler_store_action(
                &removed->continuation, (cx_action){0});
            cx_task_scheduler_store_action(
                &removed->shutdown_continuation, (cx_action){0});
            cx_task_scheduler_clear_payload(removed);
            cx_task_scheduler_release_logical_arena(removed);
            return 1;
        }
        previous = *cursor;
        cursor = &(*cursor)->next;
    }
    return 0;
}

CX_API int cx_task_scheduler_cancel_event_wait(uint64_t registration_id) {
    cx_gc_check_thread();
    cx_task_scheduler_require_owner();
    if (registration_id == 0) {
        return 0;
    }
    return cx_task_scheduler_remove_registration(
            &cx_task_scheduler_wait_head, NULL, registration_id) ||
        cx_task_scheduler_remove_registration(
            &cx_task_scheduler_head, &cx_task_scheduler_tail, registration_id);
}

static void cx_task_scheduler_promote_due_timers(uint64_t now) {
    while (cx_task_scheduler_timer_head != NULL &&
        cx_task_scheduler_timer_head->payload.due_time_milliseconds <= now) {
        struct cx_task_scheduler_item *item = cx_task_scheduler_timer_head;
        cx_task_scheduler_store_reference(
            (cx_ptr *)&cx_task_scheduler_timer_head, (cx_ptr)item->next);
        cx_task_scheduler_store_reference((cx_ptr *)&item->next, NULL);
        cx_task_scheduler_clear_payload(item);
        cx_task_scheduler_queue_ready(item);
    }
}

static void cx_task_scheduler_invoke_item(
    struct cx_task_scheduler_item *item) {
    struct cx_arena_scope arena_scope;
    volatile char callback_stack_boundary = 0;
    cx_bool entered_arena = item->logical_arena != NULL;
    struct CX_FN_136FFEE706DF35B2_invocation *invocation;
    if (entered_arena) {
        cx_arena_enter_logical(item->logical_arena, &arena_scope);
        cx_arena_allow_callback_stack_borrows(
            &arena_scope, &callback_stack_boundary);
    }
    if (item->continuation.__invocation_list != NULL) {
        for (invocation = item->continuation.__invocation_list;
                invocation != NULL; invocation = invocation->next) {
            invocation->value.__function(invocation->value.__environment);
        }
    } else {
        item->continuation.__function(item->continuation.__environment);
    }
    if (item->payload.completion.function != NULL) {
        item->payload.completion.function(item->payload.completion.environment);
    }
    if (entered_arena) cx_arena_leave(&arena_scope);
}

static void cx_task_scheduler_invoke_completion(
    struct cx_task_scheduler_item *item) {
    struct cx_arena_scope arena_scope;
    volatile char callback_stack_boundary = 0;
    if (item->logical_arena != NULL) {
        cx_arena_enter_logical(item->logical_arena, &arena_scope);
        cx_arena_allow_callback_stack_borrows(
            &arena_scope, &callback_stack_boundary);
    }
    if (item->payload.completion.function != NULL) {
        item->payload.completion.function(item->payload.completion.environment);
    }
    if (item->logical_arena != NULL) cx_arena_leave(&arena_scope);
}

struct cx_task_scheduler_wait_call {
    cx_scheduler_wait_function function;
    struct cx_scheduler_wait_entry *entries;
    size_t count;
    uint32_t timeout;
    int result;
};

static void cx_task_scheduler_wait_blocking(void *context) {
    struct cx_task_scheduler_wait_call *call = context;
    if (call->function == NULL) {
        cx_platform_thread_sleep(call->timeout);
        call->result = 0;
    } else {
        call->result = call->function(call->entries, call->count, call->timeout);
    }
}

static int cx_task_scheduler_wait_for_work(uint32_t timeout_milliseconds) {
    struct cx_task_scheduler_wait_snapshot {
        struct cx_task_scheduler_item *item;
        cx_scheduler_wait_function function;
    } *items;
    struct cx_scheduler_wait_entry *entries;
    struct cx_task_scheduler_item *item;
    struct cx_task_scheduler_wait_call call = {0};
    size_t count = 0, provider_count = 0, index = 0;
    int result = 0;
    for (item = cx_task_scheduler_wait_head; item != NULL; item = item->next) ++count;
    if (count == 0) {
        call.timeout = timeout_milliseconds > INT32_MAX ? INT32_MAX : timeout_milliseconds;
        cx_gc_run_blocking(cx_task_scheduler_wait_blocking, &call);
        return 0;
    }
    if (count > SIZE_MAX / sizeof(*items) || count > SIZE_MAX / sizeof(*entries)) return -1;
    items = malloc(count * sizeof(*items));
    entries = malloc(count * sizeof(*entries));
    if (items == NULL || entries == NULL) {
        free(items);
        free(entries);
        return -1;
    }
    for (item = cx_task_scheduler_wait_head; item != NULL; item = item->next) {
        items[index].item = item;
        items[index++].function = item->payload.wait.function;
    }
    for (index = 0; index < count; ++index) {
        size_t prior;
        for (prior = 0; prior < index; ++prior)
            if (items[prior].function == items[index].function) break;
        if (prior == index) ++provider_count;
    }
    for (index = 0; index < count; ++index) {
        size_t prior, entry_count = 0, entry_index = 0;
        for (prior = 0; prior < index; ++prior)
            if (items[prior].function == items[index].function) break;
        if (prior != index) continue;
        for (size_t i = index; i < count; ++i) {
            if (items[i].function != items[index].function) continue;
            entries[entry_count].context = items[i].item->payload.wait.context;
            entries[entry_count++].ready = 0;
        }
        call.function = items[index].function;
        call.entries = entries;
        call.count = entry_count;
        // One provider can block until the next deadline. With multiple providers,
        // poll every group first so no provider can starve another event source.
        call.timeout = provider_count == 1 ? timeout_milliseconds : 0;
        cx_gc_run_blocking(cx_task_scheduler_wait_blocking, &call);
        if (call.result < 0) { result = -1; break; }
        for (size_t i = index; i < count; ++i) {
            struct cx_task_scheduler_item **cursor;
            if (items[i].function != items[index].function) continue;
            if (!entries[entry_index++].ready) continue;
            cursor = &cx_task_scheduler_wait_head;
            while (*cursor != NULL && *cursor != items[i].item) cursor = &(*cursor)->next;
            if (*cursor != NULL) {
                item = *cursor;
                cx_task_scheduler_store_reference(
                    (cx_ptr *)cursor, (cx_ptr)item->next);
                cx_task_scheduler_clear_payload(item);
                cx_task_scheduler_store_action(
                    &item->shutdown_continuation, (cx_action){0});
                if (item->registration_id == CX_TASK_SCHEDULER_REUSABLE_EVENT_WAIT)
                    item->registration_id = CX_TASK_SCHEDULER_REUSABLE_QUEUED;
                cx_task_scheduler_queue_ready(item);
                result = 1;
            }
        }
    }
    free(items);
    free(entries);
    if (result == 0 && provider_count > 1 && timeout_milliseconds != 0) {
        call.function = NULL;
        call.timeout = 1;
        cx_gc_run_blocking(cx_task_scheduler_wait_blocking, &call);
    }
    return result;
}

cx_bool cxcore_System_Threading_Tasks_TaskScheduler_RunOne(void) {
    struct cx_task_scheduler_item *item;
    cx_gc_check_thread();
    cx_task_scheduler_require_owner();
    if (cx_task_scheduler_dispatching) {
        cx_threading_raise_invalid_operation(&cx_task_scheduler_reentrant);
    }
    while (cx_task_scheduler_head == NULL) {
        uint64_t now = cx_platform_monotonic_time_milliseconds();
        uint64_t wait_milliseconds;
        uint32_t wait_timeout;
        cx_task_scheduler_promote_due_timers(now);
        if (cx_task_scheduler_head != NULL) {
            break;
        }
        if (cx_task_scheduler_timer_head == NULL &&
            cx_task_scheduler_wait_head == NULL) {
            return CX_FALSE;
        }
        if (cx_task_scheduler_timer_head == NULL) {
            wait_timeout = UINT32_MAX;
        } else {
            wait_milliseconds =
                cx_task_scheduler_timer_head->payload.due_time_milliseconds - now;
            if (wait_milliseconds > UINT32_MAX) {
                wait_milliseconds = UINT32_MAX;
            }
            wait_timeout = (uint32_t)wait_milliseconds;
        }
        if (cx_task_scheduler_wait_for_work(wait_timeout) < 0) {
            cx_threading_raise(&cx_task_scheduler_event_wait_failed);
        }
        cx_task_scheduler_promote_due_timers(
            cx_platform_monotonic_time_milliseconds());
    }
    item = cx_task_scheduler_head;
    cx_task_scheduler_store_reference(
        (cx_ptr *)&cx_task_scheduler_head, (cx_ptr)item->next);
    if (cx_task_scheduler_head == NULL) {
        cx_task_scheduler_tail = NULL;
    }
    if (item->registration_id == CX_TASK_SCHEDULER_REUSABLE_QUEUED) {
        item->registration_id = CX_TASK_SCHEDULER_REUSABLE_DISPATCHING;
    }

    {
        struct cx_exception_frame callback_frame;
        cx_exception_push(&callback_frame);
        if (setjmp(callback_frame.environment) == 0) {
            cx_task_scheduler_dispatching = CX_TRUE;
            cx_task_scheduler_invoke_item(item);
            cx_task_scheduler_dispatching = CX_FALSE;
            cx_task_scheduler_finish_dispatch(item);
            cx_exception_pop(&callback_frame);
        } else {
            struct cx_exception failure = *cx_exception_thread_state();
            cx_exception_pop(&callback_frame);
            cx_task_scheduler_invoke_completion(item);
            cx_task_scheduler_dispatching = CX_FALSE;
            cx_task_scheduler_finish_dispatch(item);
            cx_exception_restore(&failure);
            cx_exception_rethrow();
        }
    }
    return CX_TRUE;
}

cx_uint cxcore_System_Threading_Tasks_TaskScheduler_Run(void) {
    cx_uint count = 0;
    while (cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) {
        if (count == (cx_uint)-1) {
            return count;
        }
        ++count;
    }
    return count;
}

CX_CXCORE_API void cxcore_System_Threading_Tasks_TaskScheduler_Shutdown(void) {
    struct cx_task_scheduler_item *item;
    cx_gc_check_thread();
    cx_task_scheduler_require_owner();
    if (cx_task_scheduler_dispatching) {
        cx_threading_raise_invalid_operation(&cx_task_scheduler_reentrant);
    }
    if (cx_task_scheduler_shutdown_complete) {
        cx_task_report_unobserved_faults();
        return;
    }
    cx_task_scheduler_shutting_down = CX_TRUE;
    // Drop future timers and their GC roots rather than delaying process exit
    // until an arbitrary deadline. New delays complete inline during the drain.
    while ((item = cx_task_scheduler_timer_head) != NULL) {
        cx_task_scheduler_store_reference(
            (cx_ptr *)&cx_task_scheduler_timer_head, (cx_ptr)item->next);
        cx_task_scheduler_store_reference((cx_ptr *)&item->next, NULL);
        cx_task_scheduler_store_action(&item->continuation, (cx_action){0});
        cx_task_scheduler_store_action(
            &item->shutdown_continuation, (cx_action){0});
        cx_task_scheduler_clear_payload(item);
        cx_task_scheduler_release_logical_arena(item);
    }
    while ((item = cx_task_scheduler_wait_head) != NULL) {
        cx_task_scheduler_store_reference(
            (cx_ptr *)&cx_task_scheduler_wait_head, (cx_ptr)item->next);
        if (!cx_task_scheduler_store_action_for_arena(
                &item->continuation, item->shutdown_continuation,
                item->logical_arena)) {
            cx_task_scheduler_release_logical_arena(item);
            cx_threading_raise(&cx_task_scheduler_scoped_async);
        }
        cx_task_scheduler_store_action(
            &item->shutdown_continuation, (cx_action){0});
        cx_task_scheduler_clear_payload(item);
        if (item->registration_id == CX_TASK_SCHEDULER_REUSABLE_EVENT_WAIT) {
            item->registration_id = CX_TASK_SCHEDULER_REUSABLE_QUEUED;
        }
        if (item->continuation.__function != NULL) {
            cx_task_scheduler_queue_ready(item);
        }
    }
    // Drain ready work and timers to quiescence. External waits are canceled above,
    // so RunOne can only wait for the next timer deadline.
    cxcore_System_Threading_Tasks_TaskScheduler_Run();
    cx_task_scheduler_shutdown_complete = CX_TRUE;
    cx_task_report_unobserved_faults();
}

struct cx_task_scheduler_task_owner {
    struct cxcore_System_Object __base;
    struct cxcore_System_Threading_Tasks_TaskState *_state;
};

static cx_bool *cx_task_scheduler_generic_task_completion(cx_ptr task) {
    const struct cxcore_System_Reflection_TypeInfo *typeInfo =
        (const struct cxcore_System_Reflection_TypeInfo *)
            ((union cx_vtable_entry *)CX_GET_VTABLE(task))[0].data;
    if (typeInfo == NULL || typeInfo->GenericArity != 1u ||
        (typeInfo->Flags & CX_REFLECTION_FLAG_GENERIC) == 0u) {
        return NULL;
    }

    cx_uint fieldCount = 0u;
    const struct cx_reflection_field *fields =
        cx_reflection_fields(typeInfo, &fieldCount);
    for (cx_uint index = 0u; index < fieldCount; index++) {
        if (strcmp(fields[index].name, "_isCompleted") == 0 &&
            fields[index].offset < typeInfo->Size) {
            return (cx_bool *)((unsigned char *)task + fields[index].offset);
        }
    }
    return NULL;
}

CX_API void cxcore_System_Threading_Tasks_TaskScheduler_RunUntil(
    cx_ptr task) {
    struct cx_task_scheduler_task_owner *owner =
        (struct cx_task_scheduler_task_owner *)task;
    struct cx_exception_frame frame;
    cx_ptr previousFaultOwner;
    cx_ptr faultOwner;
    cx_bool *genericCompletion;
    cx_gc_check_thread();
    cx_task_scheduler_require_owner();
    if (task == NULL) {
        cx_threading_raise(&cx_task_scheduler_null);
    }
    genericCompletion =
        cx_task_scheduler_generic_task_completion(task);
    if (genericCompletion != NULL) {
        faultOwner = task;
    } else {
        if (owner->_state == NULL) {
            cx_threading_raise(&cx_task_scheduler_null);
        }
        faultOwner = owner->_state;
    }

    previousFaultOwner = cx_task_scheduler_run_until_fault_owner;
    cx_task_scheduler_run_until_fault_owner = faultOwner;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) != 0) {
        cx_exception_pop(&frame);
        cx_task_scheduler_run_until_fault_owner = previousFaultOwner;
        cx_exception_rethrow();
    }
    if (genericCompletion != NULL) {
        while (!*genericCompletion) {
            if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) {
                cx_threading_raise(&cx_task_scheduler_no_completion_path);
            }
        }
    } else {
        while (!owner->_state->_isCompleted) {
            if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) {
                cx_threading_raise(&cx_task_scheduler_no_completion_path);
            }
        }
    }
    cx_exception_pop(&frame);
    cx_task_scheduler_run_until_fault_owner = previousFaultOwner;
}
