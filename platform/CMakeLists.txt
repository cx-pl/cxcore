target_sources(cxcore 
    PRIVATE
        platform.h)

add_subdirectory(common)
add_subdirectory(win)
add_subdirectory(linux)

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "cxcore does not support ${CMAKE_SYSTEM_NAME} yet")
endif()
