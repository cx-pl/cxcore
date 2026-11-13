function(cxcore_add_test_executable target)
    add_executable(${target} ${ARGN})
    target_compile_definitions(${target} PRIVATE CXCORE_TESTING)
    target_link_libraries(${target} PRIVATE cxcore)
    if(WIN32 AND NOT CX_BUILD_STATIC)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:cxcore>
                $<TARGET_FILE_DIR:${target}>)
    endif()
endfunction()

add_test(NAME cxcore_source_layout
    COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/CheckSourceLayout.cmake)
add_test(NAME cxcore_source_layout_negative_cases
    COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/tests/source_layout_tests.cmake)

cxcore_add_test_executable(cxcore_runtime_type_tests runtime_type_tests.c)
add_test(NAME cxcore_runtime_type_tests COMMAND cxcore_runtime_type_tests)

cxcore_add_test_executable(cxcore_decimal_tests decimal_tests.c)
add_test(NAME cxcore_decimal_tests COMMAND cxcore_decimal_tests)

cxcore_add_test_executable(cxcore_math_tests math_tests.c)
add_test(NAME cxcore_math_tests COMMAND cxcore_math_tests)

cxcore_add_test_executable(cxcore_uuid_tests uuid_tests.c)
add_test(NAME cxcore_uuid_tests COMMAND cxcore_uuid_tests)
cxcore_add_test_executable(cxcore_numeric_parsing_tests numeric_parsing_tests.c)
add_test(NAME cxcore_numeric_parsing_tests COMMAND cxcore_numeric_parsing_tests)
cxcore_add_test_executable(cxcore_string_builder_tests string_builder_tests.c)
add_test(NAME cxcore_string_builder_tests COMMAND cxcore_string_builder_tests)

cxcore_add_test_executable(cxcore_boxed_value_tests boxed_value_tests.c)
add_test(NAME cxcore_boxed_value_tests COMMAND cxcore_boxed_value_tests)

cxcore_add_test_executable(cxcore_runtime_type_failure_test runtime_type_failure_test.c)
add_test(
    NAME cxcore_runtime_type_failure_test
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_runtime_type_failure_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_runtime_type_failure.cmake)

cxcore_add_test_executable(cxcore_exception_tests exception_tests.c)
add_test(NAME cxcore_exception_tests COMMAND cxcore_exception_tests)

cxcore_add_test_executable(
    cxcore_exception_constructor_store_failure_test
    exception_constructor_store_failure_test.c)
add_test(
    NAME cxcore_exception_constructor_rejects_scoped_inner
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_exception_constructor_store_failure_test>
        -DTEST_ARGUMENT=constructor-scoped-inner
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)

cxcore_add_test_executable(
    cxcore_task_constructor_store_failure_test
    task_constructor_store_failure_test.c)
add_test(
    NAME cxcore_task_constructor_rejects_scoped_result
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_task_constructor_store_failure_test>
        -DTEST_ARGUMENT=constructor-scoped-result
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)

cxcore_add_test_executable(cxcore_uncaught_exception_test uncaught_exception_test.c)
add_test(
    NAME cxcore_uncaught_exception_test
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_uncaught_exception_test>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_uncaught_exception.cmake)

cxcore_add_test_executable(cxcore_standard_library_tests standard_library_tests.c)
cxcore_add_test_executable(cxcore_wide_string_tests wide_string_tests.c)
add_test(NAME cxcore_wide_string_tests COMMAND cxcore_wide_string_tests)
foreach(library_area IN ITEMS array char memory nullable object reflection string)
    add_test(
        NAME cxcore_${library_area}_tests
        COMMAND cxcore_standard_library_tests ${library_area})
endforeach()

cxcore_add_test_executable(cxcore_memory_allocation_failure_tests memory_allocation_failure_tests.c)
add_test(
    NAME cxcore_memory_allocation_failure_tests
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_memory_allocation_failure_tests>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_allocation_failure.cmake)

cxcore_add_test_executable(cxcore_managed_memory_tests managed_memory_tests.c)
add_test(NAME cxcore_managed_memory_tests COMMAND cxcore_managed_memory_tests)
add_test(NAME cxcore_memory_budget_combines_heap_and_arena
    COMMAND cxcore_managed_memory_tests memory-budget)
