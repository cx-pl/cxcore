#include "../Threading.internal.h"
#include "Task.internal.h"
#include "TaskScheduler.internal.h"
#include "../../../cxarena.h"

#include <string.h>

CX_STRING_DEF(cx_task_faulted, "The asynchronous operation failed");
CX_STRING_DEF(cx_task_not_completed, "The asynchronous operation has not completed");
CX_STRING_DEF(cx_task_already_completed, "The task has already completed");
CX_STRING_DEF(cx_task_null_continuation, "Task.OnCompleted requires a non-null continuation");
CX_STRING_DEF(cx_task_continuation_parameter, "continuation");
CX_STRING_DEF(cx_task_invalid_forward, "Task.ForwardTo requires valid source and destination tasks");
CX_STRING_DEF(cx_task_null_fault, "Task.SetFault requires a non-null exception");
CX_STRING_DEF(cx_task_invalid_fault_state, "Task.SetFault requires a valid task state");
CX_STRING_DEF(cx_task_exception_parameter, "exception");
CX_STRING_DEF(cx_task_scoped_suspend, "A synchronous allocation arena cannot retain a task continuation unless it is logically retained");
CX_STRING_DEF(cx_task_scoped_cancellation, "Cancellation callback registration requires a durable or retained arena");
CX_STRING_DEF(cx_task_null_durable_action, "TaskRuntime.WithDurable requires a non-null action");
CX_STRING_DEF(cx_task_action_parameter, "action");

extern cx_ptr cx_array_at(
    struct cxcore_System_Collections_Array *array,
    cx_uint index,
    cx_uint element_size);

