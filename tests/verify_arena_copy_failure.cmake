execute_process(
    COMMAND "${TEST_EXECUTABLE}" "${TEST_ARGUMENT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if("${result}" STREQUAL "0")
    message(FATAL_ERROR "Memory.Copy accessed scoped arena storage")
endif()
if(NOT "${error}" MATCHES "Memory.Copy cannot access scoped arena storage")
    message(FATAL_ERROR
        "raw copy did not report the deterministic error: ${output}${error}")
endif()
