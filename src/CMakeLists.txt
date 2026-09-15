target_sources(cxcore 
    PRIVATE
        cxcore.h
        cxcore.c)

target_include_directories(cxcore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
