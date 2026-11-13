# Run at configure time and through CTest so source organization cannot depend
# on the host OS or silently omit a newly added implementation.
get_filename_component(_cx_layout_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(GLOB_RECURSE _cx_native_sources RELATIVE "${_cx_layout_root}/platform"
    "${_cx_layout_root}/platform/*.c")
foreach(_cx_source IN LISTS _cx_native_sources)
    string(REGEX REPLACE "^(common|win|linux)/" "" _cx_relative "${_cx_source}")
    if(_cx_relative MATCHES "/")
        string(REGEX REPLACE "\\.c$" ".cx" _cx_counterpart "${_cx_relative}")
        if(NOT EXISTS "${_cx_layout_root}/src/${_cx_counterpart}")
            message(FATAL_ERROR
                "platform/${_cx_source} has no matching src/${_cx_counterpart}. "
                "Place public extern implementations beside their CX counterpart; "
                "shared helpers without a counterpart belong at the platform root.")
        endif()
    endif()

    get_filename_component(_cx_directory "${_cx_source}" DIRECTORY)
    set(_cx_search_directory "${_cx_layout_root}/platform/${_cx_directory}")
    set(_cx_registered FALSE)
    while(NOT _cx_registered)
        set(_cx_source_list "${_cx_search_directory}/CMakeLists.txt")
        if(EXISTS "${_cx_source_list}")
            file(RELATIVE_PATH _cx_listed_name "${_cx_search_directory}"
                "${_cx_layout_root}/platform/${_cx_source}")
            string(REPLACE "." "\\." _cx_filename_pattern "${_cx_listed_name}")
            file(READ "${_cx_source_list}" _cx_sources_text)
            if(_cx_sources_text MATCHES "(^|[ \t\r\n])${_cx_filename_pattern}([ \t\r\n)]|$)")
                set(_cx_registered TRUE)
            endif()
        endif()
        if(_cx_search_directory STREQUAL "${_cx_layout_root}/platform")
            if(NOT _cx_registered)
                message(FATAL_ERROR "platform/${_cx_source} is missing from its CMake source list")
            endif()
            break()
        endif()
        get_filename_component(_cx_search_directory "${_cx_search_directory}" DIRECTORY)
    endwhile()

    if(_cx_source MATCHES "^(win|linux)/")
        if(CMAKE_MATCH_1 STREQUAL "win")
            set(_cx_guard "CX_PLATFORM_WIN")
        else()
            set(_cx_guard "CX_PLATFORM_LINUX")
        endif()
        file(READ "${_cx_layout_root}/platform/${_cx_source}" _cx_source_text)
        if(NOT _cx_source_text MATCHES "#[ \t]*ifdef[ \t]+${_cx_guard}([ \t\r\n]|$)" AND
           NOT _cx_source_text MATCHES "#[ \t]*if[ \t]+defined[ \t]*\\([ \t]*${_cx_guard}[ \t]*\\)")
            message(FATAL_ERROR "platform/${_cx_source} must guard its implementation with ${_cx_guard}")
        endif()
    endif()
endforeach()

file(GLOB_RECURSE _cx_source_lists "${_cx_layout_root}/platform/CMakeLists.txt"
    "${_cx_layout_root}/platform/*/CMakeLists.txt")
foreach(_cx_list IN LISTS _cx_source_lists)
    file(STRINGS "${_cx_list}" _cx_list_lines)
    set(_cx_condition_depth 0)
    foreach(_cx_line IN LISTS _cx_list_lines)
        if(_cx_line MATCHES "^[ \t]*if[ \t]*\\(")
            math(EXPR _cx_condition_depth "${_cx_condition_depth} + 1")
        elseif(_cx_line MATCHES "^[ \t]*endif[ \t]*\\(")
            math(EXPR _cx_condition_depth "${_cx_condition_depth} - 1")
        elseif(_cx_condition_depth GREATER 0 AND
               _cx_line MATCHES "^[ \t]*(target_sources|add_subdirectory)[ \t]*\\(")
            message(FATAL_ERROR
                "${_cx_list}: source selection must be unconditional. "
                "Use platform preprocessor guards; conditions may select libraries, not C files.")
        endif()
    endforeach()
endforeach()
