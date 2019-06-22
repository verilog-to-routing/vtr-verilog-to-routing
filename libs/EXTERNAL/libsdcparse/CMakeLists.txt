cmake_minimum_required(VERSION 3.9)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(HeadersToIncludeDirs)

project("libsdcparse")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    #Only set compiler settings if we are not a sub-project
    set(WARN_FLAGS "-Wall -Wextra -Wpedantic -Wcast-qual -Wcast-align -Wshadow -Wformat=2 -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wswitch-default -Wundef -Wunused-variable -Wdisabled-optimization -Wnoexcept -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14 ${WARN_FLAGS}") 
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined") 
    set(FLEX_BISON_WARN_SUPPRESS_FLAGS "-Wno-switch-default -Wno-unused-parameter -Wno-missing-declarations")
endif()

#Flex and Bison are used to generate the parser
find_package(BISON REQUIRED 3.0)
find_package(FLEX REQUIRED)

file(GLOB_RECURSE LIB_SOURCES src/sdc*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/sdc*.hpp)
headers_to_include_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Find the flex and bison input files
file(GLOB_RECURSE LEXER_SOURCES src/sdc*.l)
file(GLOB_RECURSE PARSER_SOURCES src/sdc*.y)

#Make the flex and bison targets
flex_target(SdcLexer ${LEXER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/sdc_lexer.gen.cpp
        COMPILE_FLAGS --header-file=${CMAKE_CURRENT_BINARY_DIR}/sdc_lexer.gen.hpp)
bison_target(SdcParser ${PARSER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/sdc_parser.gen.cpp)
add_flex_bison_dependency(SdcLexer SdcParser)


#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} ${FLEX_SdcLexer_OUTPUTS} ${BISON_SdcParser_OUTPUT_SOURCE} PROPERTIES LANGUAGE CXX)

#Suppress warnings in Flex/Bison generated files
if(FLEX_BISON_WARN_SUPPRESS_FLAGS)
    set_source_files_properties(${FLEX_SdcLexer_OUTPUTS} ${BISON_SdcParser_OUTPUT_SOURCE}
                                PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})
endif()

#Create the library
add_library(libsdcparse STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
             ${FLEX_SdcLexer_OUTPUTS} 
             ${BISON_SdcParser_OUTPUT_SOURCE})
target_include_directories(libsdcparse PUBLIC ${LIB_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(libsdcparse PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Create the test executable
add_executable(sdcparse_test src/main.cpp)
target_link_libraries(sdcparse_test libsdcparse)

#Suppress IPO link warnings
get_target_property(USES_IPO sdcparse_test INTERPROCEDURAL_OPTIMIZATION)
if (USES_IPO)
    set(IPO_LINK_WARN_FLAGS_TO_CHECK "-Wno-null-dereference")
    foreach(flag ${IPO_LINK_WARN_FLAGS_TO_CHECK})
        CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
        if(CXX_COMPILER_SUPPORTS_${flag})
            #Flag supported, so enable it
            set_target_properties(sdcparse_test PROPERTIES LINK_FLAGS ${flag})
        endif()
    endforeach()
endif()

install(TARGETS libsdcparse sdcparse_test DESTINATION bin)
