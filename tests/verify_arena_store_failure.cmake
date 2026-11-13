if(NOT DEFINED TEST_ARGUMENT OR "${TEST_ARGUMENT}" STREQUAL "")
    set(TEST_ARGUMENT arena-escape)
endif()

execute_process(
    COMMAND "${TEST_EXECUTABLE}" "${TEST_ARGUMENT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if("${result}" STREQUAL "0")
    message(FATAL_ERROR "scoped reference escaped into durable storage")
endif()
if(NOT "${error}" MATCHES "CX scoped reference cannot escape its arena")
    message(FATAL_ERROR
        "reference escape did not report the deterministic error: ${output}${error}")
endif()
