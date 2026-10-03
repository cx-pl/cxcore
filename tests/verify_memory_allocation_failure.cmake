foreach(allocation_operation IN ITEMS alloc realloc)
    execute_process(
        COMMAND "${TEST_EXECUTABLE}" "${allocation_operation}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error)

    if("${result}" STREQUAL "0")
        message(FATAL_ERROR "${allocation_operation} unexpectedly succeeded")
    endif()
    if(NOT "${error}" MATCHES "Out of memory")
        message(FATAL_ERROR
            "${allocation_operation} did not report the deterministic error: ${output}${error}")
    endif()
endforeach()
