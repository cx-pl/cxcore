#ifndef __CX_THREADING_H__
#define __CX_THREADING_H__

#include "cx.h"

typedef CX_FN_136FFEE706DF35B2
cxcore_System_Threading_Tasks_CancellationCallback;

// Memory.WithArenaAsync uses a task-returning callback. This delegate type is
// shared with the generated core module API.
#ifndef CX_FUNCTION_TYPE_CX_FN_6FE0127D873E5D22
#define CX_FUNCTION_TYPE_CX_FN_6FE0127D873E5D22
struct CX_FN_6FE0127D873E5D22_invocation;
typedef struct CX_FN_6FE0127D873E5D22 {
    struct cxcore_System_Threading_Tasks_TaskHandle *(*__function)(void *__environment);
    void *__environment;
    cx_ptr __target;
    const struct cx_reflection_function *__method;
    struct CX_FN_6FE0127D873E5D22_invocation *__invocation_list;
} CX_FN_6FE0127D873E5D22;
#endif

CX_TYPE_DEF(cxcore_System_Threading_Thread) { cx_ptr _state; };
CX_TYPE_DEF(cxcore_System_Threading_Mutex) { cx_ptr _state; };
CX_TYPE_DEF(cxcore_System_Threading_AtomicInt) { cx_ptr _state; };
CX_TYPE_DEF(cxcore_System_Threading_Tasks_CancellationTokenSource) {
    struct cxcore_System_Object __base;
    cx_bool _isCancellationRequested;
    struct cxcore_System_Collections_Array *_callbacks;
    struct cxcore_System_Collections_Array *_callbackIds;
    struct cxcore_System_Collections_Array *_callbackArenas;
    cx_uint _callbackCount;
    cx_uint _nextCallbackId;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_CancellationToken) {
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *_source;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_CancellationTokenRegistration) {
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *_source;
    cx_uint _registrationId;
};
struct cxcore_System_Threading_Tasks_OperationCanceledException;
struct cxcore_System_Threading_Tasks_ReusableValueTaskSource;
CX_TYPE_DEF(cxcore_System_Threading_Tasks_VoidTask) {
    cx_ptr _value;
    cx_bool _isCompleted;
    cx_bool _isFaulted;
    struct cxcore_System_Exception *_exception;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_Task) {
    struct cxcore_System_Object __base;
    cx_bool _isCompleted;
    cx_bool _isFaulted;
    cx_bool _isCanceled;
    struct cxcore_System_Exception *_exception;
    cx_ptr _continuations;
    cx_ptr _result;
};
/* Open generic ValueTask<T> placeholder used by reflection metadata. */
CX_TYPE_DEF(cxcore_System_Threading_Tasks_ValueTask) {
    struct cxcore_System_Object __base;
    struct cxcore_System_Threading_Tasks_Task *_task;
    struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *_source;
    cx_uint _version;
    cx_bool _hasSource;
    cx_bool _consumed;
    cx_ptr _result;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_ReusableValueTask) {
    struct cxcore_System_Object __base;
    struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *_source;
    cx_uint _version;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState) {
    struct cxcore_System_Object __base;
    cx_action _continuation;
    struct cxcore_System_Object *_schedulerItem;
    cx_ptr _logicalArena;
};
CX_API void cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState_Clear(
    struct cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState *self);
CX_API void cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState___constructor(
    struct cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState *self);
