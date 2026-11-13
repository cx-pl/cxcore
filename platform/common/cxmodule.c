// Native-only cxcore publishes the runtime types supplied by its checked-in metadata.
#include "../../src/cxcore.h"

#if !defined(CX_USE_GENERATED_CORE)

CX_STRING_DEF(cx_clock_type_name, "MonotonicClock");
CX_STRING_DEF(cx_clock_type_namespace, "System.Diagnostics");
CX_STRING_DEF(cx_gc_type_name, "GC");
CX_STRING_DEF(cx_gc_type_namespace, "System");
static cx_ptr cx_clock_invoke(cx_ptr target, const cx_ptr *arguments, cx_uint count) {
    cx_ulong value;
    (void)target; (void)arguments;
    if (count != 0u) return CX_NULL;
    value = cxcore_System_Diagnostics_MonotonicClock_GetMicroseconds();
    return cx_box_value(&value, sizeof(value), &cxcore_System_ULong___typeinfo);
}
static cx_ptr cx_gc_invoke(cx_ptr target, const cx_ptr *arguments, cx_uint count) {
    (void)target; (void)arguments;
    if (count == 0u) cxcore_System_GC_Collect();
    return CX_NULL;
}
#define CX_STATIC_METHOD_FLAGS (CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_STATIC | CX_REFLECTION_FLAG_EXTERN)
static const struct cx_reflection_function cx_clock_functions[] = {
    { CX_STATIC_METHOD_FLAGS, CX_REFLECTION_NO_SLOT, &cxcore_System_ULong___typeinfo, "GetMicroseconds", CX_NULL, 0u },
};
static const struct cx_reflection_function cx_gc_functions[] = {
    { CX_STATIC_METHOD_FLAGS, CX_REFLECTION_NO_SLOT, &cxcore_System_Void___typeinfo, "Collect", CX_NULL, 0u },
};
#undef CX_STATIC_METHOD_FLAGS
static const cx_reflection_invoker cx_clock_invokers[] = { cx_clock_invoke };
static const cx_reflection_invoker cx_gc_invokers[] = { cx_gc_invoke };
static const struct cx_runtime_type_info cx_clock_runtime = {
    .functions = cx_clock_functions, .functionCount = 1u, .invokers = cx_clock_invokers,
};
static const struct cx_runtime_type_info cx_gc_runtime = {
    .functions = cx_gc_functions, .functionCount = 1u, .invokers = cx_gc_invokers,
};
CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Diagnostics_MonotonicClock___typeinfo = {
    .Hash = UINT64_C(0x3D692FD3F69CD1FB),
    .Flags = CX_REFLECTION_FLAG_STATIC | CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_TYPE_CLASS,
    .Name = &cx_clock_type_name, .Namespace = &cx_clock_type_namespace,
    .RuntimeTypeInfo = (cx_ptr)&cx_clock_runtime, ._module = (cx_ptr)&CXCORE___module_info,
};
CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_GC___typeinfo = {
    .Hash = UINT64_C(0x33AAD1BF40D21492),
    .Flags = CX_REFLECTION_FLAG_STATIC | CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_TYPE_CLASS,
    .Name = &cx_gc_type_name, .Namespace = &cx_gc_type_namespace,
    .RuntimeTypeInfo = (cx_ptr)&cx_gc_runtime, ._module = (cx_ptr)&CXCORE___module_info,
};

extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Annotation___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Bool___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Byte___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Char___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Collections_Array___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Console___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_DateTime___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Decimal___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Double___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_EncodingException___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Environment___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Exception___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Float___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_IDateTimeProvider___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_IDisposable___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_IReader___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_IWriter___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Int___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Interop_NativeBuffer___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Interop_NativeCallbackRegistration___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Interop_NativeLibrary___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Long___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Math___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Memory___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Nullable___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Object___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Random___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_AnnotationInfo___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_FieldInfo___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_FunctionInfo___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_FunctionParamInfo___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_ModuleInfo___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_TypeInfo___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_VersionInfo___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_SByte___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Short___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_String___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_StringBuilder___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_SystemDateTimeProvider___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_AtomicInt___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Mutex___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_CancellationToken___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_CancellationTokenRegistration___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_CancellationTokenSource___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_OperationCanceledException___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_ReusableValueTask___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_ReusableValueTaskSource___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_Task___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_TaskCompletionSource___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_TaskCompletionSourceVoid___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_TaskHandle___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_TaskRuntime___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_TaskScheduler___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_ValueTask___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_ValueTaskHandle___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Tasks_VoidTask___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Threading_Thread___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_TimeSpan___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_UInt___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_ULong___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_UShort___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Uuid___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_Void___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_WChar___typeinfo;
extern CX_API struct cxcore_System_Reflection_TypeInfo cxcore_System_WideString___typeinfo;

