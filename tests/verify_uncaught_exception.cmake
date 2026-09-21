execute_process(
    COMMAND "${TEST_EXECUTABLE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(result EQUAL 0)
    message(FATAL_ERROR "The uncaught-exception process unexpectedly succeeded.")
endif()

if(NOT error MATCHES "Uncaught exception thrown at")
    message(FATAL_ERROR
        "The uncaught-exception diagnostic was not reported. stderr: ${error}")
endif()