static void cx_task_require_async_use_allowed(void) {
    if (!cx_arena_scheduler_use_allowed()) {
        cx_threading_raise(&cx_task_scoped_suspend);
    }
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_RequireAsyncUseAllowed(void) {
    cx_task_require_async_use_allowed();
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_RequireCancellationRegistrationAllowed(void) {
    if (!cx_arena_scheduler_use_allowed()) {
        cx_threading_raise(&cx_task_scoped_cancellation);
    }
}

CX_API cx_ptr cxcore_System_Threading_Tasks_TaskRuntime_CaptureCancellationCallbackArena(void) {
    return (cx_ptr)cx_arena_capture_logical_context();
}

static void cx_task_store_reference(cx_ptr *destination, cx_ptr value) {
    (void)cx_gc_store_reference(destination, value);
}

static void cx_task_store_action(cx_action *destination, cx_action value) {
    destination->__function = value.__function;
    cx_task_store_reference(
        (cx_ptr *)&destination->__environment, (cx_ptr)value.__environment);
    cx_task_store_reference(
        (cx_ptr *)&destination->__target, value.__target);
    destination->__method = value.__method;
    cx_task_store_reference(
        (cx_ptr *)&destination->__invocation_list,
        (cx_ptr)value.__invocation_list);
}

static cx_bool cx_task_store_action_with_logical_arena(
    cx_action *destination,
    void **completion_environment_destination,
    cx_action value,
    void *completion_environment,
    struct cx_arena *logical_arena) {
    if (logical_arena == NULL) {
        cx_task_store_action(destination, value);
        if (completion_environment_destination != NULL) {
            cx_task_store_reference(
                (cx_ptr *)completion_environment_destination,
                (cx_ptr)completion_environment);
        }
        return CX_TRUE;
    }
    {
        cx_ptr *destinations[4];
        cx_ptr values[4];
        size_t count = 3u;
        destinations[0] = (cx_ptr *)&destination->__environment;
        destinations[1] = (cx_ptr *)&destination->__target;
        destinations[2] = (cx_ptr *)&destination->__invocation_list;
        values[0] = (cx_ptr)value.__environment;
        values[1] = value.__target;
        values[2] = (cx_ptr)value.__invocation_list;
        if (completion_environment_destination != NULL) {
            destinations[3] =
                (cx_ptr *)completion_environment_destination;
            values[3] = (cx_ptr)completion_environment;
            count = 4u;
        }
        if (!cx_gc_store_references_retained_arena(
                destinations, values, count, logical_arena)) {
            return CX_FALSE;
        }
        destination->__function = value.__function;
        destination->__method = value.__method;
    }
    return CX_TRUE;
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_StoreCancellationCallback(
    struct cxcore_System_Collections_Array *callbacks,
    cx_uint index,
    cx_action callback,
    cx_ptr logical_arena) {
    cx_action *destination = (cx_action *)cx_array_at(
        callbacks, index, (cx_uint)sizeof(cx_action));
    if (destination == NULL) {
        cx_threading_raise(&cx_task_scoped_cancellation);
    }
    if (!cx_task_store_action_with_logical_arena(
            destination, NULL, callback, NULL,
            (struct cx_arena *)logical_arena)) {
        cx_threading_raise(&cx_task_scoped_cancellation);
    }
}

static void cx_task_invoke_cancellation_action(void *context) {
    cx_action *action = (cx_action *)context;
    struct CX_FN_136FFEE706DF35B2_invocation *invocation;
    if (action->__invocation_list != NULL) {
        for (invocation = action->__invocation_list;
             invocation != NULL; invocation = invocation->next) {
            invocation->value.__function(invocation->value.__environment);
        }
    } else {
        action->__function(action->__environment);
    }
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_WithDurable(
    cx_action action) {
    struct cx_exception_frame frame;
    if (action.__function == NULL && action.__invocation_list == NULL) {
        cx_threading_raise_argument_null(
            &cx_task_null_durable_action, &cx_task_action_parameter);
    }
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        struct cx_arena_durable_scope scope;
        cx_arena_enter_durable(&scope);
        cx_task_invoke_cancellation_action(&action);
        cx_arena_leave_durable(&scope);
        cx_exception_pop(&frame);
        return;
    }
    cx_exception_pop(&frame);
    cx_exception_rethrow();
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_InvokeCancellationCallback(
    cx_action callback,
    cx_ptr logical_arena) {
    if (logical_arena == NULL) {
        cx_task_invoke_cancellation_action(&callback);
    } else {
        cx_arena_run_callback(
            (struct cx_arena *)logical_arena,
            cx_task_invoke_cancellation_action,
            &callback);
    }
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ReleaseCancellationCallbackArena(
    cx_ptr logical_arena) {
    if (logical_arena != NULL) {
        cx_arena_release_logical((struct cx_arena *)logical_arena);
    }
}

struct cx_task_unobserved_fault_node {
    void *owner;
    struct cxcore_System_Exception *exception;
    struct cx_arena *logical_arena;
    struct cx_task_unobserved_fault_node *next;
};

/* Must match Task<T>'s common prefix before its typed _result field. */
struct cx_task_generic_state {
    struct cxcore_System_Object __base;
    cx_bool _isCompleted;
    cx_bool _isFaulted;
    cx_bool _isCanceled;
    struct cxcore_System_Exception *_exception;
    cx_ptr _continuations;
};

static struct cx_task_unobserved_fault_node *cx_task_unobserved_faults;
static cx_bool cx_task_unobserved_fault_roots_registered;
static const struct cx_gc_root_region cx_task_unobserved_fault_roots[] = {
    {&cx_task_unobserved_faults, (cx_uint)sizeof(cx_task_unobserved_faults)},
};

static void cx_task_register_unobserved_fault(
    void *owner,
    struct cxcore_System_Exception *exception) {
    struct cx_task_unobserved_fault_node *node;
    struct cx_arena *logical_arena = NULL;
    struct cx_arena_durable_scope durable_scope;
    cx_gc_check_thread();
    if (owner == NULL || exception == NULL) {
        return;
    }
    if (!cx_gc_is_durable_reference((cx_ptr)owner)) {
        if (cx_arena_current_owns(owner)) {
            logical_arena = cx_arena_capture_logical_context();
        }
        // An unretained arena cannot safely publish a task into the process
        // fault registry. Such tasks are only valid while their scope is
        // active and cannot be observed after it closes.
        if (logical_arena == NULL) return;
        exception = (struct cxcore_System_Exception *)
            cx_exception_copy_durable((cx_ptr)exception);
    }
    if (!cx_task_unobserved_fault_roots_registered) {
        cx_gc_register_static_roots(
            cx_task_unobserved_fault_roots,
            (cx_uint)(sizeof(cx_task_unobserved_fault_roots) /
                sizeof(cx_task_unobserved_fault_roots[0])));
        cx_task_unobserved_fault_roots_registered = CX_TRUE;
    }
    cx_arena_enter_durable(&durable_scope);
    node = (struct cx_task_unobserved_fault_node *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*node));
    memset(node, 0, sizeof(*node));
    (void)cx_gc_store_native_pointer(
        (cx_ptr *)&node->logical_arena, (cx_ptr)logical_arena);
    if (logical_arena != NULL) {
        cx_ptr *destinations[2] = {
            (cx_ptr *)&node->owner,
            (cx_ptr *)&node->exception,
        };
        cx_ptr values[2] = {(cx_ptr)owner, (cx_ptr)exception};
        if (!cx_gc_store_references_retained_arena(
                destinations, values, 2u, logical_arena)) {
            (void)cx_gc_store_native_pointer(
                (cx_ptr *)&node->logical_arena, NULL);
            cx_arena_leave_durable(&durable_scope);
            cx_arena_release_logical(logical_arena);
            cx_threading_raise(&cx_task_scoped_suspend);
        }
    } else {
        cx_task_store_reference((cx_ptr *)&node->owner, (cx_ptr)owner);
        cx_task_store_reference((cx_ptr *)&node->exception, (cx_ptr)exception);
    }
    cx_task_store_reference(
        (cx_ptr *)&node->next, (cx_ptr)cx_task_unobserved_faults);
    cx_task_store_reference((cx_ptr *)&cx_task_unobserved_faults, (cx_ptr)node);
    cx_arena_leave_durable(&durable_scope);
}

static void cx_task_mark_fault_observed(void *owner) {
    struct cx_task_unobserved_fault_node **cursor = &cx_task_unobserved_faults;
    cx_gc_check_thread();
    while (*cursor != NULL) {
        if ((*cursor)->owner == owner) {
            struct cx_task_unobserved_fault_node *node = *cursor;
            struct cx_arena *logical_arena = node->logical_arena;
            cx_task_store_reference((cx_ptr *)cursor, (cx_ptr)node->next);
            cx_task_store_reference((cx_ptr *)&node->next, NULL);
            (void)cx_gc_store_native_pointer(
                (cx_ptr *)&node->logical_arena, NULL);
            if (logical_arena != NULL) {
                cx_arena_release_logical(logical_arena);
            }
            return;
        }
        cursor = &(*cursor)->next;
    }
}

CX_API void cxcore_System_Threading_Tasks_TaskState_RegisterUnobservedFault(
    struct cxcore_System_Threading_Tasks_TaskState *state) {
    if (state != NULL) {
        cx_task_register_unobserved_fault(state, state->_exception);
    }
}

CX_API void cxcore_System_Threading_Tasks_TaskState_MarkFaultObserved(
    struct cxcore_System_Threading_Tasks_TaskState *state) {
    cx_task_mark_fault_observed(state);
}

void cx_task_report_unobserved_faults(void) {
    struct cx_task_unobserved_fault_node **cursor = &cx_task_unobserved_faults;
    while (*cursor != NULL) {
        struct cx_task_unobserved_fault_node *node = *cursor;
        if (node->exception != NULL &&
            !cx_task_scheduler_fault_is_deferred(node->owner)) {
            struct cxcore_System_Exception *exception =
                (struct cxcore_System_Exception *)cx_exception_copy_durable(
                    (cx_ptr)node->exception);
            struct cx_arena *logical_arena = node->logical_arena;
            cx_task_store_reference((cx_ptr *)cursor, (cx_ptr)node->next);
            (void)cx_gc_store_native_pointer(
                (cx_ptr *)&node->logical_arena, NULL);
            if (logical_arena != NULL) {
                cx_arena_release_logical(logical_arena);
            }
            cx_exception_throw((cx_ptr)exception, __FILE__, __LINE__);
        }
        cursor = &node->next;
    }
}

CX_API void cxcore_System_Threading_Tasks_TaskState_SetFault(
    struct cxcore_System_Threading_Tasks_TaskState *state,
    struct cxcore_System_Exception *exception) {
    cx_gc_check_thread();
    if (state == NULL) {
        cx_threading_raise(&cx_task_invalid_fault_state);
    }
    if (exception == NULL) {
        cx_threading_raise_argument_null(
            &cx_task_null_fault, &cx_task_exception_parameter);
    }
    cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(
        state->_isCompleted);
    cx_task_store_reference((cx_ptr *)&state->_exception, (cx_ptr)exception);
    state->_isFaulted = CX_TRUE;
    state->_isCompleted = CX_TRUE;
    cxcore_System_Threading_Tasks_TaskState_RegisterUnobservedFault(state);
    cxcore_System_Threading_Tasks_TaskState_ScheduleContinuations(state);
}

struct cx_task_continuation {
    cx_action action;
    void (*completion)(void *environment);
    void *completion_environment;
    struct cx_arena *logical_arena;
    struct cx_task_continuation *next;
};

struct cx_task_continuation_queue {
    /* Keep the first subscriber's entry in the queue allocation. */
    struct cx_task_continuation first;
    struct cx_task_continuation *tail;
};

static void cx_task_on_completed(
    cx_bool isCompleted,
    cx_ptr *continuations,
    cx_action action,
    void (*completion)(void *environment),
    void *completion_environment) {
    struct cx_task_continuation_queue *queue;
    struct cx_task_continuation *item;
    cx_bool new_queue = CX_FALSE;
    struct cx_arena *logical_arena;
    cx_gc_check_thread();
    if (action.__function == NULL) {
        cx_threading_raise_argument_null(
            &cx_task_null_continuation, &cx_task_continuation_parameter);
    }
    if (isCompleted) {
        if (completion != NULL) {
            cx_task_scheduler_post_with_completion(
                action, completion, completion_environment);
        } else {
            cxcore_System_Threading_Tasks_TaskScheduler_Post(action);
        }
        return;
    }
    if (!cx_arena_scheduler_use_allowed()) {
        cx_threading_raise(&cx_task_scoped_suspend);
    }
    logical_arena = cx_arena_capture_logical_context();
    queue = (struct cx_task_continuation_queue *)*continuations;
    if (queue == NULL) {
        struct cx_arena_durable_scope durable_scope;
        cx_arena_enter_durable(&durable_scope);
        queue = (struct cx_task_continuation_queue *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*queue));
        cx_arena_leave_durable(&durable_scope);
        new_queue = CX_TRUE;
        item = &queue->first;
    } else {
        struct cx_arena_durable_scope durable_scope;
        cx_arena_enter_durable(&durable_scope);
        item = (struct cx_task_continuation *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*item));
        cx_arena_leave_durable(&durable_scope);
    }
    (void)cx_gc_store_native_pointer(
        (cx_ptr *)&item->logical_arena, (cx_ptr)logical_arena);
    if (!cx_task_store_action_with_logical_arena(
            &item->action,
            (void **)&item->completion_environment,
            action,
            completion_environment,
            logical_arena)) {
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&item->logical_arena, NULL);
        if (logical_arena != NULL) cx_arena_release_logical(logical_arena);
        cx_threading_raise(&cx_task_scoped_suspend);
    }
    item->completion = completion;
    cx_task_store_reference((cx_ptr *)&item->next, NULL);
    if (new_queue) {
        cx_task_store_reference((cx_ptr *)&queue->tail, NULL);
        cx_task_store_reference(continuations, (cx_ptr)queue);
    }
    if (queue->tail != NULL) {
        cx_task_store_reference((cx_ptr *)&queue->tail->next, (cx_ptr)item);
    }
    cx_task_store_reference((cx_ptr *)&queue->tail, (cx_ptr)item);
}

static void cx_task_schedule(cx_ptr *continuations) {
    struct cx_task_continuation_queue *queue =
        (struct cx_task_continuation_queue *)*continuations;
    struct cx_task_continuation *item;
    cx_task_store_reference(continuations, NULL);
    if (queue == NULL) {
        return;
    }
    for (item = &queue->first; item != NULL;) {
        struct cx_task_continuation *next = item->next;
        cx_task_scheduler_post_with_logical_context(
            item->action,
            item->completion,
            item->completion_environment,
            item->logical_arena);
        cx_task_store_action(&item->action, (cx_action){0});
        cx_task_store_reference(
            (cx_ptr *)&item->completion_environment, NULL);
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&item->logical_arena, NULL);
        item = next;
    }
}

