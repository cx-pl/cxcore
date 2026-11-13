#ifndef __CX_RUNTIME_H__
#define __CX_RUNTIME_H__

#include "cxdefs.h"
#include "cxtypes.h"

#define CX_MODULE_METADATA_FORMAT_VERSION 1

CX_API struct cxcore_System_Collections_Array *cx_reflection_array_values(cx_ptr value,
    const char *typeKey, cx_uint elementSize,
    const struct cxcore_System_Reflection_TypeInfo *elementType, cx_uint kind, cx_uint maximumLength);
CX_API cx_ptr cx_annotation_string_value(const unsigned char *data, cx_uint byteLength);

// Typed array keys preserve element identity across separately generated modules.
CX_API struct cxcore_System_Collections_Array *
cx_array_with_type(struct cxcore_System_Collections_Array *array, const char *typeKey);
CX_API cx_ptr cx_safe_cast_array(cx_ptr instance, const char *typeKey);
CX_API cx_ptr cx_safe_cast_interface_to_array(struct cx_iface_ref source, const char *typeKey);
CX_API struct cxcore_System_Reflection_TypeInfo
cx_typeof_array(struct cxcore_System_Reflection_TypeInfo elementType);
CX_API struct cxcore_System_Reflection_TypeInfo
cx_typeof_nullable(struct cxcore_System_Reflection_TypeInfo underlyingType);

//
// Arrays
//

CX_API struct cxcore_System_Collections_Array *cx_array_new(cx_uint length, cx_uint elementSize);
CX_API struct cxcore_System_Collections_Array *cx_array_new_for_owner(
    const void *owner, cx_uint length, cx_uint elementSize);
CX_API cx_ptr cx_memory_alloc_for_owner(const void *owner, cx_uint size);
CX_API cx_ptr cx_array_at(struct cxcore_System_Collections_Array *array, cx_uint index,
                          cx_uint elementSize);

//
// External event waits
//

CX_API uint64_t cx_task_scheduler_post_on_event(cx_ptr context,
    cx_scheduler_wait_function wait, cx_action continuation, cx_action shutdown_continuation);
CX_API void cx_task_scheduler_post_on_event_reusable(cx_ptr context,
    cx_scheduler_wait_function wait, cx_action continuation,
    cx_action shutdown_continuation, cx_ptr *cached_item);
CX_API int cx_task_scheduler_cancel_event_wait(uint64_t registration_id);
CX_API int cx_task_scheduler_cancel_reusable_event_wait(cx_ptr cached_item);

//
// Decimal
//

extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Decimal___typeinfo;

CX_API cx_decimal cx_decimal_from_i64(cx_long value);
CX_API cx_decimal cx_decimal_from_u64(cx_ulong value);
CX_API cx_decimal cx_decimal_from_double(cx_double value, cx_int significantDigits);
CX_API cx_double cx_decimal_to_double(cx_decimal value);
CX_API cx_long cx_decimal_to_i64(cx_decimal value, cx_ulong maximum);
CX_API cx_ulong cx_decimal_to_u64(cx_decimal value, cx_ulong maximum);
CX_API cx_decimal cx_decimal_parse(const char *text);
CX_API cx_bool cx_decimal_try_parse(const char *text, cx_decimal *result);
// Returns the number of characters (excluding the terminating zero).
// A buffer of 32 bytes holds every decimal value in fixed-point notation.
CX_API cx_uint cx_decimal_format(cx_decimal value, char *buffer, cx_uint capacity);
CX_API cx_decimal cx_decimal_add(cx_decimal left, cx_decimal right);
CX_API cx_decimal cx_decimal_subtract(cx_decimal left, cx_decimal right);
CX_API cx_decimal cx_decimal_multiply(cx_decimal left, cx_decimal right);
CX_API cx_decimal cx_decimal_divide(cx_decimal left, cx_decimal right);
CX_API cx_decimal cx_decimal_remainder(cx_decimal left, cx_decimal right);
CX_API cx_decimal cx_decimal_negate(cx_decimal value);
CX_API cx_int cx_decimal_compare(cx_decimal left, cx_decimal right);
CX_API cx_decimal cx_decimal_increment(cx_decimal *location, cx_int delta, cx_bool postfix);
CX_API cx_decimal cx_decimal_add_assign(cx_decimal *location, cx_decimal value);
CX_API cx_decimal cx_decimal_subtract_assign(cx_decimal *location, cx_decimal value);
CX_API cx_decimal cx_decimal_multiply_assign(cx_decimal *location, cx_decimal value);
CX_API cx_decimal cx_decimal_divide_assign(cx_decimal *location, cx_decimal value);
CX_API cx_decimal cx_decimal_remainder_assign(cx_decimal *location, cx_decimal value);

