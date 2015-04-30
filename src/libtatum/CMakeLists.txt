cmake_minimum_required(VERSION 2.8)

project("libtatum")

#
# Compiler flags come from parent
#


#
#
# Build files configuration
#
#

#Source files for the library
set(LIB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/base/sta_util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/TimingTags.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/assert.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzer/SerialTimingAnalyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzer/ParallelLevelizedCilkTimingAnalyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzer/ParallelNoDependancyCilkTimingAnalyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzer/ParallelDynamicCilkTimingAnalyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_graph/TimingGraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_graph/TimingNode.cpp)

set(LIB_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_graph)

if(NOT BOOST_ROOT)
    #Unless overriden default to the local copy of boost
    set(BOOST_ROOT libs/boost_minimal)
    set(Boost_NO_SYSTEM_PATHS ON)
endif()
find_package(Boost 1.54 REQUIRED)
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

#set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzer/SerialTimingAnalyzer.cpp
                            #PROPERTIES COMPILE_FLAGS "-fno-inline")
#set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/base/TimingTags.cpp
                            #PROPERTIES COMPILE_FLAGS "-fno-inline")


#Include directories
include_directories(${LIB_INCLUDE_DIRS})
include_directories(SYSTEM ${Boost_INCLUDE_DIRS}) #Treat boost as a system header to suppress warnings

#Required compiler defines. TODO: remove these...
add_definitions(-DTIME_VEC_WIDTH=1)

#
#
# Define the actual build targets
#
#

#Define the library 
add_library(${LIB_NAME} ${LIB_SRCS})
