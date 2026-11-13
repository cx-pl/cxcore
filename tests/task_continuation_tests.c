#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cxcore.h"
#include "../platform/common/System/Threading/Tasks/Task.internal.h"
#include "../platform/common/System/Threading/Tasks/TaskScheduler.internal.h"
#include "../platform/common/cxarena.h"

CX_API size_t cx_memory_test_allocation_count_get(void);
CX_API size_t cx_memory_test_allocation_bytes_get(void);
CX_API void cx_memory_test_allocation_tracking_set(cx_bool enabled);
CX_API void cx_task_scheduler_post_with_completion(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment);
CX_API void cx_task_scheduler_test_post_allocation_count_reset(void);
CX_API size_t cx_task_scheduler_test_post_allocation_count_get(void);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_SetReusableSourceNativeWaitState(
    struct cxcore_System_Object *source,
    struct cxcore_System_Object *item);

struct task_continuation_context {
    cx_uint *target;
};

static cx_uint observed_value;
static cx_uint timer_fired;
static struct cx_arena *logical_scheduler_arena;
static cx_bool logical_scheduler_allocation_observed;
static cx_bool logical_scheduler_current_arena_observed;
static cx_bool logical_scheduler_closure_observed;
static cx_bool logical_scheduler_stack_store_observed;
static cx_uint canceled_continuation_fired;
static cx_ptr scheduler_callback_exception;
static struct cxcore_System_Threading_Tasks_TaskState *reentrant_state;
static const struct cx_gc_root_region task_test_roots[] = {
    {&reentrant_state, (cx_uint)sizeof(reentrant_state)},
    {&scheduler_callback_exception, (cx_uint)sizeof(scheduler_callback_exception)},
};
static cx_uint reentrant_order[4];
static cx_uint reentrant_count;
static void record_late_task_callback(void *environment);
static cx_uint scheduler_callback_count;
static cx_bool native_callback_arena_observed;
static cx_bool native_callback_stack_borrow_observed;
static cx_uint scheduler_completion_order[2];
static cx_uint scheduler_completion_count;
static cx_uint reusable_scheduler_callback_count;
static cx_uint reusable_scheduler_ack_count;
static cx_ptr *reusable_scheduler_cache_slot;
static cx_bool reusable_scheduler_dispatch_rejected;
static void reusable_scheduler_acknowledged(void *environment);
static struct cxcore_System_Threading_Tasks_CancellationTokenRegistration
    cancellation_registration_to_dispose;
static cx_uint cancellation_interleave_order[2];
static cx_uint cancellation_interleave_count;
static struct cxcore_System_Threading_Tasks_CancellationTokenRegistration
    seeded_cancellation_registrations[4];
static cx_uint seeded_cancellation_order[4];
static cx_uint seeded_cancellation_count;
struct seeded_cancellation_callback_context {
    cx_uint value;
    cx_uint registration_to_dispose;
};

static struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *
create_reusable_value_task_source(void) {
    struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *source;
    struct cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState *state;
    struct cx_arena_durable_scope durable_scope;
    cx_arena_enter_durable(&durable_scope);
    source = (struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *)
        cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
    state = (struct cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState *)
        cxcore_System_Memory_Alloc((cx_uint)sizeof(*state));
    cxcore_System_Threading_Tasks_ReusableValueTaskSource___constructor(source);
    cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState___constructor(state);
    cx_gc_store_reference((cx_ptr *)&source->_continuationState, (cx_ptr)state);
    cx_arena_leave_durable(&durable_scope);
    return source;
}

static void trigger_reusable_source_native_wait_state_store_barrier(void) {
    struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *source =
        create_reusable_value_task_source();
    struct cxcore_System_Object *item;
    struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
    struct cx_arena_scope scope;

    cx_arena_enter(arena, &scope);
    item = (struct cxcore_System_Object *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*item));
    cxcore_System_Object___constructor(item);
    if (!cx_arena_current_owns(item)) {
        fputs("native wait-state test value is not arena-owned\n", stderr);
        exit(EXIT_FAILURE);
    }
    cxcore_System_Threading_Tasks_TaskRuntime_SetReusableSourceNativeWaitState(
        (struct cxcore_System_Object *)source, item);
    cx_arena_leave(&scope);
    cx_arena_destroy(arena);
}

static void throw_logical_scheduler_callback(void *environment) {
    (void)environment;
    logical_scheduler_current_arena_observed =
        cx_arena_current_is(logical_scheduler_arena) != 0;
    cx_exception_throw(scheduler_callback_exception, __FILE__, __LINE__);
}

static void nested_run_callback(void *environment) {
    (void)environment;
    ++scheduler_callback_count;
    cxcore_System_Threading_Tasks_TaskScheduler_Run();
}

static void nested_run_one_callback(void *environment) {
    (void)environment;
    ++scheduler_callback_count;
    cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
}

static void record_scheduler_callback(void *environment) {
    (void)environment;
    ++scheduler_callback_count;
}

static void native_arena_registration_callback(void *environment) {
    cx_ptr stack_slot = NULL;
    native_callback_arena_observed =
        cx_arena_current_is(logical_scheduler_arena) &&
        cx_arena_current_owns(environment) != 0;
    native_callback_stack_borrow_observed =
        cx_arena_test_reference_store_allowed(&stack_slot, environment, 1) != 0;
}

static void throw_native_arena_registration_callback(void *environment) {
    static const char failure_text[] = "native arena callback failure";
    struct cxcore_System_String *message;
    struct cxcore_System_Exception *error;
    (void)environment;
    message = (struct cxcore_System_String *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*message));
    cxcore_System_String___constructor__2(message,
        (cx_uint)(sizeof(failure_text) - 1u), (cx_ptr)failure_text);
    error = (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, message);
    cx_exception_throw(error, __FILE__, __LINE__);
}

static cx_bool invoke_native_arena_registration(
    struct cxcore_System_Interop_NativeCallbackRegistration *registration) {
    struct cx_native_registration_scope registration_scope = {0};
    volatile char callback_stack_boundary = 0;
    void *context = (void *)cxcore_System_Interop_NativeCallbackRegistration_Context___const_get(registration);
    const cx_action *callback;
    struct cx_exception_frame frame;
    cx_bool callback_failed = CX_FALSE;
    cx_bool entered;
    if (!cx_interop_registration_acquire(context, &registration_scope)) return CX_FALSE;
    entered = cx_interop_registration_enter_logical(
        &registration_scope, &callback_stack_boundary);
    if (!entered) {
        cx_interop_registration_release(&registration_scope);
        return CX_FALSE;
    }
    callback = (const cx_action *)cx_interop_registration_payload(context);
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        callback->__function(callback->__environment);
        cx_exception_pop(&frame);
    } else {
        cx_interop_registration_failed(context, cx_exception_current());
        callback_failed = CX_TRUE;
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    cx_interop_registration_leave_logical(&registration_scope);
    cx_interop_registration_release(&registration_scope);
    (void)callback_failed;
    return CX_TRUE;
}

static void allocate_in_logical_scheduler_arena(void *environment) {
    cx_ptr allocation;
    (void)environment;
    allocation = cxcore_System_Memory_Alloc(24u);
    logical_scheduler_current_arena_observed =
        allocation != NULL && cx_arena_current_owns(allocation);
    logical_scheduler_allocation_observed =
        allocation != NULL && cx_arena_owns(logical_scheduler_arena, allocation);
}

static void throw_scheduler_callback(void *environment) {
    (void)environment;
    scheduler_completion_order[scheduler_completion_count++] = 1u;
    cx_exception_throw(scheduler_callback_exception, __FILE__, __LINE__);
}

static void record_before_scheduler_completion(void *environment) {
    (void)environment;
    scheduler_completion_order[scheduler_completion_count++] = 1u;
}