//
// Exceptions
//

CX_API void cx_exception_push(struct cx_exception_frame *frame);
CX_API void cx_exception_pop(struct cx_exception_frame *frame);
CX_API cx_ptr cx_exception_allocate_durable(cx_uint size);
CX_API cx_ptr cx_exception_copy_durable(cx_ptr exception);
CX_API CX_NORETURN void cx_exception_raise_message(
    const struct cxcore_System_String *message, const char *fileName, int line);
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
// Generated reference assignments use this helper to enforce arena lifetimes.
CX_API cx_ptr cx_gc_store_reference(cx_ptr *destination, cx_ptr value);
// Native data-pointer fields use the same destination-lifetime checks without
// requiring the pointed-to storage to be a managed allocation.
CX_API cx_ptr cx_gc_store_native_pointer(cx_ptr *destination, cx_ptr value);
// Native retention boundaries use this to reject scoped arena and attached
// thread stack references.
CX_API cx_bool cx_gc_is_durable_reference(cx_ptr value);
CX_API struct cx_iface_ref cx_gc_store_interface_reference(
    struct cx_iface_ref *destination, struct cx_iface_ref value);
CX_API cx_ptr cx_gc_copy_value(void *destination, const void *source,
    const struct cxcore_System_Reflection_TypeInfo *typeInfo);
// Copies a native-layout value after validating its projected data-pointer
// fields against the destination lifetime. Offsets are byte offsets from the
// start of the value and must each identify one cx_ptr-sized field.
CX_API cx_ptr cx_gc_copy_native_value(void *destination, const void *source,
    size_t size, const size_t *pointer_offsets, size_t pointer_count);
// Native SDK records may expose only a partial field projection. Their values
// can be copied to the current stack, but not published to longer-lived storage
// without a complete pointer layout.
CX_API cx_ptr cx_gc_copy_native_value_unknown_layout(void *destination,
    const void *source, size_t size);
CX_API cx_ptr cxcore_System_Nullable_CreateTypedValueStorage(cx_ptr value,
    cx_uint size, const struct cxcore_System_Reflection_TypeInfo *typeInfo);
CX_API void cx_gc_run_blocking(cx_gc_blocking_callback callback, void *context);
CX_API void cx_gc_collect(void);
// Sets the maximum combined bytes in managed heap allocations and reserved
// scoped-arena chunks. Zero disables the limit; returns false if current usage
// already exceeds the requested limit or the value cannot fit in size_t.
CX_API cx_bool cx_gc_set_process_memory_budget(cx_ulong maximum_bytes);

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
CX_API cx_uint cxcore_System_Interop_NativeError_GetLastError(void);
CX_API cx_int cxcore_System_Interop_NativeError_GetErrno(void);
CX_API void cx_interop_require_function(cx_bool nonnull);
CX_API void cx_interop_symbol_require(const void *symbol, size_t size);
CX_API void cx_interop_callback_enter(struct cx_native_callback_scope *scope);
CX_API void cx_interop_callback_leave(struct cx_native_callback_scope *scope);
CX_API void cx_interop_callback_failed(cx_ptr exception);
CX_API void cx_gc_run_native_call(cx_native_call_callback callback, void *context);
CX_API void cx_gc_run_native_call_with_roots(
    cx_native_call_callback callback, void *context,
    const void *root_address, size_t root_size);