add_test(NAME cxcore_memory_budget_rejects_arena_reservation
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=memory-budget-arena-overflow
        -DEXPECTED_DIAGNOSTIC=Out of memory
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_memory_budget_rejects_heap_growth
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=memory-budget-heap-overflow
        -DEXPECTED_DIAGNOSTIC=Out of memory
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_exception_unwinds_scoped_contexts
    COMMAND cxcore_managed_memory_tests arena-exception-unwinds-scopes)
add_test(NAME cxcore_arena_callback_exception_unwinds
    COMMAND cxcore_managed_memory_tests arena-callback-exception-unwinds)
add_test(NAME cxcore_arena_zero_initial_growth
    COMMAND cxcore_managed_memory_tests arena-zero-initial-growth)
add_test(NAME cxcore_arena_reset_zeroes_reused_allocation
    COMMAND cxcore_managed_memory_tests arena-reset-zeroes-reused-allocation)
add_test(NAME cxcore_arena_fixed_capacity_metadata_is_bounded
    COMMAND cxcore_managed_memory_tests arena-fixed-metadata-bounded)
add_test(NAME cxcore_bounded_arena_avoids_growth_heap_fallback_and_collection
    COMMAND cxcore_managed_memory_tests arena-bounded-no-fallback)
add_test(NAME cxcore_arena_array_allocation_follows_owner
    COMMAND cxcore_managed_memory_tests arena-array-allocation-follows-owner)
add_test(NAME cxcore_arena_object_allocation_follows_owner_during_durable_override
    COMMAND cxcore_managed_memory_tests arena-object-allocation-follows-owner)
add_test(NAME cxcore_arena_string_builder_growth_follows_owner
    COMMAND cxcore_managed_memory_tests arena-string-builder-growth-follows-owner)
add_test(NAME cxcore_arena_string_builder_to_string_follows_context
    COMMAND cxcore_managed_memory_tests arena-string-builder-to-string-follows-context)
add_test(NAME cxcore_arena_native_library_wrapper_is_durable
    COMMAND cxcore_managed_memory_tests arena-native-library-wrapper-is-durable)
add_test(NAME cxcore_arena_string_buffers_follow_owner
    COMMAND cxcore_managed_memory_tests arena-string-buffers-follow-owner)
add_test(NAME cxcore_arena_array_rejects_padding_owner
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-array-rejects-padding-owner
        "-DEXPECTED_DIAGNOSTIC=CX allocation owner does not name a live arena object"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_thread_start_rejects_scoped_publication
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-thread-start
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_uncaught_exception.cmake)
add_test(NAME cxcore_thread_start_uses_durable_override
    COMMAND cxcore_managed_memory_tests arena-durable-thread-start)
add_test(NAME cxcore_arena_fixed_capacity_overflow
    COMMAND cxcore_managed_memory_tests arena-overflow)
add_test(NAME cxcore_arena_public_callback_scope
    COMMAND cxcore_managed_memory_tests arena-public-callback)
add_test(NAME cxcore_arena_gc_retains_cycle_from_active_arena
    COMMAND cxcore_managed_memory_tests arena-cycle-root)
add_test(NAME cxcore_arena_destroy_removes_gc_registration
    COMMAND cxcore_managed_memory_tests arena-destroy-removes-gc-registration)
add_test(NAME cxcore_arena_gc_retains_string_array_and_boxed_roots
    COMMAND cxcore_managed_memory_tests arena-runtime-object-roots)
add_test(NAME cxcore_arena_logical_retention_roots_inactive_arena
    COMMAND cxcore_managed_memory_tests arena-logical-retention)
add_test(NAME cxcore_arena_logical_reset_rejects_live_owner
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-logical-reset-rejects-live-owner
        "-DEXPECTED_DIAGNOSTIC=CX arena cannot be reset while active or logically retained"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_logical_destroy_rejects_live_owner
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-logical-destroy-rejects-live-owner
        "-DEXPECTED_DIAGNOSTIC=CX arena cannot be destroyed while active or logically retained"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_concurrent_reset_rejects_active_scope
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-concurrent-reset-rejects-active-scope
        "-DEXPECTED_DIAGNOSTIC=CX arena cannot be reset while active or logically retained"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_concurrent_destroy_rejects_active_scope
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-concurrent-destroy-rejects-active-scope
        "-DEXPECTED_DIAGNOSTIC=CX arena cannot be destroyed while active or logically retained"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_public_api_reflection
    COMMAND cxcore_managed_memory_tests arena-api-reflection)
