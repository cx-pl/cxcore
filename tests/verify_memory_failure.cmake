if(NOT DEFINED TEST_ARGUMENT OR "${TEST_ARGUMENT}" STREQUAL "")
    message(FATAL_ERROR "TEST_ARGUMENT is required")
endif()
if(NOT DEFINED EXPECTED_DIAGNOSTIC OR "${EXPECTED_DIAGNOSTIC}" STREQUAL "")
    message(FATAL_ERROR "EXPECTED_DIAGNOSTIC is required")
endif()

execute_process(
    COMMAND "${TEST_EXECUTABLE}" "${TEST_ARGUMENT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if("${result}" STREQUAL "0")
    message(FATAL_ERROR "memory operation unexpectedly accepted scoped storage")
endif()
if(NOT "${error}" MATCHES "${EXPECTED_DIAGNOSTIC}")
    message(FATAL_ERROR
        "memory operation did not report the deterministic error: ${output}${error}")
endif()
