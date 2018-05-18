cmake_minimum_required(VERSION 2.8.12)

project("libtatumparse")

#Flex and Bison are used to generate the parser
find_package(BISON REQUIRED 3.0)
find_package(FLEX REQUIRED)

file(GLOB_RECURSE LIB_SOURCES */tatumparse*.cpp)
file(GLOB_RECURSE LIB_HEADERS tatumparse.hpp tatumparse/*.hpp)

#Include directories
set(LIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

#Find the flex and bison input files
file(GLOB LEXER_SOURCES */tatumparse.l)
file(GLOB PARSER_SOURCES */tatumparse.y)

#Make the flex and bison targets
flex_target(TatumLexer ${LEXER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/tatumparse_lexer.gen.cpp
        COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/tatumparse_lexer.gen.h)
bison_target(TatumParser ${PARSER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/tatumparse_parser.gen.cpp)
add_flex_bison_dependency(TatumLexer TatumParser)
#Apply suppression flags to the relevant files, must come after flex/bision
#targets for output vars to be defined
set_source_files_properties(${FLEX_vpr_timing_graph_lexer_OUTPUTS}
                            ${BISON_vpr_timing_graph_parser_OUTPUTS}
                            PROPERTIES COMPILE_FLAGS "${FLEX_BISON_WARN_SUPPRESS_FLAGS}")


#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} ${FLEX_TatumLexer_OUTPUTS} ${BISON_TatumParser_OUTPUT_SOURCE} PROPERTIES LANGUAGE CXX)

#Suppress attribute warnings from bison header
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-Wno-attributes" COMPILER_SUPPORTS_-Wno-attributes)
if(COMPILER_SUPPORTS_-Wno-attributes)
    add_compile_options("-Wno-attributes")
endif()

#Suppress warnings in Flex/Bison generated files
if(FLEX_BISON_WARN_SUPPRESS_FLAGS)
    add_compile_options(${FLEX_BISON_WARN_SUPPRESS_FLAGS})
endif()

#Create the library
add_library(libtatumparse STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
             ${FLEX_TatumLexer_OUTPUTS} 
             ${BISON_TatumParser_OUTPUT_SOURCE})

#Need binary dir for flex/bison generated headers
target_include_directories(libtatumparse PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(libtatumparse PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libtatumparse PROPERTIES PREFIX "") #Avoid extra 'lib' prefix
