cmake_minimum_required(VERSION 3.3.0)

include(CMakeParseArguments)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(addprefix var prefix)
    foreach( s ${ARGN} )
        list(APPEND tmp "-I${s}")
    endforeach()
    set(${var} ${tmp} PARENT_SCOPE)
endfunction()

# filter out flags that are not appropriate for the compiler being used
function(target_compile_options_filtered target visibility)
    foreach( flag ${ARGN} )
        if( flag MATCHES "^-D.*" )
            target_compile_options( ${target} ${visibility} ${flag} )
        else()
            check_c_compiler_flag( ${flag} C_COMPILER_SUPPORTS__${flag} )
            if( C_COMPILER_SUPPORTS__${flag} )
                target_compile_options( ${target} ${visibility} $<$<COMPILE_LANGUAGE:C>:${flag}> )
            endif()

            check_cxx_compiler_flag( ${flag} CXX_COMPILER_SUPPORTS__${flag} )
            if( CXX_COMPILER_SUPPORTS__${flag} )
                target_compile_options( ${target} ${visibility} $<$<COMPILE_LANGUAGE:CXX>:${flag}>  )
            endif()
        endif()
    endforeach()
endfunction()

project(abc)

if(READLINE_FOUND MATCHES TRUE)
    addprefix(ABC_READLINE_INCLUDES_FLAGS "-I" ${READLINE_INCLUDE})
    string(REPLACE ";" " " ABC_READLINE_INCLUDES_FLAGS "${ABC_READLINE_INCLUDES_FLAGS}")
    list(APPEND ABC_READLINE_FLAGS "ABC_READLINE_INCLUDES=${ABC_READLINE_INCLUDES_FLAGS}")

    string(REPLACE ";" " " ABC_READLINE_LIBRARIES_FLAGS "${READLINE_LIBRARIES}")
    list(APPEND ABC_READLINE_FLAGS "ABC_READLINE_LIBRARIES=${ABC_READLINE_LIBRARIES_FLAGS}")
elseif(READLINE_FOUND MATCHES FALSE)
    list(APPEND ABC_READLINE_FLAGS "ABC_USE_NO_READLINE=1")
endif()

if(ABC_USE_NAMESPACE)
    set(ABC_USE_NAMESPACE_FLAGS "ABC_USE_NAMESPACE=${ABC_USE_NAMESPACE}")
endif()

if( APPLE )
    set(make_env ${CMAKE_COMMAND} -E env SDKROOT=${CMAKE_OSX_SYSROOT})
endif()

# run make to extract compiler options, linker options and list of source files
execute_process(
  COMMAND
    ${make_env}
    make
        ${ABC_READLINE_FLAGS}
        ${ABC_USE_NAMESPACE_FLAGS}
        ARCHFLAGS_EXE=${CMAKE_CURRENT_BINARY_DIR}/abc_arch_flags_program.exe
        ABC_MAKE_NO_DEPS=1
        CC=${CMAKE_C_COMPILER}
        CXX=${CMAKE_CXX_COMPILER}
        LD=${CMAKE_CXX_COMPILER}
        cmake_info
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE MAKE_OUTPUT
)

# extract options from make output
function(extract_var SEPARATOR DEST_VARIABLE MAKE_OUTPUT)
  string(REGEX MATCH "${SEPARATOR} .* ${SEPARATOR}" TMP "${MAKE_OUTPUT}")
  string(REGEX REPLACE "${SEPARATOR} (.*) ${SEPARATOR}" "\\1" TMP "${TMP}")

  separate_arguments(TMP)

  set(${DEST_VARIABLE} ${TMP} PARENT_SCOPE)
endfunction()

extract_var(SEPARATOR_SRC ABC_SRC ${MAKE_OUTPUT})
extract_var(SEPARATOR_LIBS ABC_LIBS ${MAKE_OUTPUT})
extract_var(SEPARATOR_CFLAGS ABC_CFLAGS ${MAKE_OUTPUT})
extract_var(SEPARATOR_CXXFLAGS ABC_CXXFLAGS ${MAKE_OUTPUT})

if(ABC_USE_NAMESPACE)
    set_source_files_properties(${ABC_SRC} PROPERTIES LANGUAGE CXX)
endif()

function(abc_properties target visibility)
    target_include_directories(${target} ${visibility} ${CMAKE_CURRENT_SOURCE_DIR}/src )
    target_compile_options_filtered(${target} ${visibility} ${ABC_CFLAGS} ${ABC_CXXFLAGS} -Wno-unused-but-set-variable )
    target_link_libraries(${target} ${visibility} ${ABC_LIBS})
endfunction()

set(ABC_MAIN_SRC src/base/main/main.c)
list(REMOVE_ITEM ABC_SRC ${ABC_MAIN_SRC})

add_library(libabc EXCLUDE_FROM_ALL ${ABC_SRC})
abc_properties(libabc PUBLIC)
set_property(TARGET libabc PROPERTY OUTPUT_NAME abc)

add_executable(abc ${ABC_MAIN_SRC})
target_link_libraries(abc PRIVATE libabc)
abc_properties(abc PRIVATE)

add_library(libabc-pic EXCLUDE_FROM_ALL ${ABC_SRC})
abc_properties(libabc-pic PUBLIC)
set_property(TARGET libabc-pic PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libabc-pic PROPERTY OUTPUT_NAME abc-pic)
