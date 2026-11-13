#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../platform.h"
#include "../../src/cxcore.h"
#include "cxarena.h"
#include "cxgc.h"
#include "../../include/cxruntime.h"

#define CX_ARENA_DEFAULT_CHUNK ((size_t)64 * 1024)

CX_STRING_DEF(cx_arena_capacity_exceeded_message,
              "CX arena capacity exceeded");

#if defined(_MSC_VER)
#define CX_ARENA_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define CX_ARENA_NOINLINE __attribute__((noinline))
#else
#define CX_ARENA_NOINLINE
#endif

struct cx_arena_allocation;

struct cx_arena_chunk {
    unsigned char *memory;
    size_t capacity;
    size_t used;
    struct cx_arena_chunk *next;
};

struct cx_arena_allocation {
    const unsigned char *address;
    size_t size;
    size_t alignment;
    uint64_t generation;
    struct cx_arena_allocation *next;
};

struct cx_arena_allocation_alignment_probe {
    char prefix;
    struct cx_arena_allocation allocation;
};

struct cx_arena_max_alignment_probe {
    char prefix;
    union {
        long double floating;
        uint64_t integer;
        void *pointer;
    } value;
};

struct cx_arena {
    struct cx_arena_chunk *first;
    struct cx_arena_chunk *last;
    struct cx_arena_chunk *cursor;
    size_t maximum_capacity;
    size_t capacity_bytes;
    size_t allocation_count;
    size_t requested_bytes;
    size_t used_bytes;
    size_t peak_used_bytes;
    size_t chunk_count;
    size_t overflow_count;
    size_t growth_count;
    size_t reset_count;
    uint64_t generation;
    struct cx_arena_allocation *allocations;
    uint64_t owner_thread_id;
    size_t active_depth;
    int fixed_capacity;
    int close_when_unretained;
    struct cx_arena *registry_next;
    size_t logical_reference_count;
};

static CX_THREAD_LOCAL struct cx_arena_scope *cx_arena_scope_current;
static CX_THREAD_LOCAL struct cx_arena_durable_scope *cx_arena_durable_scope_current;
static CX_THREAD_LOCAL struct cx_arena *cx_arena_current;
static struct cx_arena *cx_arena_registry;
static int cx_arena_is_active_in_current_context_locked(
    const struct cx_arena *arena);

static CX_ARENA_NOINLINE int cx_arena_stack_grows_down(
    const volatile char *outer_marker) {
    volatile char inner_marker = 0;
    return (uintptr_t)&inner_marker < (uintptr_t)outer_marker;
}

static void cx_arena_out_of_memory(void) {
    fputs("Out of memory\n", stderr);
    exit(EXIT_FAILURE);
}

static CX_NORETURN void cx_arena_raise_capacity_exceeded(void) {
    cx_exception_raise_message(&cx_arena_capacity_exceeded_message,
                               __FILE__, __LINE__);
}

