#include "../src/cxcore.h"

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) type_info;

struct test_exception {
    cx_ptr vtable;
};

static type_info exception_type = {0};
static union cx_vtable_entry exception_vtable[] = {{.data = &exception_type}};

int main(void) {
    struct test_exception exception_object = {exception_vtable};
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    CX_THROW(&exception_object);
    return 0;
}
