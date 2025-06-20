cmake_minimum_required(VERSION 3.16)

project(yosys_wrapper NONE)

# Create a target out of the library compilation result
#SET(BINARY_LIB_FILE1 ${CMAKE_CURRENT_BINARY_DIR}/yosys1)
#SET(BINARY_LIB_FILE2 ${CMAKE_CURRENT_BINARY_DIR}/yosys2)
#SET(YOSYS_EXEC ${CMAKE_CURRENT_SOURCE_DIR}/yosys)
#SET(BINARY_EXEC_FILE ${CMAKE_CURRENT_BINARY_DIR}/yosys)
#SET(YOSYS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
SET(YOSYS_SRC_DIR ${CMAKE_SOURCE_DIR}/yosys CACHE PATH "")
SET(YOSYS_BUILD_DIR ${CMAKE_BINARY_DIR}/yosys CACHE PATH "")

set(CURRENT_CPPFLAGS "$(CPPFLAGS)-w")
if(${CMAKE_GENERATOR} STREQUAL "Ninja")
    set(CURRENT_CPPFLAGS "-w")
endif()

#Initialize yosys submodules
execute_process(
 COMMAND git submodule update --init
 WORKING_DIRECTORY ${YOSYS_SRC_DIR}
#  RESULT_VARIABLE _submod_ret
#  OUTPUT_QUIET
#  ERROR_QUIET
)
execute_process(
  COMMAND ${GIT_EXECUTABLE} submodule foreach --recursive git\ submodule\ update\ --init
  WORKING_DIRECTORY ${YOSYS_SRC_DIR}
)
# if(NOT _submod_ret EQUAL 0)
#  message(FATAL_ERROR
#    "Failed to `git submodule update --init --recursive` in ${YOSYS_SRC_DIR}")
# endif()

# how to build the result of the library
add_custom_command(OUTPUT yosys-bin
        #COMMAND ${MAKE_PROGRAM} ENABLE_ABC=0
#            -C ${CMAKE_CURRENT_BINARY_DIR}
#            -f ${CMAKE_CURRENT_SOURCE_DIR}/Makefile #(out-of-tree) build directory
            #PREFIX=${CMAKE_BINARY_DIR}
        COMMAND ${MAKE_PROGRAM} -C ${YOSYS_SRC_DIR}
            ENABLE_ABC=0
            PREFIX=${YOSYS_BUILD_DIR}
        COMMAND 
#            -j${CUSTOM_BUILD_PARALLEL_LEVEL}
            > /dev/null

        #COMMAND ${MAKE_PROGRAM} install ENABLE_ABC=0
#            -C ${CMAKE_CURRENT_BINARY_DIR}
#            -f ${CMAKE_CURRENT_SOURCE_DIR}/Makefile #(out-of-tree) build directory
            #PREFIX=${CMAKE_BINARY_DIR}
         COMMAND ${MAKE_PROGRAM} -C ${YOSYS_SRC_DIR}
            install ENABLE_ABC=0
            PREFIX=${YOSYS_BUILD_DIR}
            > /dev/null

        WORKING_DIRECTORY ${YOSYS_SRC_DIR})

add_custom_target(yosys ALL DEPENDS yosys-bin)

# create a target out of the library compilation result
#add_custom_target(yosys-qwerty ALL DEPENDS ${BINARY_LIB_FILE1})

# specify where the library is and where to find the headers
#set_target_properties(yosys-qwerty
#        PROPERTIES PREFIX "" #Avoid extra 'lib' prefix
#        IMPORTED_LOCATION ${BINARY_LIB_FILE1}
#        INTERFACE_INCLUDE_DIRECTORIES ${YOSYS_INCLUDE_DIRS})


#install(FILES ${BINARY_LIB_FILE1} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