CX_TYPE_DEF(cxcore_System_Threading_Tasks_ReusableValueTaskSource) {
    struct cxcore_System_Object __base;
    cx_uint _version;
    cx_bool _isCompleted;
    cx_bool _isConsumed;
    cx_bool _isRetired;
    cx_uint _pendingCallbacks;
    cx_bool _continuationRegistered;
    struct cxcore_System_Threading_Tasks_ReusableValueTaskContinuationState *_continuationState;
    cx_ptr _nativeWaitState;
    cx_ptr _result;
    struct cxcore_System_Exception *_exception;
    cx_bool _isFaulted;
    cx_bool _isCanceled;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_TaskHandle) {
    struct cxcore_System_Object __base;
    struct cxcore_System_Threading_Tasks_TaskState *_state;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_ValueTaskHandle) {
    struct cxcore_System_Threading_Tasks_TaskHandle *_task;
    cx_bool _hasTask;
    cx_bool _consumed;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_TaskState) {
    struct cxcore_System_Object __base;
    struct cxcore_System_Object *_value;
    cx_bool _isCompleted;
    cx_bool _isFaulted;
    cx_bool _isCanceled;
    struct cxcore_System_Exception *_exception;
    cx_ptr _continuations;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_TaskCompletionSource) {
    struct cxcore_System_Object __base;
    struct cxcore_System_Threading_Tasks_Task *_task;
};
CX_TYPE_DEF(cxcore_System_Threading_Tasks_TaskCompletionSourceVoid) {
    struct cxcore_System_Object __base;
    struct cxcore_System_Threading_Tasks_TaskHandle *_task;
};

CX_API void cxcore_System_Threading_Tasks_TaskScheduler_Post(
    cx_action continuation);
CX_API void cxcore_System_Threading_Tasks_TaskScheduler_PostAfter(
    cx_uint milliseconds,
    cx_action continuation);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskScheduler_RunOne(void);
CX_API cx_uint cxcore_System_Threading_Tasks_TaskScheduler_Run(void);
CX_API struct cxcore_System_Threading_Tasks_CancellationToken
cxcore_System_Threading_Tasks_CancellationToken_None___const_get(void);
CX_API cx_bool cxcore_System_Threading_Tasks_CancellationToken_IsCancellationRequested___const_get(
    const struct cxcore_System_Threading_Tasks_CancellationToken *self);
CX_API void cxcore_System_Threading_Tasks_CancellationToken_ThrowIfCancellationRequested(
    struct cxcore_System_Threading_Tasks_CancellationToken *self);
CX_API struct cxcore_System_Threading_Tasks_CancellationTokenRegistration
cxcore_System_Threading_Tasks_CancellationToken_Register(
    struct cxcore_System_Threading_Tasks_CancellationToken *self,
    cxcore_System_Threading_Tasks_CancellationCallback callback);
CX_API void cxcore_System_Threading_Tasks_CancellationToken___constructor(
    struct cxcore_System_Threading_Tasks_CancellationToken *self,
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *source);
CX_API cx_bool cxcore_System_Threading_Tasks_CancellationTokenSource_IsCancellationRequested___const_get(
    const struct cxcore_System_Threading_Tasks_CancellationTokenSource *self);
CX_API struct cxcore_System_Threading_Tasks_CancellationToken
cxcore_System_Threading_Tasks_CancellationTokenSource_Token___const_get(
    const struct cxcore_System_Threading_Tasks_CancellationTokenSource *self);
CX_API void cxcore_System_Threading_Tasks_CancellationTokenSource_Cancel(
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *self);
CX_API struct cxcore_System_Threading_Tasks_CancellationTokenRegistration
cxcore_System_Threading_Tasks_CancellationTokenSource_Register(
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *self,
    cxcore_System_Threading_Tasks_CancellationCallback callback);
CX_API void cxcore_System_Threading_Tasks_CancellationTokenSource_Unregister(
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *self,
    cx_uint registrationId);
CX_API struct cxcore_System_Threading_Tasks_CancellationTokenRegistration
cxcore_System_Threading_Tasks_CancellationTokenRegistration_None___const_get(void);
CX_API void cxcore_System_Threading_Tasks_CancellationTokenRegistration_Dispose(
    struct cxcore_System_Threading_Tasks_CancellationTokenRegistration *self);
CX_API void cxcore_System_Threading_Tasks_CancellationTokenRegistration___constructor(
    struct cxcore_System_Threading_Tasks_CancellationTokenRegistration *self,
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *source,
    cx_uint registrationId);
CX_API void cxcore_System_Threading_Tasks_CancellationTokenSource___constructor(
    struct cxcore_System_Threading_Tasks_CancellationTokenSource *self);
