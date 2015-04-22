cmake_minimum_required(VERSION 2.8)

project("libtatum")

set(EXEC_NAME tatum_sta)
set(EXEC_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/tatum_sta)

set(LIB_NAME tatum)
set(LIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/libtatum)

set(EXEC_SRCS
    ${EXEC_SRC_DIR}/main.cpp)
set(LIB_SRCS
    ${LIB_SRC_DIR}/base/sta_util.cpp
    ${LIB_SRC_DIR}/timing_analyzer/SerialTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelLevelizedCilkTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelNoDependancyCilkTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelLevelizedOpenMPTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelDynamicCilkTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_graph/TimingGraph.cpp
    ${LIB_SRC_DIR}/timing_graph/TimingNode.cpp)

include_directories(${EXEC_SRC_DIR}
                    ${EXEC_SRC_DIR}/parsers
                    ${LIB_SRC_DIR}
                    ${LIB_SRC_DIR}/base
                    ${LIB_SRC_DIR}/timing_analyzer
                    ${LIB_SRC_DIR}/timing_graph)
add_definitions(-DTIME_VEC_WIDTH=1)


#Parser for VPR timing graph
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen_files)
flex_target(vpr_timing_graph_lexer src/tatum_sta/parsers/vpr_timing_graph.l ${CMAKE_CURRENT_BINARY_DIR}/gen_files/vpr_timing_graph.lex.cpp)
bison_target(vpr_timing_graph_parser src/tatum_sta/parsers/vpr_timing_graph.y ${CMAKE_CURRENT_BINARY_DIR}/gen_files/vpr_timing_graph.parse.cpp)
add_flex_bison_dependency(vpr_timing_graph_lexer vpr_timing_graph_parser) 

#Library 
add_library(${LIB_NAME} ${LIB_SRCS})

#Executable
add_executable(${EXEC_NAME}
               ${EXEC_SRCS}
               ${FLEX_vpr_timing_graph_lexer_OUTPUTS} 
               ${BISON_vpr_timing_graph_parser_OUTPUTS})
target_link_libraries(${EXEC_NAME} ${LIB_NAME})

#Compiler flag configuration checks
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
CHECK_CXX_COMPILER_FLAG("-fcilkplus" COMPILER_SUPPORTS_CILK)
CHECK_CXX_COMPILER_FLAG("-lcilkrts" COMPILER_SUPPORTS_CILK_RTS)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

if(COMPILER_SUPPORTS_CILK AND COMPILER_SUPPORTS_CILK_RTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus -lcilkrts")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no Cilk Plus support. Please use a different C++ compiler.")
endif()