static void record_scheduler_completion(void *environment) {
    (void)environment;
    scheduler_completion_order[scheduler_completion_count++] = 2u;
}

static void reusable_scheduler_callback(void *environment) {
    struct cx_exception_frame frame;
    cx_action action = {0};
    cx_bool caught = CX_FALSE;
    (void)environment;
    ++reusable_scheduler_callback_count;
    action.__function = reusable_scheduler_callback;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cx_task_scheduler_post_reusable_with_completion(
            action, reusable_scheduler_acknowledged, NULL,
            reusable_scheduler_cache_slot);
        cx_exception_pop(&frame);
    } else {
        caught = CX_TRUE;
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    if (caught) {
        reusable_scheduler_dispatch_rejected = CX_TRUE;
    }
}

static void reusable_scheduler_acknowledged(void *environment) {
    (void)environment;
    ++reusable_scheduler_ack_count;
}

static void dispose_during_cancellation(void *environment) {
    (void)environment;
    if (cancellation_interleave_count < 2u) {
        cancellation_interleave_order[cancellation_interleave_count] = 1u;
    }
    ++cancellation_interleave_count;
    cxcore_System_Threading_Tasks_CancellationTokenRegistration_Dispose(
        &cancellation_registration_to_dispose);
}

static void record_during_cancellation(void *environment) {
    (void)environment;
    if (cancellation_interleave_count < 2u) {
        cancellation_interleave_order[cancellation_interleave_count] = 2u;
    }
    ++cancellation_interleave_count;
}

static void seeded_cancellation_callback(void *environment) {
    struct seeded_cancellation_callback_context *context =
        (struct seeded_cancellation_callback_context *)environment;
    if (seeded_cancellation_count < 4u) {
        seeded_cancellation_order[seeded_cancellation_count] = context->value;
    }
    ++seeded_cancellation_count;
    if (context->registration_to_dispose < 4u) {
        cxcore_System_Threading_Tasks_CancellationTokenRegistration_Dispose(
            &seeded_cancellation_registrations[context->registration_to_dispose]);
    }
}

static void cross_thread_scheduler_post(void *environment) {
    cx_action continuation = {0};
    (void)environment;
    continuation.__function = record_scheduler_callback;
    cxcore_System_Threading_Tasks_TaskScheduler_Post(continuation);
}

static void record_reentrant(cx_uint value) {
    if (reentrant_count < 4u) reentrant_order[reentrant_count] = value;
    ++reentrant_count;
}

static void complete_secondary_task(void *environment) {
    (void)environment;
    record_reentrant(1u);
    cxcore_System_Threading_Tasks_TaskState_SetResult(reentrant_state, NULL);
}

static void register_after_secondary_completion(void *environment) {
    cx_action late = {0};
    (void)environment;
    record_reentrant(2u);
    late.__function = record_late_task_callback;
    late.__environment = NULL;
    late.__target = NULL;
    late.__method = NULL;
    late.__invocation_list = NULL;
    cxcore_System_Threading_Tasks_TaskState_OnCompleted(
        reentrant_state, late);
}

static void record_secondary_task(void *environment) {
    (void)environment;
    record_reentrant(3u);
}

static void record_late_task_callback(void *environment) {
    (void)environment;
    record_reentrant(4u);
}

static void observe_task_continuation(void *environment) {
    const struct task_continuation_context *context =
        (const struct task_continuation_context *)environment;
    observed_value = context->target == NULL ? 0 : *context->target;
}

static void observe_logical_scheduler_closure(void *environment) {
    const struct task_continuation_context *context =
        (const struct task_continuation_context *)environment;
    cx_ptr stack_slot = NULL;
    logical_scheduler_closure_observed =
        context != NULL && context->target != NULL &&
        cx_arena_current_owns(context) &&
        cx_arena_current_owns(context->target);
    logical_scheduler_current_arena_observed =
        logical_scheduler_closure_observed;
    logical_scheduler_stack_store_observed =
        cx_arena_test_reference_store_allowed(
            &stack_slot, environment, 1) != 0;
    if (logical_scheduler_closure_observed) {
        observed_value = *context->target;
    }
}

static void observe_timer(void *environment) {
    (void)environment;
    ++timer_fired;
}

static void observe_canceled_task(void *environment) {
    (void)environment;
    ++canceled_continuation_fired;
}

static cx_action create_timer_with_captured_target(void) {
    struct task_continuation_context *context =
        (struct task_continuation_context *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*context));
    cx_action timer = {0};
    context->target = (cx_uint *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*context->target));
    *context->target = 0x18c0ffeeu;
    timer.__function = observe_task_continuation;
    timer.__environment = context;
    timer.__target = (cx_ptr)context;
    return timer;
}

#ifdef _MSC_VER
__declspec(noinline)
#else
__attribute__((noinline))
#endif
static struct cxcore_System_Threading_Tasks_TaskState *
create_pending_state_with_captured_target(void) {
    struct cxcore_System_Threading_Tasks_TaskState *state =
        (struct cxcore_System_Threading_Tasks_TaskState *)
        cxcore_System_Memory_Alloc((cx_uint)sizeof(*state));
    struct task_continuation_context *context =
        (struct task_continuation_context *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*context));
    cx_action continuation = {0};

    context->target = (cx_uint *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*context->target));
    *context->target = 0x18c0ffeeu;
    continuation.__function = observe_task_continuation;
    continuation.__environment = context;
    continuation.__target = (cx_ptr)context;
    continuation.__method = NULL;
    continuation.__invocation_list = NULL;

    cxcore_System_Threading_Tasks_TaskState___constructor(state);
    cxcore_System_Threading_Tasks_TaskState_OnCompleted(
        state, continuation);
    context = NULL;
    continuation.__environment = NULL;
    return state;
}

