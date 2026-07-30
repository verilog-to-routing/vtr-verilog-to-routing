# https://github.com/povik/yosys-slang/blob/76b83eb5b73ba871797e6db7bc5fed10af380be4/cmake/GitRevision.cmake
function(git_rev_parse output_var source_dir)

    if (NOT DEFINED ${output_var})
        execute_process(
            COMMAND git -C ${source_dir} rev-parse HEAD
            OUTPUT_VARIABLE ${output_var}
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    message(STATUS "Got ${output_var}: ${${output_var}}")

    set(${output_var} ${${output_var}} PARENT_SCOPE)

endfunction()