add_test(NAME cxcore_arena_constructed_type_info_stack_reference
    COMMAND cxcore_managed_memory_tests arena-type-info-array)
add_test(NAME cxcore_arena_type_info_native_pointer_store_rejects_durable_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-type-info-native-store-escape
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_module_info_stack_result_references
    COMMAND cxcore_managed_memory_tests arena-module-info-result)
add_test(NAME cxcore_arena_function_info_stack_result_references
    COMMAND cxcore_managed_memory_tests arena-function-info-result)
add_test(NAME cxcore_arena_durable_construction_context
    COMMAND cxcore_managed_memory_tests arena-durable-context)
add_test(NAME cxcore_arena_rejects_durable_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_managed_reference_store_rejects_durable_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-managed-reference-store-escape
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_managed_reference_store_allows_same_arena
    COMMAND cxcore_managed_memory_tests arena-managed-reference-store-allows-same-arena)
add_test(NAME cxcore_arena_boxed_type_info_store_rejects_durable_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-boxed-type-info-store-escape
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_boxed_type_info_store_allows_same_arena
    COMMAND cxcore_managed_memory_tests arena-boxed-type-info-store-allows-same-arena)
add_test(NAME cxcore_arena_rejects_stack_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-stack-escape
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_allows_nested_stack_local
    COMMAND cxcore_managed_memory_tests arena-stack-nested-local)
add_test(NAME cxcore_arena_nested_reference_store_ordering
    COMMAND cxcore_managed_memory_tests arena-nested-reference-store-ordering)
add_test(NAME cxcore_arena_callback_rejects_outer_stack_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-callback-outer-stack-escape
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_rejects_runtime_buffer_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-string-store
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_rejects_interface_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_interface_failure.cmake)
add_test(NAME cxcore_arena_rejects_typed_aggregate_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-typed-copy-escape
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_store_failure.cmake)
add_test(NAME cxcore_arena_rejects_nullable_return_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-nullable-return-escape
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_rejects_raw_copy
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-copy
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_copy_failure.cmake)
add_test(NAME cxcore_arena_rejects_raw_copy_from
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-copy-source
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_arena_copy_failure.cmake)
add_test(NAME cxcore_arena_rejects_raw_copy_reference_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-copy-reference-escape
        "-DEXPECTED_DIAGNOSTIC=Memory.Copy cannot publish a scoped reference"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_rejects_raw_copy_padding_reference_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-copy-padding-reference-escape
        "-DEXPECTED_DIAGNOSTIC=Memory.Copy cannot publish a scoped reference"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_rejects_raw_copy_superseded_reference_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-copy-superseded-reference-escape
        "-DEXPECTED_DIAGNOSTIC=Memory.Copy cannot publish a scoped reference"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_rejects_raw_copy_unaligned_reference_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-copy-unaligned-reference-escape
        "-DEXPECTED_DIAGNOSTIC=Memory.Copy cannot publish a scoped reference"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_allows_raw_copy_of_durable_scalars
    COMMAND cxcore_managed_memory_tests arena-copy-durable-scalars)
add_test(NAME cxcore_arena_native_layout_copy_allows_same_arena_reference
    COMMAND cxcore_managed_memory_tests arena-native-value-copy-local)
add_test(NAME cxcore_arena_native_layout_copy_validates_nested_array_offsets
    COMMAND cxcore_managed_memory_tests arena-native-nested-array-copy-local)
add_test(NAME cxcore_arena_reflected_native_value_copy_allows_same_arena_reference
    COMMAND cxcore_managed_memory_tests arena-reflected-native-copy-local)
add_test(NAME cxcore_typed_copy_treats_native_function_fields_as_raw_bytes
    COMMAND cxcore_managed_memory_tests typed-copy-native-function-field)