int main(int argc, char **argv) {
    struct cxcore_System_Threading_Tasks_TaskState *volatile state;
    cx_gc_check_thread();
    if (argc == 2 && strcmp(argv[1],
            "durable-reusable-source-rejects-scoped-native-state") == 0) {
        trigger_reusable_source_native_wait_state_store_barrier();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "logical-arena-scheduler-context") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        cx_action callback = {0};
        logical_scheduler_arena = cx_arena_create(256u, 0u, 0);
        logical_scheduler_allocation_observed = CX_FALSE;
        cx_arena_retain_logical(logical_scheduler_arena);
        callback.__function = allocate_in_logical_scheduler_arena;
        cx_arena_enter(logical_scheduler_arena, &scope);
        cxcore_System_Threading_Tasks_TaskScheduler_Post(callback);
        cx_arena_leave(&scope);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 1;
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            !logical_scheduler_allocation_observed) return 2;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 3;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "logical-arena-task-continuation") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct cxcore_System_Threading_Tasks_TaskState *pending;
        cx_action callback = {0};
        struct task_continuation_context *captured_context;
        cx_uint *captured_target;
        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        logical_scheduler_allocation_observed = CX_FALSE;
        logical_scheduler_current_arena_observed = CX_FALSE;
        logical_scheduler_stack_store_observed = CX_FALSE;
        cx_arena_retain_logical(logical_scheduler_arena);
        pending = (struct cxcore_System_Threading_Tasks_TaskState *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*pending));
        cxcore_System_Threading_Tasks_TaskState___constructor(pending);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x22c0ffeeu;
        captured_context->target = captured_target;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(pending, callback);
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_target = NULL;
        callback.__environment = NULL;
        callback.__target = NULL;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 11;
        cx_gc_collect();
        cxcore_System_Threading_Tasks_TaskState_SetResult(pending, NULL);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 12;
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) return 16;
        if (!logical_scheduler_current_arena_observed ||
            !logical_scheduler_stack_store_observed) return 17;
        if (!logical_scheduler_closure_observed ||
            observed_value != 0x22c0ffeeu) return 18;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 13;

        logical_scheduler_allocation_observed = CX_FALSE;
        pending = (struct cxcore_System_Threading_Tasks_TaskState *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*pending));
        cxcore_System_Threading_Tasks_TaskState___constructor(pending);
        callback = (cx_action){0};
        callback.__function = allocate_in_logical_scheduler_arena;
        cx_arena_enter(logical_scheduler_arena, &scope);
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(pending, callback);
        cx_arena_leave(&scope);
        cxcore_System_Threading_Tasks_TaskState_SetCanceled(pending);
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            !logical_scheduler_allocation_observed) return 14;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 15;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "logical-arena-cancellation-callback") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct cxcore_System_Threading_Tasks_CancellationTokenSource *source;
        struct task_continuation_context *captured_context;
        cx_uint *captured_target;
        cx_action callback = {0};
        struct cx_exception_frame frame;
        cx_bool registration_failed = CX_FALSE;
        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        logical_scheduler_closure_observed = CX_FALSE;
        logical_scheduler_current_arena_observed = CX_FALSE;
        logical_scheduler_stack_store_observed = CX_FALSE;
        source = (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
        cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x22c0ffeeu;
        captured_context->target = captured_target;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
                source, callback);
            cx_exception_pop(&frame);
        } else {
            registration_failed = CX_TRUE;
            cx_exception_pop(&frame);
        }
        cx_arena_leave(&scope);
        if (registration_failed) {
            struct cxcore_System_Exception *exception =
                (struct cxcore_System_Exception *)cx_exception_current();
            if (exception != NULL && exception->_message != NULL) {
                fprintf(stderr, "retained cancellation registration failed: %.*s\n",
                    (int)exception->_message->_length,
                    (const char *)exception->_message->_data);
            } else {
                fputs("retained cancellation callback registration failed\n", stderr);
            }
            return 34;
        }
        captured_context = NULL;
        captured_target = NULL;
        callback = (cx_action){0};
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 31;
        cx_gc_collect();
        cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(source);
        if (!logical_scheduler_closure_observed ||
            !logical_scheduler_current_arena_observed ||
            observed_value != 0x22c0ffeeu) return 32;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 33;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        source = (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
        cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
        struct cxcore_System_Exception *cancellation_exception =
            (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*cancellation_exception));
        cxcore_System_Exception___constructor(cancellation_exception, NULL);
        scheduler_callback_exception = (cx_ptr)cancellation_exception;
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_context->target = NULL;
        callback.__function = throw_scheduler_callback;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        (void)cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
            source, callback);
        cx_arena_leave(&scope);
        captured_context = NULL;
        callback = (cx_action){0};
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 37;
        struct cx_exception_frame cancellation_frame;
        cx_bool cancellation_threw = CX_FALSE;
        cx_exception_push(&cancellation_frame);
        if (setjmp(cancellation_frame.environment) == 0) {
            cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(source);
            cx_exception_pop(&cancellation_frame);
        } else {
            cancellation_threw =
                cx_exception_current() == scheduler_callback_exception;
            cx_exception_pop(&cancellation_frame);
            cx_exception_clear();
        }
        scheduler_callback_exception = NULL;
        if (!cancellation_threw) return 38;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 39;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        logical_scheduler_closure_observed = CX_FALSE;
        source = (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
        cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x11c0ffeeu;
        captured_context->target = captured_target;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        struct cxcore_System_Threading_Tasks_CancellationTokenRegistration registration =
            cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
                source, callback);
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_target = NULL;
        callback = (cx_action){0};
        cxcore_System_Threading_Tasks_CancellationTokenRegistration_Dispose(
            &registration);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 35;
        cx_gc_collect();
        cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(source);
        if (logical_scheduler_closure_observed) return 36;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "logical-arena-thread-context") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct task_continuation_context *captured_context;
        cx_uint *captured_target;
        struct cxcore_System_Threading_Thread worker;
        cx_action callback = {0};
        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        logical_scheduler_closure_observed = CX_FALSE;
        logical_scheduler_current_arena_observed = CX_FALSE;
        logical_scheduler_stack_store_observed = CX_FALSE;
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x22c0ffeeu;
        captured_context->target = captured_target;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        worker = cxcore_System_Threading_Thread_Start(callback);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 41;
        {
            struct cx_exception_frame join_frame;
            cx_bool join_rejected = CX_FALSE;
            cx_exception_push(&join_frame);
            if (setjmp(join_frame.environment) == 0) {
                cxcore_System_Threading_Thread_Join(&worker);
                cx_exception_pop(&join_frame);
            } else {
                join_rejected = CX_TRUE;
                cx_exception_pop(&join_frame);
                cx_exception_clear();
            }
            if (!join_rejected) return 46;
        }
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_target = NULL;
        callback = (cx_action){0};
        cx_gc_collect();
        cxcore_System_Threading_Thread_Join(&worker);
        if (!logical_scheduler_closure_observed ||
            !logical_scheduler_current_arena_observed ||
            !logical_scheduler_stack_store_observed ||
            observed_value != 0x22c0ffeeu) return 42;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 43;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        cx_arena_retain_logical(logical_scheduler_arena);
        struct cxcore_System_Exception *thread_exception =
            (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*thread_exception));
        cxcore_System_Exception___constructor(thread_exception, NULL);
        scheduler_callback_exception = (cx_ptr)thread_exception;
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        callback = (cx_action){0};
        callback.__function = throw_scheduler_callback;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        worker = cxcore_System_Threading_Thread_Start(callback);
        cx_arena_leave(&scope);
        captured_context = NULL;
        callback = (cx_action){0};
        cx_gc_collect();
        struct cx_exception_frame frame;
        cx_bool caught = CX_FALSE;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Threading_Thread_Join(&worker);
            cx_exception_pop(&frame);
        } else {
            caught = cx_exception_current() == scheduler_callback_exception;
            cx_exception_pop(&frame);
            cx_exception_clear();
        }
        scheduler_callback_exception = NULL;
        if (!caught) return 44;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 45;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "retained-cancellation-foreign-capture-unhandled") == 0) {
        struct cx_arena *capture_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena *registration_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct task_continuation_context *context;
        struct cxcore_System_Threading_Tasks_CancellationTokenSource *source;
        cx_action callback = {0};
        cx_arena_retain_logical(capture_arena);
        cx_arena_retain_logical(registration_arena);
        cx_arena_enter(capture_arena, &scope);
        context = (struct task_continuation_context *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*context));
        context->target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*context->target));
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = context;
        callback.__target = (cx_ptr)context;
        cx_arena_leave(&scope);
        source = (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
        cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
        cx_arena_enter(registration_arena, &scope);
        (void)cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
            source, callback);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "retained-thread-foreign-capture-unhandled") == 0) {
        struct cx_arena *capture_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena *registration_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct task_continuation_context *context;
        cx_action callback = {0};
        cx_arena_retain_logical(capture_arena);
        cx_arena_retain_logical(registration_arena);
        cx_arena_enter(capture_arena, &scope);
        context = (struct task_continuation_context *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*context));
        context->target = NULL;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = context;
        callback.__target = (cx_ptr)context;
        cx_arena_leave(&scope);
        cx_arena_enter(registration_arena, &scope);
        (void)cxcore_System_Threading_Thread_Start(callback);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "logical-arena-reusable-scheduler-context") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct task_continuation_context *captured_context;
        cx_uint *captured_target;
        cx_ptr cached_item = NULL;
        cx_action callback = {0};
        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        logical_scheduler_closure_observed = CX_FALSE;
        logical_scheduler_current_arena_observed = CX_FALSE;
        scheduler_completion_count = 0u;
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x22c0ffeeu;
        captured_context->target = captured_target;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        cx_task_scheduler_post_reusable_with_completion(
            callback, record_scheduler_completion, NULL, &cached_item);
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_target = NULL;
        callback.__environment = NULL;
        callback.__target = NULL;
        cx_gc_collect();
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            !logical_scheduler_closure_observed ||
            !logical_scheduler_current_arena_observed ||
            observed_value != 0x22c0ffeeu ||
            scheduler_completion_count != 1u ||
            scheduler_completion_order[0] != 2u) return 21;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 22;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "logical-arena-reusable-value-source-context") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *source;
        struct cxcore_System_Exception *thread_exception;
        struct cx_exception_frame frame;
        struct task_continuation_context *captured_context;
        cx_uint *captured_target;
        cx_action callback = {0};
        cx_bool caught = CX_FALSE;
        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        logical_scheduler_closure_observed = CX_FALSE;
        logical_scheduler_current_arena_observed = CX_FALSE;
        observed_value = 0u;
        source = create_reusable_value_task_source();
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x22c0ffeeu;
        captured_context->target = captured_target;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        cxcore_System_Threading_Tasks_TaskRuntime_RegisterReusableSourceContinuation(
            (struct cxcore_System_Object *)source, 0u, callback);
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_target = NULL;
        callback = (cx_action){0};
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 47;
        cx_gc_collect();
        source->_isCompleted = CX_TRUE;
        cxcore_System_Threading_Tasks_TaskRuntime_ScheduleReusableSourceContinuation(
            (struct cxcore_System_Object *)source, 0u);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 48;
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) return 49;
        if (!logical_scheduler_closure_observed ||
            !logical_scheduler_current_arena_observed ||
            !logical_scheduler_stack_store_observed ||
            observed_value != 0x22c0ffeeu) return 50;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u ||
            source->_pendingCallbacks != 0u) return 51;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;
        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        cx_arena_retain_logical(logical_scheduler_arena);
        source = create_reusable_value_task_source();
        {
            struct cx_arena_durable_scope durable_scope;
            cx_arena_enter_durable(&durable_scope);
            thread_exception = (struct cxcore_System_Exception *)
                cxcore_System_Memory_Alloc((cx_uint)sizeof(*thread_exception));
            cxcore_System_Exception___constructor(thread_exception, NULL);
            cx_arena_leave_durable(&durable_scope);
            scheduler_callback_exception = (cx_ptr)thread_exception;
        }
        cx_arena_enter(logical_scheduler_arena, &scope);
        callback = (cx_action){0};
        callback.__function = throw_logical_scheduler_callback;
        cxcore_System_Threading_Tasks_TaskRuntime_RegisterReusableSourceContinuation(
            (struct cxcore_System_Object *)source, 0u, callback);
        cx_arena_leave(&scope);
        source->_isCompleted = CX_TRUE;
        cxcore_System_Threading_Tasks_TaskRuntime_ScheduleReusableSourceContinuation(
            (struct cxcore_System_Object *)source, 0u);
        caught = CX_FALSE;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
            cx_exception_pop(&frame);
        } else {
            caught = cx_exception_current() == scheduler_callback_exception;
            cx_exception_pop(&frame);
            cx_exception_clear();
        }
        scheduler_callback_exception = NULL;
        if (!caught || !logical_scheduler_current_arena_observed ||
            source->_pendingCallbacks != 0u) return 52;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 53;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        logical_scheduler_closure_observed = CX_FALSE;
        logical_scheduler_current_arena_observed = CX_FALSE;
        logical_scheduler_stack_store_observed = CX_FALSE;
        observed_value = 0u;
        cx_arena_retain_logical(logical_scheduler_arena);
        source = create_reusable_value_task_source();
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x33c0ffeeu;
        captured_context->target = captured_target;
        callback = (cx_action){0};
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        cxcore_System_Threading_Tasks_TaskRuntime_RegisterReusableSourceContinuation(
            (struct cxcore_System_Object *)source, 0u, callback);
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_target = NULL;
        callback = (cx_action){0};
        source->_isCompleted = CX_TRUE;
        cxcore_System_Threading_Tasks_TaskRuntime_ScheduleReusableSourceContinuation(
            (struct cxcore_System_Object *)source, 0u);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 56;
        cxcore_System_Threading_Tasks_TaskScheduler_Shutdown();
        if (!logical_scheduler_closure_observed ||
            !logical_scheduler_current_arena_observed ||
            !logical_scheduler_stack_store_observed ||
            observed_value != 0x33c0ffeeu) return 57;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 58;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;

        {
            struct cx_arena *capture_arena = cx_arena_create(256u, 0u, 0);
            struct cx_arena *registration_arena = cx_arena_create(256u, 0u, 0);
            struct cx_arena_scope registration_scope;
            struct cx_arena_statistics capture_statistics;
            struct cx_arena_statistics registration_statistics;
            struct task_continuation_context *foreign_context;
            struct cx_exception_frame foreign_frame;
            cx_bool foreign_registration_caught = CX_FALSE;
            source = create_reusable_value_task_source();
            cx_arena_retain_logical(capture_arena);
            cx_arena_retain_logical(registration_arena);
            cx_arena_enter(capture_arena, &scope);
            foreign_context = (struct task_continuation_context *)
                cxcore_System_Memory_Alloc((cx_uint)sizeof(*foreign_context));
            callback = (cx_action){0};
            callback.__function = observe_logical_scheduler_closure;
            callback.__environment = foreign_context;
            callback.__target = (cx_ptr)foreign_context;
            cx_arena_leave(&scope);
            cx_arena_enter(registration_arena, &registration_scope);
            cx_exception_push(&foreign_frame);
            if (setjmp(foreign_frame.environment) == 0) {
                cxcore_System_Threading_Tasks_TaskRuntime_RegisterReusableSourceContinuation(
                    (struct cxcore_System_Object *)source, 0u, callback);
                cx_exception_pop(&foreign_frame);
            } else {
                foreign_registration_caught = CX_TRUE;
                cx_exception_pop(&foreign_frame);
                cx_exception_clear();
            }
            cx_arena_leave(&registration_scope);
            cx_arena_get_statistics(capture_arena, &capture_statistics);
            cx_arena_get_statistics(registration_arena, &registration_statistics);
            if (!foreign_registration_caught || capture_statistics.logical_reference_count != 1u ||
                registration_statistics.logical_reference_count != 1u ||
                source->_continuationRegistered || source->_pendingCallbacks != 0u) {
                return 54;
            }
            cx_arena_release_logical(capture_arena);
            cx_arena_release_logical(registration_arena);
            cx_arena_reset(capture_arena);
            cx_arena_destroy(capture_arena);
            cx_arena_reset(registration_arena);
            cx_arena_destroy(registration_arena);
        }

        {
            struct cx_arena *unretained_arena = cx_arena_create(256u, 0u, 0);
            struct cx_arena_statistics unretained_statistics;
            struct cx_exception_frame unretained_frame;
            cx_bool unretained_registration_caught = CX_FALSE;
            source = create_reusable_value_task_source();
            cx_arena_enter(unretained_arena, &scope);
            callback = (cx_action){0};
            callback.__function = record_scheduler_callback;
            cx_exception_push(&unretained_frame);
            if (setjmp(unretained_frame.environment) == 0) {
                cxcore_System_Threading_Tasks_TaskRuntime_RegisterReusableSourceContinuation(
                    (struct cxcore_System_Object *)source, 0u, callback);
                cx_exception_pop(&unretained_frame);
            } else {
                unretained_registration_caught = CX_TRUE;
                cx_exception_pop(&unretained_frame);
                cx_exception_clear();
            }
            cx_arena_leave(&scope);
            cx_arena_get_statistics(unretained_arena, &unretained_statistics);
            if (!unretained_registration_caught || unretained_statistics.logical_reference_count != 0u ||
                source->_continuationRegistered || source->_pendingCallbacks != 0u) {
                return 55;
            }
            cx_arena_reset(unretained_arena);
            cx_arena_destroy(unretained_arena);
        }
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "logical-arena-scheduler-timer-context") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        cx_action callback = {0};
        struct task_continuation_context *captured_context;
        cx_uint *captured_target;
        logical_scheduler_arena = cx_arena_create(256u, 0u, 0);
        logical_scheduler_allocation_observed = CX_FALSE;
        logical_scheduler_closure_observed = CX_FALSE;
        logical_scheduler_current_arena_observed = CX_FALSE;
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        captured_context = (struct task_continuation_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_target = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*captured_target));
        *captured_target = 0x22c0ffeeu;
        captured_context->target = captured_target;
        callback.__function = observe_logical_scheduler_closure;
        callback.__environment = captured_context;
        callback.__target = (cx_ptr)captured_context;
        cxcore_System_Threading_Tasks_TaskScheduler_PostAfter(1u, callback);
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_target = NULL;
        callback.__environment = NULL;
        callback.__target = NULL;
        cx_gc_collect();
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            !logical_scheduler_closure_observed ||
            !logical_scheduler_current_arena_observed ||
            observed_value != 0x22c0ffeeu) return 4;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 5;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "logical-arena-scheduler-exception-releases") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct cx_exception_frame frame;
        struct cxcore_System_Exception *exception;
        cx_action callback = {0};
        cx_bool caught = CX_FALSE;
        logical_scheduler_arena = cx_arena_create(256u, 0u, 0);
        cx_arena_retain_logical(logical_scheduler_arena);
        exception = (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*exception));
        cxcore_System_Exception___constructor(exception, NULL);
        scheduler_callback_exception = (cx_ptr)exception;
        scheduler_completion_count = 0u;
        callback.__function = throw_scheduler_callback;
        cx_arena_enter(logical_scheduler_arena, &scope);
        cx_task_scheduler_post_with_completion(
            callback, record_scheduler_completion, NULL);
        cx_arena_leave(&scope);
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
        } else {
            caught = cx_exception_current() == scheduler_callback_exception;
        }
        cx_exception_pop(&frame);
        cx_exception_clear();
        scheduler_callback_exception = NULL;
        if (!caught || scheduler_completion_count != 2u ||
            scheduler_completion_order[0] != 1u ||
            scheduler_completion_order[1] != 2u) return 6;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 7;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "logical-arena-scheduler-shutdown-releases") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        cx_action callback = {0};
        logical_scheduler_arena = cx_arena_create(256u, 0u, 0);
        cx_arena_retain_logical(logical_scheduler_arena);
        callback.__function = allocate_in_logical_scheduler_arena;
        cx_arena_enter(logical_scheduler_arena, &scope);
        cxcore_System_Threading_Tasks_TaskScheduler_PostAfter(60000u, callback);
        cx_arena_leave(&scope);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 8;
        cxcore_System_Threading_Tasks_TaskScheduler_Shutdown();
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 9;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "nested-run-unhandled") == 0) {
        cx_action callback = {0};
        callback.__function = nested_run_callback;
        cxcore_System_Threading_Tasks_TaskScheduler_Post(callback);
        cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "wrong-thread-unhandled") == 0) {
        struct cxcore_System_Threading_Thread worker;
        cx_action callback = {0};
        cx_action establish_owner = {0};
        establish_owner.__function = record_scheduler_callback;
        cxcore_System_Threading_Tasks_TaskScheduler_Post(establish_owner);
        cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
        callback.__function = cross_thread_scheduler_post;
        worker = cxcore_System_Threading_Thread_Start(callback);
        cxcore_System_Threading_Thread_Join(&worker);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "scoped-post-unhandled") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        cx_action callback = {0};
        callback.__function = record_scheduler_callback;
        cx_arena_enter(arena, &scope);
        cxcore_System_Threading_Tasks_TaskScheduler_Post(callback);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "scoped-continuation-unhandled") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_Threading_Tasks_TaskState *pending;
        cx_action callback = {0};
        cx_arena_enter(arena, &scope);
        pending = (struct cxcore_System_Threading_Tasks_TaskState *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*pending));
        cxcore_System_Threading_Tasks_TaskState___constructor(pending);
        callback.__function = record_scheduler_callback;
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(pending, callback);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "retained-continuation-foreign-capture-unhandled") == 0) {
        struct cx_arena *callback_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena *registration_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_Threading_Tasks_TaskState *pending;
        cx_action callback = {0};
        void *captured_environment;
        cx_arena_retain_logical(callback_arena);
        cx_arena_enter(callback_arena, &scope);
        captured_environment = cxcore_System_Memory_Alloc(16u);
        cx_arena_leave(&scope);
        cx_arena_retain_logical(registration_arena);
        pending = (struct cxcore_System_Threading_Tasks_TaskState *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*pending));
        cxcore_System_Threading_Tasks_TaskState___constructor(pending);
        callback.__function = record_scheduler_callback;
        callback.__environment = captured_environment;
        cx_arena_enter(registration_arena, &scope);
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(pending, callback);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "scoped-cancellation-registration-unhandled") == 0) {
        struct cx_arena *arena = cx_arena_create(512u, 0u, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_Threading_Tasks_CancellationTokenSource *source;
        cx_action callback = {0};
        source = (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
        cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
        cx_arena_enter(arena, &scope);
        callback.__function = record_scheduler_callback;
        (void)cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
            source, callback);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "scoped-native-callback-registration-unhandled") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        cx_action callback = {0};
        callback.__function = record_scheduler_callback;
        cx_arena_enter(arena, &scope);
        (void)cx_interop_registration_create(
            &callback, sizeof(callback), "test-callback", NULL);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "scoped-native-callback-capture-unhandled") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_action callback = {0};
        cx_ptr scoped_capture;
        callback.__function = record_scheduler_callback;
        cx_arena_enter(arena, &scope);
        scoped_capture = cxcore_System_Memory_Alloc(16u);
        cx_arena_enter_durable(&durable_scope);
        callback.__environment = scoped_capture;
        (void)cx_interop_registration_create(
            &callback, sizeof(callback), "test-callback", NULL);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "scoped-native-callback-owner-unhandled") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_action callback = {0};
        cx_ptr scoped_owner;
        callback.__function = record_scheduler_callback;
        cx_arena_enter(arena, &scope);
        scoped_owner = cxcore_System_Memory_Alloc(16u);
        cx_arena_enter_durable(&durable_scope);
        (void)cx_interop_registration_create(
            &callback, sizeof(callback), "test-callback", scoped_owner);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "stack-native-callback-owner-unhandled") == 0) {
        cx_action callback = {0};
        cx_uint stack_owner = 42u;
        callback.__function = record_scheduler_callback;
        (void)cx_interop_registration_create(
            &callback, sizeof(callback), "test-callback", &stack_owner);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "durable-native-callback-registration") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_action callback = {0};
        struct cxcore_System_Interop_NativeCallbackRegistration *registration;
        callback.__function = record_scheduler_callback;
        cx_arena_enter(arena, &scope);
        cx_arena_enter_durable(&durable_scope);
        registration = cx_interop_registration_create(
            &callback, sizeof(callback), "test-callback", NULL);
        cx_arena_leave_durable(&durable_scope);
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        cxcore_System_Interop_NativeCallbackRegistration_Unregister(
            registration, callback);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1],
            "retained-native-callback-foreign-capture-unhandled") == 0) {
        struct cx_arena *retained_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena *foreign_arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        cx_action callback = {0};
        cx_ptr foreign_capture;
        cx_arena_retain_logical(retained_arena);
        cx_arena_enter(foreign_arena, &scope);
        foreign_capture = cxcore_System_Memory_Alloc(16u);
        cx_arena_leave(&scope);
        callback.__function = native_arena_registration_callback;
        callback.__environment = foreign_capture;
        callback.__target = NULL;
        callback.__invocation_list = NULL;
        cx_arena_enter(retained_arena, &scope);
        (void)cx_interop_registration_create(&callback, sizeof(callback),
            "foreign-native-callback", NULL);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "logical-arena-native-callback-registration") == 0) {
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct cx_exception_frame frame;
        struct cxcore_System_Interop_NativeCallbackRegistration *registration;
        struct cxcore_System_Interop_NativeCallbackRegistration *failure_registration;
        cx_action callback = {0};
        cx_action stack_callback = {0};
        cx_action unregister = {0};
        cx_ptr environment;
        cx_uint stack_capture = 7u;
        cx_bool unregister_rejected = CX_FALSE;
        logical_scheduler_arena = cx_arena_create(512u, 0u, 0);
        cx_arena_retain_logical(logical_scheduler_arena);
        cx_arena_enter(logical_scheduler_arena, &scope);
        environment = cxcore_System_Memory_Alloc(32u);
        stack_callback.__function = native_arena_registration_callback;
        stack_callback.__environment = &stack_capture;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cx_interop_registration_create(&stack_callback,
                sizeof(stack_callback), "stack-native-callback", NULL);
            cx_exception_pop(&frame);
            return 57;
        } else {
            cx_exception_pop(&frame);
            cx_exception_clear();
        }
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 58;
        callback.__function = native_arena_registration_callback;
        callback.__environment = environment;
        callback.__target = NULL;
        callback.__invocation_list = NULL;
        registration = cx_interop_registration_create(
            &callback, sizeof(callback), "logical-arena-native-callback", environment);
        cx_arena_leave(&scope);
        environment = NULL;
        callback = (cx_action){0};
        cx_gc_collect();
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u ||
            !invoke_native_arena_registration(registration) ||
            !native_callback_arena_observed ||
            !native_callback_stack_borrow_observed) return 60;
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 61;
        unregister.__function = record_scheduler_callback;
        cx_arena_enter(logical_scheduler_arena, &scope);
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Interop_NativeCallbackRegistration_Unregister(
                registration, unregister);
            cx_exception_pop(&frame);
        } else {
            unregister_rejected = CX_TRUE;
            cx_exception_pop(&frame);
            cx_exception_clear();
        }
        cx_arena_leave(&scope);
        if (!unregister_rejected) return 62;
        cxcore_System_Interop_NativeCallbackRegistration_Unregister(
            registration, unregister);
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 63;
        cx_arena_enter(logical_scheduler_arena, &scope);
        callback = (cx_action){0};
        callback.__function = throw_native_arena_registration_callback;
        failure_registration = cx_interop_registration_create(
            &callback, sizeof(callback), "logical-arena-native-callback-failure", NULL);
        cx_arena_leave(&scope);
        if (!invoke_native_arena_registration(failure_registration)) return 64;
        {
            struct cxcore_System_Exception *failure =
                cxcore_System_Interop_NativeCallbackRegistration_LastException___const_get(
                    failure_registration);
            if (failure == NULL || !cx_gc_is_durable_reference((cx_ptr)failure) ||
                failure->_message == NULL ||
                failure->_message->_length != sizeof("native arena callback failure") - 1u ||
                memcmp(failure->_message->_data, "native arena callback failure",
                    sizeof("native arena callback failure") - 1u) != 0) return 65;
        }
        cxcore_System_Interop_NativeCallbackRegistration_Unregister(
            failure_registration, unregister);
        cx_gc_collect();
        {
            struct cxcore_System_Exception *failure =
                cxcore_System_Interop_NativeCallbackRegistration_LastException___const_get(
                    failure_registration);
            if (failure == NULL || !cx_gc_is_durable_reference((cx_ptr)failure) ||
                failure->_message == NULL ||
                failure->_message->_length != sizeof("native arena callback failure") - 1u ||
                memcmp(failure->_message->_data, "native arena callback failure",
                    sizeof("native arena callback failure") - 1u) != 0) return 66;
        }
        cx_arena_get_statistics(logical_scheduler_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 67;
        cx_arena_release_logical(logical_scheduler_arena);
        cx_arena_reset(logical_scheduler_arena);
        cx_arena_destroy(logical_scheduler_arena);
        logical_scheduler_arena = NULL;
        return 0;
    }
    cx_gc_register_static_roots(task_test_roots,
        (cx_uint)(sizeof(task_test_roots) / sizeof(task_test_roots[0])));
    {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_action callback = {0};
        scheduler_callback_count = 0;
        callback.__function = record_scheduler_callback;
        cx_arena_enter(arena, &scope);
        cx_arena_enter_durable(&durable_scope);
        cxcore_System_Threading_Tasks_TaskScheduler_Post(callback);
        cx_arena_leave_durable(&durable_scope);
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        if (cxcore_System_Threading_Tasks_TaskScheduler_Run() != 1u ||
            scheduler_callback_count != 1u) {
            fputs("durable override did not allow scheduler publication", stderr);
            return 1;
        }
    }
    {
        size_t bytes_before;
        cx_memory_test_allocation_tracking_set(CX_TRUE);
        bytes_before = cx_memory_test_allocation_bytes_get();
        (void)cxcore_System_Memory_Alloc(37u);
        cx_memory_test_allocation_tracking_set(CX_FALSE);
        if (cx_memory_test_allocation_bytes_get() - bytes_before != 37u) {
            fputs("managed allocation byte counter must report requested object bytes\n", stderr);
            return 1;
        }
    }
    state = create_pending_state_with_captured_target();
    cx_gc_collect();
    cxcore_System_Threading_Tasks_TaskState_SetResult(
        (struct cxcore_System_Threading_Tasks_TaskState *)state,
        NULL);
    if (cxcore_System_Threading_Tasks_TaskScheduler_Run() != 1u ||
        observed_value != 0x18c0ffeeu) {
        fputs("pending-task continuation lost its captured GC object\n", stderr);
        return 1;
    }
    state = NULL;
    {
        struct cxcore_System_Threading_Tasks_TaskState *canceled_state =
            (struct cxcore_System_Threading_Tasks_TaskState *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*canceled_state));
        cx_action continuation = {0};
        continuation.__function = observe_canceled_task;
        cxcore_System_Threading_Tasks_TaskState___constructor(
            canceled_state);
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(
            canceled_state, continuation);
        cxcore_System_Threading_Tasks_TaskState_SetCanceled(
            canceled_state);
        if (!cxcore_System_Threading_Tasks_TaskState_IsCompleted___const_get(
                canceled_state) ||
            !cxcore_System_Threading_Tasks_TaskState_IsCanceled___const_get(
                canceled_state) ||
            cxcore_System_Threading_Tasks_TaskState_IsFaulted___const_get(
                canceled_state) || canceled_continuation_fired != 0 ||
            cxcore_System_Threading_Tasks_TaskScheduler_Run() != 1u ||
            canceled_continuation_fired != 1u) {
            fputs("canceled task did not remain distinct or schedule its continuation\n", stderr);
            return 1;
        }
    }
    {
        struct cxcore_System_Threading_Tasks_TaskState *primary =
            (struct cxcore_System_Threading_Tasks_TaskState *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*primary));
        cx_action complete_secondary = {0};
        cx_action register_late = {0};
        cx_action observe_secondary = {0};
        size_t allocations_before;
        size_t first_registration_allocations;
        size_t second_registration_allocations;
        reentrant_state =
            (struct cxcore_System_Threading_Tasks_TaskState *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*reentrant_state));
        cxcore_System_Threading_Tasks_TaskState___constructor(primary);
        cxcore_System_Threading_Tasks_TaskState___constructor(reentrant_state);
        complete_secondary.__function = complete_secondary_task;
        register_late.__function = register_after_secondary_completion;
        observe_secondary.__function = record_secondary_task;
        complete_secondary.__method = register_late.__method = NULL;
        observe_secondary.__method = NULL;
        complete_secondary.__invocation_list = register_late.__invocation_list = NULL;
        observe_secondary.__invocation_list = NULL;
        cx_memory_test_allocation_tracking_set(CX_TRUE);
        allocations_before = cx_memory_test_allocation_count_get();
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(
            primary, complete_secondary);
        first_registration_allocations =
            cx_memory_test_allocation_count_get() - allocations_before;
        allocations_before = cx_memory_test_allocation_count_get();
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(
            primary, register_late);
        second_registration_allocations =
            cx_memory_test_allocation_count_get() - allocations_before;
        cx_memory_test_allocation_tracking_set(CX_FALSE);
        if (first_registration_allocations != 1u ||
            second_registration_allocations != 1u) {
            fputs("pending continuation registration must allocate one entry per subscriber\n", stderr);
            return 1;
        }
        cxcore_System_Threading_Tasks_TaskState_OnCompleted(
            reentrant_state, observe_secondary);
        cx_gc_collect();
        cxcore_System_Threading_Tasks_TaskState_SetResult(primary, NULL);
        cx_gc_collect();
        if (cxcore_System_Threading_Tasks_TaskScheduler_Run() != 4u ||
            reentrant_count != 4u || reentrant_order[0] != 1u ||
            reentrant_order[1] != 2u || reentrant_order[2] != 3u ||
            reentrant_order[3] != 4u) {
            fputs("reentrant task completion or late callback registration changed FIFO order\n", stderr);
            return 1;
        }
        reentrant_state = NULL;
    }
    {
        struct cx_exception_frame frame;
        cx_action nested = {0};
        cx_action nested_one = {0};
        cx_action followup = {0};
        cx_bool caught = CX_FALSE;
        scheduler_callback_count = 0;
        nested.__function = nested_run_callback;
        nested_one.__function = nested_run_one_callback;
        followup.__function = record_scheduler_callback;
        cxcore_System_Threading_Tasks_TaskScheduler_Post(nested);
        cxcore_System_Threading_Tasks_TaskScheduler_Post(nested_one);
        cxcore_System_Threading_Tasks_TaskScheduler_Post(followup);
        for (cx_uint rejected = 0; rejected < 2u; ++rejected) {
            caught = CX_FALSE;
            cx_exception_push(&frame);
            if (setjmp(frame.environment) == 0) {
                cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
            } else {
                caught = CX_TRUE;
            }
            cx_exception_pop(&frame);
            cx_exception_clear();
            if (!caught) {
                fputs("nested scheduler loop entry was not rejected\n", stderr);
                return 1;
            }
        }
        if (scheduler_callback_count != 2u ||
            cxcore_System_Threading_Tasks_TaskScheduler_Run() != 1u ||
            scheduler_callback_count != 3u) {
            fputs("nested scheduler entry was not rejected or dispatch state was not restored\n", stderr);
            return 1;
        }
    }
    {
        struct cxcore_System_Threading_Tasks_CancellationTokenSource *source =
            (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
        cx_action first = {0};
        cx_action second = {0};
        cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
        first.__function = dispose_during_cancellation;
        second.__function = record_during_cancellation;
        cancellation_interleave_count = 0;
        (void)cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
            source, first);
        cancellation_registration_to_dispose =
            cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
                source, second);
        cx_gc_collect();
        cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(source);
        cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(source);
        if (cancellation_interleave_count != 2u ||
            cancellation_interleave_order[0] != 1u ||
            cancellation_interleave_order[1] != 2u) {
            fputs("disposal during cancellation changed snapshot callback order or count\n", stderr);
            return 1;
        }
    }
    {
        cx_uint seed = 0x18c0ffeeu;
        for (cx_uint iteration = 0u; iteration < 32u; ++iteration) {
            struct cxcore_System_Threading_Tasks_CancellationTokenSource *source =
                (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
                cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
            struct seeded_cancellation_callback_context *contexts =
                (struct seeded_cancellation_callback_context *)
                cxcore_System_Memory_Alloc(
                    (cx_uint)(sizeof(*contexts) * 4u));
            cx_uint callback_count;
            seed = seed * 1664525u + 1013904223u;
            callback_count = 2u + (seed & 3u);
            if (callback_count > 4u) callback_count = 4u;
            cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
            seeded_cancellation_count = 0u;
            for (cx_uint index = 0u; index < callback_count; ++index) {
                cx_action callback = {0};
                contexts[index].value = index + 1u;
                contexts[index].registration_to_dispose = index == 0u
                    ? callback_count - 1u : (cx_uint)-1;
                callback.__function = seeded_cancellation_callback;
                callback.__environment = &contexts[index];
                callback.__target = (cx_ptr)&contexts[index];
                seeded_cancellation_registrations[index] =
                    cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
                        source, callback);
            }
            cx_gc_collect();
            cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(source);
            cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(source);
            if (seeded_cancellation_count != callback_count) {
                fprintf(stderr,
                    "seeded cancellation count mismatch (seed=0x%08x, iteration=%u, expected=%u, actual=%u)\n",
                    (unsigned)seed, (unsigned)iteration, (unsigned)callback_count,
                    (unsigned)seeded_cancellation_count);
                return 1;
            }
            for (cx_uint index = 0u; index < callback_count; ++index) {
                if (seeded_cancellation_order[index] != index + 1u) {
                    fprintf(stderr,
                        "seeded cancellation order mismatch (seed=0x%08x, iteration=%u, index=%u)\n",
                        (unsigned)seed, (unsigned)iteration, (unsigned)index);
                    return 1;
                }
            }
        }
    }
    {
        struct cx_exception_frame frame;
        cx_action failing = {0};
        cx_action followup = {0};
        struct cxcore_System_Exception *exception =
            (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*exception));
        cx_bool caught = CX_FALSE;
        cxcore_System_Exception___constructor(exception, NULL);
        scheduler_callback_exception = (cx_ptr)exception;
        scheduler_callback_count = 0;
        failing.__function = throw_scheduler_callback;
        followup.__function = record_scheduler_callback;
        cxcore_System_Threading_Tasks_TaskScheduler_Post(failing);
        cxcore_System_Threading_Tasks_TaskScheduler_Post(followup);
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
        } else {
            caught = cx_exception_current() == scheduler_callback_exception;
        }
        cx_exception_pop(&frame);
        cx_exception_clear();
        scheduler_callback_exception = NULL;
        if (!caught ||
            !cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            scheduler_callback_count != 1u ||
            cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) {
            fputs("faulting callback did not preserve FIFO follow-up dispatch\n", stderr);
            return 1;
        }
    }
    {
        struct cx_exception_frame frame;
        cx_action action = {0};
        struct cxcore_System_Exception *exception =
            (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*exception));
        cx_bool caught = CX_FALSE;
        cxcore_System_Exception___constructor(exception, NULL);
        scheduler_callback_exception = (cx_ptr)exception;
        scheduler_completion_count = 0u;
        action.__function = throw_scheduler_callback;
        cx_task_scheduler_post_with_completion(
            action, record_scheduler_completion, NULL);
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
        } else {
            caught = cx_exception_current() == scheduler_callback_exception;
        }
        cx_exception_pop(&frame);
        cx_exception_clear();
        scheduler_callback_exception = NULL;
        if (!caught || scheduler_completion_count != 2u ||
            scheduler_completion_order[0] != 1u ||
            scheduler_completion_order[1] != 2u) {
            fputs("scheduler completion hook did not run after a faulting callback\n", stderr);
            return 1;
        }

        scheduler_completion_count = 0u;
        action.__function = record_before_scheduler_completion;
        cx_task_scheduler_post_with_completion(
            action, record_scheduler_completion, NULL);
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            scheduler_completion_count != 2u ||
            scheduler_completion_order[0] != 1u ||
            scheduler_completion_order[1] != 2u) {
            fputs("scheduler completion hook did not follow successful callback dispatch\n", stderr);
            return 1;
        }
    }
    {
        struct cxcore_System_Threading_Tasks_TaskState pending;
        struct cxcore_System_Threading_Tasks_TaskState completed;
        cx_action action = {0};
        action.__function = record_before_scheduler_completion;
        scheduler_completion_count = 0u;
        cxcore_System_Threading_Tasks_TaskState___constructor(
            &pending);
        cx_task_state_on_completed_with_acknowledgement(
            &pending, action, record_scheduler_completion, NULL);
        if (scheduler_completion_count != 0u) {
            fputs("pending task callback ran before task completion\n", stderr);
            return 1;
        }
        cxcore_System_Threading_Tasks_TaskState_SetResult(
            &pending, NULL);
        if (scheduler_completion_count != 0u ||
            !cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            scheduler_completion_count != 2u ||
            scheduler_completion_order[0] != 1u ||
            scheduler_completion_order[1] != 2u) {
            fputs("pending TaskState callback acknowledgment was not carried through scheduling\n", stderr);
            return 1;
        }

        cxcore_System_Threading_Tasks_TaskState___constructor(
            &completed);
        cxcore_System_Threading_Tasks_TaskState_SetResult(
            &completed, NULL);
        scheduler_completion_count = 0u;
        cx_task_state_on_completed_with_acknowledgement(
            &completed, action, record_scheduler_completion, NULL);
        if (scheduler_completion_count != 0u ||
            !cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            scheduler_completion_count != 2u ||
            scheduler_completion_order[0] != 1u ||
            scheduler_completion_order[1] != 2u) {
            fputs("completed TaskState callback acknowledgment was not scheduled\n", stderr);
            return 1;
        }
    }
    {
        struct cx_exception_frame frame;
        cx_action worker_action = {0};
        struct cxcore_System_Threading_Thread worker;
        cx_bool caught = CX_FALSE;
        worker_action.__function = cross_thread_scheduler_post;
        worker = cxcore_System_Threading_Thread_Start(worker_action);
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Threading_Thread_Join(&worker);
        } else {
            caught = CX_TRUE;
        }
        cx_exception_pop(&frame);
        cx_exception_clear();
        if (!caught ||
            cxcore_System_Threading_Tasks_TaskScheduler_Run() != 0u) {
            fputs("scheduler access from a non-owner thread was not rejected\n", stderr);
            return 1;
        }
    }
    {
        struct cx_exception_frame frame;
        cx_ptr cached_item = NULL;
        cx_action action = {0};
        cx_bool duplicate_rejected = CX_FALSE;
        cx_uint index;
        action.__function = reusable_scheduler_callback;
        reusable_scheduler_cache_slot = &cached_item;
        reusable_scheduler_callback_count = 0u;
        reusable_scheduler_ack_count = 0u;
        reusable_scheduler_dispatch_rejected = CX_FALSE;
        cx_task_scheduler_test_post_allocation_count_reset();
        cx_task_scheduler_post_reusable_with_completion(
            action, reusable_scheduler_acknowledged, NULL, &cached_item);
        if (cached_item == NULL ||
            cx_task_scheduler_test_post_allocation_count_get() != 1u) {
            fputs("reusable scheduler item was not allocated and cached once\n", stderr);
            return 1;
        }
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cx_task_scheduler_post_reusable_with_completion(
                action, reusable_scheduler_acknowledged, NULL, &cached_item);
            cx_exception_pop(&frame);
        } else {
            duplicate_rejected = CX_TRUE;
            cx_exception_pop(&frame);
            cx_exception_clear();
        }
        cx_gc_collect();
        if (!duplicate_rejected ||
            !cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            reusable_scheduler_callback_count != 1u ||
            reusable_scheduler_ack_count != 1u ||
            !reusable_scheduler_dispatch_rejected) {
            fputs("reusable scheduler item accepted a queued or dispatching duplicate\n", stderr);
            return 1;
        }
        for (index = 0; index < 32u; ++index) {
            cx_task_scheduler_post_reusable_with_completion(
                action, reusable_scheduler_acknowledged, NULL, &cached_item);
            if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) {
                fputs("reusable scheduler item did not dispatch after reuse\n", stderr);
                return 1;
            }
        }
        if (reusable_scheduler_callback_count != 33u ||
            reusable_scheduler_ack_count != 33u ||
            cx_task_scheduler_test_post_allocation_count_get() != 1u) {
            fputs("reusable scheduler item allocated again or skipped its completion hook\n", stderr);
            return 1;
        }
        reusable_scheduler_cache_slot = NULL;
    }
    {
        cx_action timer = {0};
        timer.__function = observe_timer;
        cxcore_System_Threading_Tasks_TaskScheduler_PostAfter(
            2u, timer);
        if (timer_fired != 0 ||
            !cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            timer_fired != 1u ||
            cxcore_System_Threading_Tasks_TaskScheduler_RunOne()) {
            fputs("task scheduler timer did not fire exactly once\n", stderr);
            return 1;
        }
    }
    {
        cx_action timer = create_timer_with_captured_target();
        observed_value = 0;
        cxcore_System_Threading_Tasks_TaskScheduler_PostAfter(
            2u, timer);
        timer.__environment = NULL;
        timer.__target = NULL;
        cx_gc_collect();
        if (!cxcore_System_Threading_Tasks_TaskScheduler_RunOne() ||
            observed_value != 0x18c0ffeeu) {
            fputs("scheduled timer lost its captured GC object\n", stderr);
            return 1;
        }
    }
    // Closing the async owner must defer destruction until every captured
    // callback lease has been released and the final active scope has left.
    {
        struct cx_arena *closing_arena = cx_arena_create(0u, 0u, 0);
        struct cx_arena_scope closing_scope;
        struct cx_arena_statistics statistics;
        struct cx_arena *callback_lease;
        cx_arena_retain_logical(closing_arena); // async scope owner
        cx_arena_retain_logical(closing_arena); // outstanding child operation
        cx_arena_enter(closing_arena, &closing_scope);
        callback_lease = cx_arena_capture_logical_context();
        if (callback_lease != closing_arena) return 72;
        cx_arena_close_logical_owner(closing_arena);
        if (!cx_arena_current_is(closing_arena)) return 73;
        cx_arena_get_statistics(closing_arena, &statistics);
        if (statistics.logical_reference_count != 2u) return 74;
        cx_arena_leave(&closing_scope);
        cx_arena_get_statistics(closing_arena, &statistics);
        if (statistics.active_depth != 0u ||
            statistics.logical_reference_count != 2u) return 75;
        cx_arena_release_logical(callback_lease);
        cx_arena_get_statistics(closing_arena, &statistics);
        if (statistics.logical_reference_count != 1u) return 76;
        cx_arena_release_logical(closing_arena);
        // The last release destroys the closed arena; do not use its pointer.
    }
    {
        struct cx_arena *closing_arena = cx_arena_create(0u, 0u, 0);
        struct cx_arena_scope closing_scope;
        struct cx_arena_statistics statistics;
        cx_arena_retain_logical(closing_arena); // async scope owner
        cx_arena_retain_logical(closing_arena); // active continuation
        cx_arena_enter(closing_arena, &closing_scope);
        cx_arena_close_logical_owner(closing_arena);
        cx_arena_release_logical(closing_arena);
        cx_arena_get_statistics(closing_arena, &statistics);
        if (statistics.logical_reference_count != 0u ||
            statistics.active_depth != 1u) return 77;
        cx_arena_leave(&closing_scope);
        // Leaving the final active scope destroys the closed arena.
    }
    cx_gc_collect();
    return 0;
}