struct cx_task_forward_context {
    struct cxcore_System_Threading_Tasks_TaskState *source;
    struct cxcore_System_Threading_Tasks_TaskState *destination;
};

static void cx_task_forward_completed(void *environment);

static void cx_task_forward_states(
    struct cxcore_System_Threading_Tasks_TaskState *source,
    struct cxcore_System_Threading_Tasks_TaskState *destination) {
    struct cx_task_forward_context *context;
    cx_action continuation = {0};
    if (source == NULL || destination == NULL || source == destination ||
        destination->_isCompleted) {
        cx_threading_raise(&cx_task_invalid_forward);
    }
    context = (struct cx_task_forward_context *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*context));
    cx_task_store_reference((cx_ptr *)&context->source, (cx_ptr)source);
    cx_task_store_reference((cx_ptr *)&context->destination, (cx_ptr)destination);
    continuation.__function = cx_task_forward_completed;
    continuation.__environment = context;
    continuation.__target = (cx_ptr)context;
    continuation.__method = NULL;
    continuation.__invocation_list = NULL;
    cxcore_System_Threading_Tasks_TaskState_OnCompleted(
        source, continuation);
}

static void cx_task_forward_completed(void *environment) {
    struct cx_task_forward_context *context =
        (struct cx_task_forward_context *)environment;
    struct cxcore_System_Threading_Tasks_TaskState *source =
        context->source;
    struct cxcore_System_Threading_Tasks_TaskState *destination =
        context->destination;
    cx_task_store_reference(
        (cx_ptr *)&destination->_value, (cx_ptr)source->_value);
    destination->_isFaulted = source->_isFaulted;
    destination->_isCanceled = source->_isCanceled;
    cx_task_store_reference(
        (cx_ptr *)&destination->_exception, (cx_ptr)source->_exception);
    destination->_isCompleted = CX_TRUE;
    if (source->_isFaulted) {
        cxcore_System_Threading_Tasks_TaskState_MarkFaultObserved(source);
        cxcore_System_Threading_Tasks_TaskState_RegisterUnobservedFault(destination);
    }
    cxcore_System_Threading_Tasks_TaskState_ScheduleContinuations(
        destination);
}

