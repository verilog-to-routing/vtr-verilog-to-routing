cmake_minimum_required(VERSION 3.9)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(HeadersToIncludeDirs)

project("libblifparse")

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

file(GLOB_RECURSE LIB_SOURCES src/blif*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/blif*.hpp)
headers_to_include_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Find the flex and bison input files
file(GLOB_RECURSE LEXER_SOURCES src/blif*.l)
file(GLOB_RECURSE PARSER_SOURCES src/blif*.y)

#Make the flex and bison targets
flex_target(BlifLexer ${LEXER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/blif_lexer.gen.cpp
        COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/blif_lexer.gen.hpp)
bison_target(BlifParser ${PARSER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/blif_parser.gen.cpp)
add_flex_bison_dependency(BlifLexer BlifParser)

#Suppress warnings in Flex/Bison generated files
if(FLEX_BISON_WARN_SUPPRESS_FLAGS)
    set_source_files_properties(${FLEX_BlifLexer_OUTPUTS} ${BISON_BlifParser_OUTPUT_SOURCE}
                                PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})
endif()

#Create the library
add_library(libblifparse STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
             ${FLEX_BlifLexer_OUTPUTS} 
             ${BISON_BlifParser_OUTPUT_SOURCE})
target_include_directories(libblifparse PUBLIC ${LIB_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(libblifparse PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Create the test executable
add_executable(blifparse_test src/main.cpp)
target_link_libraries(blifparse_test libblifparse)

#Suppress IPO link warnings
get_target_property(USES_IPO blifparse_test INTERPROCEDURAL_OPTIMIZATION)
if (USES_IPO)
    set(IPO_LINK_WARN_FLAGS_TO_CHECK "-Wno-null-dereference")
    foreach(flag ${IPO_LINK_WARN_FLAGS_TO_CHECK})
        CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
        if(CXX_COMPILER_SUPPORTS_${flag})
            #Flag supported, so enable it
            set_target_properties(blifparse_test PROPERTIES LINK_FLAGS ${flag})
        endif()
    endforeach()
endif()

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    #Add the tests if we are the root project
    enable_testing()
    add_test(NAME test_parse_blif
             COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test/test_parser.sh ${CMAKE_CURRENT_SOURCE_DIR}/test/*/*.blif ${CMAKE_CURRENT_SOURCE_DIR}/test/*/*.eblif)
endif()

install(TARGETS libblifparse blifparse_test DESTINATION bin)
