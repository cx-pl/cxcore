#ifndef __CX_ARENA_INTERNAL_H__
#define __CX_ARENA_INTERNAL_H__

#include <stddef.h>
#include <stdint.h>
#include "../../include/cx.h"

#if defined(CXCORE_TESTING) || defined(CXCORE_ARENA_BENCHMARK)
#define CX_ARENA_TEST_API CX_API
#else
#define CX_ARENA_TEST_API
#endif

struct cx_arena;
typedef void (*cx_arena_callback)(void *context);

struct cx_arena_scope {
    struct cx_arena_scope *previous;
    struct cx_arena *entered;
    uint64_t generation;
    uint64_t owner_thread_id;
    uintptr_t stack_boundary;
    int stack_grows_down;
    int stack_borrows_allowed;
};

struct cx_arena_durable_scope {
    struct cx_arena_durable_scope *previous;
    uint64_t owner_thread_id;
};

struct cx_arena_statistics {
    size_t allocation_count;
    size_t requested_bytes;
    size_t used_bytes;
    size_t peak_used_bytes;
    size_t capacity_bytes;
    size_t chunk_count;
    size_t overflow_count;
    size_t growth_count;
    size_t reset_count;
    uint64_t generation;
    size_t active_depth;
    size_t logical_reference_count;
    int fixed_capacity;
};

// Internal only. Each entered scope is thread-affine. Explicit logical retains
// permit sequential re-entry on another thread. Scheduler work, pending task
// continuations, and retained reusable-source callbacks carry these retains; a
// public async arena scope is not yet exposed.
CX_ARENA_TEST_API struct cx_arena *cx_arena_create(size_t initial_capacity,
                                                   size_t maximum_capacity,
                                                   int fixed_capacity);
CX_ARENA_TEST_API void cx_arena_destroy(struct cx_arena *arena);
// Releases the logical owner and destroys the arena after its final retained
// callback/task lease has been released and its final active scope has left.
// The arena pointer becomes invalid immediately or after that last release.
CX_ARENA_TEST_API void cx_arena_close_logical_owner(struct cx_arena *arena);
// Keeps an arena's storage alive and GC-visible while no thread has it entered.
// Every successful retain must be paired with exactly one release.
CX_ARENA_TEST_API void cx_arena_retain_logical(struct cx_arena *arena);
CX_ARENA_TEST_API void cx_arena_release_logical(struct cx_arena *arena);
// Captures the current explicitly retained arena for deferred work. The result
// carries one logical retain that the caller must release after dispatch or
// cancellation. Returns NULL for durable or ordinary heap contexts.
CX_ARENA_TEST_API struct cx_arena *cx_arena_capture_logical_context(void);
CX_ARENA_TEST_API void cx_arena_enter(struct cx_arena *arena,
                                      struct cx_arena_scope *scope);
CX_ARENA_TEST_API int cx_arena_current_is(const struct cx_arena *arena);
// Re-enters a logically retained arena on an attached worker. If another
// thread currently owns an active scope, waits until that scope leaves.
void cx_arena_enter_logical(struct cx_arena *arena,
                            struct cx_arena_scope *scope);
void cx_arena_allow_callback_stack_borrows(
    struct cx_arena_scope *scope,
    const volatile char *callback_stack_boundary);
CX_ARENA_TEST_API void cx_arena_leave(struct cx_arena_scope *scope);
CX_ARENA_TEST_API void cx_arena_run_callback(
    struct cx_arena *arena, cx_arena_callback callback, void *context);
CX_ARENA_TEST_API void cx_arena_enter_durable(
    struct cx_arena_durable_scope *scope);
CX_ARENA_TEST_API void cx_arena_leave_durable(
    struct cx_arena_durable_scope *scope);
struct cx_arena_scope *cx_arena_scope_checkpoint(void);
struct cx_arena_durable_scope *cx_arena_durable_scope_checkpoint(void);
void cx_arena_scope_unwind_to(struct cx_arena_scope *checkpoint);
void cx_arena_durable_scope_unwind_to(
    struct cx_arena_durable_scope *checkpoint);
CX_ARENA_TEST_API void *cx_arena_allocate(struct cx_arena *arena, size_t size,
                                          size_t alignment);
CX_ARENA_TEST_API void *cx_arena_allocate_current(size_t size, size_t alignment);
void *cx_arena_allocate_for_owner(const void *owner, size_t size,
                                 size_t alignment);
CX_ARENA_TEST_API void *cx_arena_reallocate_current(void *pointer, size_t size);
CX_ARENA_TEST_API size_t cx_arena_maximum_alignment(void);
CX_ARENA_TEST_API void cx_arena_reset(struct cx_arena *arena);
CX_ARENA_TEST_API void cx_arena_get_statistics(
    const struct cx_arena *arena, struct cx_arena_statistics *statistics);
CX_ARENA_TEST_API int cx_arena_owns(const struct cx_arena *arena,
                                   const void *pointer);
CX_ARENA_TEST_API int cx_arena_current_owns(const void *pointer);
CX_ARENA_TEST_API int cx_arena_async_use_allowed(void);
// Scheduler work may capture only an explicitly retained current arena.
int cx_arena_scheduler_use_allowed(void);
CX_ARENA_TEST_API int cx_arena_test_reference_store_allowed(
    const void *destination, const void *source,
    int destination_is_current_stack);
// The caller must hold the shared GC/runtime mutex while scanning. Active and
// logically retained arenas are roots; inactive unretained arenas are omitted.
void cx_arena_scan_active(void (*scan)(const void *address, size_t size,
                                       void *context),
                          void *context);
// The GC/runtime mutex must be held for these queries.
struct cx_arena *cx_arena_find_owner_locked(const void *pointer);
int cx_arena_has_allocations_locked(void);
int cx_arena_has_storage_locked(void);
int cx_arena_contains_storage_locked(const void *pointer);
int cx_arena_find_store_range_owner_locked(
    const void *address, size_t size, struct cx_arena **owner);
int cx_arena_reference_store_allowed_locked(const void *destination,
                                            size_t destination_size,
                                            const void *source,
                                            int destination_is_current_stack);
int cx_arena_retained_reference_source_allowed_locked(
    const struct cx_arena *retained_arena, const void *source);
int cx_arena_memory_copy_allowed_locked(const void *destination,
                                       const void *source,
                                       size_t size);

#endif
