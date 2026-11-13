target_sources(cxcore PRIVATE
    cxarena.c
    cxarena.h
    cxabi.c
    cxmodule.c
    cxnumeric.c
    cxnumeric.h
    cxgc.h
    cxinterop.c
    cxtext.c
    cxtype.c)

target_include_directories(cxcore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(System)
