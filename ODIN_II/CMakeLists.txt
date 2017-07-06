cmake_minimum_required(VERSION 2.8.12)

project("ODIN_II")

#Flex and Bison are used to generate the parser
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

#Find the flex and bison input files
file(GLOB_RECURSE LEXER_SOURCES SRC/*.l)
file(GLOB_RECURSE PARSER_SOURCES SRC/*.y)

#Make the flex and bison targets
flex_target(VerilogLexer ${LEXER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/verilog_flex.c)
bison_target(VerilogParser ${PARSER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/verilog_bison.c)
add_flex_bison_dependency(VerilogLexer VerilogParser)

#Get the include directories for the generated headers
files_to_dirs(BISON_VerilogParser_OUTPUT_HEADER PARSER_INCLUDE_DIRS)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES SRC/*.cpp)
file(GLOB_RECURSE EXEC_HEADERS SRC/*.h)
files_to_dirs(EXEC_HEADERS EXEC_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${FLEX_VerilogLexer_OUTPUTS} ${BISON_VerilogParser_OUTPUT_SOURCE} PROPERTIES LANGUAGE CXX)

#Suppress warnings in Flex/Bison generated files
if(FLEX_BISON_WARN_SUPPRESS_FLAGS)
    set_source_files_properties(${FLEX_VerilogLexer_OUTPUTS} ${BISON_VerilogParser_OUTPUT_SOURCE}
                                PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})
endif()

add_definitions(-DUSING_BISON -DYYERROR_VERBOSE)
include_directories(${EXEC_INCLUDE_DIRS} ${PARSER_INCLUDE_DIRS})

#Create the executable
add_executable(odin_II ${EXEC_SOURCES} ${EXEC_HEADERS} ${FLEX_VerilogLexer_OUTPUTS} ${BISON_VerilogParser_OUTPUT_SOURCE})
target_link_libraries(odin_II
                        libvtrutil
                        libarchfpga
                        libabc
                        libargparse
                        ${CMAKE_DL_LIBS})

install(TARGETS odin_II DESTINATION bin)
