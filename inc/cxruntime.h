#ifndef __CX_RUNTIME_H__
#define __CX_RUNTIME_H__

#include "cxdefs.h"
#include "cxtypes.h"

struct CX_ID_4(cxcore, System, Collections, Array);
struct CX_ID_4(cxcore, System, Reflection, TypeInfo);

//
// Arrays
//

CX_API struct CX_ID_4(cxcore, System, Collections, Array) *
    cx_array_new(cx_uint length, cx_uint elementSize);
CX_API cx_ptr cx_array_at(struct CX_ID_4(cxcore, System, Collections, Array) * array, cx_uint index,
                          cx_uint elementSize);

//
// Exceptions
//

CX_API void cx_exception_push(struct cx_exception_frame *frame);
CX_API void cx_exception_pop(struct cx_exception_frame *frame);
CX_API CX_NORETURN void cx_exception_throw(cx_ptr exceptionObject, const char *fileName, int line);
CX_API CX_NORETURN void cx_exception_rethrow(void);
CX_API cx_ptr cx_exception_current(void);
CX_API cx_bool cx_exception_pending(void);
CX_API void cx_exception_clear(void);

//
// Generics
//

CX_API void
cx_register_generic_virtual_method(struct cx_generic_virtual_registration *registration);
CX_API cx_vtable_function cx_get_generic_virtual_method(cx_ptr receiverTypeInfo,
                                                        const char *methodIdentity,
                                                        const cx_ptr *methodTypeArguments,
                                                        cx_uint methodTypeArgumentCount);
CX_API cx_vtable_function cx_require_generic_virtual_method(cx_ptr receiverTypeInfo,
                                                            const char *methodIdentity,
                                                            const cx_ptr *methodTypeArguments,
                                                            cx_uint methodTypeArgumentCount);

//
// GC
//

CX_API void cx_gc_register_static_roots(const struct cx_gc_root_region *roots, cx_uint count);
CX_API void cx_gc_check_thread(void);
// Native worker/embedding API; see docs/threading-gc-design.md for cooperation rules.
CX_API void cx_gc_attach_thread(void);
CX_API void cx_gc_detach_thread(void);
// Foreign callback scopes detach only when this scope attached the OS thread.
CX_API cx_bool cx_gc_enter_thread(void);
CX_API void cx_gc_leave_thread(cx_bool attachedHere);
CX_API struct cx_gc_root_handle *cx_gc_add_root(const void *address, cx_uint size);
CX_API void cx_gc_remove_root(struct cx_gc_root_handle *handle);
CX_API void cx_gc_safepoint(void);
CX_API void cx_gc_run_blocking(cx_gc_blocking_callback callback, void *context);
CX_API void cx_gc_collect(void);

//
// Interop
//

CX_API cx_ulong cx_interop_stack_size(cx_long count, cx_ulong elementSize);
static inline void *cx_interop_align_pointer(void *storage, cx_ulong alignment) {
    uintptr_t mask = (uintptr_t)alignment - 1;
    return (void *)(((uintptr_t)storage + mask) & ~mask);
}
CX_API void cx_interop_capture_last_error(void);
CX_API void cx_interop_capture_errno(void);
CX_API cx_uint cx_interop_last_error(void);
CX_API cx_int cx_interop_errno(void);
CX_API cx_uint CX_ID_5(cxcore, System, Interop, NativeError, GetLastError)(void);
CX_API cx_int CX_ID_5(cxcore, System, Interop, NativeError, GetErrno)(void);
CX_API void cx_interop_require_function(cx_bool nonnull);
CX_API void cx_interop_symbol_require(const void *symbol, size_t size);
CX_API void cx_interop_callback_enter(struct cx_native_callback_scope *scope);
CX_API void cx_interop_callback_leave(struct cx_native_callback_scope *scope);
CX_API void cx_interop_callback_failed(cx_ptr exception);
CX_API void cx_gc_run_native_call(cx_native_call_callback callback, void *context);
CX_API void *cx_interop_context_alloc(size_t size);
CX_API void *cx_interop_context_alloc_aligned(size_t size, size_t alignment);
CX_API void cx_interop_context_free_aligned(void *context);
CX_API struct CX_ID_3(cxcore, System, Exception) *
    CX_ID_5(cxcore, System, Interop, NativeCallback, GetLastException)(void);
CX_API void CX_ID_5(cxcore, System, Interop, NativeCallback, ClearLastException)(void);

//
// Reflection
//

CX_API const struct cx_interface_impl *
cx_reflection_interfaces(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * typeInfo,
                         cx_uint *count);
CX_API const struct cx_reflection_field *
cx_reflection_fields(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * typeInfo,
                     cx_uint *count);
CX_API const struct cx_reflection_function *
cx_reflection_functions(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * typeInfo,
                        cx_uint *count);
CX_API const struct cx_reflection_function *cx_reflection_find_function(
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * typeInfo, const char *name,
    cx_uint parameterCount,
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *const *parameterTypes);

//
// Runtime ABI
//

CX_API cx_uint cx_runtime_abi_version(void);
CX_API void cx_runtime_require_abi(cx_uint expectedVersion);

//
// Runtime once
//

CX_API cx_bool cx_runtime_once_begin(struct cx_runtime_once *once);
CX_API void cx_runtime_once_complete(struct cx_runtime_once *once);
CX_API void cx_runtime_once_fail(struct cx_runtime_once *once, cx_ptr failure);

//
// Threads
//

CX_API void cx_thread_wait_all(void);

//
// Types
//

static inline struct cx_iface_ref cx_iface_upcast(struct cx_iface_ref source, cx_uint vtableSlot) {
    struct cx_iface_ref result = {source.vtable == (cx_ptr)0
                                      ? (cx_ptr)0
                                      : ((union cx_vtable_entry *)source.vtable)[vtableSlot].data,
                                  source.instance, source.owner};
    return result;
}
CX_API cx_bool cx_type_is(const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * actualType,
                          const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * targetType);
CX_API cx_ptr cx_box_value(const void *value, cx_uint valueSize,
                           const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) * valueType);
CX_API cx_ptr cx_unbox_value(cx_ptr instance,
                             const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *
                                 valueType);
CX_API const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *
    cx_boxed_value_type(cx_ptr instance);
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

#endif // __CX_RUNTIME_H__
