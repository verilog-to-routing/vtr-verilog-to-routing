function(pmgen_lib)

# Parse args
set(options "")
set(oneValueArgs NAME PMG_HEADER_DIR PMG_FILE)
set(multiValueArgs "")

cmake_parse_arguments(
    ARG  # prefix on the parsed args
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
)

add_library(${ARG_NAME} INTERFACE)

set(INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME})
set(PMGSCRIPT ${CMAKE_SOURCE_DIR}/utils/pmgen.py)
set(HEADER ${INCLUDE_DIR}/${ARG_NAME}.h)

add_custom_command(
    OUTPUT ${HEADER}
    COMMAND mkdir -p ${INCLUDE_DIR}
    COMMAND python3 ${PMGSCRIPT} -o ${HEADER} -p ${ARG_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_PMG_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${PMGSCRIPT} ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_PMG_FILE}
)

add_custom_target(${ARG_NAME}_PMGEN_LIB
    DEPENDS ${HEADER}
)
add_dependencies(${ARG_NAME} ${ARG_NAME}_PMGEN_LIB)
target_include_directories(${ARG_NAME}
    INTERFACE
        ${INCLUDE_DIR}
)

endfunction()
