// Checked-in fallback metadata for System.MemoryArena. The normal build emits
// this metadata as part of the generated cxcore translation unit.

#include "cxcore.h"

extern struct cxcore_System_String __namespace_System;

CX_STRING_DEF(__name_System_MemoryArena, "MemoryArena");

static void cxcore_System_MemoryArena___iface_System_IDisposable___vtable_slot_1(cx_ptr root)
{
    cxcore_System_MemoryArena_Dispose((struct cxcore_System_MemoryArena *)root);
}

CX_BEGIN_VTABLE_DEF(cxcore_System_MemoryArena)
CX_END_VTABLE_DEF;
CX_BEGIN_INTERFACE_VTABLE_DEF(cxcore_System_MemoryArena___iface_System_IDisposable___vtable,
                              cxcore_System_MemoryArena)
CX_VTABLE_ENTRY(cxcore_System_MemoryArena___iface_System_IDisposable___vtable_slot_1)
CX_END_VTABLE_DEF;

static const struct cx_interface_impl cxcore_System_MemoryArena___interfaces[] = {
    { &cxcore_System_IDisposable___typeinfo,
      cxcore_System_MemoryArena___iface_System_IDisposable___vtable },
};

static const struct cx_reflection_field cxcore_System_MemoryArena___reflection_fields[] = {
    { CX_REFLECTION_FLAG_VISIBILITY_PRIVATE | CX_REFLECTION_FLAG_FIELD_POINTER,
      (cx_uint)offsetof(struct cxcore_System_MemoryArena, _nativeArena), CX_NULL,
      "_nativeArena", (cx_uint)sizeof(((struct cxcore_System_MemoryArena *)0)->_nativeArena) },
    { CX_REFLECTION_FLAG_VISIBILITY_PRIVATE,
      (cx_uint)offsetof(struct cxcore_System_MemoryArena, _disposed),
      &cxcore_System_Bool___typeinfo, "_disposed",
      (cx_uint)sizeof(((struct cxcore_System_MemoryArena *)0)->_disposed) },
    { CX_REFLECTION_FLAG_VISIBILITY_PRIVATE,
      (cx_uint)offsetof(struct cxcore_System_MemoryArena, _running),
      &cxcore_System_Bool___typeinfo, "_running",
      (cx_uint)sizeof(((struct cxcore_System_MemoryArena *)0)->_running) },
};

static const struct cx_reflection_parameter cxcore_System_MemoryArena___reflection_params_0[] = {
    { 0, &cxcore_System_UInt___typeinfo, "initialCapacity", CX_NULL },
};
static const struct cx_reflection_parameter cxcore_System_MemoryArena___reflection_params_1[] = {
    { 0, &cxcore_System_UInt___typeinfo, "initialCapacity", CX_NULL },
    { 0, &cxcore_System_UInt___typeinfo, "maximumCapacity", CX_NULL },
};
static const struct cx_reflection_parameter cxcore_System_MemoryArena___reflection_params_2[] = {
    { 0, CX_NULL, "action", CX_NULL },
};
static const struct cx_reflection_function cxcore_System_MemoryArena___reflection_functions[] = {
    { CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_EXTERN |
          CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR,
      CX_REFLECTION_NO_SLOT, &cxcore_System_Void___typeinfo, "MemoryArena",
      cxcore_System_MemoryArena___reflection_params_0, 1 },
    { CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_EXTERN |
          CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR,
      CX_REFLECTION_NO_SLOT, &cxcore_System_Void___typeinfo, "MemoryArena",
      cxcore_System_MemoryArena___reflection_params_1, 2 },
    { CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_EXTERN,
      CX_REFLECTION_NO_SLOT, &cxcore_System_Void___typeinfo, "Run",
      cxcore_System_MemoryArena___reflection_params_2, 1 },
    { CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_EXTERN,
      CX_REFLECTION_NO_SLOT, &cxcore_System_Void___typeinfo, "Dispose", CX_NULL, 0 },
};

static const cx_reflection_invoker cxcore_System_MemoryArena___reflection_invokers[] = {
    CX_NULL, CX_NULL, CX_NULL, CX_NULL,
};

static const struct cx_runtime_type_info cxcore_System_MemoryArena___runtime_type_info = {
    .interfaces = (const struct cx_interface_impl *)cxcore_System_MemoryArena___interfaces,
    .interfaceCount = 1,
    .fields = (const struct cx_reflection_field *)cxcore_System_MemoryArena___reflection_fields,
    .fieldCount = 3,
    .functions = (const struct cx_reflection_function *)cxcore_System_MemoryArena___reflection_functions,
    .functionCount = 4,
    .createInstance = CX_NULL,
    .invokers = (const cx_reflection_invoker *)cxcore_System_MemoryArena___reflection_invokers,
};

struct cxcore_System_Reflection_TypeInfo cxcore_System_MemoryArena___typeinfo = {
    .Hash = 0xEB85E2EFA44A547D,
    ._module = (cx_ptr)&CXCORE___module_info,
    .Flags = CX_REFLECTION_FLAG_FINAL | CX_REFLECTION_FLAG_VISIBILITY_PUBLIC |
             CX_REFLECTION_FLAG_TYPE_CLASS,
    .Size = sizeof(struct cxcore_System_MemoryArena),
    .Name = &__name_System_MemoryArena,
    .Namespace = &__namespace_System,
    .BaseType = { ._obj = &cxcore_System_Object___typeinfo },
    .RuntimeTypeInfo = (cx_ptr)&cxcore_System_MemoryArena___runtime_type_info,
    .GenericArity = 0,
};
