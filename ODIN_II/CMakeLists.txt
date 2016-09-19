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
headers_to_include_dirs(BISON_VerilogParser_OUTPUT_HEADER PARSER_INCLUDE_DIRS)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES SRC/*.c)
file(GLOB_RECURSE EXEC_HEADERS SRC/*.h)
headers_to_include_dirs(EXEC_HEADERS EXEC_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${EXEC_SOURCES} ${FLEX_VerilogLexer_OUTPUTS} ${BISON_VerilogParser_OUTPUT_SOURCE} PROPERTIES LANGUAGE CXX)


add_definitions(-DVPR6 -DUSING_BISON -DYYERROR_VERBOSE)
include_directories(${EXEC_INCLUDE_DIRS} ${PARSER_INCLUDE_DIRS})

#Create the executable
add_executable(odin_II.exe ${EXEC_SOURCES} ${EXEC_HEADERS} ${FLEX_VerilogLexer_OUTPUTS} ${BISON_VerilogParser_OUTPUT_SOURCE})
target_link_libraries(odin_II.exe
                        libvtrutil
                        libarchfpga
                        libabc
                        ${CMAKE_DL_LIBS})

install(TARGETS odin_II.exe DESTINATION bin)
