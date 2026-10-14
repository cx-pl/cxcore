function(cxcore_add_test_executable target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE cxcore)
    if(WIN32 AND NOT CX_BUILD_STATIC)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:cxcore>
                $<TARGET_FILE_DIR:${target}>)
    endif()
endfunction()

cxcore_add_test_executable(cxcore_runtime_type_tests runtime_type_tests.c)
add_test(NAME cxcore_runtime_type_tests COMMAND cxcore_runtime_type_tests)

cxcore_add_test_executable(cxcore_boxed_value_tests boxed_value_tests.c)
add_test(NAME cxcore_boxed_value_tests COMMAND cxcore_boxed_value_tests)

cxcore_add_test_executable(cxcore_runtime_type_failure_test runtime_type_failure_test.c)
add_test(
    NAME cxcore_runtime_type_failure_test
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_runtime_type_failure_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_runtime_type_failure.cmake)

cxcore_add_test_executable(cxcore_exception_tests exception_tests.c)
add_test(NAME cxcore_exception_tests COMMAND cxcore_exception_tests)

cxcore_add_test_executable(cxcore_uncaught_exception_test uncaught_exception_test.c)
add_test(
    NAME cxcore_uncaught_exception_test
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_uncaught_exception_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_uncaught_exception.cmake)

cxcore_add_test_executable(cxcore_standard_library_tests standard_library_tests.c)
cxcore_add_test_executable(cxcore_wide_string_tests wide_string_tests.c)
add_test(NAME cxcore_wide_string_tests COMMAND cxcore_wide_string_tests)
foreach(library_area IN ITEMS array char memory nullable object reflection string)
    add_test(
        NAME cxcore_${library_area}_tests
        COMMAND cxcore_standard_library_tests ${library_area})
endforeach()

cxcore_add_test_executable(cxcore_memory_allocation_failure_tests memory_allocation_failure_tests.c)
add_test(
    NAME cxcore_memory_allocation_failure_tests
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_memory_allocation_failure_tests>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_allocation_failure.cmake)

cxcore_add_test_executable(cxcore_managed_memory_tests managed_memory_tests.c)
add_test(NAME cxcore_managed_memory_tests COMMAND cxcore_managed_memory_tests)
add_test(NAME cxcore_managed_memory_second_thread
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_MODE=second-thread
        "-DEXPECTED_DIAGNOSTIC=requires cx_gc_attach_thread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_threading_gc_failure.cmake)

cxcore_add_test_executable(cxcore_threading_gc_tests threading_gc_tests.c)
cxcore_add_test_executable(cxcore_runtime_once_tests runtime_once_tests.c)
cxcore_add_test_executable(cxcore_foreign_thread_tests foreign_thread_tests.c)
add_test(NAME cxcore_foreign_thread_tests COMMAND cxcore_foreign_thread_tests)
set_tests_properties(cxcore_foreign_thread_tests PROPERTIES TIMEOUT 30)
foreach(mode IN ITEMS invalid-root detach-handler)
    if(mode STREQUAL "invalid-root")
        set(expected_diagnostic "root handle is not registered")
    else()
        set(expected_diagnostic "active exception handlers")
    endif()
    add_test(NAME cxcore_foreign_thread_${mode}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_foreign_thread_tests>
            -DTEST_MODE=${mode}
            -DEXPECTED_DIAGNOSTIC=${expected_diagnostic}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_threading_gc_failure.cmake)
endforeach()
add_test(NAME cxcore_runtime_once_tests COMMAND cxcore_runtime_once_tests)
set_tests_properties(cxcore_runtime_once_tests PROPERTIES TIMEOUT 30)
add_test(NAME cxcore_threading_gc_tests COMMAND cxcore_threading_gc_tests)
set_tests_properties(cxcore_threading_gc_tests PROPERTIES TIMEOUT 60)
foreach(mode IN ITEMS invalid-detach invalid-blocking-entry entry-after-detach)
    if(mode STREQUAL "invalid-detach")
        set(expected_diagnostic "detach requires an attached thread")
    elseif(mode STREQUAL "invalid-blocking-entry")
        set(expected_diagnostic "forbidden inside a blocking callback")
    else()
        set(expected_diagnostic "requires cx_gc_attach_thread")
    endif()
    add_test(NAME cxcore_threading_gc_${mode}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_threading_gc_tests>
            -DTEST_MODE=${mode}
            -DEXPECTED_DIAGNOSTIC=${expected_diagnostic}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_threading_gc_failure.cmake)
endforeach()

cxcore_add_test_executable(cxcore_datetime_tests datetime_tests.c)
add_test(NAME cxcore_datetime_tests COMMAND cxcore_datetime_tests)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(cxcore_managed_memory_tests PRIVATE Threads::Threads)
    target_link_libraries(cxcore_threading_gc_tests PRIVATE Threads::Threads)
    target_link_libraries(cxcore_runtime_once_tests PRIVATE Threads::Threads)
    target_link_libraries(cxcore_foreign_thread_tests PRIVATE Threads::Threads)
    cxcore_add_test_executable(cxcore_linux_platform_tests linux_platform_tests.c)
    add_test(NAME cxcore_linux_platform_tests COMMAND cxcore_linux_platform_tests)
endif()

cxcore_add_test_executable(cxcore_console_read_tests console_read_tests.c)
add_test(
    NAME cxcore_console_read_tests
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_console_read_tests>
        -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_console_read_tests.cmake)
