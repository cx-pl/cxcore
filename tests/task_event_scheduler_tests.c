#include "cxcore.h"
#include "../platform/common/cxarena.h"
// Scheduler calls inside assertions must execute in Release acceptance builds.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

struct test_event { int ready; };
static struct test_event first_event, second_event;
static unsigned first_polls, second_polls, completions, timers;
static struct cx_arena *expected_logical_arena;
static cx_bool logical_event_allocation_observed;
static cx_bool logical_event_closure_observed;

struct logical_event_context {
    unsigned value;
};

static int poll_entries(struct cx_scheduler_wait_entry *entries, size_t count) {
    int ready = 0;
    for (size_t i = 0; i < count; ++i) {
        entries[i].ready = ((struct test_event *)entries[i].context)->ready;
        ready += entries[i].ready != 0;
    }
    return ready;
}

static int first_provider(struct cx_scheduler_wait_entry *entries, size_t count, uint32_t timeout) {
    (void)timeout;
    ++first_polls;
    return poll_entries(entries, count);
}

static int second_provider(struct cx_scheduler_wait_entry *entries, size_t count, uint32_t timeout) {
    (void)timeout;
    ++second_polls;
    return poll_entries(entries, count);
}

static int failed_provider(struct cx_scheduler_wait_entry *entries, size_t count, uint32_t timeout) {
    (void)entries;
    (void)count;
    (void)timeout;
    return -1;
}

static void completed(void *context) {
    (void)context;
    ++completions;
}

static void completed_in_logical_arena(void *context) {
    cx_ptr allocation;
    (void)context;
    allocation = cxcore_System_Memory_Alloc(16u);
    logical_event_allocation_observed =
        allocation != NULL && cx_arena_owns(expected_logical_arena, allocation);
}

static void completed_with_logical_event_closure(void *environment) {
    const struct logical_event_context *context =
        (const struct logical_event_context *)environment;
    logical_event_closure_observed = context != NULL &&
        context->value == 0x22c0ffeeu &&
        cx_arena_current_owns(context);
}

static void timer_ready(void *context) {
    (void)context;
    second_event.ready = 1;
    ++timers;
}

