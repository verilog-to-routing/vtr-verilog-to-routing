find_package(Doxygen)

if(NOT DOXYGEN_FOUND)
  message(FATAL_ERROR "Doxygen is needed to build the documentation.")
endif()

set(DOXYFILE_IN Doxyfile.in)
set(DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

# add a target to generate API documentation with Doxygen
add_custom_target(
  doc
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM
)
