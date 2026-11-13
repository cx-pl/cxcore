execute_process(
    COMMAND "${TEST_EXECUTABLE}" arena-interface-escape
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if("${result}" STREQUAL "0")
    message(FATAL_ERROR "scoped interface reference escaped into durable storage")
endif()
if(NOT "${error}" MATCHES "CX scoped interface reference cannot escape its arena")
    message(FATAL_ERROR
        "interface escape did not report the deterministic error: ${output}${error}")
endif()
