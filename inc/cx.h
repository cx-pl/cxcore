#ifndef __CX_H__
#define __CX_H__

#include "cxdefs.h"
#include "cxtypes.h"

struct CX_ID_3(cxcore, System, Array);
struct CX_ID_4(cxcore, System, Reflection, TypeInfo);

CX_API struct CX_ID_3(cxcore, System, Array) * cx_array_new(cx_uint length, cx_uint elementSize);
CX_API cx_ptr cx_array_at(struct CX_ID_3(cxcore, System, Array) * array, cx_uint index,
                          cx_uint elementSize);

CX_API void cx_exception_push(struct cx_exception_frame *frame);
CX_API void cx_exception_pop(struct cx_exception_frame *frame);
CX_API void cx_exception_throw(cx_ptr exceptionObject, const char *fileName, int line);
CX_API void cx_exception_rethrow(void);
CX_API cx_ptr cx_exception_current(void);
CX_API cx_bool cx_exception_pending(void);
CX_API void cx_exception_clear(void);
CX_API cx_bool cx_type_is(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * actualType,
                          const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * targetType);
CX_API cx_bool cx_is_object(cx_ptr instance,
                            const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *
                                targetType);
CX_API cx_bool cx_is_interface(struct cx_iface_ref source,
                               const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *
                                   targetType);
CX_API cx_ptr cx_checked_cast_object(cx_ptr instance,
                                     const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *
                                         targetType);
CX_API cx_ptr cx_checked_cast_interface(struct cx_iface_ref source,
                                        const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *
                                            targetType);
CX_API struct cx_iface_ref cx_checked_cast_object_to_interface(
    cx_ptr instance, const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * targetType);
CX_API struct cx_iface_ref cx_checked_cast_interface_to_interface(
    struct cx_iface_ref source,
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * targetType);

CX_API const struct cx_interface_impl *
cx_reflection_interfaces(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * typeInfo,
                         cx_uint *count);
CX_API const struct cx_reflection_field *
cx_reflection_fields(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * typeInfo,
                     cx_uint *count);
CX_API const struct cx_reflection_function *
cx_reflection_functions(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * typeInfo,
                        cx_uint *count);

#endif // __CX_H__
