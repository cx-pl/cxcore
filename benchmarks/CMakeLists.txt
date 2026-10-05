cmake_minimum_required(VERSION 3.31)
project(cxcore_benchmarks C)

if(NOT DEFINED CXCORE_SOURCE_DIR)
    message(FATAL_ERROR "Set CXCORE_SOURCE_DIR to the cxcore source checkout.")
endif()

option(CX_BENCH_STATIC "Link cxcore statically" OFF)
if(CX_BENCH_STATIC)
    set(CX_BUILD_STATIC ON CACHE BOOL "" FORCE)
else()
    set(CX_BUILD_STATIC OFF CACHE BOOL "" FORCE)
endif()
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("${CXCORE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/cxcore")

add_executable(cxcore_benchmarks cxcore_benchmarks.c)
target_link_libraries(cxcore_benchmarks PRIVATE cxcore)
target_include_directories(cxcore_benchmarks PRIVATE
    "${CXCORE_SOURCE_DIR}/inc"
    "${CXCORE_SOURCE_DIR}/src")
target_compile_features(cxcore_benchmarks PRIVATE c_std_11)
if(MSVC)
    target_compile_options(cxcore_benchmarks PRIVATE /W4 /WX /wd4100)
endif()
set_target_properties(cxcore_benchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
add_custom_command(TARGET cxcore_benchmarks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_benchmarks>)