add_test(NAME cxcore_typed_copy_still_rejects_arena_reference_with_native_function_field
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=typed-copy-native-function-field-escape
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_reflected_native_value_copy_rejects_durable_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-reflected-native-copy-escape
        "-DEXPECTED_DIAGNOSTIC=CX scoped or stack reference cannot be copied into this native value destination"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_incomplete_value_layout_copies_to_stack
    COMMAND cxcore_managed_memory_tests arena-reflected-unknown-copy-stack)
add_test(NAME cxcore_arena_incomplete_value_layout_rejects_durable_copy
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-reflected-unknown-copy-escape
        "-DEXPECTED_DIAGNOSTIC=CX native value with an unknown pointer layout cannot be copied outside the current stack"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_native_layout_copy_rejects_durable_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-native-value-copy-escape
        "-DEXPECTED_DIAGNOSTIC=CX scoped or stack reference cannot be copied into this native value destination"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_unknown_native_layout_copy_allows_scoped_stack_value
    COMMAND cxcore_managed_memory_tests arena-native-unknown-copy-stack)
add_test(NAME cxcore_arena_unknown_native_layout_copy_rejects_durable_destination
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-native-unknown-copy-escape
        "-DEXPECTED_DIAGNOSTIC=CX native value with an unknown pointer layout cannot be copied outside the current stack"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_rejects_memory_free
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-memory-free
        "-DEXPECTED_DIAGNOSTIC=Memory.Free cannot release scoped arena storage"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_routes_allocations_and_durable_override
    COMMAND cxcore_managed_memory_tests arena-memory-alloc-context)
add_test(NAME cxcore_arena_numeric_parser_uses_native_scratch
    COMMAND cxcore_managed_memory_tests arena-numeric-scratch)
add_test(NAME cxcore_arena_runtime_once_uses_durable_context
    COMMAND cxcore_managed_memory_tests arena-runtime-once-durable)
add_test(NAME cxcore_arena_runtime_once_publishes_durable_static
    COMMAND cxcore_managed_memory_tests arena-runtime-once-publishes-durable-static)
add_test(NAME cxcore_runtime_once_rejects_scoped_failure
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-runtime-once-scoped-failure
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_reallocates_within_owner
    COMMAND cxcore_managed_memory_tests arena-memory-realloc)
add_test(NAME cxcore_managed_reference_store_checks_extent
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=managed-store-crosses-boundary
        "-DEXPECTED_DIAGNOSTIC=CX managed reference store exceeds its allocation"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_managed_reference_store_rejects_prefix_overlap
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=managed-store-overlaps-start
        "-DEXPECTED_DIAGNOSTIC=CX managed reference store exceeds its allocation"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_managed_reference_store_rejects_stack_source
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=managed-store-stack-source
        "-DEXPECTED_DIAGNOSTIC=CX stack reference cannot be stored in managed storage"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_managed_reference_store_allows_native_static_source
    COMMAND cxcore_managed_memory_tests managed-store-native-static-source)
add_test(NAME cxcore_managed_native_pointer_store_allows_external_source
    COMMAND cxcore_managed_memory_tests managed-store-native-pointer)
add_test(NAME cxcore_static_root_registry_index
    COMMAND cxcore_managed_memory_tests static-root-registry-index)
add_test(NAME cxcore_static_root_registry_index_allocation_failure
    COMMAND cxcore_managed_memory_tests static-root-registry-index-allocation-failure)
add_test(NAME cxcore_managed_reference_durability_rejects_foreign_stack
    COMMAND cxcore_managed_memory_tests durable-rejects-foreign-stack)
add_test(NAME cxcore_managed_interface_store_rejects_stack_source
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=managed-interface-stack-source
        "-DEXPECTED_DIAGNOSTIC=CX stack reference cannot be stored in managed storage"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_reference_store_rejects_extent_overlap
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-store-overlaps-extents
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_reference_store_rejects_interior_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-interior-reference-escape
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_object_root_rejects_scoped_reference
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-object-root-escape
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_durable_object_root_keeps_object_alive
    COMMAND cxcore_managed_memory_tests durable-object-root)
add_test(NAME cxcore_arena_reference_store_allows_interior_within_scope
    COMMAND cxcore_managed_memory_tests arena-interior-reference-local)