static const struct cxcore_System_Reflection_TypeInfo *const cx_core_types[] = {
    &cxcore_System_Annotation___typeinfo,
    &cxcore_System_Bool___typeinfo,
    &cxcore_System_Byte___typeinfo,
    &cxcore_System_Char___typeinfo,
    &cxcore_System_Collections_Array___typeinfo,
    &cxcore_System_Console___typeinfo,
    &cxcore_System_DateTime___typeinfo,
    &cxcore_System_Decimal___typeinfo,
    &cxcore_System_Diagnostics_MonotonicClock___typeinfo,
    &cxcore_System_Double___typeinfo,
    &cxcore_System_EncodingException___typeinfo,
    &cxcore_System_Environment___typeinfo,
    &cxcore_System_Exception___typeinfo,
    &cxcore_System_Float___typeinfo,
    &cxcore_System_GC___typeinfo,
    &cxcore_System_IDateTimeProvider___typeinfo,
    &cxcore_System_IDisposable___typeinfo,
    &cxcore_System_IReader___typeinfo,
    &cxcore_System_IWriter___typeinfo,
    &cxcore_System_Int___typeinfo,
    &cxcore_System_Interop_NativeBuffer___typeinfo,
    &cxcore_System_Interop_NativeCallbackRegistration___typeinfo,
    &cxcore_System_Interop_NativeLibrary___typeinfo,
    &cxcore_System_Long___typeinfo,
    &cxcore_System_Math___typeinfo,
    &cxcore_System_Memory___typeinfo,
    &cxcore_System_Nullable___typeinfo,
    &cxcore_System_Object___typeinfo,
    &cxcore_System_Random___typeinfo,
    &cxcore_System_Reflection_AnnotationInfo___typeinfo,
    &cxcore_System_Reflection_FieldInfo___typeinfo,
    &cxcore_System_Reflection_FunctionInfo___typeinfo,
    &cxcore_System_Reflection_FunctionParamInfo___typeinfo,
    &cxcore_System_Reflection_ModuleInfo___typeinfo,
    &cxcore_System_Reflection_TypeInfo___typeinfo,
    &cxcore_System_Reflection_VersionInfo___typeinfo,
    &cxcore_System_SByte___typeinfo,
    &cxcore_System_Short___typeinfo,
    &cxcore_System_String___typeinfo,
    &cxcore_System_StringBuilder___typeinfo,
    &cxcore_System_SystemDateTimeProvider___typeinfo,
    &cxcore_System_Threading_AtomicInt___typeinfo,
    &cxcore_System_Threading_Mutex___typeinfo,
    &cxcore_System_Threading_Tasks_CancellationToken___typeinfo,
    &cxcore_System_Threading_Tasks_CancellationTokenRegistration___typeinfo,
    &cxcore_System_Threading_Tasks_CancellationTokenSource___typeinfo,
    &cxcore_System_Threading_Tasks_OperationCanceledException___typeinfo,
    &cxcore_System_Threading_Tasks_ReusableValueTask___typeinfo,
    &cxcore_System_Threading_Tasks_ReusableValueTaskSource___typeinfo,
    &cxcore_System_Threading_Tasks_Task___typeinfo,
    &cxcore_System_Threading_Tasks_TaskCompletionSource___typeinfo,
    &cxcore_System_Threading_Tasks_TaskCompletionSourceVoid___typeinfo,
    &cxcore_System_Threading_Tasks_TaskHandle___typeinfo,
    &cxcore_System_Threading_Tasks_TaskRuntime___typeinfo,
    &cxcore_System_Threading_Tasks_TaskScheduler___typeinfo,
    &cxcore_System_Threading_Tasks_ValueTask___typeinfo,
    &cxcore_System_Threading_Tasks_ValueTaskHandle___typeinfo,
    &cxcore_System_Threading_Tasks_VoidTask___typeinfo,
    &cxcore_System_Threading_Thread___typeinfo,
    &cxcore_System_TimeSpan___typeinfo,
    &cxcore_System_UInt___typeinfo,
    &cxcore_System_ULong___typeinfo,
    &cxcore_System_UShort___typeinfo,
    &cxcore_System_Uuid___typeinfo,
    &cxcore_System_Void___typeinfo,
    &cxcore_System_WChar___typeinfo,
    &cxcore_System_WideString___typeinfo,
};

CX_API const struct cx_module_info CXCORE___module_info = {
    .formatVersion = CX_MODULE_METADATA_FORMAT_VERSION,
    .name = "cxcore", .version = "0.1.0",
    .description = "CX Core System Library", .author = "",
    .types = cx_core_types,
    .typeCount = (cx_uint)(sizeof(cx_core_types) / sizeof(cx_core_types[0])),
};

CX_API const struct cx_module_info *__cx_module_info_CXCORE(void) {
    return &CXCORE___module_info;
}
#endif
