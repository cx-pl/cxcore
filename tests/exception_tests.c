#include <assert.h>
#include <stddef.h>

#include "../src/cxcore.h"
#include "../src/cxtext.api.h"
#include "../src/cxthreading.h"
#include "../platform/common/cxarena.h"

typedef struct cxcore_System_Reflection_TypeInfo type_info;

struct test_exception {
    cx_ptr vtable;
};

struct test_delegate_exception {
    struct cxcore_System_Exception base;
    cx_action callback;
};

static cx_ptr observed_callback_environment;
static struct cxcore_System_Reflection_TypeInfo cx_test_action___typeinfo;
static const struct cx_reflection_field test_delegate_exception_fields[] = {
    {
        .flags = CX_REFLECTION_FLAG_VISIBILITY_PUBLIC,
        .offset = (cx_uint)offsetof(struct test_delegate_exception, callback),
        .typeInfo = &cx_test_action___typeinfo,
        .name = "Callback",
        .size = (cx_uint)sizeof(cx_action),
    },
};
static const struct cx_runtime_type_info test_delegate_exception_runtime = {
    .fields = test_delegate_exception_fields,
    .fieldCount = 1u,
};
static struct cxcore_System_Reflection_TypeInfo cx_test_action___typeinfo = {
    .Hash = 0xC0A001u,
    .Flags = CX_REFLECTION_FLAG_TYPE_DELEGATE,
    .Size = (cx_uint)sizeof(cx_action),
};
static struct cxcore_System_Reflection_TypeInfo test_delegate_exception_typeinfo = {
    .Hash = 0xC0A002u,
    .Flags = CX_REFLECTION_FLAG_TYPE_CLASS,
    .Size = (cx_uint)sizeof(struct test_delegate_exception),
    .BaseType = {._obj = CX_NULL},
    .RuntimeTypeInfo = (cx_ptr)&test_delegate_exception_runtime,
};
static union cx_vtable_entry test_delegate_exception_vtable[] = {
    {.data = &test_delegate_exception_typeinfo},
};

static type_info exception_type = {.Hash = 1};
static type_info unrelated_type = {.Hash = 2};
static union cx_vtable_entry exception_vtable[] = {{.data = &exception_type}};
static struct test_exception exception_object = {exception_vtable};

static void throw_across_function_boundary(void) {
    CX_THROW(&exception_object);
}

static void throw_arena_owned_derived_exception(
    struct cx_arena *arena,
    struct cxcore_System_Exception *innerException) {
    struct cx_arena_scope arena_scope;
    struct cxcore_System_EncodingException *error;
    assert(arena != NULL && innerException != NULL);
    cx_arena_enter(arena, &arena_scope);
    error = (struct cxcore_System_EncodingException *)cx_arena_allocate_current(
        sizeof(*error), _Alignof(struct cxcore_System_EncodingException));
    assert(error != NULL);
    cxcore_System_EncodingException___constructor__2(error, 37u);
    (void)cx_gc_store_reference(
        (cx_ptr *)&error->__base._innerException._obj,
        innerException);
    CX_THROW(error);
}

static void throw_arena_owned_exception_with_scoped_field(
    struct cx_arena *arena) {
    struct cx_arena_scope arena_scope;
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *source;
    struct cxcore_System_Threading_Tasks_CancellationToken token;
    struct cxcore_System_Threading_Tasks_OperationCanceledException *error;
    cx_arena_enter(arena, &arena_scope);
    source = (struct cxcore_System_Threading_Tasks_CancellationTokenSource *)
        cx_arena_allocate_current(
            sizeof(*source), _Alignof(struct cxcore_System_Threading_Tasks_CancellationTokenSource));
    assert(source != NULL);
    cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
    token._source = source;
    error = (struct cxcore_System_Threading_Tasks_OperationCanceledException *)
        cx_arena_allocate_current(
            sizeof(struct cxcore_System_Threading_Tasks_OperationCanceledException),
            cx_arena_maximum_alignment());
    assert(error != NULL);
    cxcore_System_Threading_Tasks_OperationCanceledException___constructor__2(
        error, token);
    CX_THROW(error);
}

static void throw_arena_owned_exception_with_durable_field(
    struct cx_arena *arena,
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *source) {
    struct cx_arena_scope arena_scope;
    struct cxcore_System_Threading_Tasks_CancellationToken token;
    struct cxcore_System_Threading_Tasks_OperationCanceledException *error;
    assert(arena != NULL && source != NULL);
    cx_arena_enter(arena, &arena_scope);
    token._source = source;
    error = (struct cxcore_System_Threading_Tasks_OperationCanceledException *)
        cx_arena_allocate_current(
            sizeof(*error), _Alignof(struct cxcore_System_Threading_Tasks_OperationCanceledException));
    assert(error != NULL);
    cxcore_System_Threading_Tasks_OperationCanceledException___constructor__2(
        error, token);
    CX_THROW(error);
}

