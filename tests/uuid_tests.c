#include "cxcore.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "uuid_name_vectors.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif
typedef struct cxcore_System_Uuid uuid;
#define UUID(name) cxcore_System_Uuid_ ## name
static int order(const void *left, const void *right) {
    const uuid *a = left, *b = right;
    if (a->_a != b->_a) return a->_a < b->_a ? -1 : 1;
    if (a->_b != b->_b) return a->_b < b->_b ? -1 : 1;
    if (a->_c != b->_c) return a->_c < b->_c ? -1 : 1;
    return a->_d == b->_d ? 0 : a->_d < b->_d ? -1 : 1;
}
static void text(uuid value, const char *expected) {
    const struct cxcore_System_String *formatted = UUID(ToString)(&value);
    assert(formatted->_length == 36 && memcmp(formatted->_data,expected,36) == 0);
}
static void version(uuid value, unsigned expected) {
    assert(((value._b >> 12) & 15) == expected);
    assert((value._c >> 30) == 2);
    assert(cxcore_System_Uuid_Version___const_get(&value) == expected);
}
#define WORKERS 8
#define PER_WORKER 1000
static uuid concurrent[WORKERS * PER_WORKER];
#if defined(_WIN32)
static DWORD WINAPI worker(void *context)
#else
static void *worker(void *context)
#endif
{
    size_t index = (size_t)(uintptr_t)context;
    for (size_t i = 0; i < PER_WORKER; ++i) {
        uuid value = UUID(NewUuidV7)();
        version(value,7);
        concurrent[index*PER_WORKER+i] = value;
        if (i) assert(order(&concurrent[index*PER_WORKER+i-1],&value) < 0);
    }
    return 0;
}
int main(void) {
    CX_STRING_DEF(example_name,"www.example.com");
    text(UUID(NamespaceDns),"6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    text(UUID(NewUuidV3)(UUID(NamespaceDns),&example_name),"5df41881-3aed-3515-88a7-2f4a814cf09e");
    text(UUID(NewUuidV5)(UUID(NamespaceDns),&example_name),"2ed6657d-e927-568b-95e1-2665a8aea6a2");
    char repeated_name[1000]; memset(repeated_name,'a',sizeof(repeated_name));
    for (unsigned i = 0; i < sizeof(name_vectors)/sizeof(name_vectors[0]); ++i) {
        struct cxcore_System_String name;
        cxcore_System_String___constructor__2(&name,name_vectors[i].length,repeated_name);
        text(UUID(NewUuidV3)(UUID(NamespaceDns),&name),name_vectors[i].v3);
        text(UUID(NewUuidV5)(UUID(NamespaceDns),&name),name_vectors[i].v5);
    }
    uuid value = UUID(NewUuidV1)(); version(value,1); assert(value._c & 0x100);
    value = UUID(NewUuidV2)(1,12345); version(value,2);
    assert(value._a == 12345 && ((value._c >> 16) & 255) == 1 && (value._c & 0x100));
    value = UUID(NewUuidV4)(); version(value,4);
    uuid previous = UUID(NewUuidV6)(); version(previous,6);
    for (int i = 0; i < 1000; ++i) { value = UUID(NewUuidV6)(); assert(order(&previous,&value) < 0); previous = value; }
    value = UUID(NewUuidV7)(); version(value,7);
    value = UUID(NewUuidV8)(); version(value,8);
    value = cxcore_System_Uuid_NewUuidV8__2(0x123456789abcULL,0xdef,0x123456789abcdefULL);
    text(value,"12345678-9abc-8def-8123-456789abcdef");
#if defined(_WIN32)
    HANDLE threads[WORKERS];
    for (size_t i = 0; i < WORKERS; ++i) { threads[i] = CreateThread(NULL,0,worker,(void *)(uintptr_t)i,0,NULL); assert(threads[i]); }
    assert(WaitForMultipleObjects(WORKERS,threads,TRUE,INFINITE) == WAIT_OBJECT_0);
    for (unsigned i = 0; i < WORKERS; ++i) CloseHandle(threads[i]);
#else
    pthread_t threads[WORKERS];
    for (size_t i = 0; i < WORKERS; ++i) assert(pthread_create(&threads[i],NULL,worker,(void *)(uintptr_t)i) == 0);
    for (unsigned i = 0; i < WORKERS; ++i) assert(pthread_join(threads[i],NULL) == 0);
#endif
    qsort(concurrent,WORKERS*PER_WORKER,sizeof(uuid),order);
    for (unsigned i = 1; i < WORKERS*PER_WORKER; ++i) assert(order(&concurrent[i-1],&concurrent[i]) != 0);
    return 0;
}
