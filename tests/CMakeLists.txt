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
add_test(NAME cxcore_runtime_type_failure_test COMMAND cxcore_runtime_type_failure_test)
set_tests_properties(cxcore_runtime_type_failure_test PROPERTIES WILL_FAIL TRUE)

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
add_test(NAME cxcore_standard_library_tests COMMAND cxcore_standard_library_tests)

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