add_test(NAME cxcore_arena_gc_retains_interior_pointer_from_active_arena
    COMMAND cxcore_managed_memory_tests arena-retains-interior-pointer)
add_test(NAME cxcore_arena_locked_reference_store_allows_same_arena
    COMMAND cxcore_managed_memory_tests arena-locked-store-allows-same-arena)
add_test(NAME cxcore_arena_locked_reference_store_rejects_durable_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=arena-locked-store-rejects-durable-escape
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_reflection_array_follows_source_owner
    COMMAND cxcore_managed_memory_tests arena-reflection-array-follows-owner)
add_test(NAME cxcore_array_type_key_rejects_stack_escape
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_ARGUMENT=array-type-key-stack-escape
        "-DEXPECTED_DIAGNOSTIC=CX stack reference cannot be stored in managed storage"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_arena_runtime_state_follows_owner
    COMMAND cxcore_managed_memory_tests arena-runtime-state-follows-owner)
add_test(NAME cxcore_managed_memory_second_thread
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_managed_memory_tests>
        -DTEST_MODE=second-thread
        "-DEXPECTED_DIAGNOSTIC=requires cx_gc_attach_thread"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_threading_gc_failure.cmake)

cxcore_add_test_executable(cxcore_threading_gc_tests threading_gc_tests.c)
cxcore_add_test_executable(cxcore_native_call_gc_tests native_call_gc_tests.c)
cxcore_add_test_executable(cxcore_task_continuation_tests task_continuation_tests.c)
add_test(NAME cxcore_task_continuation_tests COMMAND cxcore_task_continuation_tests)
add_test(NAME cxcore_reusable_source_native_wait_state_store_barrier
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_task_continuation_tests>
        -DTEST_ARGUMENT=durable-reusable-source-rejects-scoped-native-state
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_task_scheduler_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-scheduler-context)
add_test(NAME cxcore_task_continuation_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-task-continuation)
add_test(NAME cxcore_task_cancellation_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-cancellation-callback)
add_test(NAME cxcore_thread_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-thread-context)
add_test(NAME cxcore_reusable_value_source_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-reusable-value-source-context)
add_test(NAME cxcore_task_scheduler_reusable_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-reusable-scheduler-context)
add_test(NAME cxcore_task_scheduler_timer_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-scheduler-timer-context)
add_test(NAME cxcore_task_scheduler_releases_logical_arena_after_fault
    COMMAND cxcore_task_continuation_tests logical-arena-scheduler-exception-releases)
add_test(NAME cxcore_task_scheduler_shutdown_releases_timer_arena
    COMMAND cxcore_task_continuation_tests logical-arena-scheduler-shutdown-releases)
cxcore_add_test_executable(cxcore_task_event_scheduler_tests task_event_scheduler_tests.c)
add_test(NAME cxcore_task_event_scheduler_tests COMMAND cxcore_task_event_scheduler_tests)

cxcore_add_test_executable(cxcore_task_fault_observation_tests task_fault_observation_tests.c)
add_test(NAME cxcore_task_fault_observed COMMAND cxcore_task_fault_observation_tests observed)
add_test(NAME cxcore_task_fault_unobserved COMMAND cxcore_task_fault_observation_tests unobserved)
add_test(NAME cxcore_task_fault_forwarded_observed COMMAND cxcore_task_fault_observation_tests forwarded-observed)
add_test(NAME cxcore_task_fault_forwarded_unobserved COMMAND cxcore_task_fault_observation_tests forwarded-unobserved)
foreach(mode IN ITEMS nested-run-unhandled wrong-thread-unhandled scoped-post-unhandled scoped-continuation-unhandled retained-continuation-foreign-capture-unhandled retained-thread-foreign-capture-unhandled scoped-cancellation-registration-unhandled scoped-native-callback-registration-unhandled scoped-native-callback-capture-unhandled scoped-native-callback-owner-unhandled retained-native-callback-foreign-capture-unhandled stack-native-callback-owner-unhandled)
    add_test(NAME cxcore_task_scheduler_${mode}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_task_continuation_tests>
            -DTEST_ARGUMENT=${mode}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_uncaught_exception.cmake)
