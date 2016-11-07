cmake_minimum_required(VERSION 2.8.12)

project("tatumparse")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    #Only set compiler settings if we are not a sub-project
    set(WARN_FLAGS "-Wall -Wextra -Wpedantic -Wcast-qual -Wcast-align -Wshadow -Wformat=2 -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wswitch-default -Wundef -Wunused-variable -Wdisabled-optimization -Wnoexcept -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 ${WARN_FLAGS}") 
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined") 
    set(FLEX_BISON_WARN_SUPPRESS_FLAGS "-Wno-switch-default -Wno-unused-parameter -Wno-missing-declarations")
endif()

#Flex and Bison are used to generate the parser
find_package(BISON REQUIRED 3.0)
find_package(FLEX REQUIRED)

file(GLOB_RECURSE LIB_SOURCES tatumparse*.cpp)
file(GLOB_RECURSE LIB_HEADERS tatumparse*.hpp)

#Include directories
foreach(header ${LIB_HEADERS})
    get_filename_component(incl_dir ${header} DIRECTORY)
    list(APPEND LIB_INCLUDE_DIRS ${incl_dir})
endforeach()
#Remove duplicate include directories
list(REMOVE_DUPLICATES LIB_INCLUDE_DIRS)

#Find the flex and bison input files
file(GLOB LEXER_SOURCES tatumparse.l)
file(GLOB PARSER_SOURCES tatumparse.y)

#Make the flex and bison targets
flex_target(TatumLexer ${LEXER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/tatumparse_lexer.gen.c
        COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/tatumparse_lexer.gen.h)
bison_target(TatumParser ${PARSER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/tatumparse_parser.gen.c)
add_flex_bison_dependency(TatumLexer TatumParser)


#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} ${FLEX_TatumLexer_OUTPUTS} ${BISON_TatumParser_OUTPUT_SOURCE} PROPERTIES LANGUAGE CXX)

#Suppress warnings in Flex/Bison generated files
if(FLEX_BISON_WARN_SUPPRESS_FLAGS)
    set_source_files_properties(${FLEX_TatumLexer_OUTPUTS} ${BISON_TatumParser_OUTPUT_SOURCE}
                                PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})
endif()

#Create the library
add_library(libtatumparse STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
             ${FLEX_TatumLexer_OUTPUTS} 
             ${BISON_TatumParser_OUTPUT_SOURCE})
target_include_directories(libtatumparse PUBLIC ${LIB_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(libtatumparse PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Create the test executable
add_executable(tatumparse_test main.cpp)
target_link_libraries(tatumparse_test libtatumparse)

install(TARGETS libtatumparse tatumparse_test DESTINATION bin)
