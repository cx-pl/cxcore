target_sources(cxcore 
    PUBLIC
        cx.h
        cxdefs.h
        cxtypes.h)

target_include_directories(cxcore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})