endforeach()
add_test(NAME cxcore_task_scheduler_retained-cancellation-foreign-capture-unhandled
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_task_continuation_tests>
        -DTEST_ARGUMENT=retained-cancellation-foreign-capture-unhandled
        "-DEXPECTED_DIAGNOSTIC=CX scoped reference cannot escape its arena"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_memory_failure.cmake)
add_test(NAME cxcore_durable_native_callback_registration
    COMMAND cxcore_task_continuation_tests durable-native-callback-registration)
add_test(NAME cxcore_native_callback_carries_logical_arena_context
    COMMAND cxcore_task_continuation_tests logical-arena-native-callback-registration)
cxcore_add_test_executable(cxcore_runtime_once_tests runtime_once_tests.c)
cxcore_add_test_executable(cxcore_foreign_thread_tests foreign_thread_tests.c)
add_test(NAME cxcore_foreign_thread_tests COMMAND cxcore_foreign_thread_tests)
set_tests_properties(cxcore_foreign_thread_tests PROPERTIES TIMEOUT 30)
foreach(mode IN ITEMS invalid-root detach-handler)
    if(mode STREQUAL "invalid-root")
        set(expected_diagnostic "root handle is not registered")
    else()
        set(expected_diagnostic "active exception handlers")
    endif()
    add_test(NAME cxcore_foreign_thread_${mode}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_foreign_thread_tests>
            -DTEST_MODE=${mode}
            -DEXPECTED_DIAGNOSTIC=${expected_diagnostic}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_threading_gc_failure.cmake)
endforeach()
add_test(NAME cxcore_runtime_once_tests COMMAND cxcore_runtime_once_tests)
set_tests_properties(cxcore_runtime_once_tests PROPERTIES TIMEOUT 30)
add_test(NAME cxcore_threading_gc_tests COMMAND cxcore_threading_gc_tests)
set_tests_properties(cxcore_threading_gc_tests PROPERTIES TIMEOUT 60)
add_test(NAME cxcore_native_call_context_roots_managed_arguments
    COMMAND cxcore_native_call_gc_tests)
set_tests_properties(cxcore_native_call_context_roots_managed_arguments PROPERTIES TIMEOUT 30)
foreach(mode IN ITEMS invalid-detach invalid-blocking-entry entry-after-detach)
    if(mode STREQUAL "invalid-detach")
        set(expected_diagnostic "detach requires an attached thread")
    elseif(mode STREQUAL "invalid-blocking-entry")
        set(expected_diagnostic "forbidden inside a blocking callback")
    else()
        set(expected_diagnostic "requires cx_gc_attach_thread")
    endif()
    add_test(NAME cxcore_threading_gc_${mode}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_threading_gc_tests>
            -DTEST_MODE=${mode}
            -DEXPECTED_DIAGNOSTIC=${expected_diagnostic}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_threading_gc_failure.cmake)
endforeach()

cxcore_add_test_executable(cxcore_datetime_tests datetime_tests.c)
add_test(NAME cxcore_datetime_tests COMMAND cxcore_datetime_tests)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Threads REQUIRED)
    target_link_libraries(cxcore_math_tests PRIVATE m)
    target_link_libraries(cxcore_managed_memory_tests PRIVATE Threads::Threads)
    target_link_libraries(cxcore_threading_gc_tests PRIVATE Threads::Threads)
    target_link_libraries(cxcore_native_call_gc_tests PRIVATE Threads::Threads)
    target_link_libraries(cxcore_runtime_once_tests PRIVATE Threads::Threads)
    target_link_libraries(cxcore_foreign_thread_tests PRIVATE Threads::Threads)
    cxcore_add_test_executable(cxcore_linux_platform_tests linux_platform_tests.c)
    add_test(NAME cxcore_linux_platform_tests COMMAND cxcore_linux_platform_tests)
endif()

cxcore_add_test_executable(cxcore_console_read_tests console_read_tests.c)
add_test(
    NAME cxcore_console_read_tests
    COMMAND ${CMAKE_COMMAND}
        -DTEST_EXECUTABLE=$<TARGET_FILE:cxcore_console_read_tests>
        -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_console_read_tests.cmake)
