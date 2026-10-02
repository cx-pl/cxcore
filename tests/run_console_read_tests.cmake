string(REPEAT "a" 300 input_line)
set(input_path "${CMAKE_CURRENT_BINARY_DIR}/cxcore_console_input.txt")
file(WRITE "${input_path}" "${input_line}\nxyz")

execute_process(
    COMMAND "${TEST_EXECUTABLE}"
    INPUT_FILE "${input_path}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
file(REMOVE "${input_path}")

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Console read test failed (${result}): ${output}${error}")
endif()
