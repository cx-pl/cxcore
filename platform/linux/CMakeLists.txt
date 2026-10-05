find_package(Threads REQUIRED)

target_sources(cxcore PRIVATE platform.c)
target_link_libraries(cxcore PRIVATE Threads::Threads)

add_subdirectory(System)
