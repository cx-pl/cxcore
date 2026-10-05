execute_process(
    COMMAND "${TEST_EXECUTABLE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if("${result}" STREQUAL "0")
    message(FATAL_ERROR "The invalid-cast process unexpectedly succeeded.")
endif()
