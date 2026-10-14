find_package(Threads REQUIRED)

target_sources(cxcore PRIVATE
    platform.c
    cxexceptions.c)
target_link_libraries(cxcore PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

add_subdirectory(System)
