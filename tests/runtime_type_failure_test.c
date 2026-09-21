#include "../src/cxcore.h"

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) type_info;

struct test_object { cx_ptr vtable; };

static type_info actual_type = { 0 };
static type_info unrelated_type = { 0 };
static union cx_vtable_entry object_vtable[] = { { .data = &actual_type } };

int main(void)
{
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    struct test_object object = { object_vtable };
    (void)cx_checked_cast_object(&object, &unrelated_type);
    return 0;
}