void cxcore_System_Threading_Tasks_Task_OnCompleted(
    struct cxcore_System_Threading_Tasks_Task *self,
    cx_action continuation) {
    struct cx_task_generic_state *state =
        (struct cx_task_generic_state *)self;
    if (self == NULL) {
        cx_threading_raise(&cx_task_null_continuation);
    }
    cx_task_on_completed(
        state->_isCompleted, &state->_continuations, continuation, NULL, NULL);
}

void cxcore_System_Threading_Tasks_TaskRuntime_OnCompleted(
    struct cxcore_System_Object *taskObject,
    cx_action continuation) {
    struct cx_task_generic_state *state =
        (struct cx_task_generic_state *)taskObject;
    if (taskObject == NULL) {
        cx_threading_raise(&cx_task_null_continuation);
    }
    cx_task_on_completed(
        state->_isCompleted, &state->_continuations, continuation, NULL, NULL);
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ThrowIfAlreadyCompleted(
    struct cxcore_System_Object *taskObject) {
    struct cx_task_generic_state *state =
        (struct cx_task_generic_state *)taskObject;
    if (state == NULL) {
        cx_threading_raise(&cx_task_already_completed);
    }
    cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(
        state->_isCompleted);
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ScheduleContinuations(
    struct cxcore_System_Object *taskObject) {
    struct cx_task_generic_state *state =
        (struct cx_task_generic_state *)taskObject;
    if (state == NULL) {
        return;
    }
    cx_task_schedule(&state->_continuations);
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_MarkFaultObserved(
    struct cxcore_System_Object *taskObject) {
    cx_task_mark_fault_observed(taskObject);
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_SetFault(
    struct cxcore_System_Object *taskObject,
    struct cxcore_System_Exception *exception) {
    struct cx_task_generic_state *state =
        (struct cx_task_generic_state *)taskObject;
    cx_gc_check_thread();
    if (state == NULL) cx_threading_raise(&cx_task_invalid_fault_state);
    if (exception == NULL) {
        cx_threading_raise_argument_null(
            &cx_task_null_fault, &cx_task_exception_parameter);
    }
    cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(
        state->_isCompleted);
    cx_task_store_reference((cx_ptr *)&state->_exception, (cx_ptr)exception);
    state->_isFaulted = CX_TRUE;
    state->_isCanceled = CX_FALSE;
    state->_isCompleted = CX_TRUE;
    cx_task_register_unobserved_fault(taskObject, exception);
    cx_task_schedule(&state->_continuations);
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_SetCanceled(
    struct cxcore_System_Object *taskObject) {
    struct cx_task_generic_state *state =
        (struct cx_task_generic_state *)taskObject;
    cx_gc_check_thread();
    if (state == NULL) {
        cx_threading_raise(&cx_task_already_completed);
    }
    cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(
        state->_isCompleted);
    state->_isFaulted = CX_FALSE;
    state->_isCanceled = CX_TRUE;
    cx_task_store_reference((cx_ptr *)&state->_exception, NULL);
    state->_isCompleted = CX_TRUE;
    cx_task_schedule(&state->_continuations);
}

struct cx_reusable_value_task_source_callback_state {
    struct cxcore_System_Object __base;
    cx_action continuation;
    struct cxcore_System_Object *scheduler_item;
    struct cx_arena *logical_arena;
};

struct cx_reusable_value_task_source_prefix {
    struct cxcore_System_Object __base;
    cx_uint version;
    cx_bool is_completed;
    cx_bool is_consumed;
    cx_bool is_retired;
    cx_uint pending_callbacks;
    cx_bool continuation_registered;
    struct cx_reusable_value_task_source_callback_state *continuation_state;
    struct cxcore_System_Object *native_wait_state;
};

static void cx_reusable_source_callback_acknowledged(void *environment) {
    struct cx_reusable_value_task_source_prefix *source =
        (struct cx_reusable_value_task_source_prefix *)environment;
    if (source != NULL && source->pending_callbacks != 0u) {
        --source->pending_callbacks;
    }
}

static void cx_reusable_source_schedule_registered(
    struct cx_reusable_value_task_source_prefix *source) {
    struct cx_exception_frame frame;
    struct cx_exception failure;
    struct cx_reusable_value_task_source_callback_state *state =
        source->continuation_state;
    cx_action continuation = state == NULL ? (cx_action){0} : state->continuation;
    struct cx_arena *logical_arena = state == NULL ? NULL : state->logical_arena;
    if (continuation.__function == NULL) {
        return;
    }
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cx_task_scheduler_post_reusable_with_logical_context(
            continuation,
            cx_reusable_source_callback_acknowledged,
            source,
            (cx_ptr *)&state->scheduler_item,
            logical_arena);
        cx_task_store_action(&state->continuation, (cx_action){0});
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&state->logical_arena, NULL);
        cx_exception_pop(&frame);
        return;
    }
    failure = *cx_exception_thread_state();
    cx_exception_pop(&frame);
    if (state != NULL) {
        cx_task_store_action(&state->continuation, (cx_action){0});
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&state->logical_arena, NULL);
    }
    if (logical_arena != NULL) cx_arena_release_logical(logical_arena);
    source->pending_callbacks = 0u;
    source->continuation_registered = CX_FALSE;
    cx_exception_restore(&failure);
    cx_exception_rethrow();
}

CX_API cx_bool cxcore_System_Threading_Tasks_TaskRuntime_ReusableSourceIsCompleted(
    struct cxcore_System_Object *sourceObject,
    cx_uint version) {
    struct cx_reusable_value_task_source_prefix *source =
        (struct cx_reusable_value_task_source_prefix *)sourceObject;
    cx_gc_check_thread();
    if (source == NULL || source->is_retired || source->version != version) {
        cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(CX_TRUE);
    }
    return source->is_completed;
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_RegisterReusableSourceContinuation(
    struct cxcore_System_Object *sourceObject,
    cx_uint version,
    cx_action continuation) {
    struct cx_reusable_value_task_source_prefix *source =
        (struct cx_reusable_value_task_source_prefix *)sourceObject;
    cx_gc_check_thread();
    if (source == NULL || source->is_retired || source->version != version) {
        cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(CX_TRUE);
    }
    if (continuation.__function == NULL || source->continuation_state == NULL) {
        cx_threading_raise(&cx_task_null_continuation);
    }
    cx_task_require_async_use_allowed();
    cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(
        source->is_consumed || source->continuation_registered ||
        source->pending_callbacks != 0u);
    {
        struct cx_arena *logical_arena = cx_arena_capture_logical_context();
        if (!cx_task_store_action_with_logical_arena(
                &source->continuation_state->continuation,
                NULL,
                continuation,
                NULL,
                logical_arena)) {
            if (logical_arena != NULL) cx_arena_release_logical(logical_arena);
            cx_threading_raise(&cx_task_scoped_suspend);
        }
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&source->continuation_state->logical_arena,
            (cx_ptr)logical_arena);
    }
    source->continuation_registered = CX_TRUE;
    source->pending_callbacks = 1u;
    if (source->is_completed) {
        cx_reusable_source_schedule_registered(source);
    }
}

CX_API void cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState_Clear(
    struct cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState *self) {
    if (self != NULL) {
        cx_task_store_action(&self->_continuation, (cx_action){0});
        if (self->_logicalArena != NULL) {
            cx_arena_release_logical((struct cx_arena *)self->_logicalArena);
            (void)cx_gc_store_native_pointer(&self->_logicalArena, NULL);
        }
    }
}

CX_API void cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState___constructor(
    struct cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState *self) {
    if (self == NULL) {
        return;
    }
    cxcore_System_Object___constructor(&self->__base);
    cx_task_store_action(&self->_continuation, (cx_action){0});
    cx_task_store_reference((cx_ptr *)&self->_schedulerItem, NULL);
    (void)cx_gc_store_native_pointer(&self->_logicalArena, NULL);
    CX_INIT_VTABLE(self, cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState);
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ScheduleReusableSourceContinuation(
    struct cxcore_System_Object *sourceObject,
    cx_uint version) {
    struct cx_reusable_value_task_source_prefix *source =
        (struct cx_reusable_value_task_source_prefix *)sourceObject;
    cx_gc_check_thread();
    if (source == NULL || source->is_retired || source->version != version) {
        cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(CX_TRUE);
    }
    if (!source->is_completed || !source->continuation_registered) {
        return;
    }
    if (source->pending_callbacks != 1u) {
        cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(CX_TRUE);
    }
    cx_reusable_source_schedule_registered(source);
}

CX_API struct cxcore_System_Object *
cxcore_System_Threading_Tasks_TaskRuntime_GetReusableSourceNativeWaitState(
    struct cxcore_System_Object *sourceObject) {
    struct cx_reusable_value_task_source_prefix *source =
        (struct cx_reusable_value_task_source_prefix *)sourceObject;
    if (source == NULL) {
        cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(CX_TRUE);
    }
    return source->native_wait_state;
}

CX_API void cxcore_System_Threading_Tasks_TaskRuntime_SetReusableSourceNativeWaitState(
    struct cxcore_System_Object *sourceObject,
    struct cxcore_System_Object *item) {
    struct cx_reusable_value_task_source_prefix *source =
        (struct cx_reusable_value_task_source_prefix *)sourceObject;
    if (source == NULL) {
        cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(CX_TRUE);
    }
    cx_task_store_reference((cx_ptr *)&source->native_wait_state, (cx_ptr)item);
}

void cxcore_System_Threading_Tasks_TaskHandle_OnCompleted(
    struct cxcore_System_Threading_Tasks_TaskHandle *self,
    cx_action continuation) {
    if (self == NULL || self->_state == NULL) {
        cx_threading_raise(&cx_task_null_continuation);
    }
    cxcore_System_Threading_Tasks_TaskState_OnCompleted(
        self->_state, continuation);
}

void cxcore_System_Threading_Tasks_TaskRuntime_ForwardVoid(
    struct cxcore_System_Object *sourceObject,
    struct cxcore_System_Object *destinationObject) {
    struct cxcore_System_Threading_Tasks_TaskHandle *source =
        (struct cxcore_System_Threading_Tasks_TaskHandle *)sourceObject;
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *completion =
        (struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *)destinationObject;
    if (source == NULL || source->_state == NULL || completion == NULL ||
        completion->_task == NULL || completion->_task->_state == NULL ||
        source->_state == completion->_task->_state) {
        cx_threading_raise(&cx_task_invalid_forward);
    }
    cx_task_forward_states(source->_state, completion->_task->_state);
}

struct cxcore_System_Object *
cxcore_System_Threading_Tasks_TaskRuntime_GetVoidTask(
    struct cxcore_System_Object *sourceObject) {
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *source =
        (struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *)sourceObject;
    if (source == NULL || source->_task == NULL) {
        cx_threading_raise(&cx_task_invalid_forward);
    }
    return (struct cxcore_System_Object *)source->_task;
}

void cxcore_System_Threading_Tasks_TaskState_OnCompleted(
    struct cxcore_System_Threading_Tasks_TaskState *self,
    cx_action continuation) {
    cx_task_on_completed(
        self->_isCompleted,
        &self->_continuations,
        continuation,
        NULL,
        NULL);
}

CX_API void cx_task_state_on_completed_with_acknowledgement(
    struct cxcore_System_Threading_Tasks_TaskState *self,
    cx_action continuation,
    void (*completion)(void *environment),
    void *completion_environment) {
    if (self == NULL || completion == NULL) {
        cx_threading_raise(&cx_task_null_continuation);
    }
    cx_task_on_completed(
        self->_isCompleted,
        &self->_continuations,
        continuation,
        completion,
        completion_environment);
}

void cxcore_System_Threading_Tasks_TaskState_ScheduleContinuations(
    struct cxcore_System_Threading_Tasks_TaskState *self) {
    struct cx_task_continuation_queue *queue =
        (struct cx_task_continuation_queue *)self->_continuations;
    struct cx_task_continuation *item;
    cx_task_store_reference((cx_ptr *)&self->_continuations, NULL);
    if (queue == NULL) {
        return;
    }
    for (item = &queue->first; item != NULL;) {
        struct cx_task_continuation *next = item->next;
        cx_task_scheduler_post_with_logical_context(
            item->action,
            item->completion,
            item->completion_environment,
            item->logical_arena);
        cx_task_store_action(&item->action, (cx_action){0});
        cx_task_store_reference(
            (cx_ptr *)&item->completion_environment, NULL);
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&item->logical_arena, NULL);
        item = next;
    }
}

void cxcore_System_Threading_Tasks_VoidTask_ThrowIfFaulted(
    cx_bool isFaulted,
    const struct cxcore_System_Exception *exception) {
    if (isFaulted) {
        if (exception != NULL) {
            cx_exception_throw((cx_ptr)exception, __FILE__, __LINE__);
        }
        cx_threading_raise_invalid_operation(&cx_task_faulted);
    }
}

void cxcore_System_Threading_Tasks_TaskState_ThrowIfNotCompleted(
    cx_bool isCompleted) {
    if (!isCompleted) {
        cx_threading_raise_invalid_operation(&cx_task_not_completed);
    }
}

void cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(
    cx_bool isCompleted) {
    if (isCompleted) {
        cx_threading_raise_invalid_operation(&cx_task_already_completed);
    }
}

