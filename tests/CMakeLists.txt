function(cxcore_add_test_executable target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE cxcore)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:cxcore>
            $<TARGET_FILE_DIR:${target}>)
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
foreach(library_area IN ITEMS array char memory nullable object string)
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
add_test(NAME cxcore_managed_memory_second_thread COMMAND cxcore_managed_memory_tests second-thread)
set_tests_properties(cxcore_managed_memory_second_thread PROPERTIES WILL_FAIL TRUE)

cxcore_add_test_executable(cxcore_datetime_tests datetime_tests.c)
add_test(NAME cxcore_datetime_tests COMMAND cxcore_datetime_tests)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
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
