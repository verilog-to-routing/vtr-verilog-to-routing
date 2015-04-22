cmake_minimum_required(VERSION 2.8)

project("tatum")

#
#
# Determine compiler configuration
#
#

#Compiler flag configuration checks
include(CheckCXXCompilerFlag)

#
# We require c++11 support
#
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(WARNING "Could not determine C++11 support for compiler ${CMAKE_CXX_COMPILER}. C++11 support is required.")
endif()

#
# Determine if the compiler has Cilk Plus support
#  This is required for the analyzer to run in parallel mode
#  As of April 2015 the following compilers have full support 
#  for Cilk Plus:
#    * GCC 5+ (Note: GCC 4.9 has partial support, but doesn't support the cilk_for keyword)
#    * Intel
#
CHECK_CXX_COMPILER_FLAG("-fcilkplus" COMPILER_SUPPORTS_CILK)
CHECK_CXX_COMPILER_FLAG("-lcilkrts" COMPILER_SUPPORTS_CILK_RTS)
if(COMPILER_SUPPORTS_CILK AND COMPILER_SUPPORTS_CILK_RTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus -lcilkrts")
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has Cilk Plus support. Build will support parallel execution.")
else()
    message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no Cilk Plus support. Build will support only serial execution.")
endif()

#
# Debug/Opt flags
#
#Support for optimization that don't interfere with debugging?
CHECK_CXX_COMPILER_FLAG("-Og" COMPILER_SUPPORTS_Og)
if(COMPILER_SUPPORTS_Og)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
endif()

#Support for including non-inlined versions of inlined functions (useful for interactive debugging)
CHECK_CXX_COMPILER_FLAG("-fkeep-inline-functions" COMPILER_SUPPORTS_KEEP_INLINE)
if(COMPILER_SUPPORTS_KEEP_INLINE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fkeep-inline-functions")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fkeep-inline-functions")
endif()

#
# Warning flags
#
set(WARN_FLAGS "") #The actual warning flags to be applied
set(WARN_FLAGS_TO_CHECK  #The flags to check if the compiler supports
    #GCC-like
    "-Wall"                     #Most warnings, typically good
    "-Wextra"                   #Extra warning, usually good
    "-pedantic"                 #Ensure ISO compliance (i.e. no non-standard extensions)
    "-Wcast-qual"               #Warn if cast removes qualifier (e.g. const char* -> char*)
    "-Wcast-align"              #Warn if a cast causes memory alignment changes
    "-Wshadow"                  #Warn if local variable shadows another variable
    "-Wformat=2"                #Sanity checks for printf-like formatting
    "-Wlogical-op"              #Checks for logical op when bit-wise expected
    "-Wmissing-declarations"    #Warn if a global function is defined with no delcaration in a header file
    "-Wmissing-include-dirs"    #Warn if a user include directory is missing
    "-Wredundant-decls"         #Warn if there are overlapping declarations
    #"-Wsign-conversion"         #Warn if a conversion may change the sign
    "-Wswitch-default"          #Warn if a switch has no default
    "-Wundef"                   #Warn if #if() preprocessor refers to an undefined directive
    "-Wno-unused"               #Disable warning on unused parameters
    "-Wunused-variable"         #Warn about variables that are not used
    )

#Check and see if the compiler supports the various warning flags,
#and add valid flags
foreach(flag ${WARN_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${flag})
    if(COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(WARN_FLAGS "${WARN_FLAGS} ${flag}")
    endif()
endforeach(flag)

#Apply the warning flags to all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS}")

#
#
# Build files configuration
#
#

#Demo executable
set(EXEC_NAME tatum_sta)
#Main demo exectuable directory
set(EXEC_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/tatum_sta)

#Library name
set(LIB_NAME tatum)
#Main library source directory
set(LIB_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/libtatum)

#Source files for the demo executable (not generated)
set(EXEC_SRCS
    ${EXEC_SRC_DIR}/main.cpp)

#Source files for the library
set(LIB_SRCS
    ${LIB_SRC_DIR}/base/sta_util.cpp
    ${LIB_SRC_DIR}/timing_analyzer/SerialTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelLevelizedCilkTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelNoDependancyCilkTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelLevelizedOpenMPTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_analyzer/ParallelDynamicCilkTimingAnalyzer.cpp
    ${LIB_SRC_DIR}/timing_graph/TimingGraph.cpp
    ${LIB_SRC_DIR}/timing_graph/TimingNode.cpp)

set(LEXER_SRC ${EXEC_SRC_DIR}/parsers/vpr_timing_graph.l)
set(PARSER_SRC ${EXEC_SRC_DIR}/parsers/vpr_timing_graph.y)

#Include directories
include_directories(${EXEC_SRC_DIR}
                    ${EXEC_SRC_DIR}/parsers
                    ${LIB_SRC_DIR}
                    ${LIB_SRC_DIR}/base
                    ${LIB_SRC_DIR}/timing_analyzer
                    ${LIB_SRC_DIR}/timing_graph)

#Required compiler defines. TODO: remove these...
add_definitions(-DTIME_VEC_WIDTH=1)

#
# Configure intermediate files
#

#Directory to put the generated files (avoid clogging up main build directory)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen_files)

#The demo executable uses a flex/bision generated
#parser to read VPR timing graph files
#
#We use cmake to find the relevant executables
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

#The flex/bison code is not warning clean so we need to suppress some warnings
set(FLEX_BISON_WARN_SUPPRESS_FLAGS "")
if(COMPILER_SUPPORTS_-Wredundant-decls)
    #Flex/bison generates code with redundant declarations
    set(FLEX_BISON_WARN_SUPPRESS_FLAGS "${FLEX_BISON_WARN_SUPPRESS_FLAGS} -Wno-redundant-decls")
endif()
if(COMPILER_SUPPORTS_-Wswitch-default)
    #Flex/bison generates switch statments w/o default cases
    set(FLEX_BISON_WARN_SUPPRESS_FLAGS "${FLEX_BISON_WARN_SUPPRESS_FLAGS} -Wno-switch-default")
endif()


#
#
# Define the actual build targets
#
#

#Generate the lexer and parser
flex_target(vpr_timing_graph_lexer ${LEXER_SRC} 
            ${CMAKE_CURRENT_BINARY_DIR}/gen_files/vpr_timing_graph.lex.cpp)
bison_target(vpr_timing_graph_parser ${PARSER_SRC} 
            ${CMAKE_CURRENT_BINARY_DIR}/gen_files/vpr_timing_graph.parse.cpp)
#Capture lexer/parser dependance
add_flex_bison_dependency(vpr_timing_graph_lexer vpr_timing_graph_parser) 
#Apply suppression flags to the relevant files, must come after flex/bision 
#targets for output vars to be defined
set_source_files_properties(${FLEX_vpr_timing_graph_lexer_OUTPUTS} 
                            ${BISON_vpr_timing_graph_parser_OUTPUTS} 
                            PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})

#Define the library 
add_library(${LIB_NAME} ${LIB_SRCS})

#Define Executable
add_executable(${EXEC_NAME}
               ${EXEC_SRCS}
               ${FLEX_vpr_timing_graph_lexer_OUTPUTS} 
               ${BISON_vpr_timing_graph_parser_OUTPUTS})

#Executable links to the library
target_link_libraries(${EXEC_NAME} ${LIB_NAME})

