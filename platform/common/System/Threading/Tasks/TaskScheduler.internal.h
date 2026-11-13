#ifndef __CX_TASK_SCHEDULER_INTERNAL_H__
#define __CX_TASK_SCHEDULER_INTERNAL_H__

#include <stdint.h>
#include "cxthreading.h"

struct cx_arena;

/* Completion hooks run once after dispatch, including when the action throws.
   They must not raise CX exceptions. */
CX_API void cx_task_scheduler_post_with_completion(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment);
CX_API void cx_task_scheduler_post_reusable_with_completion(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    cx_ptr *cached_item);
// Transfers a lease captured when a reusable-source callback was registered.
CX_API void cx_task_scheduler_post_reusable_with_logical_context(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    cx_ptr *cached_item,
    struct cx_arena *logical_arena);
// Transfers the caller's logical retain to the queued item. The queue releases
// it after dispatch, cancellation, or scheduler shutdown.
CX_API void cx_task_scheduler_post_with_logical_context(
    cx_action continuation,
    void (*completion)(void *environment),
    void *environment,
    struct cx_arena *logical_arena);
CX_API cx_bool cx_task_scheduler_fault_is_deferred(void *owner);

#endif // __CX_TASK_SCHEDULER_INTERNAL_H__
