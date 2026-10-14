#include "../../../platform.h"

#include <string.h>
#include <windows.h>

int cx_platform_native_library_uses_utf16(void) {
    return 1;
}

void *cx_platform_native_library_open(const void *path) {
    return LoadLibraryW((LPCWSTR)path);
}

int cx_platform_native_library_resolve(void *handle, const char *name,
    void *result, size_t result_size) {
    FARPROC symbol = GetProcAddress((HMODULE)handle, name);
    if (symbol == NULL || sizeof(symbol) != result_size) {
        return 0;
    }
    memcpy(result, &symbol, result_size);
    return 1;
}
