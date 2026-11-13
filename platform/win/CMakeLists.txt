target_sources(cxcore PRIVATE
    platform.c
    stdlib.c)
if(WIN32)
    target_link_libraries(cxcore PRIVATE bcrypt)
endif()
add_subdirectory(System)
