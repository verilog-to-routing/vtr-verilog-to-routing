cmake_minimum_required(VERSION 3.9)
project("libvqm")

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

file(GLOB_RECURSE LIB_SOURCES *.c *.cpp)
file(GLOB_RECURSE LIB_HEADERS *.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Find the flex and bison input files
file(GLOB_RECURSE LEXER_SOURCES *.l)
file(GLOB_RECURSE PARSER_SOURCES *.y)

#Make the flex and bison targets
flex_target(VqmLexer ${LEXER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/vqm_lexer.gen.c
        COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/vqm_lexer.gen.h)
bison_target(VqmParser ${PARSER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/vqm_parser.gen.c)
add_flex_bison_dependency(VqmLexer VqmParser)


#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} ${FLEX_VqmLexer_OUTPUTS} ${BISON_VqmParser_OUTPUT_SOURCE} PROPERTIES LANGUAGE CXX)

#Suppress warnings in Flex/Bison generated files
if(FLEX_BISON_WARN_SUPPRESS_FLAGS)
    set_source_files_properties(${FLEX_VqmLexer_OUTPUTS} ${BISON_VqmParser_OUTPUT_SOURCE}
                                PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})
endif()

#Create the library
add_library(libvqm STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
             ${FLEX_VqmLexer_OUTPUTS} 
             ${BISON_VqmParser_OUTPUT_SOURCE})
target_include_directories(libvqm PUBLIC ${LIB_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(libvqm PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

target_link_libraries(libvqm
                        libvtrutil)
