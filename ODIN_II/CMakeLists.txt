cmake_minimum_required(VERSION 2.8.12)

project("ODIN_II")

# Allow extra Flags on Odin for debugging
set(ODIN_EXTRA_FLAGS
    "-Wsign-compare"
    "-Wsign-conversion"
    "-Wsign-promo"
    "-Wconversion"
    "-Wshift-count-overflow"
    "-Wshift-count-negative"
    "-Wstrict-overflow"
    "-Wtype-limits"
)

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

include(CheckCXXSymbolExists)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES main.cpp)
file(GLOB_RECURSE LIB_SOURCES SRC/*.cpp)
file(GLOB_RECURSE LIB_HEADERS SRC/include/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${FLEX_VerilogLexer_OUTPUTS} ${BISON_VerilogParser_OUTPUT_SOURCE} PROPERTIES LANGUAGE CXX)

#Suppress warnings in Flex/Bison generated files
if(FLEX_BISON_WARN_SUPPRESS_FLAGS)
    set_source_files_properties(${FLEX_VerilogLexer_OUTPUTS} ${BISON_VerilogParser_OUTPUT_SOURCE}
                                PROPERTIES COMPILE_FLAGS ${FLEX_BISON_WARN_SUPPRESS_FLAGS})
endif()

add_definitions(-DUSING_BISON -DYYERROR_VERBOSE)
include_directories(${LIB_INCLUDE_DIRS} ${PARSER_INCLUDE_DIRS})

#Create the library
add_library(libodin_ii STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
             ${FLEX_VerilogLexer_OUTPUTS} 
             ${BISON_VerilogParser_OUTPUT_SOURCE})

target_include_directories(libodin_ii PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libodin_ii PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

if(ODIN_DEBUG)
    foreach(ODIN_FLAG ${ODIN_EXTRA_FLAGS})
        CHECK_CXX_COMPILER_FLAG(${ODIN_FLAG} COMPILER_SUPPORTS_ODIN_EXTRA_FLAG)
        if(COMPILER_SUPPORTS_ODIN_EXTRA_FLAG)
            target_compile_options(libodin_ii PRIVATE ${ODIN_FLAG})
        else()
            message("*** Unable to set ${ODIN_FLAG} extra flags ***")
        endif()
    endforeach()
endif()

target_link_libraries(libodin_ii
                        libvtrutil
                        libarchfpga
                        libabc
                        libargparse
                        ${CMAKE_DL_LIBS})

#Create the executable
add_executable(odin_II ${EXEC_SOURCES})
target_link_libraries(odin_II
                        libodin_ii)

install(TARGETS odin_II libodin_ii DESTINATION bin)
