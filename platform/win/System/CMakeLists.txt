target_sources(cxcore 
    PRIVATE
        Environment.c
		SystemDateTimeProvider.c)

add_subdirectory(Interop)
add_subdirectory(Threading)
