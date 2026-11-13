execute_process(
    COMMAND "${TEST_EXECUTABLE}" ${TEST_ARGUMENT}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(result EQUAL 0)
    message(FATAL_ERROR "The uncaught-exception process unexpectedly succeeded.")
endif()

if(NOT error MATCHES "Uncaught [^:\r\n]+:")
    message(FATAL_ERROR
        "The uncaught-exception diagnostic was not reported. stderr: ${error}")
endif()

if(DEFINED EXPECTED_DIAGNOSTIC)
    string(FIND "${error}" "${EXPECTED_DIAGNOSTIC}" diagnostic_index)
    if(diagnostic_index EQUAL -1)
        message(FATAL_ERROR
            "Expected diagnostic '${EXPECTED_DIAGNOSTIC}' was not reported. stderr: ${error}")
    endif()
endif()

if(DEFINED EXPECTED_INNER_DIAGNOSTIC)
    if(NOT error MATCHES "caused by [^\r\n]*${EXPECTED_INNER_DIAGNOSTIC}")
        message(FATAL_ERROR
            "The inner exception was not reported. stderr: ${error}")
    endif()
endif()

if(NOT error MATCHES "Thrown at .+:[0-9]+")
    message(FATAL_ERROR
        "The uncaught-exception source location was not reported. stderr: ${error}")
endif()
