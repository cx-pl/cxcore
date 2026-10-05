#include <stdio.h>
#include <stdlib.h>

#include "cx.h"

cx_uint cx_runtime_abi_version(void) {
    return CX_RUNTIME_ABI_VERSION;
}

void cx_runtime_require_abi(cx_uint expectedVersion) {
    cx_uint actualVersion = cx_runtime_abi_version();
    if (actualVersion == expectedVersion) {
        return;
    }

    fprintf(stderr,
            "CX runtime ABI mismatch: generated code requires ABI %u, loaded runtime provides ABI %u.\n",
            (unsigned)expectedVersion, (unsigned)actualVersion);
    exit(EXIT_FAILURE);
}