static void record_callback_environment(void *environment) {
    observed_callback_environment = environment;
}

static void throw_arena_owned_delegate_exception(
    struct cx_arena *arena, cx_ptr callbackEnvironment) {
    struct cx_arena_scope arena_scope;
    struct test_delegate_exception *error;
    cx_arena_enter(arena, &arena_scope);
    error = (struct test_delegate_exception *)cx_arena_allocate_current(
        sizeof(*error), _Alignof(struct test_delegate_exception));
    assert(error != NULL);
    cxcore_System_Exception___constructor(&error->base, CX_NULL);
    error->base.__base.__vtable = test_delegate_exception_vtable;
    error->callback = (cx_action){
        .__function = record_callback_environment,
        .__environment = callbackEnvironment,
        .__target = callbackEnvironment,
    };
    CX_THROW(error);
}

static void throw_arena_owned_delegate_exception_with_scoped_environment(
    struct cx_arena *arena) {
    struct cx_arena_scope arena_scope;
    struct cxcore_System_Exception *callbackEnvironment;
    struct test_delegate_exception *error;
    cx_arena_enter(arena, &arena_scope);
    callbackEnvironment = (struct cxcore_System_Exception *)
        cx_arena_allocate_current(
            sizeof(*callbackEnvironment), _Alignof(struct cxcore_System_Exception));
    assert(callbackEnvironment != NULL);
    cxcore_System_Exception___constructor(callbackEnvironment, CX_NULL);
    error = (struct test_delegate_exception *)cx_arena_allocate_current(
        sizeof(*error), _Alignof(struct test_delegate_exception));
    assert(error != NULL);
    cxcore_System_Exception___constructor(&error->base, CX_NULL);
    error->base.__base.__vtable = test_delegate_exception_vtable;
    error->callback = (cx_action){
        .__function = record_callback_environment,
        .__environment = callbackEnvironment,
        .__target = callbackEnvironment,
    };
    CX_THROW(error);
}

