#include <assert.h>

#include "../src/cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) type_info;

struct test_exception {
    cx_ptr vtable;
};

static type_info exception_type = {.Hash = 1};
static type_info unrelated_type = {.Hash = 2};
static union cx_vtable_entry exception_vtable[] = {{.data = &exception_type}};
static struct test_exception exception_object = {exception_vtable};

static void throw_across_function_boundary(void) {
    CX_THROW(&exception_object);
}

int main(void) {
    struct cx_exception_frame outer_frame;
    volatile int reached_handler = 0;

    cx_exception_push(&outer_frame);
    if (setjmp(outer_frame.environment) == 0) {
        throw_across_function_boundary();
        assert(0);
    } else {
        cx_exception_pop(&outer_frame);
        reached_handler = 1;
        assert(cx_exception_current() == &exception_object);
        assert(CX_ID_4(cxcore, System, Exception, Matches)(
            (const struct CX_ID_3(cxcore, System, Exception) *)&exception_object, exception_type));
        assert(!CX_ID_4(cxcore, System, Exception, Matches)(
            (const struct CX_ID_3(cxcore, System, Exception) *)&exception_object, unrelated_type));
        cx_exception_clear();
    }
    assert(reached_handler == 1);

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
