target_sources(cxcore PRIVATE
    cxabi.c
    cxexceptions.c
    cxgc.h
    cxinterop.c
    cxtext.c
    cxtype.c)

target_include_directories(cxcore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(System)