CX_API struct cxcore_System_Threading_Tasks_CancellationToken
cxcore_System_Threading_Tasks_OperationCanceledException_CancellationToken___const_get(
    const struct cxcore_System_Threading_Tasks_OperationCanceledException *self);
CX_API void cxcore_System_Threading_Tasks_OperationCanceledException___constructor(
    struct cxcore_System_Threading_Tasks_OperationCanceledException *self);
CX_API void cxcore_System_Threading_Tasks_OperationCanceledException___constructor__2(
    struct cxcore_System_Threading_Tasks_OperationCanceledException *self,
    struct cxcore_System_Threading_Tasks_CancellationToken token);
CX_API void cxcore_System_Threading_Tasks_OperationCanceledException_ThrowIfCanceled(
    cx_bool isCanceled,
    struct cxcore_System_Threading_Tasks_CancellationToken token);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_OnCompleted(
    struct cxcore_System_Object *task,
    cx_action continuation);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_RequireAsyncUseAllowed(void);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_WithDurable(cx_action action);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_RequireCancellationRegistrationAllowed(void);
CX_API cx_ptr cxcore_System_Threading_Tasks_TaskRuntime_CaptureCancellationCallbackArena(void);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_StoreCancellationCallback(
    struct cxcore_System_Collections_Array *callbacks,
    cx_uint index,
    cxcore_System_Threading_Tasks_CancellationCallback callback,
    cx_ptr logicalArena);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_InvokeCancellationCallback(
    cxcore_System_Threading_Tasks_CancellationCallback callback,
    cx_ptr logicalArena);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ReleaseCancellationCallbackArena(
    cx_ptr logicalArena);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ThrowIfAlreadyCompleted(
    struct cxcore_System_Object *task);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ScheduleContinuations(
    struct cxcore_System_Object *task);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_MarkFaultObserved(
    struct cxcore_System_Object *task);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_SetFault(
    struct cxcore_System_Object *task,
    struct cxcore_System_Exception *exception);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_SetCanceled(
    struct cxcore_System_Object *task);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskRuntime_ReusableSourceIsCompleted(
    struct cxcore_System_Object *source,
    cx_uint version);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_RegisterReusableSourceContinuation(
    struct cxcore_System_Object *source,
    cx_uint version,
    cx_action continuation);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ScheduleReusableSourceContinuation(
    struct cxcore_System_Object *source,
    cx_uint version);
CX_API cx_bool cxcore_System_Threading_Tasks_ReusableValueTask_IsCompleted___const_get(
    const struct cxcore_System_Threading_Tasks_ReusableValueTask *self);
CX_API void cxcore_System_Threading_Tasks_ReusableValueTaskSource___constructor(
    struct cxcore_System_Threading_Tasks_ReusableValueTaskSource *self);
CX_API void cxcore_System_Threading_Tasks_TaskRuntime_ForwardVoid(
    struct cxcore_System_Object *source,
    struct cxcore_System_Object *destination);
CX_API struct cxcore_System_Object *
cxcore_System_Threading_Tasks_TaskRuntime_GetVoidTask(
    struct cxcore_System_Object *source);

CX_API struct cxcore_System_Threading_Thread
cxcore_System_Threading_Thread_Start(cx_action action);
CX_API void cxcore_System_Threading_Thread___constructor(
    struct cxcore_System_Threading_Thread *self);
CX_API void cxcore_System_Threading_Thread_Join(
    const struct cxcore_System_Threading_Thread *self);
CX_API cx_bool cxcore_System_Threading_Thread_IsAlive___const_get(
    const struct cxcore_System_Threading_Thread *self);
CX_API cx_ulong cxcore_System_Threading_Thread_Id___const_get(
    const struct cxcore_System_Threading_Thread *self);
CX_API cx_ulong cxcore_System_Threading_Thread_CurrentId___const_get(void);
CX_API void cxcore_System_Threading_Thread_Sleep(cx_int milliseconds);
CX_API void cxcore_System_Threading_Thread_Yield(void);

CX_API void cxcore_System_Threading_Mutex___constructor(
    struct cxcore_System_Threading_Mutex *self);
CX_API void cxcore_System_Threading_Mutex_Lock(
    const struct cxcore_System_Threading_Mutex *self);
