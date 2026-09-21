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
