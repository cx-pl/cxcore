target_sources(cxcore 
    PRIVATE
        cxcore.h
        cxcore.c
        cxexceptions.c
        cxnullable.c
        cxtype.c)

target_include_directories(cxcore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
