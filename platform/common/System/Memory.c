#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../platform.h"
#include "../../../src/cxcore.h"

#define CX_GC_INITIAL_THRESHOLD ((size_t)1024 * 1024)

struct cx_gc_allocation {
    void *address;
    size_t size;
    cx_bool marked;
    struct cx_gc_allocation *next;
};

struct cx_gc_registered_root {
    const void *address;
    size_t size;
    struct cx_gc_registered_root *next;
};

static size_t cx_gc_heap_bytes;
static CX_THREAD_LOCAL cx_platform_thread_id cx_gc_cached_thread;

#ifdef CXCORE_TESTING
static cx_bool cx_memory_fail_next_allocation;
static size_t cx_memory_test_collection_count;
static size_t cx_memory_test_collected_bytes;

CX_API void cx_memory_test_fail_next_allocation(void) {
    cx_memory_fail_next_allocation = CX_TRUE;
}

CX_API size_t cx_memory_test_collection_count_get(void) {
    return cx_memory_test_collection_count;
}

CX_API size_t cx_memory_test_collected_bytes_get(void) {
    return cx_memory_test_collected_bytes;
}

CX_API size_t cx_memory_test_heap_bytes_get(void) {
    return cx_gc_heap_bytes;
}
#endif

static struct cx_gc_allocation *cx_gc_allocations;
static struct cx_gc_registered_root *cx_gc_roots;
static size_t cx_gc_next_collection = CX_GC_INITIAL_THRESHOLD;
static cx_platform_atomic_thread_id cx_gc_owner_thread;

static cx_bool cx_memory_should_fail_allocation(void) {
#ifdef CXCORE_TESTING
    if (cx_memory_fail_next_allocation) {
        cx_memory_fail_next_allocation = CX_FALSE;
        return CX_TRUE;
    }
#endif
    return CX_FALSE;
}