int main(void) {
    struct cx_exception_frame outer_frame;
    struct cx_arena *arena;
    struct cxcore_System_Exception *durable_inner_exception;
    volatile int reached_handler = 0;

    test_delegate_exception_typeinfo.BaseType._obj =
        &cxcore_System_Exception___typeinfo;

    cx_exception_push(&outer_frame);
    if (setjmp(outer_frame.environment) == 0) {
        throw_across_function_boundary();
        assert(0);
    } else {
        cx_exception_pop(&outer_frame);
        reached_handler = 1;
        assert(cx_exception_current() == &exception_object);
        assert(cxcore_System_Exception_Matches(
            (const struct cxcore_System_Exception *)&exception_object, exception_type));
        assert(!cxcore_System_Exception_Matches(
            (const struct cxcore_System_Exception *)&exception_object, unrelated_type));
        cx_exception_clear();
    }

    arena = cx_arena_create(4096u, 4096u, 1);
    assert(arena != NULL);
    cx_exception_push(&outer_frame);
    if (setjmp(outer_frame.environment) == 0) {
        throw_arena_owned_exception_with_scoped_field(arena);
        assert(0);
    } else {
        struct cxcore_System_Exception *error = cx_exception_current();
        cx_exception_pop(&outer_frame);
        assert(CX_GET_TYPEINFO(error).Hash ==
               cxcore_System_Exception___typeinfo.Hash);
        assert(cx_gc_is_durable_reference(error));
        cx_arena_destroy(arena);
        cx_gc_collect();
        assert(cx_exception_current() == error);
        cx_exception_clear();
    }
    assert(reached_handler == 1);

    arena = cx_arena_create(4096u, 4096u, 1);
    assert(arena != NULL);
    durable_inner_exception = cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*durable_inner_exception));
    assert(durable_inner_exception != NULL);
    cxcore_System_Exception___constructor(durable_inner_exception, CX_NULL);
    cx_exception_push(&outer_frame);
    if (setjmp(outer_frame.environment) == 0) {
        throw_arena_owned_derived_exception(arena, durable_inner_exception);
        assert(0);
    } else {
        struct cxcore_System_EncodingException *error = cx_exception_current();
        cx_exception_pop(&outer_frame);
        assert(CX_GET_TYPEINFO(error).Hash ==
               cxcore_System_EncodingException___typeinfo.Hash);
        assert(error->_position == 37u);
        assert(error->__base._innerException._obj == durable_inner_exception);
        assert(cx_gc_is_durable_reference(error->__base._innerException._obj));
        assert(cx_gc_is_durable_reference(error));
        cx_arena_destroy(arena);
        cx_gc_collect();
        error = cx_exception_current();
        assert(CX_GET_TYPEINFO(error).Hash ==
               cxcore_System_EncodingException___typeinfo.Hash);
        assert(error->_position == 37u);
        assert(error->__base._innerException._obj == durable_inner_exception);
        cx_exception_clear();
    }

    {
        struct cxcore_System_Threading_Tasks_CancellationTokenSource *source =
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
        struct cxcore_System_Threading_Tasks_OperationCanceledException *error;
        struct cxcore_System_Threading_Tasks_CancellationToken token;
        assert(source != NULL);
        cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(source);
        arena = cx_arena_create(4096u, 4096u, 1);
        assert(arena != NULL);
        cx_exception_push(&outer_frame);
        if (setjmp(outer_frame.environment) == 0) {
            throw_arena_owned_exception_with_durable_field(arena, source);
            assert(0);
        } else {
            error = cx_exception_current();
            cx_exception_pop(&outer_frame);
            assert(CX_GET_TYPEINFO(error).Hash ==
                   cxcore_System_Threading_Tasks_OperationCanceledException___typeinfo.Hash);
            token = cxcore_System_Threading_Tasks_OperationCanceledException_CancellationToken___const_get(error);
            assert(token._source == source);
            assert(cx_gc_is_durable_reference(token._source));
            assert(cx_gc_is_durable_reference(error));
            cx_arena_destroy(arena);
            cx_gc_collect();
            error = cx_exception_current();
            assert(CX_GET_TYPEINFO(error).Hash ==
                   cxcore_System_Threading_Tasks_OperationCanceledException___typeinfo.Hash);
            token = cxcore_System_Threading_Tasks_OperationCanceledException_CancellationToken___const_get(error);
            assert(token._source == source);
            cx_exception_clear();
        }
    }

    {
        struct cxcore_System_Exception *callback_environment =
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*callback_environment));
        struct test_delegate_exception *error;
        assert(callback_environment != NULL);
        cxcore_System_Exception___constructor(callback_environment, CX_NULL);
        arena = cx_arena_create(4096u, 4096u, 1);
        assert(arena != NULL);
        observed_callback_environment = NULL;
        cx_exception_push(&outer_frame);
        if (setjmp(outer_frame.environment) == 0) {
            throw_arena_owned_delegate_exception(arena, callback_environment);
            assert(0);
        } else {
            error = cx_exception_current();
            cx_exception_pop(&outer_frame);
            assert(CX_GET_TYPEINFO(error).Hash ==
                   test_delegate_exception_typeinfo.Hash);
            assert(error->callback.__environment == callback_environment);
            assert(error->callback.__target == callback_environment);
            assert(cx_gc_is_durable_reference(error));
            cx_arena_destroy(arena);
            cx_gc_collect();
            error = cx_exception_current();
            assert(CX_GET_TYPEINFO(error).Hash ==
                   test_delegate_exception_typeinfo.Hash);
            error->callback.__function(error->callback.__environment);
            assert(observed_callback_environment == callback_environment);
            cx_exception_clear();
        }
    }

    {
        struct cxcore_System_Exception *error;
        arena = cx_arena_create(4096u, 4096u, 1);
        assert(arena != NULL);
        cx_exception_push(&outer_frame);
        if (setjmp(outer_frame.environment) == 0) {
            throw_arena_owned_delegate_exception_with_scoped_environment(arena);
            assert(0);
        } else {
            error = cx_exception_current();
            cx_exception_pop(&outer_frame);
            assert(CX_GET_TYPEINFO(error).Hash ==
                   cxcore_System_Exception___typeinfo.Hash);
            cx_arena_destroy(arena);
            cx_gc_collect();
            assert(cx_exception_current() == error);
            cx_exception_clear();
        }
    }

    cx_exception_push(&outer_frame);
    if (setjmp(outer_frame.environment) == 0) {
        struct cx_exception_frame inner_frame;
        cx_exception_push(&inner_frame);
        if (setjmp(inner_frame.environment) == 0) {
            throw_across_function_boundary();
            assert(0);
        } else {
            cx_exception_pop(&inner_frame);
            assert(cx_exception_current() == &exception_object);
            CX_RETHROW();
        }
    } else {
        cx_exception_pop(&outer_frame);
        assert(cx_exception_current() == &exception_object);
        cx_exception_clear();
    }

    assert(!cx_exception_pending());
    return 0;
}
