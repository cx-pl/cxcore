execute_process(
    COMMAND "${TEST_EXECUTABLE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(result EQUAL 0)
    message(FATAL_ERROR "Throwing null unexpectedly succeeded.")
endif()

if(NOT error MATCHES "Cannot throw a null exception")
    message(FATAL_ERROR "Throwing null did not report its defined diagnostic. stderr: ${error}")
endif()

if(error MATCHES "Uncaught [^:\r\n]+:")
    message(FATAL_ERROR "Throwing null was treated as a managed exception. stderr: ${error}")
endif()
