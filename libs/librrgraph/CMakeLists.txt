cmake_minimum_required(VERSION 3.9)

project("librrgraph")

# Source file and library
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
#file(GLOB_RECURSE VPR_SOURCES src/*/*.cpp)
#file(GLOB_RECURSE VPR_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)
#files_to_dirs(VPR_HEADERS VPR_INCLUDE_DIRS)

#Create the library
add_library(librrgraph STATIC
    ${LIB_HEADERS}
    ${LIB_SOURCES}
)

#target_include_directories(librrgraph PUBLIC  ${PROJECT_SOURCE_DIR}/../../vpr )
#target_link_directories (librrgraph PUBLIC  ${PROJECT_SOURCE_DIR}/../../vpr)
#target_include_directories(librrgraph PUBLIC  ${PROJECT_SOURCE_DIR}/../../vpr/src/base/ )
#target_link_directories (librrgraph PUBLIC  ${PROJECT_SOURCE_DIR}/../../vpr/src/base/)


set_target_properties(librrgraph PROPERTIES PREFIX "") #Avoid extra 'lib' prefix
message(STATUS "**********************************************going through librrgraph*****************************************************")
#Specify link-time dependancies
target_link_libraries(librrgraph 
                      libvtrutil
                      libarchfpga
                      libvpr
                     
)
#target_link_libraries(librrgraph  PUBLIC libvpr)
target_compile_definitions(librrgraph PUBLIC ${INTERCHANGE_SCHEMA_HEADERS})

# Unit tests
#file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
#add_executable(test_rr_graph ${TEST_SOURCES})
#target_link_libraries(test_rr_graph
#                      librrgraph)

# Run unit tests: comment out for now
#add_test(NAME test_rr_graph COMMAND test_rr_graph --use-colour=yes)