static void cx_memory_fail(const char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

static void cx_memory_out_of_memory(void) {
    cx_memory_fail("Out of memory");
}

static void cx_gc_require_owner_thread(void) {
    cx_platform_thread_id owner =
        cx_platform_owner_thread_load(&cx_gc_owner_thread);
    cx_platform_thread_id current_thread;
    if (cx_gc_cached_thread != 0 && cx_gc_cached_thread == owner) {
        return;
    }
    current_thread = cx_platform_current_thread_id();
    if (owner == 0) {
        owner = cx_platform_owner_thread_claim(&cx_gc_owner_thread, current_thread);
        if (owner == 0) {
            owner = current_thread;
        }
    }
    if (owner != 0 && owner != current_thread) {
        cx_memory_fail("CX managed memory currently supports one thread per process");
    }
    cx_gc_cached_thread = current_thread;
}

void cx_gc_check_thread(void) {
    cx_gc_require_owner_thread();
}

static struct cx_gc_allocation *cx_gc_find_allocation(const void *candidate) {
    uintptr_t address = (uintptr_t)candidate;
    struct cx_gc_allocation *allocation;
    for (allocation = cx_gc_allocations; allocation != NULL; allocation = allocation->next) {
        uintptr_t begin = (uintptr_t)allocation->address;
        if (address >= begin && address - begin < allocation->size) {
            return allocation;
        }
    }
    return NULL;
}

static void cx_gc_mark_candidate(const void *candidate,
                                 struct cx_gc_allocation **worklist,
                                 size_t worklist_capacity,
                                 size_t *worklist_count) {
    struct cx_gc_allocation *allocation = cx_gc_find_allocation(candidate);
    if (allocation == NULL || allocation->marked) {
        return;
    }
    allocation->marked = CX_TRUE;
    if (*worklist_count < worklist_capacity) {
        worklist[(*worklist_count)++] = allocation;
    }
}

static void cx_gc_scan_region(const void *address,
                              size_t size,
                              struct cx_gc_allocation **worklist,
                              size_t worklist_capacity,
                              size_t *worklist_count) {
    uintptr_t begin = (uintptr_t)address;
    uintptr_t aligned;
    size_t offset;
    if (address == NULL || size < sizeof(cx_ptr) || begin > UINTPTR_MAX - (sizeof(cx_ptr) - 1)) {
        return;
    }
    aligned = (begin + sizeof(cx_ptr) - 1) & ~(uintptr_t)(sizeof(cx_ptr) - 1);
    offset = (size_t)(aligned - begin);
    while (offset <= size - sizeof(cx_ptr)) {
        cx_ptr candidate;
        memcpy(&candidate, (const cx_byte *)address + offset, sizeof(candidate));
        cx_gc_mark_candidate(candidate, worklist, worklist_capacity, worklist_count);
        if (size - sizeof(cx_ptr) - offset < sizeof(cx_ptr)) {
            break;
        }
        offset += sizeof(cx_ptr);
    }
}

static size_t cx_gc_count_allocations(void) {
    size_t count = 0;
    struct cx_gc_allocation *allocation;
    for (allocation = cx_gc_allocations; allocation != NULL; allocation = allocation->next) {
        if (count == SIZE_MAX) {
            return SIZE_MAX;
        }
        ++count;
    }
    return count;
}

void cx_gc_collect(void) {
    struct cx_gc_allocation **worklist;
    struct cx_gc_allocation *allocation;
    struct cx_gc_allocation *previous;
    struct cx_gc_registered_root *root;
    size_t allocation_count;
    size_t worklist_count = 0;
    size_t worklist_index = 0;
    size_t live_bytes = 0;
    size_t collected_bytes = 0;
    jmp_buf registers;
    volatile cx_byte stack_marker = 0;
    uintptr_t stack_low;
    uintptr_t stack_high;

    cx_gc_require_owner_thread();
    allocation_count = cx_gc_count_allocations();
    if (allocation_count == 0) {
        return;
    }
    if (allocation_count > SIZE_MAX / sizeof(*worklist)) {
        return;
    }
    worklist = (struct cx_gc_allocation **)malloc(allocation_count * sizeof(*worklist));
    if (worklist == NULL) {
        return;
    }
    if (!cx_platform_current_stack_bounds(&stack_low, &stack_high)) {
        free(worklist);
        return;
    }
    if ((uintptr_t)&stack_marker < stack_low ||
        (uintptr_t)&stack_marker >= stack_high) {
        free(worklist);
        return;
    }

    for (allocation = cx_gc_allocations; allocation != NULL; allocation = allocation->next) {
        allocation->marked = CX_FALSE;
    }
    for (root = cx_gc_roots; root != NULL; root = root->next) {
        cx_gc_scan_region(root->address, root->size, worklist, allocation_count, &worklist_count);
    }
    if (setjmp(registers) == 0) {
        cx_gc_scan_region(registers, sizeof(registers), worklist, allocation_count, &worklist_count);
    }
    cx_gc_scan_region((const void *)&stack_marker,
                      (size_t)(stack_high - (uintptr_t)&stack_marker),
                      worklist, allocation_count, &worklist_count);

    while (worklist_index < worklist_count) {
        allocation = worklist[worklist_index++];
        cx_gc_scan_region(allocation->address, allocation->size,
                          worklist, allocation_count, &worklist_count);
    }

    previous = NULL;
    allocation = cx_gc_allocations;
    while (allocation != NULL) {
        struct cx_gc_allocation *next = allocation->next;
        if (allocation->marked) {
            live_bytes += allocation->size;
            previous = allocation;
        } else {
            if (previous == NULL) {
                cx_gc_allocations = next;
            } else {
                previous->next = next;
            }
            cx_gc_heap_bytes -= allocation->size;
            collected_bytes += allocation->size;
            free(allocation->address);
            free(allocation);
        }
        allocation = next;
    }
    free(worklist);

    if (live_bytes > SIZE_MAX / 2) {
        cx_gc_next_collection = SIZE_MAX;
    } else {
        size_t next_threshold = live_bytes * 2;
        cx_gc_next_collection = next_threshold < CX_GC_INITIAL_THRESHOLD
            ? CX_GC_INITIAL_THRESHOLD : next_threshold;
    }
#ifdef CXCORE_TESTING
    ++cx_memory_test_collection_count;
    cx_memory_test_collected_bytes += collected_bytes;
#else
    (void)collected_bytes;
#endif
}

void cx_gc_register_static_roots(const struct cx_gc_root_region *roots, cx_uint count) {
    cx_uint index;
    cx_gc_require_owner_thread();
    if (count != 0 && roots == NULL) {
        cx_memory_fail("Invalid CX static root table");
    }
    for (index = 0; index < count; ++index) {
        struct cx_gc_registered_root *root;
        const void *address = roots[index].address;
        size_t size = roots[index].size;
        if (address == NULL || size == 0) {
            continue;
        }
        for (root = cx_gc_roots; root != NULL; root = root->next) {
            if (root->address == address && root->size == size) {
                break;
            }
        }
        if (root == NULL) {
            root = (struct cx_gc_registered_root *)malloc(sizeof(*root));
            if (root == NULL) {
                cx_memory_out_of_memory();
            }
            root->address = address;
            root->size = size;
            root->next = cx_gc_roots;
            cx_gc_roots = root;
        }
    }
}

cx_ptr CX_ID_4(cxcore, System, Memory, Alloc)(cx_uint size) {
    size_t allocation_size = size == 0 ? 1 : (size_t)size;
    struct cx_gc_allocation *record;
    void *block;

    cx_gc_require_owner_thread();
    if (cx_memory_should_fail_allocation()) {
        cx_memory_out_of_memory();
    }
    if (allocation_size > SIZE_MAX - cx_gc_heap_bytes) {
        cx_memory_out_of_memory();
    }
    if (allocation_size > cx_gc_next_collection -
            (cx_gc_heap_bytes < cx_gc_next_collection ? cx_gc_heap_bytes : cx_gc_next_collection)) {
        cx_gc_collect();
    }
    block = calloc(1, allocation_size);
    if (block == NULL) {
        cx_gc_collect();
        block = calloc(1, allocation_size);
    }
    if (block == NULL) {
        cx_memory_out_of_memory();
    }
    record = (struct cx_gc_allocation *)malloc(sizeof(*record));
    if (record == NULL) {
        free(block);
        cx_gc_collect();
        record = (struct cx_gc_allocation *)malloc(sizeof(*record));
    }
    if (record == NULL) {
        cx_memory_out_of_memory();
    }
    record->address = block;
    record->size = allocation_size;
    record->marked = CX_FALSE;
    record->next = cx_gc_allocations;
    cx_gc_allocations = record;
    cx_gc_heap_bytes += allocation_size;
    if (cx_gc_heap_bytes > cx_gc_next_collection) {
        cx_gc_next_collection = cx_gc_heap_bytes > SIZE_MAX / 2
            ? SIZE_MAX : cx_gc_heap_bytes * 2;
    }
    return block;
}

cx_ptr CX_ID_4(cxcore, System, Memory, Realloc)(cx_ptr block, cx_uint size) {
    struct cx_gc_allocation *record;
    void *resized;
    size_t new_size;
    if (block == NULL) {
        return CX_ID_4(cxcore, System, Memory, Alloc)(size);
    }
    if (size == 0) {
        CX_ID_4(cxcore, System, Memory, Free)(block);
        return CX_NULL;
    }
    cx_gc_require_owner_thread();
    record = cx_gc_find_allocation(block);
    if (record == NULL || record->address != block) {
        cx_memory_fail("Memory.Realloc requires a pointer returned by Memory.Alloc");
    }
    if (cx_memory_should_fail_allocation()) {
        cx_memory_out_of_memory();
    }
    new_size = (size_t)size;
    if (new_size > record->size) {
        size_t growth = new_size - record->size;
        size_t available = cx_gc_next_collection -
            (cx_gc_heap_bytes < cx_gc_next_collection ? cx_gc_heap_bytes : cx_gc_next_collection);
        if (growth > available) {
            cx_gc_collect();
            record = cx_gc_find_allocation(block);
            if (record == NULL || record->address != block) {
                cx_memory_fail("Memory.Realloc lost its managed allocation during collection");
            }
        }
    }
    if (new_size > SIZE_MAX - (cx_gc_heap_bytes - record->size)) {
        cx_memory_out_of_memory();
    }
    resized = realloc(block, new_size);
    if (resized == NULL) {
        cx_gc_collect();
        resized = realloc(block, new_size);
    }
    if (resized == NULL) {
        cx_memory_out_of_memory();
    }
    cx_gc_heap_bytes -= record->size;
    record->address = resized;
    record->size = new_size;
    cx_gc_heap_bytes += new_size;
    if (cx_gc_heap_bytes > cx_gc_next_collection) {
        cx_gc_next_collection = cx_gc_heap_bytes > SIZE_MAX / 2
            ? SIZE_MAX : cx_gc_heap_bytes * 2;
    }
    return resized;
}

void CX_ID_4(cxcore, System, Memory, Free)(cx_ptr block) {
    struct cx_gc_allocation *record;
    struct cx_gc_allocation *previous = NULL;
    if (block == NULL) {
        return;
    }
    cx_gc_require_owner_thread();
    record = cx_gc_allocations;
    while (record != NULL && record->address != block) {
        previous = record;
        record = record->next;
    }
    if (record != NULL) {
        if (previous == NULL) {
            cx_gc_allocations = record->next;
        } else {
            previous->next = record->next;
        }
        cx_gc_heap_bytes -= record->size;
        free(record);
    }
    free(block);
}

void CX_ID_4(cxcore, System, Memory, Copy)(cx_ptr dest, cx_ptr src, cx_uint size) {
    cx_gc_require_owner_thread();
    if (size == 0) {
        return;
    }
    if (dest == NULL || src == NULL) {
        abort();
    }
    memcpy(dest, src, size);
}
