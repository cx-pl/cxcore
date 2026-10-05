target_sources(cxcore 
    PRIVATE
        cxcore.h
        cxcore.c
        cxabi.c
        cxexceptions.c
        cxtype.c)

target_include_directories(cxcore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