int main(void) {
    cx_action completion = {0}, timer = {0};
    uint64_t first_id;
    cx_ptr cached = NULL;
    completion.__function = completed;
    timer.__function = timer_ready;
    first_id = cx_task_scheduler_post_on_event(&first_event, first_provider,
        completion, (cx_action){0});
    cx_task_scheduler_post_on_event(&second_event, second_provider,
        completion, (cx_action){0});
    cxcore_System_Threading_Tasks_TaskScheduler_PostAfter(5u, timer);
    assert(cxcore_System_Threading_Tasks_TaskScheduler_RunOne());
    assert(timers == 1 && completions == 0);
    assert(cxcore_System_Threading_Tasks_TaskScheduler_RunOne());
    assert(completions == 1 && first_polls && second_polls);
    assert(cx_task_scheduler_cancel_event_wait(first_id));
    assert(!cx_task_scheduler_cancel_event_wait(first_id));
    assert(!cxcore_System_Threading_Tasks_TaskScheduler_RunOne());

    first_event.ready = 1;
    cx_task_scheduler_post_on_event_reusable(&first_event, first_provider,
        completion, (cx_action){0}, &cached);
    {
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cx_task_scheduler_post_on_event_reusable(&second_event, second_provider,
                completion, (cx_action){0}, &cached);
            assert(0 && "a pending reusable event must reject a second registration");
        }
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    assert(cxcore_System_Threading_Tasks_TaskScheduler_RunOne());
    assert(completions == 2);
    {
        cx_ptr original = cached;
        cx_task_scheduler_post_on_event_reusable(&first_event, first_provider,
            completion, (cx_action){0}, &cached);
        assert(cached == original);
        assert(cx_task_scheduler_cancel_reusable_event_wait(cached));
        assert(!cx_task_scheduler_cancel_reusable_event_wait(cached));
        assert(!cxcore_System_Threading_Tasks_TaskScheduler_RunOne());
    }
    {
        struct cx_exception_frame frame;
        uint64_t failed_id = cx_task_scheduler_post_on_event(&first_event,
            failed_provider, completion, (cx_action){0});
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Threading_Tasks_TaskScheduler_RunOne();
            assert(0 && "a failed event provider must raise a scheduler error");
        }
        cx_exception_pop(&frame);
        cx_exception_clear();
        assert(cx_task_scheduler_cancel_event_wait(failed_id));
        cx_gc_collect();
    }
    {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        cx_action logical_continuation = {0};
        cx_action captured_continuation = {0};
        struct logical_event_context *captured_context;
        cx_ptr logical_cached = NULL;
        cx_arena_retain_logical(arena);
        expected_logical_arena = arena;
        logical_event_allocation_observed = CX_FALSE;
        logical_continuation.__function = completed_in_logical_arena;
        first_event.ready = 1;
        cx_arena_enter(arena, &scope);
        cx_task_scheduler_post_on_event(&first_event, first_provider,
            logical_continuation, (cx_action){0});
        cx_arena_leave(&scope);
        assert(cxcore_System_Threading_Tasks_TaskScheduler_RunOne());
        assert(logical_event_allocation_observed);
        cx_arena_get_statistics(arena, &statistics);
        assert(statistics.logical_reference_count == 1u);

        logical_event_closure_observed = CX_FALSE;
        first_event.ready = 1;
        cx_arena_enter(arena, &scope);
        captured_context = (struct logical_event_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_context->value = 0x22c0ffeeu;
        captured_continuation.__function =
            completed_with_logical_event_closure;
        captured_continuation.__environment = captured_context;
        captured_continuation.__target = (cx_ptr)captured_context;
        cx_task_scheduler_post_on_event(&first_event, first_provider,
            captured_continuation, (cx_action){0});
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_continuation.__environment = NULL;
        captured_continuation.__target = NULL;
        cx_gc_collect();
        assert(cxcore_System_Threading_Tasks_TaskScheduler_RunOne());
        assert(logical_event_closure_observed);
        cx_arena_get_statistics(arena, &statistics);
        assert(statistics.logical_reference_count == 1u);

        first_event.ready = 0;
        cx_arena_enter(arena, &scope);
        first_id = cx_task_scheduler_post_on_event(&first_event, first_provider,
            completion, (cx_action){0});
        cx_arena_leave(&scope);
        cx_arena_get_statistics(arena, &statistics);
        assert(statistics.logical_reference_count == 2u);
        assert(cx_task_scheduler_cancel_event_wait(first_id));
        cx_arena_get_statistics(arena, &statistics);
        assert(statistics.logical_reference_count == 1u);

        cx_arena_enter(arena, &scope);
        captured_context = (struct logical_event_context *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*captured_context));
        captured_context->value = 0x22c0ffeeu;
        captured_continuation.__function =
            completed_with_logical_event_closure;
        captured_continuation.__environment = captured_context;
        captured_continuation.__target = (cx_ptr)captured_context;
        cx_task_scheduler_post_on_event_reusable(&first_event, first_provider,
            captured_continuation, (cx_action){0}, &logical_cached);
        cx_arena_leave(&scope);
        captured_context = NULL;
        captured_continuation.__environment = NULL;
        captured_continuation.__target = NULL;
        cx_arena_get_statistics(arena, &statistics);
        assert(statistics.logical_reference_count == 2u);
        cx_gc_collect();
        assert(cx_task_scheduler_cancel_reusable_event_wait(logical_cached));
        cx_arena_get_statistics(arena, &statistics);
        assert(statistics.logical_reference_count == 1u);
        cx_arena_release_logical(arena);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        expected_logical_arena = NULL;
    }
    return 0;
}
