target_sources(cxcore PRIVATE
    platform.c
    stdlib.c)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(cxcore PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

add_subdirectory(System)
