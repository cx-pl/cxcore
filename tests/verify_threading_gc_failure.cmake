execute_process(COMMAND "${TEST_EXECUTABLE}" "${TEST_MODE}"
    RESULT_VARIABLE result ERROR_VARIABLE diagnostic TIMEOUT 15)
if(NOT result STREQUAL "1")
    message(FATAL_ERROR "Expected exit 1, got ${result}: ${diagnostic}")
endif()
if(NOT diagnostic MATCHES "${EXPECTED_DIAGNOSTIC}")
    message(FATAL_ERROR "Missing diagnostic '${EXPECTED_DIAGNOSTIC}': ${diagnostic}")
endif()
