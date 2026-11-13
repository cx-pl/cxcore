target_sources(cxcore
    PRIVATE
        Environment.c
        Exception.c
        SystemDateTimeProvider.c)

add_subdirectory(Interop)
add_subdirectory(Threading)
