cmake_minimum_required(VERSION 3.9)

project("libyosys")

# Create a target out of the library compilation result
SET(LIB_FILE ${CMAKE_CURRENT_SOURCE_DIR}/libyosys.so)
SET(BINARY_LIB_FILE ${CMAKE_CURRENT_BINARY_DIR}/lib/yosys/libyosys.so)
SET(YOSYS_EXEC ${CMAKE_CURRENT_SOURCE_DIR}/yosys)
SET(BINARY_EXEC_FILE ${CMAKE_CURRENT_BINARY_DIR}/bin/yosys)
SET(YOSYS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

# handle make program with both ninja and unix style
set(MAKE_PROGRAM "$(MAKE)")
# handle cppflags to suppress yosys warning with both ninja and unix style
set(CURRENT_CPPFLAGS "$(CPPFLAGS)-w")
if(${CMAKE_GENERATOR} STREQUAL "Ninja")
set(MAKE_PROGRAM "make")
set(CURRENT_CPPFLAGS "-w")
endif()

# how to build the result of the library
add_custom_command(OUTPUT  ${LIB_FILE} ${BINARY_LIB_FILE} ${YOSYS_EXEC} ${BINARY_EXEC_FILE}
                COMMAND ${MAKE_PROGRAM} -k "ENABLE_LIBYOSYS=1" "ENABLE_ABC=0"
                                        CPPFLAGS=${CURRENT_CPPFLAGS}
                                        -j${CMAKE_BUILD_PARALLEL_LEVEL}
                                        PREFIX=${CMAKE_CURRENT_BINARY_DIR}
                                        > /dev/null
                COMMAND ${MAKE_PROGRAM} install 
                                        PREFIX=${CMAKE_CURRENT_BINARY_DIR}
                                        > /dev/null
                COMMAND ${CMAKE_COMMAND} -E 
                                        make_directory ${CMAKE_CURRENT_BINARY_DIR}/lib/yosys/
                COMMAND ${CMAKE_COMMAND} -E 
                                        copy ${LIB_FILE} ${CMAKE_CURRENT_BINARY_DIR}/lib/yosys/
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# create a target out of the library compilation result
add_custom_target(yosys ALL DEPENDS ${LIB_FILE} ${BINARY_LIB_FILE})

# create an library target out of the library compilation result
add_library(libyosys SHARED IMPORTED GLOBAL)
add_dependencies(libyosys yosys)

# specify where the library is and where to find the headers
set_target_properties(libyosys
    PROPERTIES PREFIX "" #Avoid extra 'lib' prefix
    IMPORTED_LOCATION ${LIB_FILE}
    INTERFACE_INCLUDE_DIRECTORIES ${YOSYS_INCLUDE_DIRS})


install(FILES ${YOSYS_EXEC} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})