CX_API void cxcore_System_Threading_Mutex_Unlock(
    const struct cxcore_System_Threading_Mutex *self);

CX_API void cxcore_System_Threading_AtomicInt___constructor(
    struct cxcore_System_Threading_AtomicInt *self, cx_int value);
CX_API void cxcore_System_Threading_AtomicInt___constructor__2(
    struct cxcore_System_Threading_AtomicInt *self);
CX_API cx_int cxcore_System_Threading_AtomicInt_Value___const_get(
    const struct cxcore_System_Threading_AtomicInt *self);
CX_API void cxcore_System_Threading_AtomicInt_Value___set(
    struct cxcore_System_Threading_AtomicInt *self, cx_int value);
CX_API cx_int cxcore_System_Threading_AtomicInt_Increment(
    const struct cxcore_System_Threading_AtomicInt *self);
CX_API cx_int cxcore_System_Threading_AtomicInt_Decrement(
    const struct cxcore_System_Threading_AtomicInt *self);
CX_API cx_int cxcore_System_Threading_AtomicInt_Exchange(
    const struct cxcore_System_Threading_AtomicInt *self, cx_int value);
CX_API cx_int cxcore_System_Threading_AtomicInt_CompareExchange(
    const struct cxcore_System_Threading_AtomicInt *self, cx_int value, cx_int comparand);

CX_API cx_bool cxcore_System_Threading_Tasks_Task_IsCompleted___const_get(
    const struct cxcore_System_Threading_Tasks_Task *self);
CX_API void cxcore_System_Threading_Tasks_Task_OnCompleted(
    struct cxcore_System_Threading_Tasks_Task *self,
    cx_action continuation);
CX_API cx_bool cxcore_System_Threading_Tasks_Task_IsCanceled___const_get(
    const struct cxcore_System_Threading_Tasks_Task *self);
CX_API cx_bool cxcore_System_Threading_Tasks_Task_IsCanceled___const_get(
    const struct cxcore_System_Threading_Tasks_Task *self);
CX_API cx_bool cxcore_System_Threading_Tasks_ValueTask_IsCompleted___const_get(
    const struct cxcore_System_Threading_Tasks_ValueTask *self);
CX_API cx_ptr cxcore_System_Threading_Tasks_ValueTask_GetResult(
    struct cxcore_System_Threading_Tasks_ValueTask *self);
CX_API struct cxcore_System_Threading_Tasks_Task *
cxcore_System_Threading_Tasks_ValueTask_AsTask(
    struct cxcore_System_Threading_Tasks_ValueTask *self);
CX_API void cxcore_System_Threading_Tasks_ValueTask_SetTask(
    struct cxcore_System_Threading_Tasks_ValueTask *self,
    struct cxcore_System_Threading_Tasks_Task *task);
CX_API void cxcore_System_Threading_Tasks_ValueTask_OnCompleted(
    struct cxcore_System_Threading_Tasks_ValueTask *self,
    CX_FN_136FFEE706DF35B2 continuation);
CX_API void cxcore_System_Threading_Tasks_ValueTask___constructor(
    struct cxcore_System_Threading_Tasks_ValueTask *self,
    cx_ptr result);
