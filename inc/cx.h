#ifndef __CX_H__
#define __CX_H__

#include "cxdefs.h"
#include "cxtypes.h"

struct CX_ID_3(cxcore, System, Array);

CX_EXPORT struct CX_ID_3(cxcore, System, Array)* cx_array_new(
    cx_uint length,
    cx_uint elementSize);
CX_EXPORT cx_ptr cx_array_at(
    struct CX_ID_3(cxcore, System, Array)* array,
    cx_uint index,
    cx_uint elementSize);

#endif // __CX_H__
