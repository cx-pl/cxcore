#include "../../../platform.h"

#include <dlfcn.h>
#include <string.h>

int cx_platform_native_library_uses_utf16(void) {
    return 0;
}

void *cx_platform_native_library_open(const void *path) {
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}

int cx_platform_native_library_resolve(void *handle, const char *name,
    void *result, size_t result_size) {
    void *symbol;
    dlerror();
    symbol = dlsym(handle, name);
    if (symbol == NULL || dlerror() != NULL || sizeof(symbol) != result_size) {
        return 0;
    }
    // POSIX defines this representation for the supported Linux x64 ABI.
    // Copy object representation instead of casting arbitrary void* values.
    memcpy(result, &symbol, result_size);
    return 1;
}
