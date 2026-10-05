add_executable(cxcore_runtime_type_tests runtime_type_tests.c)
target_link_libraries(cxcore_runtime_type_tests PRIVATE cxcore)
add_custom_command(TARGET cxcore_runtime_type_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_runtime_type_tests>)
add_test(NAME cxcore_runtime_type_tests COMMAND cxcore_runtime_type_tests)

add_executable(cxcore_runtime_type_failure_test runtime_type_failure_test.c)
target_link_libraries(cxcore_runtime_type_failure_test PRIVATE cxcore)
add_custom_command(TARGET cxcore_runtime_type_failure_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_runtime_type_failure_test>)
add_test(
    NAME cxcore_runtime_type_failure_test
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_runtime_type_failure_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_runtime_type_failure.cmake)

add_executable(cxcore_exception_tests exception_tests.c)
target_link_libraries(cxcore_exception_tests PRIVATE cxcore)
add_custom_command(TARGET cxcore_exception_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_exception_tests>)
add_test(NAME cxcore_exception_tests COMMAND cxcore_exception_tests)

add_executable(cxcore_uncaught_exception_test uncaught_exception_test.c)
target_link_libraries(cxcore_uncaught_exception_test PRIVATE cxcore)
add_custom_command(TARGET cxcore_uncaught_exception_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_uncaught_exception_test>)
add_test(
    NAME cxcore_uncaught_exception_test
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_uncaught_exception_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_uncaught_exception.cmake)

add_executable(cxcore_standard_library_tests standard_library_tests.c)
target_link_libraries(cxcore_standard_library_tests PRIVATE cxcore)
add_custom_command(TARGET cxcore_standard_library_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_standard_library_tests>)
foreach(library_area IN ITEMS array char memory nullable object string)
    add_test(
        NAME cxcore_${library_area}_tests
        COMMAND cxcore_standard_library_tests ${library_area})
endforeach()

add_executable(cxcore_memory_allocation_failure_tests memory_allocation_failure_tests.c)
target_link_libraries(cxcore_memory_allocation_failure_tests PRIVATE cxcore)
add_custom_command(TARGET cxcore_memory_allocation_failure_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_memory_allocation_failure_tests>)
add_test(
    NAME cxcore_memory_allocation_failure_tests
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_memory_allocation_failure_tests>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_allocation_failure.cmake)

add_executable(cxcore_managed_memory_tests managed_memory_tests.c)
target_link_libraries(cxcore_managed_memory_tests PRIVATE cxcore)
add_custom_command(TARGET cxcore_managed_memory_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_managed_memory_tests>)
add_test(NAME cxcore_managed_memory_tests COMMAND cxcore_managed_memory_tests)
add_test(NAME cxcore_managed_memory_second_thread COMMAND cxcore_managed_memory_tests second-thread)
set_tests_properties(cxcore_managed_memory_second_thread PROPERTIES WILL_FAIL TRUE)

add_executable(cxcore_datetime_tests datetime_tests.c)
target_link_libraries(cxcore_datetime_tests PRIVATE cxcore)
add_custom_command(TARGET cxcore_datetime_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_datetime_tests>)
add_test(NAME cxcore_datetime_tests COMMAND cxcore_datetime_tests)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(cxcore_linux_platform_tests linux_platform_tests.c)
    target_link_libraries(cxcore_linux_platform_tests PRIVATE cxcore)
    add_test(NAME cxcore_linux_platform_tests COMMAND cxcore_linux_platform_tests)
endif()

add_executable(cxcore_console_read_tests console_read_tests.c)
target_link_libraries(cxcore_console_read_tests PRIVATE cxcore)
add_custom_command(TARGET cxcore_console_read_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:cxcore>
        $<TARGET_FILE_DIR:cxcore_console_read_tests>)
add_test(
    NAME cxcore_console_read_tests
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_console_read_tests>
        -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_console_read_tests.cmake)
