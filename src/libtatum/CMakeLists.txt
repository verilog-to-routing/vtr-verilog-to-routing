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
set(LIB_TATUM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/base/sta_util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/base/assert.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzers/ParallelLevelizedCilkTimingAnalyzer.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzers/ParallelNoDependancyCilkTimingAnalyzer.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzers/ParallelDynamicCilkTimingAnalyzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_tags/TimingTag.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_tags/TimingTags.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_graph/TimingGraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_graph/TimingConstraints.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_graph/TimingNode.cpp)

set(LIB_TATUM_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_analyzers
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_tags
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_traversals
    ${CMAKE_CURRENT_SOURCE_DIR}/timing_graph)

#set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/base/TimingTags.cpp
                            #PROPERTIES COMPILE_FLAGS "-fno-inline")


#Include directories
include_directories(${LIB_TATUM_INCLUDE_DIRS})

#Required compiler defines. TODO: remove these...
add_definitions(-DTIME_VEC_WIDTH=1)

#
#
# Define the actual build targets
#
#

#Define the library
add_library(${LIB_NAME} ${LIB_TATUM_SRCS})