CX_API void cxcore_System_Threading_Tasks_ValueTask___constructor__2(
    struct cxcore_System_Threading_Tasks_ValueTask *self);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskHandle_IsCompleted___const_get(
    const struct cxcore_System_Threading_Tasks_TaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_TaskHandle_GetResult(
    const struct cxcore_System_Threading_Tasks_TaskHandle *self);
CX_API struct cxcore_System_Threading_Tasks_TaskHandle *
cxcore_System_Threading_Tasks_TaskHandle_Faulted(
    struct cxcore_System_Exception *exception);
CX_API struct cxcore_System_Threading_Tasks_TaskHandle *
cxcore_System_Threading_Tasks_TaskHandle_Delay(cx_uint milliseconds);
CX_API struct cxcore_System_Threading_Tasks_TaskHandle *
cxcore_System_Threading_Tasks_TaskHandle_Delay__2(
    cx_uint milliseconds,
    struct cxcore_System_Threading_Tasks_CancellationToken cancellationToken);
CX_API void cxcore_System_Threading_Tasks_TaskHandle_SetResult(
    struct cxcore_System_Threading_Tasks_TaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_TaskHandle_SetFault(
    struct cxcore_System_Threading_Tasks_TaskHandle *self,
    struct cxcore_System_Exception *exception);
CX_API void cxcore_System_Threading_Tasks_TaskHandle_SetCanceled(
    struct cxcore_System_Threading_Tasks_TaskHandle *self);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskHandle_IsCanceled___const_get(
    const struct cxcore_System_Threading_Tasks_TaskHandle *self);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskHandle_IsCanceled___const_get(
    const struct cxcore_System_Threading_Tasks_TaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_TaskHandle_OnCompleted(
    struct cxcore_System_Threading_Tasks_TaskHandle *self,
    cx_action continuation);
CX_API void cxcore_System_Threading_Tasks_TaskHandle___constructor(
    struct cxcore_System_Threading_Tasks_TaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_TaskHandle___constructor__2(
    struct cxcore_System_Threading_Tasks_TaskHandle *self,
    cx_bool completed);
CX_API cx_bool cxcore_System_Threading_Tasks_ValueTaskHandle_IsCompleted___const_get(
    const struct cxcore_System_Threading_Tasks_ValueTaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_ValueTaskHandle_GetResult(
    struct cxcore_System_Threading_Tasks_ValueTaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_ValueTaskHandle_OnCompleted(
    struct cxcore_System_Threading_Tasks_ValueTaskHandle *self,
    CX_FN_136FFEE706DF35B2 continuation);
CX_API struct cxcore_System_Threading_Tasks_TaskHandle *
cxcore_System_Threading_Tasks_ValueTaskHandle_AsTask(
    struct cxcore_System_Threading_Tasks_ValueTaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_ValueTaskHandle___constructor(
    struct cxcore_System_Threading_Tasks_ValueTaskHandle *self);
CX_API void cxcore_System_Threading_Tasks_ValueTaskHandle___constructor__2(
    struct cxcore_System_Threading_Tasks_ValueTaskHandle *self,
    struct cxcore_System_Threading_Tasks_TaskHandle *task);
CX_API void cxcore_System_Threading_Tasks_Task___constructor(
    struct cxcore_System_Threading_Tasks_Task *self, void *result);
CX_API void cxcore_System_Threading_Tasks_Task___constructor__2(
    struct cxcore_System_Threading_Tasks_Task *self);
CX_API cx_bool cxcore_System_Threading_Tasks_VoidTask_IsCompleted___const_get(
    const struct cxcore_System_Threading_Tasks_VoidTask *self);
CX_API void cxcore_System_Threading_Tasks_VoidTask_GetResult(
    const struct cxcore_System_Threading_Tasks_VoidTask *self);
CX_API void cxcore_System_Threading_Tasks_VoidTask_ThrowIfFaulted(
    cx_bool isFaulted,
    const struct cxcore_System_Exception *exception);
CX_API void cxcore_System_Threading_Tasks_TaskState_ThrowIfNotCompleted(
    cx_bool isCompleted);
CX_API void cxcore_System_Threading_Tasks_TaskState_ThrowIfAlreadyCompleted(
    cx_bool isCompleted);
CX_API void cxcore_System_Threading_Tasks_TaskState_SetResult(
    struct cxcore_System_Threading_Tasks_TaskState *self,
    struct cxcore_System_Object *value);
CX_API void cxcore_System_Threading_Tasks_TaskState_SetFault(
    struct cxcore_System_Threading_Tasks_TaskState *self,
    struct cxcore_System_Exception *exception);
CX_API void cxcore_System_Threading_Tasks_TaskState_RegisterUnobservedFault(
    struct cxcore_System_Threading_Tasks_TaskState *state);
CX_API void cxcore_System_Threading_Tasks_TaskState_MarkFaultObserved(
    struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API void cxcore_System_Threading_Tasks_TaskState_SetCanceled(
    struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API void cxcore_System_Threading_Tasks_TaskState_OnCompleted(
    struct cxcore_System_Threading_Tasks_TaskState *self,
    cx_action continuation);
CX_API void cxcore_System_Threading_Tasks_TaskState_ScheduleContinuations(
    struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API void cxcore_System_Threading_Tasks_TaskState___constructor(
    struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskState_IsCompleted___const_get(
    const struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API struct cxcore_System_Object *
cxcore_System_Threading_Tasks_TaskState_Value___const_get(
    const struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskState_IsFaulted___const_get(
    const struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API cx_bool cxcore_System_Threading_Tasks_TaskState_IsCanceled___const_get(
    const struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API struct cxcore_System_Exception *
cxcore_System_Threading_Tasks_TaskState_Exception___const_get(
    const struct cxcore_System_Threading_Tasks_TaskState *self);
CX_API void cxcore_System_Threading_Tasks_TaskCompletionSource___constructor(
    struct cxcore_System_Threading_Tasks_TaskCompletionSource *self);
CX_API struct cxcore_System_Threading_Tasks_Task *
cxcore_System_Threading_Tasks_TaskCompletionSource_Task___const_get(
    const struct cxcore_System_Threading_Tasks_TaskCompletionSource *self);
CX_API void cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_SetResult(
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *self);
CX_API void cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_SetException(
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *self,
    struct cxcore_System_Exception *exception);
CX_API void cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_SetCanceled(
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *self);
CX_API void cxcore_System_Threading_Tasks_TaskCompletionSourceVoid___constructor(
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *self);
CX_API struct cxcore_System_Threading_Tasks_TaskHandle *
cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_Task___const_get(
    const struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *self);
CX_API struct cxcore_System_Threading_Tasks_VoidTask
cxcore_System_Threading_Tasks_VoidTask_Faulted(
    struct cxcore_System_Exception *exception);
CX_API void cxcore_System_Threading_Tasks_VoidTask___constructor(
    struct cxcore_System_Threading_Tasks_VoidTask *self);

CX_TYPE_DEF(cxcore_System_Threading_Tasks_TaskHandleWhenAllState) {
    struct cxcore_System_Object __base;
    struct cxcore_System_Collections_Array* _children;
    struct cxcore_System_Threading_Tasks_TaskHandle* _completion;
    cx_uint _remaining;
    struct cxcore_System_Exception* _firstException;
    cx_bool _wasCanceled;
};

extern CX_API struct cxcore_System_Threading_Tasks_TaskHandle* cxcore_System_Threading_Tasks_TaskHandle_WhenAll(
    struct cxcore_System_Threading_Tasks_TaskHandle* first,
    struct cxcore_System_Threading_Tasks_TaskHandle* second
);
extern struct cxcore_System_Threading_Tasks_TaskHandle* cxcore_System_Threading_Tasks_TaskHandleWhenAllState_Task___const_get(
    const struct cxcore_System_Threading_Tasks_TaskHandleWhenAllState* __this
);
extern void cxcore_System_Threading_Tasks_TaskHandleWhenAllState___constructor(
    struct cxcore_System_Threading_Tasks_TaskHandleWhenAllState* __this,
    struct cxcore_System_Collections_Array* children
);
extern void cxcore_System_Threading_Tasks_TaskHandleWhenAllState_Start(
    struct cxcore_System_Threading_Tasks_TaskHandleWhenAllState* __this
);
extern void cxcore_System_Threading_Tasks_TaskHandleWhenAllState_Complete(
    struct cxcore_System_Threading_Tasks_TaskHandleWhenAllState* __this,
    cx_uint index
);

extern CX_API struct cxcore_System_Threading_Tasks_TaskHandle* cxcore_System_Threading_Tasks_TaskHandle_WhenAll__2(
    struct cxcore_System_Collections_Array* children
);
extern void cxcore_System_Threading_Tasks_TaskHandleWhenAllState_RegisterChild(
    struct cxcore_System_Threading_Tasks_TaskHandleWhenAllState* __this,
    cx_uint index
);

#endif // __CX_THREADING_H__
