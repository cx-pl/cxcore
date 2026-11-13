if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.obj/cxcore.c")
    target_compile_definitions(cxcore PRIVATE CX_USE_GENERATED_CORE)
    target_sources(cxcore
        PRIVATE
            cxcore.h
            .obj/cxcore.c)
else()
    target_sources(cxcore
        PRIVATE
            cxcore.h
            cxcore.c
            cxtext.metadata.c
            cxstringbuilder.metadata.c
            cxmemoryarena.metadata.c
            cxthreading.h
            cxthreading.metadata.c)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.obj/cxcore.generated.c")
        target_sources(cxcore PRIVATE .obj/cxcore.generated.c)
    endif()
endif()

target_include_directories(cxcore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})
