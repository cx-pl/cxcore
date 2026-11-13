#ifndef __CX_GC_INTERNAL_H__
#define __CX_GC_INTERNAL_H__

#include "cx.h"

struct cx_arena;

// Internal runtime registries share the heap mutex. Callers must not allocate
// or call other managed entry points while holding it.
void cx_gc_runtime_lock(void);
void cx_gc_runtime_unlock(void);
// Validates and writes a managed reference while the caller already holds the
// collector mutex. The caller must be an attached thread and must unlock after
// the containing runtime update is complete.
cx_ptr cx_gc_store_reference_locked(cx_ptr *destination, cx_ptr value);
// Same locked update path for a pointer to native storage or an OS resource.
cx_ptr cx_gc_store_native_pointer_locked(cx_ptr *destination, cx_ptr value);
#ifdef CXCORE_TESTING
CX_API cx_ptr cx_gc_test_store_reference_locked(
    cx_ptr *destination, cx_ptr value);
#endif
cx_bool cx_gc_arena_reserve_locked(size_t bytes);
void cx_gc_arena_release_locked(size_t bytes);
void *cx_gc_alloc_for_owner(const void *owner, size_t size, size_t alignment);
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
// Publishes a batch of references into durable managed storage or explicitly
// registered native root regions while the destination owner holds a logical
// retain on the source arena. No stores are applied if any destination or
// source fails validation.
cx_bool cx_gc_store_references_retained_arena(
    cx_ptr *const *destinations, const cx_ptr *values, size_t count,
    struct cx_arena *retained_arena);

#ifdef CXCORE_ARENA_BENCHMARK
CX_API size_t cx_memory_benchmark_collection_count_get(void);
CX_API size_t cx_memory_benchmark_collected_bytes_get(void);
CX_API uint64_t cx_memory_benchmark_collection_pause_ns_get(void);
CX_API size_t cx_memory_benchmark_heap_bytes_get(void);
#endif

#endif // __CX_GC_INTERNAL_H__
