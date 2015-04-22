cmake_minimum_required(VERSION 2.8)

project("libtatum")

set(tatum_sta_srcs 
    src/main.cpp)
set(libtatum_srcs 
    src/base/sta_util.cpp

    src/timing_analyzer/SerialTimingAnalyzer.cpp
    src/timing_analyzer/ParallelLevelizedCilkTimingAnalyzer.cpp
    src/timing_analyzer/ParallelNoDependancyCilkTimingAnalyzer.cpp
    src/timing_analyzer/ParallelLevelizedOpenMPTimingAnalyzer.cpp
    src/timing_analyzer/ParallelDynamicCilkTimingAnalyzer.cpp

    src/timing_graph/TimingGraph.cpp
    src/timing_graph/TimingNode.cpp)

include_directories(src/base
                    src/parsers
                    src/timing_analyzer
                    src/timing_graph)
add_definitions(-DTIME_VEC_WIDTH=1)


#Parser for VPR timing graph
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen_files)
flex_target(VPR_timing_graph_lexer src/parsers/vpr_timing_graph.l ${CMAKE_CURRENT_BINARY_DIR}/gen_files/vpr_timing_graph.lex.cpp)
bison_target(VPR_timing_graph_parser src/parsers/vpr_timing_graph.y ${CMAKE_CURRENT_BINARY_DIR}/gen_files/vpr_timing_graph.parse.cpp COMILE_FLAGS "-d")
add_flex_bison_dependency(VPR_timing_graph_lexer VPR_timing_graph_parser) 

#Executable
add_executable(tatum_sta 
                ${tatum_sta_srcs} 
                ${libtatum_srcs}
                ${FLEX_VPR_timing_graph_lexer_OUTPUTS} 
                ${BISON_VPR_timing_graph_parser_OUTPUTS})

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
