cmake_minimum_required(VERSION 3.31)
project(cxcore)

set(CXCORE_ABI_VERSION 1 CACHE INTERNAL "CX generated-code ABI version" FORCE)

include(CTest)
include(cmake/CheckSourceLayout.cmake)

if(CX_BUILD_STATIC)
    add_library(cxcore STATIC)
    target_compile_definitions(cxcore PUBLIC CX_STATIC_LINK)
else()
    add_library(cxcore SHARED)
    # A shared cxcore is one compiled runtime module. Its own metadata remains
    # statically initialized; CX_DYNAMIC_MODULE is for consuming project DLLs.
    target_compile_definitions(cxcore PRIVATE CX_STATIC_LINK)
endif()
target_compile_definitions(cxcore PRIVATE CX_CXCORE_BUILD)
add_subdirectory(include)
add_subdirectory(platform)
add_subdirectory(src)
if(BUILD_TESTING)
    target_compile_definitions(cxcore PRIVATE CXCORE_TESTING)
    add_subdirectory(tests)
endif()