CX_API void *cx_interop_context_alloc(size_t size);
CX_API void *cx_interop_context_alloc_aligned(size_t size, size_t alignment);
CX_API void cx_interop_context_free_aligned(void *context);
CX_API struct cxcore_System_Exception *cxcore_System_Interop_NativeCallback_GetLastException(void);
CX_API void cxcore_System_Interop_NativeCallback_ClearLastException(void);

//
// Reflection
//

CX_API const struct cx_interface_impl *
cx_reflection_interfaces(const struct cxcore_System_Reflection_TypeInfo *typeInfo, cx_uint *count);
CX_API const struct cx_reflection_field *
cx_reflection_fields(const struct cxcore_System_Reflection_TypeInfo *typeInfo, cx_uint *count);
CX_API const struct cx_reflection_function *
cx_reflection_functions(const struct cxcore_System_Reflection_TypeInfo *typeInfo, cx_uint *count);
CX_API const struct cx_reflection_function *
cx_reflection_find_function(const struct cxcore_System_Reflection_TypeInfo *typeInfo,
                            const char *name, cx_uint parameterCount,
                            const struct cxcore_System_Reflection_TypeInfo *const *parameterTypes);
CX_API struct cxcore_System_Reflection_FunctionInfo
cx_reflection_get_function_info(const struct cx_reflection_function *function);

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
CX_API cx_bool cx_type_is(const struct cxcore_System_Reflection_TypeInfo *actualType,
                          const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API cx_ptr cx_box_value(const void *value, cx_uint valueSize,
                           const struct cxcore_System_Reflection_TypeInfo *valueType);
CX_API cx_ptr cx_box_nullable_value(const void *value, cx_uint valueSize,
                                    const struct cxcore_System_Reflection_TypeInfo *valueType);
CX_API cx_ptr cx_box_generic_value(const void *value, cx_uint valueSize,
                                   const struct cxcore_System_Reflection_TypeInfo *valueType);
CX_API cx_ptr cx_unbox_value(cx_ptr instance,
                             const struct cxcore_System_Reflection_TypeInfo *valueType);
CX_API cx_ptr cx_unbox_generic_value(cx_ptr instance,
                                     const struct cxcore_System_Reflection_TypeInfo *valueType);
CX_API const struct cxcore_System_Reflection_TypeInfo *cx_boxed_value_type(cx_ptr instance);
CX_API cx_bool cx_is_object(cx_ptr instance,
                            const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API cx_bool cx_is_interface(struct cx_iface_ref source,
                               const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API cx_ptr cx_checked_cast_object(cx_ptr instance,
                                     const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API cx_ptr cx_safe_cast_object(cx_ptr instance,
                                  const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API cx_ptr cx_safe_cast_interface(struct cx_iface_ref source,
                                     const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API struct cx_iface_ref
cx_safe_cast_object_to_interface(cx_ptr instance,
                                 const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API struct cx_iface_ref
cx_safe_cast_interface_to_interface(struct cx_iface_ref source,
                                    const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API cx_ptr cx_safe_unbox_nullable(cx_ptr instance,
                                     const struct cxcore_System_Reflection_TypeInfo *valueType);
CX_API cx_ptr cx_checked_cast_interface(struct cx_iface_ref source,
                                        const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API struct cx_iface_ref
cx_checked_cast_object_to_interface(cx_ptr instance,
                                    const struct cxcore_System_Reflection_TypeInfo *targetType);
CX_API struct cx_iface_ref
cx_checked_cast_interface_to_interface(struct cx_iface_ref source,
                                       const struct cxcore_System_Reflection_TypeInfo *targetType);

#endif // __CX_RUNTIME_H__
