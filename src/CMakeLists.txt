target_sources(cxcore
    PRIVATE
        cxcore.h
        cxcore.c
        cxtext.metadata.c
        cxthreading.h
        cxthreading.metadata.c)

target_include_directories(cxcore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
