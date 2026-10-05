target_sources(cxcore 
    PRIVATE
        platform.h)

add_subdirectory(common)

if(WIN32)
    add_subdirectory(win)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(linux)
else()
    message(FATAL_ERROR "cxcore does not support ${CMAKE_SYSTEM_NAME} yet")
endif()
