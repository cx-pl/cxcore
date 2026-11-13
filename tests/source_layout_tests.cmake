get_filename_component(_runtime_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_fixtures "${CMAKE_CURRENT_BINARY_DIR}/source-layout-fixtures")

function(layout_fixture name)
    set(root "${_fixtures}/${name}")
    file(MAKE_DIRECTORY "${root}/cmake" "${root}/platform/common/System"
        "${root}/platform/win" "${root}/platform/linux" "${root}/src/System")
    configure_file("${_runtime_root}/cmake/CheckSourceLayout.cmake"
        "${root}/cmake/CheckSourceLayout.cmake" COPYONLY)
    file(WRITE "${root}/src/System/Value.cx" "namespace System; public struct Value {}\n")
    file(WRITE "${root}/platform/common/System/Value.c" "void value(void) {}\n")
    file(WRITE "${root}/platform/common/System/CMakeLists.txt" "target_sources(cxcore PRIVATE Value.c)\n")
    file(WRITE "${root}/platform/CMakeLists.txt"
        "add_subdirectory(common)\nadd_subdirectory(win)\nadd_subdirectory(linux)\n")
    foreach(os IN ITEMS win linux)
        if(os STREQUAL "win")
            set(guard CX_PLATFORM_WIN)
        else()
            set(guard CX_PLATFORM_LINUX)
        endif()
        file(WRITE "${root}/platform/${os}/platform.c"
            "#include \"../platform.h\"\n#ifdef ${guard}\nvoid platform(void) {}\n#endif\n")
        file(WRITE "${root}/platform/${os}/CMakeLists.txt"
            "target_sources(cxcore PRIVATE platform.c)\nif(WIN32)\n  target_link_libraries(cxcore PRIVATE example)\nendif()\n")
    endforeach()
    set(fixture "${root}" PARENT_SCOPE)
endfunction()

function(expect_layout expected_error)
    execute_process(COMMAND "${CMAKE_COMMAND}" -P "${fixture}/cmake/CheckSourceLayout.cmake"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(expected_error STREQUAL "")
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Valid layout was rejected: ${error}")
        endif()
    elseif(result EQUAL 0 OR NOT error MATCHES "${expected_error}")
        message(FATAL_ERROR "Expected '${expected_error}' rejection; got ${result}: ${error}")
    endif()
endfunction()

layout_fixture(valid)
expect_layout("")

layout_fixture(missing_counterpart)
file(WRITE "${fixture}/platform/common/System/Misplaced.c" "void misplaced(void) {}\n")
expect_layout("has no matching src/System/Misplaced.cx")

layout_fixture(missing_registration)
file(WRITE "${fixture}/src/System/Unregistered.cx" "namespace System; public struct Unregistered {}\n")
file(WRITE "${fixture}/platform/common/System/Unregistered.c" "void unregistered(void) {}\n")
expect_layout("missing from its CMake source list")

layout_fixture(missing_guard)
file(WRITE "${fixture}/platform/win/platform.c" "void windows(void) {}\n")
expect_layout("must guard its implementation with CX_PLATFORM_WIN")

layout_fixture(conditional_sources)
file(WRITE "${fixture}/platform/win/CMakeLists.txt"
    "if(WIN32)\n target_sources(cxcore PRIVATE platform.c)\nendif()\n")
expect_layout("source selection must be unconditional")

layout_fixture(conditional_subdirectory)
file(WRITE "${fixture}/platform/CMakeLists.txt"
    "add_subdirectory(common)\nif(WIN32)\n add_subdirectory(win)\nendif()\nadd_subdirectory(linux)\n")
expect_layout("source selection must be unconditional")
