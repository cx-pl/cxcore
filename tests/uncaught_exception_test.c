#include "../src/cxcore.h"

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct cxcore_System_Reflection_TypeInfo type_info;

CX_STRING_DEF(test_exception_message, "diagnostic message");
CX_STRING_DEF(test_inner_exception_message, "inner diagnostic");

extern void cxcore_System_Exception___constructor__2(
    struct cxcore_System_Exception *self,
    const struct cxcore_System_String *message,
    const struct cxcore_System_Exception *innerException);

int main(void) {
    struct cxcore_System_Exception *inner_exception =
        (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*inner_exception));
    struct cxcore_System_Exception *exception_object =
        (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*exception_object));
    cxcore_System_Exception___constructor(inner_exception,
                                          &test_inner_exception_message);
    cxcore_System_Exception___constructor__2(
        exception_object, &test_exception_message, inner_exception);
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    CX_THROW(exception_object);
}