static int cx_arena_is_power_of_two(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

size_t cx_arena_maximum_alignment(void) {
    return offsetof(struct cx_arena_max_alignment_probe, value);
}

static int cx_arena_add_overflows(size_t left, size_t right) {
    return right > SIZE_MAX - left;
}

static int cx_arena_chunk_contains_address(
    const struct cx_arena_chunk *chunk, uintptr_t address) {
    uintptr_t begin = (uintptr_t)chunk->memory;
    return address >= begin && address - begin < chunk->capacity;
}

static struct cx_arena_allocation *cx_arena_find_allocation_locked(
    const struct cx_arena *arena, uintptr_t address) {
    struct cx_arena_allocation *allocation;
    for (allocation = arena->allocations; allocation != NULL;
         allocation = allocation->next) {
        uintptr_t allocation_begin = (uintptr_t)allocation->address;
        if (allocation->generation == arena->generation &&
            address >= allocation_begin &&
            address - allocation_begin < allocation->size) {
            return allocation;
        }
    }
    return NULL;
}

static struct cx_arena_chunk *cx_arena_new_chunk(size_t capacity) {
    struct cx_arena_chunk *chunk;
    if (capacity == 0) return NULL;
    chunk = (struct cx_arena_chunk *)malloc(sizeof(*chunk));
    if (chunk == NULL) cx_arena_out_of_memory();
    chunk->memory = (unsigned char *)malloc(capacity);
    if (chunk->memory == NULL) {
        free(chunk);
        cx_arena_out_of_memory();
    }
    chunk->capacity = capacity;
    chunk->used = 0;
    chunk->next = NULL;
    return chunk;
}

static void cx_arena_link_chunk(struct cx_arena *arena,
                                struct cx_arena_chunk *chunk) {
    if (chunk == NULL || arena->chunk_count == SIZE_MAX ||
        cx_arena_add_overflows(arena->capacity_bytes, chunk->capacity) ||
        (arena->chunk_count != 0 && arena->growth_count == SIZE_MAX)) {
        cx_arena_out_of_memory();
    }
    if (arena->last == NULL) arena->first = chunk;
    else arena->last->next = chunk;
    arena->last = chunk;
    arena->cursor = chunk;
    arena->capacity_bytes += chunk->capacity;
    ++arena->chunk_count;
    if (arena->chunk_count > 1) ++arena->growth_count;
}

struct cx_arena *cx_arena_create(size_t initial_capacity,
                                 size_t maximum_capacity,
                                 int fixed_capacity) {
    struct cx_arena *arena;
    if (fixed_capacity && maximum_capacity == 0) {
        fputs("CX arena fixed capacity requires a nonzero maximum\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (maximum_capacity != 0 && initial_capacity > maximum_capacity) {
        fputs("CX arena initial capacity exceeds its maximum\n", stderr);
        exit(EXIT_FAILURE);
    }
    arena = (struct cx_arena *)calloc(1, sizeof(*arena));
    if (arena == NULL) cx_arena_out_of_memory();
    arena->maximum_capacity = maximum_capacity;
    arena->fixed_capacity = fixed_capacity != 0;
    arena->generation = 1;

    if (arena->fixed_capacity) {
        cx_gc_runtime_lock();
        if (!cx_gc_arena_reserve_locked(maximum_capacity)) {
            cx_gc_runtime_unlock();
            free(arena);
            cx_arena_out_of_memory();
        }
        cx_gc_runtime_unlock();
        struct cx_arena_chunk *chunk = cx_arena_new_chunk(maximum_capacity);
        // Fixed storage is committed and touched up front so allocation cannot
        // lazily fault in pages after the arena starts being used.
        memset(chunk->memory, 0, chunk->capacity);
        cx_arena_link_chunk(arena, chunk);
    } else if (initial_capacity != 0) {
        cx_gc_runtime_lock();
        if (!cx_gc_arena_reserve_locked(initial_capacity)) {
            cx_gc_runtime_unlock();
            free(arena);
            cx_arena_out_of_memory();
        }
        cx_gc_runtime_unlock();
        cx_arena_link_chunk(arena, cx_arena_new_chunk(initial_capacity));
    }
    cx_gc_runtime_lock();
    arena->registry_next = cx_arena_registry;
    cx_arena_registry = arena;
    cx_gc_runtime_unlock();
    return arena;
}

static void cx_arena_destroy_locked(struct cx_arena *arena) {
    struct cx_arena_chunk *chunk;
    struct cx_arena **link;
    if (arena->active_depth != 0 || arena->owner_thread_id != 0 ||
        arena->logical_reference_count != 0 || cx_arena_current == arena) {
        fputs("CX arena cannot be destroyed while active or logically retained\n",
              stderr);
        exit(EXIT_FAILURE);
    }
    link = &cx_arena_registry;
    while (*link != NULL && *link != arena) link = &(*link)->registry_next;
    if (*link == NULL) {
        fputs("CX arena is not registered\n", stderr);
        exit(EXIT_FAILURE);
    }
    *link = arena->registry_next;
    cx_gc_arena_release_locked(arena->capacity_bytes);
    chunk = arena->first;
    while (chunk != NULL) {
        struct cx_arena_chunk *next = chunk->next;
        free(chunk->memory);
        free(chunk);
        chunk = next;
    }
    free(arena);
}

void cx_arena_destroy(struct cx_arena *arena) {
    if (arena == NULL) return;
    cx_gc_runtime_lock();
    if (arena->active_depth != 0 || arena->owner_thread_id != 0 ||
        arena->logical_reference_count != 0) {
        cx_gc_runtime_unlock();
        fputs("CX arena cannot be destroyed while active or logically retained\n",
              stderr);
        exit(EXIT_FAILURE);
    }
    if (arena->close_when_unretained) {
        cx_gc_runtime_unlock();
        fputs("CX arena logical owner has already closed\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_arena_destroy_locked(arena);
    cx_gc_runtime_unlock();
}

void cx_arena_close_logical_owner(struct cx_arena *arena) {
    if (arena == NULL) {
        fputs("CX arena logical close requires an arena\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_runtime_lock();
    if (arena->close_when_unretained ||
        arena->logical_reference_count == 0) {
        cx_gc_runtime_unlock();
        fputs("CX arena logical owner has no live retain\n", stderr);
        exit(EXIT_FAILURE);
    }
    arena->close_when_unretained = 1;
    --arena->logical_reference_count;
    if (arena->logical_reference_count == 0 && arena->active_depth == 0) {
        cx_arena_destroy_locked(arena);
    }
    cx_gc_runtime_wake();
    cx_gc_runtime_unlock();
}

void cx_arena_retain_logical(struct cx_arena *arena) {
    if (arena == NULL) {
        fputs("CX arena logical retain requires an arena\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_runtime_lock();
    if (arena->logical_reference_count == SIZE_MAX) {
        cx_gc_runtime_unlock();
        fputs("CX arena logical retain count exhausted\n", stderr);
        exit(EXIT_FAILURE);
    }
    ++arena->logical_reference_count;
    cx_gc_runtime_unlock();
}

void cx_arena_release_logical(struct cx_arena *arena) {
    if (arena == NULL) {
        fputs("CX arena logical release requires an arena\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_runtime_lock();
    if (arena->logical_reference_count == 0) {
        cx_gc_runtime_unlock();
        fputs("CX arena logical release has no matching retain\n", stderr);
        exit(EXIT_FAILURE);
    }
    --arena->logical_reference_count;
    if (arena->close_when_unretained && arena->logical_reference_count == 0 &&
        arena->active_depth == 0) {
        cx_arena_destroy_locked(arena);
    }
    cx_gc_runtime_unlock();
}

struct cx_arena *cx_arena_capture_logical_context(void) {
    struct cx_arena *arena = cx_arena_current;
    if (arena == NULL || cx_arena_durable_scope_current != NULL) return NULL;
    cx_gc_runtime_lock();
    if (arena->logical_reference_count == 0) {
        cx_gc_runtime_unlock();
        return NULL;
    }
    if (arena->logical_reference_count == SIZE_MAX) {
        cx_gc_runtime_unlock();
        fputs("CX arena logical retain count exhausted\n", stderr);
        exit(EXIT_FAILURE);
    }
    ++arena->logical_reference_count;
    cx_gc_runtime_unlock();
    return arena;
}

void cx_arena_enter(struct cx_arena *arena, struct cx_arena_scope *scope) {
    uint64_t owner;
    volatile char stack_marker = 0;
    if (arena == NULL || scope == NULL) {
        fputs("CX arena scope requires an arena\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_check_thread();
    cx_gc_runtime_lock();
    owner = cx_gc_runtime_thread_id();
    if (owner == 0 || (arena->owner_thread_id != 0 &&
                       arena->owner_thread_id != owner)) {
        cx_gc_runtime_unlock();
        fputs("CX arena is active on another thread\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (arena->active_depth == SIZE_MAX) {
        cx_gc_runtime_unlock();
        fputs("CX arena nesting limit exceeded\n", stderr);
        exit(EXIT_FAILURE);
    }
    arena->owner_thread_id = owner;
    ++arena->active_depth;
    scope->previous = cx_arena_scope_current;
    scope->entered = arena;
    scope->generation = arena->generation;
    scope->owner_thread_id = owner;
    scope->stack_boundary = (uintptr_t)&stack_marker;
    scope->stack_grows_down = cx_arena_stack_grows_down(&stack_marker);
    scope->stack_borrows_allowed = 0;
    cx_arena_scope_current = scope;
    cx_arena_current = arena;
    cx_gc_runtime_unlock();
}

void cx_arena_allow_callback_stack_borrows(
    struct cx_arena_scope *scope,
    const volatile char *callback_stack_boundary) {
    if (scope == NULL || callback_stack_boundary == NULL ||
        cx_arena_scope_current != scope || scope->entered == NULL ||
        cx_arena_current != scope->entered) {
        fputs("CX callback stack boundary requires the current arena scope\n",
              stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_runtime_lock();
    scope->stack_boundary = (uintptr_t)callback_stack_boundary;
    scope->stack_grows_down =
        cx_arena_stack_grows_down(callback_stack_boundary);
    scope->stack_borrows_allowed = 1;
    cx_gc_runtime_unlock();
}

int cx_arena_current_is(const struct cx_arena *arena) {
    return arena != NULL && cx_arena_current == arena;
}

void cx_arena_enter_logical(struct cx_arena *arena,
                            struct cx_arena_scope *scope) {
    uint64_t owner;
    volatile char stack_marker = 0;
    if (arena == NULL || scope == NULL) {
        fputs("CX logical arena scope requires an arena\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_check_thread();
    cx_gc_runtime_lock();
    owner = cx_gc_runtime_thread_id();
    if (owner == 0) {
        cx_gc_runtime_unlock();
        fputs("CX logical arena scope requires an attached thread\n", stderr);
        exit(EXIT_FAILURE);
    }
    while (arena->active_depth != 0 && arena->owner_thread_id != owner) {
        if (arena->logical_reference_count == 0) {
            cx_gc_runtime_unlock();
            fputs("CX logical arena context has no live owner\n", stderr);
            exit(EXIT_FAILURE);
        }
        cx_gc_runtime_wait();
    }
    if (arena->logical_reference_count == 0 ||
        arena->active_depth == SIZE_MAX) {
        cx_gc_runtime_unlock();
        fputs("CX logical arena context has no live owner\n", stderr);
        exit(EXIT_FAILURE);
    }
    arena->owner_thread_id = owner;
    ++arena->active_depth;
    scope->previous = cx_arena_scope_current;
    scope->entered = arena;
    scope->generation = arena->generation;
    scope->owner_thread_id = owner;
    scope->stack_boundary = (uintptr_t)&stack_marker;
    scope->stack_grows_down = cx_arena_stack_grows_down(&stack_marker);
    scope->stack_borrows_allowed = 0;
    cx_arena_scope_current = scope;
    cx_arena_current = arena;
    cx_gc_runtime_unlock();
}

void cx_arena_leave(struct cx_arena_scope *scope) {
    struct cx_arena *arena = scope == NULL ? NULL : scope->entered;
    cx_gc_runtime_lock();
    if (arena == NULL || cx_arena_scope_current != scope ||
        cx_arena_current != arena || arena->generation != scope->generation ||
        arena->active_depth == 0 ||
        arena->owner_thread_id != scope->owner_thread_id ||
        cx_gc_runtime_thread_id() != scope->owner_thread_id) {
        cx_gc_runtime_unlock();
        fputs("CX arena scopes must leave in LIFO order on their owner thread\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_arena_scope_current = scope->previous;
    cx_arena_current = scope->previous == NULL ? NULL : scope->previous->entered;
    --arena->active_depth;
    if (arena->active_depth == 0) arena->owner_thread_id = 0;
    scope->previous = NULL;
    scope->entered = NULL;
    if (arena->close_when_unretained &&
        arena->logical_reference_count == 0 && arena->active_depth == 0) {
        cx_gc_runtime_wake();
        cx_arena_destroy_locked(arena);
        cx_gc_runtime_unlock();
        return;
    }
    cx_gc_runtime_wake();
    cx_gc_runtime_unlock();
}

void cx_arena_run_callback(struct cx_arena *arena,
                           cx_arena_callback callback,
                           void *context) {
    struct cx_arena_scope scope;
    volatile char stack_marker = 0;
    if (callback == NULL) {
        fputs("CX arena callback cannot be null\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_arena_enter(arena, &scope);
    // The callback and all of its callees use frames below this boundary.
    // The runner frame itself and its callers remain outside the arena.
    cx_arena_allow_callback_stack_borrows(&scope, &stack_marker);
    callback(context);
    cx_arena_leave(&scope);
}

void cx_arena_enter_durable(struct cx_arena_durable_scope *scope) {
    if (scope == NULL) {
        fputs("CX durable allocation context requires a scope record\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_check_thread();
    cx_gc_runtime_lock();
    scope->previous = cx_arena_durable_scope_current;
    scope->owner_thread_id = cx_gc_runtime_thread_id();
    if (scope->owner_thread_id == 0) {
        cx_gc_runtime_unlock();
        fputs("CX durable allocation context requires an attached thread\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_arena_durable_scope_current = scope;
    cx_gc_runtime_unlock();
}

void cx_arena_leave_durable(struct cx_arena_durable_scope *scope) {
    cx_gc_runtime_lock();
    if (scope == NULL || cx_arena_durable_scope_current != scope ||
        scope->owner_thread_id == 0 ||
        cx_gc_runtime_thread_id() != scope->owner_thread_id) {
        cx_gc_runtime_unlock();
        fputs("CX durable allocation contexts must leave in LIFO order on their owner thread\n",
              stderr);
        exit(EXIT_FAILURE);
    }
    cx_arena_durable_scope_current = scope->previous;
    scope->previous = NULL;
    scope->owner_thread_id = 0;
    cx_gc_runtime_unlock();
}

struct cx_arena_scope *cx_arena_scope_checkpoint(void) {
    return cx_arena_scope_current;
}

struct cx_arena_durable_scope *cx_arena_durable_scope_checkpoint(void) {
    return cx_arena_durable_scope_current;
}

void cx_arena_scope_unwind_to(struct cx_arena_scope *checkpoint) {
    while (cx_arena_scope_current != checkpoint) {
        if (cx_arena_scope_current == NULL) {
            fputs("Invalid CX arena exception checkpoint\n", stderr);
            abort();
        }
        cx_arena_leave(cx_arena_scope_current);
    }
}

void cx_arena_durable_scope_unwind_to(
    struct cx_arena_durable_scope *checkpoint) {
    while (cx_arena_durable_scope_current != checkpoint) {
        if (cx_arena_durable_scope_current == NULL) {
            fputs("Invalid CX durable exception checkpoint\n", stderr);
            abort();
        }
        cx_arena_leave_durable(cx_arena_durable_scope_current);
    }
}

static struct cx_arena_chunk *cx_arena_add_growth_chunk(struct cx_arena *arena,
                                                         size_t minimum) {
    size_t capacity = minimum > CX_ARENA_DEFAULT_CHUNK
        ? minimum : CX_ARENA_DEFAULT_CHUNK;
    struct cx_arena_chunk *chunk;
    if (arena->fixed_capacity) return NULL;
    if (arena->maximum_capacity != 0) {
        size_t remaining;
        if (arena->capacity_bytes >= arena->maximum_capacity) return NULL;
        remaining = arena->maximum_capacity - arena->capacity_bytes;
        if (capacity > remaining) capacity = remaining;
        if (capacity < minimum) return NULL;
    }
    if (!cx_gc_arena_reserve_locked(capacity)) return NULL;
    chunk = cx_arena_new_chunk(capacity);
    cx_arena_link_chunk(arena, chunk);
    return chunk;
}

static int cx_arena_required_chunk_size(size_t allocation_size,
                                       size_t alignment,
                                       size_t *minimum) {
    size_t extent_alignment =
        offsetof(struct cx_arena_allocation_alignment_probe, allocation);
    size_t overhead = sizeof(struct cx_arena_allocation);
    if (overhead > SIZE_MAX - (extent_alignment - 1) ||
        overhead + extent_alignment - 1 > SIZE_MAX - (alignment - 1) ||
        overhead + extent_alignment - 1 + alignment - 1 >
            SIZE_MAX - allocation_size) {
        return 0;
    }
    *minimum = allocation_size + alignment - 1 + overhead + extent_alignment - 1;
    return 1;
}

static int cx_arena_try_allocate_from_chunk(struct cx_arena_chunk *chunk,
                                           size_t size,
                                           size_t alignment,
                                           void **result,
                                           struct cx_arena_allocation **extent,
                                           size_t *consumed) {
    uintptr_t base = (uintptr_t)chunk->memory;
    uintptr_t extent_address;
    uintptr_t current;
    uintptr_t aligned;
    size_t extent_offset;
    size_t current_offset;
    size_t alignment_of_extent =
        offsetof(struct cx_arena_allocation_alignment_probe, allocation);
    size_t end;
    if (chunk->used > UINTPTR_MAX - base ||
        chunk->used > SIZE_MAX - (alignment_of_extent - 1)) return 0;
    extent_offset = (chunk->used + alignment_of_extent - 1) &
        ~(alignment_of_extent - 1);
    if (extent_offset > UINTPTR_MAX - base) return 0;
    if (extent_offset > SIZE_MAX - sizeof(struct cx_arena_allocation)) return 0;
    current_offset = extent_offset + sizeof(struct cx_arena_allocation);
    if (current_offset > UINTPTR_MAX - base) return 0;
    current = base + current_offset;
    if (current > UINTPTR_MAX - (alignment - 1)) return 0;
    aligned = (current + alignment - 1) & ~(uintptr_t)(alignment - 1);
    if (aligned < base || aligned - base > SIZE_MAX) return 0;
    current_offset = (size_t)(aligned - base);
    if (cx_arena_add_overflows(current_offset, size)) return 0;
    end = current_offset + size;
    if (end > chunk->capacity) return 0;
    memset(chunk->memory + chunk->used, 0, end - chunk->used);
    extent_address = base + extent_offset;
    *extent = (struct cx_arena_allocation *)extent_address;
    *result = chunk->memory + current_offset;
    *consumed = end - chunk->used;
    chunk->used = end;
    return 1;
}

void *cx_arena_allocate(struct cx_arena *arena, size_t size, size_t alignment) {
    struct cx_arena_chunk *chunk;
    struct cx_arena_allocation *allocation = NULL;
    void *result = NULL;
    size_t consumed = 0;
    size_t allocation_size = size == 0 ? 1 : size;
    if (arena == NULL || !cx_arena_is_power_of_two(alignment)) {
        fputs("CX arena allocation requires a valid arena and power-of-two alignment\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_runtime_lock();
    if (!cx_arena_is_active_in_current_context_locked(arena) ||
        arena->owner_thread_id != cx_gc_runtime_thread_id()) {
        cx_gc_runtime_unlock();
        fputs("CX arena allocation requires its active scope\n", stderr);
        exit(EXIT_FAILURE);
    }
    chunk = arena->cursor;
    if (chunk == NULL && arena->first == NULL) {
        size_t minimum;
        if (!cx_arena_required_chunk_size(allocation_size, alignment, &minimum)) {
            ++arena->overflow_count;
            cx_gc_runtime_unlock();
            cx_arena_out_of_memory();
        }
        chunk = cx_arena_add_growth_chunk(arena, minimum);
        if (chunk == NULL) {
            ++arena->overflow_count;
            cx_gc_runtime_unlock();
            cx_arena_raise_capacity_exceeded();
        }
    }
    while (chunk != NULL) {
        if (cx_arena_try_allocate_from_chunk(chunk, allocation_size, alignment,
                                             &result, &allocation, &consumed)) break;
        if (chunk->next != NULL) chunk = chunk->next;
        else {
            size_t minimum;
            if (!cx_arena_required_chunk_size(allocation_size, alignment,
                                              &minimum)) {
                ++arena->overflow_count;
                cx_gc_runtime_unlock();
                cx_arena_out_of_memory();
            }
            chunk = cx_arena_add_growth_chunk(arena, minimum);
            if (chunk == NULL) {
                ++arena->overflow_count;
                cx_gc_runtime_unlock();
                cx_arena_raise_capacity_exceeded();
            }
        }
    }
    if (chunk == NULL || allocation == NULL || result == NULL) {
        ++arena->overflow_count;
        cx_gc_runtime_unlock();
        cx_arena_raise_capacity_exceeded();
    }
    arena->cursor = chunk;
    if (cx_arena_add_overflows(arena->used_bytes, consumed) ||
        cx_arena_add_overflows(arena->requested_bytes, size) ||
        arena->allocation_count == SIZE_MAX) {
        cx_gc_runtime_unlock();
        cx_arena_out_of_memory();
    }
    allocation->address = (const unsigned char *)result;
    allocation->size = allocation_size;
    allocation->alignment = alignment;
    allocation->generation = arena->generation;
    allocation->next = arena->allocations;
    arena->allocations = allocation;
    arena->used_bytes += consumed;
    arena->requested_bytes += size;
    ++arena->allocation_count;
    if (arena->used_bytes > arena->peak_used_bytes) {
        arena->peak_used_bytes = arena->used_bytes;
    }
    cx_gc_runtime_unlock();
    return result;
}

void *cx_arena_allocate_current(size_t size, size_t alignment) {
    struct cx_arena *arena = cx_arena_current;
    if (arena == NULL || cx_arena_durable_scope_current != NULL) return NULL;
    return cx_arena_allocate(arena, size, alignment);
}

void *cx_arena_allocate_for_owner(const void *owner, size_t size,
                                  size_t alignment) {
    struct cx_arena *arena;
    cx_gc_runtime_lock();
    arena = cx_arena_find_owner_locked(owner);
    if (arena == NULL && cx_arena_contains_storage_locked(owner)) {
        cx_gc_runtime_unlock();
        fputs("CX allocation owner does not name a live arena object\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (arena != NULL &&
        (!cx_arena_is_active_in_current_context_locked(arena) ||
         arena->owner_thread_id != cx_gc_runtime_thread_id())) {
        cx_gc_runtime_unlock();
        fputs("CX owner arena is not active in the current allocation context\n",
              stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_runtime_unlock();
    // An explicit durable context changes ambient allocations only. An
    // allocation attached to an active arena owner must remain with that owner.
    return arena == NULL ? NULL : cx_arena_allocate(arena, size, alignment);
}

int cx_arena_async_use_allowed(void) {
    return cx_arena_current == NULL || cx_arena_durable_scope_current != NULL;
}

int cx_arena_scheduler_use_allowed(void) {
    int allowed;
    if (cx_arena_current == NULL || cx_arena_durable_scope_current != NULL) {
        return 1;
    }
    cx_gc_runtime_lock();
    allowed = cx_arena_current->logical_reference_count != 0;
    cx_gc_runtime_unlock();
    return allowed;
}

void *cx_arena_reallocate_current(void *pointer, size_t size) {
    struct cx_arena_allocation *allocation;
    struct cx_arena *arena = NULL;
    size_t previous_size = 0;
    size_t alignment = 0;
    void *replacement;
    if (pointer == NULL) return cx_arena_allocate_current(size, sizeof(void *));

    cx_gc_runtime_lock();
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        for (allocation = arena->allocations; allocation != NULL;
             allocation = allocation->next) {
            if (allocation->generation == arena->generation &&
                allocation->address == (const unsigned char *)pointer &&
                allocation->size != 0) {
                previous_size = allocation->size;
                alignment = allocation->alignment;
                goto found_allocation;
            }
        }
    }
found_allocation:
    if (arena == NULL || arena != cx_arena_current ||
        arena->owner_thread_id != cx_gc_runtime_thread_id() ||
        arena->active_depth == 0) {
        cx_gc_runtime_unlock();
        fputs("Memory.Realloc requires an exact allocation in the active owning arena\n",
              stderr);
        exit(EXIT_FAILURE);
    }
    if (size == 0) {
        for (allocation = arena->allocations; allocation != NULL;
             allocation = allocation->next) {
            if (allocation->generation == arena->generation &&
                allocation->address == (const unsigned char *)pointer &&
                allocation->size != 0) {
                allocation->size = 0;
                break;
            }
        }
        cx_gc_runtime_unlock();
        return NULL;
    }
    cx_gc_runtime_unlock();

    // Arena growth is allocate-and-copy: the original extent remains reserved
    // until reset, but ceases to be a valid arena allocation after success.
    replacement = cx_arena_allocate(arena, size, alignment);
    memcpy(replacement, pointer, previous_size < size ? previous_size : size);

    cx_gc_runtime_lock();
    for (allocation = arena->allocations; allocation != NULL;
         allocation = allocation->next) {
        if (allocation->generation == arena->generation &&
            allocation->address == (const unsigned char *)pointer &&
            allocation->size == previous_size) {
            allocation->size = 0;
            break;
        }
    }
    if (allocation == NULL) {
        cx_gc_runtime_unlock();
        fputs("Memory.Realloc lost the original arena extent\n", stderr);
        exit(EXIT_FAILURE);
    }
    cx_gc_runtime_unlock();
    return replacement;
}

void cx_arena_reset(struct cx_arena *arena) {
    struct cx_arena_chunk *chunk;
    if (arena == NULL) return;
    cx_gc_runtime_lock();
    if (arena->active_depth != 0 || arena->owner_thread_id != 0 ||
        arena->logical_reference_count != 0) {
        cx_gc_runtime_unlock();
        fputs("CX arena cannot be reset while active or logically retained\n",
              stderr);
        exit(EXIT_FAILURE);
    }
    if (arena->generation == UINT64_MAX || arena->reset_count == SIZE_MAX) {
        cx_gc_runtime_unlock();
        fputs("CX arena generation exhausted\n", stderr);
        exit(EXIT_FAILURE);
    }
        for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
            chunk->used = 0;
        }
    arena->allocations = NULL;
    arena->cursor = arena->first;
    arena->allocation_count = 0;
    arena->requested_bytes = 0;
    arena->used_bytes = 0;
    ++arena->generation;
    ++arena->reset_count;
    cx_gc_runtime_unlock();
}

void cx_arena_get_statistics(const struct cx_arena *arena,
                             struct cx_arena_statistics *statistics) {
    if (arena == NULL || statistics == NULL) return;
    cx_gc_runtime_lock();
    statistics->allocation_count = arena->allocation_count;
    statistics->requested_bytes = arena->requested_bytes;
    statistics->used_bytes = arena->used_bytes;
    statistics->peak_used_bytes = arena->peak_used_bytes;
    statistics->capacity_bytes = arena->capacity_bytes;
    statistics->chunk_count = arena->chunk_count;
    statistics->overflow_count = arena->overflow_count;
    statistics->growth_count = arena->growth_count;
    statistics->reset_count = arena->reset_count;
    statistics->generation = arena->generation;
    statistics->active_depth = arena->active_depth;
    statistics->logical_reference_count = arena->logical_reference_count;
    statistics->fixed_capacity = arena->fixed_capacity;
    cx_gc_runtime_unlock();
}

int cx_arena_owns(const struct cx_arena *arena, const void *pointer) {
    uintptr_t address = (uintptr_t)pointer;
    struct cx_arena_chunk *chunk;
    int result = 0;
    if (arena == NULL || pointer == NULL) return 0;
    cx_gc_runtime_lock();
    for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
        if (cx_arena_chunk_contains_address(chunk, address)) {
            result = cx_arena_find_allocation_locked(arena, address) != NULL;
            break;
        }
    }
    cx_gc_runtime_unlock();
    return result;
}

int cx_arena_current_owns(const void *pointer) {
    return cx_arena_owns(cx_arena_current, pointer);
}

struct cx_arena *cx_arena_find_owner_locked(const void *pointer) {
    uintptr_t address = (uintptr_t)pointer;
    struct cx_arena *arena;
    if (pointer == NULL) return NULL;
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        struct cx_arena_chunk *chunk;
        for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
            if (cx_arena_chunk_contains_address(chunk, address)) {
                struct cx_arena_allocation *allocation =
                    cx_arena_find_allocation_locked(arena, address);
                return allocation != NULL &&
                    allocation->generation == arena->generation &&
                    address - (uintptr_t)allocation->address < allocation->size
                    ? arena : NULL;
            }
        }
    }
    return NULL;
}

int cx_arena_has_allocations_locked(void) {
    struct cx_arena *arena;
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        if (arena->allocations != NULL) return 1;
    }
    return 0;
}

int cx_arena_has_storage_locked(void) {
    struct cx_arena *arena;
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        if (arena->first != NULL) return 1;
    }
    return 0;
}

static struct cx_arena *cx_arena_find_storage_owner_locked(const void *pointer) {
    uintptr_t address = (uintptr_t)pointer;
    struct cx_arena *arena;
    if (pointer == NULL) return NULL;
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        struct cx_arena_chunk *chunk;
        for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
            uintptr_t begin = (uintptr_t)chunk->memory;
            if (address >= begin && address - begin < chunk->capacity) return arena;
        }
    }
    return NULL;
}

int cx_arena_contains_storage_locked(const void *pointer) {
    return cx_arena_find_storage_owner_locked(pointer) != NULL;
}

static int cx_arena_contains_range_locked(const struct cx_arena *arena,
                                          const void *address,
                                          size_t size) {
    uintptr_t begin = (uintptr_t)address;
    struct cx_arena_chunk *chunk;
    if (address == NULL || size == 0 || size > UINTPTR_MAX - begin) return 0;
    for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
        if (cx_arena_chunk_contains_address(chunk, begin)) {
            struct cx_arena_allocation *allocation =
                cx_arena_find_allocation_locked(arena, begin);
            uintptr_t allocation_begin;
            if (allocation == NULL || allocation->generation != arena->generation) {
                return 0;
            }
            allocation_begin = (uintptr_t)allocation->address;
            return begin >= allocation_begin &&
                begin - allocation_begin <= allocation->size &&
                size <= allocation->size - (size_t)(begin - allocation_begin);
        }
    }
    return 0;
}

// Return whether the complete store range is either outside arena storage or
// contained in one current-generation allocation. The range can begin before
// a chunk/allocation while still overlapping it, so checking only its first
// byte is not sufficient.
int cx_arena_find_store_range_owner_locked(
    const void *address, size_t size, struct cx_arena **owner) {
    uintptr_t begin = (uintptr_t)address;
    uintptr_t end;
    struct cx_arena *arena;
    int intersects_storage = 0;
    *owner = NULL;
    if (address == NULL || size == 0 || size > UINTPTR_MAX - begin) return 0;
    end = begin + size;
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        struct cx_arena_chunk *chunk;
        for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
            uintptr_t chunk_begin = (uintptr_t)chunk->memory;
            uintptr_t chunk_end;
            if (chunk->capacity > UINTPTR_MAX - chunk_begin) return 0;
            chunk_end = chunk_begin + chunk->capacity;
            if (begin < chunk_end && chunk_begin < end) {
                intersects_storage = 1;
                if (!cx_arena_contains_range_locked(arena, address, size) ||
                    (*owner != NULL && *owner != arena)) {
                    return 0;
                }
                *owner = arena;
            }
        }
    }
    return !intersects_storage || *owner != NULL;
}

static int cx_arena_is_active_in_current_context_locked(
    const struct cx_arena *arena) {
    struct cx_arena_scope *scope;
    for (scope = cx_arena_scope_current; scope != NULL; scope = scope->previous) {
        if (scope->entered == arena) return 1;
    }
    return 0;
}

static int cx_arena_is_ancestor_of_destination_locked(
    const struct cx_arena *source,
    const struct cx_arena *destination) {
    struct cx_arena_scope *scope;
    for (scope = cx_arena_scope_current; scope != NULL; scope = scope->previous) {
        if (scope->entered == source) return 0;
        if (scope->entered == destination) return 1;
    }
    return 0;
}

static int cx_arena_stack_slot_expires_before_scope_locked(
    const struct cx_arena *source,
    const void *destination,
    size_t destination_size) {
    struct cx_arena_scope *scope;
    struct cx_arena_scope *source_scope = NULL;
    uintptr_t begin = (uintptr_t)destination;

    // Scope records form a newest-to-oldest chain. Keep the oldest active
    // scope for this arena because its exit is the earliest point at which
    // allocations made in the arena can become invalid.
    for (scope = cx_arena_scope_current; scope != NULL; scope = scope->previous) {
        if (scope->entered == source) source_scope = scope;
    }
    if (source_scope == NULL || !source_scope->stack_borrows_allowed ||
        destination_size > UINTPTR_MAX - begin) return 0;
    if (source_scope->stack_grows_down) {
        return begin <= source_scope->stack_boundary &&
            destination_size <= source_scope->stack_boundary - begin;
    }
    return begin >= source_scope->stack_boundary;
}

int cx_arena_reference_store_allowed_locked(const void *destination,
                                            size_t destination_size,
                                            const void *source,
                                            int destination_is_current_stack) {
    struct cx_arena *source_arena;
    struct cx_arena *destination_arena;
    struct cx_arena *source_storage;
    source_arena = cx_arena_find_owner_locked(source);
    source_storage = cx_arena_find_storage_owner_locked(source);
    if ((source_storage != NULL && source_arena == NULL) ||
        !cx_arena_find_store_range_owner_locked(
            destination, destination_size, &destination_arena)) {
        return 0;
    }
    if (destination_arena != NULL &&
        !cx_arena_is_active_in_current_context_locked(destination_arena)) {
        return 0;
    }
    if (source_arena == NULL) return 1;
    if (!cx_arena_is_active_in_current_context_locked(source_arena)) {
        return 0;
    }
    if (destination_arena == source_arena) return 1;
    if (destination_arena != NULL) {
        return cx_arena_is_ancestor_of_destination_locked(source_arena,
                                                          destination_arena);
    }
    // A stack slot in a frame entered after the arena scope began necessarily
    // expires before that scope can end. Caller-frame slots remain outside the
    // boundary and cannot retain the arena value after reset.
    if (!(destination_is_current_stack &&
          cx_arena_stack_slot_expires_before_scope_locked(
              source_arena, destination, destination_size))) {
        return 0;
    }
    return 1;
}

int cx_arena_retained_reference_source_allowed_locked(
    const struct cx_arena *retained_arena, const void *source) {
    struct cx_arena *source_arena;
    if (retained_arena == NULL || retained_arena->logical_reference_count == 0) {
        return 0;
    }
    if (source == NULL) return 1;
    source_arena = cx_arena_find_owner_locked(source);
    if (source_arena != NULL) return source_arena == retained_arena;
    return !cx_arena_contains_storage_locked(source);
}

int cx_arena_test_reference_store_allowed(const void *destination,
                                          const void *source,
                                          int destination_is_current_stack) {
    int allowed;
    cx_gc_runtime_lock();
    allowed = cx_arena_reference_store_allowed_locked(
        destination, sizeof(cx_ptr), source, destination_is_current_stack);
    cx_gc_runtime_unlock();
    return allowed;
}

static int cx_arena_range_intersects_locked(const void *address, size_t size) {
    uintptr_t begin = (uintptr_t)address;
    uintptr_t end;
    struct cx_arena *arena;
    if (size == 0) return 0;
    if (address == NULL || size > UINTPTR_MAX - begin) return 1;
    end = begin + size;
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        struct cx_arena_chunk *chunk;
        for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
            uintptr_t chunk_begin = (uintptr_t)chunk->memory;
            uintptr_t chunk_end;
            if (chunk->capacity > UINTPTR_MAX - chunk_begin) return 1;
            chunk_end = chunk_begin + chunk->capacity;
            if (begin < chunk_end && chunk_begin < end) return 1;
        }
    }
    return 0;
}

int cx_arena_memory_copy_allowed_locked(const void *destination,
                                        const void *source,
                                        size_t size) {
    return !cx_arena_range_intersects_locked(destination, size) &&
        !cx_arena_range_intersects_locked(source, size);
}

void cx_arena_scan_active(void (*scan)(const void *, size_t, void *),
                          void *context) {
    struct cx_arena *arena;
    if (scan == NULL) return;
    for (arena = cx_arena_registry; arena != NULL; arena = arena->registry_next) {
        struct cx_arena_chunk *chunk;
        if (arena->active_depth == 0 &&
            arena->logical_reference_count == 0) continue;
        for (chunk = arena->first; chunk != NULL; chunk = chunk->next) {
            if (chunk->used != 0) scan(chunk->memory, chunk->used, context);
        }
    }
}
