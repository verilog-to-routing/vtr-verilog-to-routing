cmake_minimum_required(VERSION 3.16)

project("odin_ii")

if(ODIN_DEBUG)

    message("*** Compiling with Odin debug flags")

    set(ODIN_EXTRA_FLAGS
        "-g3"
        "-ggdb3"
        "-O0"
        ${ODIN_EXTRA_FLAGS}
    )

endif()

if(ODIN_WARN)

    message("*** Compiling with Odin with extra warning flags")

    set(ODIN_REF_FLAGS
        "-Wchkp"                    # Warn about memory access errors found by Pointer Bounds Checker
        "-Wreturn-local-addr"       # Warn about returning a pointer/reference to a local or temporary variable.
        "-Wnonnull"                 # Warn about NULL being passed to argument slots marked as requiring non-NULL
    )

    set(ODIN_SIGN_FLAGS
        "-Wsign-compare"            # Warn about signed-unsigned comparisons
        "-Wsign-conversion"         # Warn about signedness conversion
        "-Wsign-promo"              # Warn when overload promotes from unsigned to signed
        "-Wconversion"              # Warn for implicit type conversions that may change a value
    )

    set(ODIN_LIMITS_FLAGS
        "-Wshift-count-overflow"    # Warn if shift count >= width of type
        "-Wshift-count-negative"    # Warn if shift count is negative
        "-Wstrict-overflow"         # Warn about optimizations that assume that signed overflow is undefined
        "-Wtype-limits"             # Warn if a comparison is always true or always false due to the limited range of the data type
    )

    set(ODIN_EXTRA_FLAGS
        ${ODIN_EXTRA_FLAGS}
        ${ODIN_REF_FLAGS}
        ${ODIN_SIGN_FLAGS}
        ${ODIN_LIMITS_FLAGS}
    )

endif()

if(ODIN_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,readability-*")
endif()

if(ODIN_COVERAGE)

    message("*** Compiling with Odin Coverage flags")

    set(ODIN_EXTRA_FLAGS
        "-g"
        "-fprofile-arcs"
        "-ftest-coverage"
        ${ODIN_EXTRA_FLAGS}
    )

    set(ODIN_EXTRA_LINK_FLAGS
        "-lgcov"
        "--coverage"
        ${ODIN_EXTRA_LINK_FLAGS}
    )

endif()

if(ODIN_SANITIZE)

    message("*** Compiling with Odin Coverage flags")

    set(ODIN_EXTRA_FLAGS
        "-O1"
        "-fno-omit-frame-pointer"
        "-fsanitize=address"
        "-fsanitize=leak"
        "-fsanitize=undefined"
        "-fuse-ld=gold"
        ${ODIN_EXTRA_FLAGS}
    )

    set(ODIN_EXTRA_LINK_FLAGS
        "-O1"
        "-fno-omit-frame-pointer"
        "-fsanitize=address"
        "-fsanitize=leak"
        "-fsanitize=undefined"        
        "-fuse-ld=gold"
        ${ODIN_EXTRA_LINK_FLAGS}
    )

endif()

#Flex and Bison are used to generate the parser
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

#Find the flex and bison input files
file(GLOB_RECURSE LEXER_SOURCES src/verilog/*.l)
file(GLOB_RECURSE PARSER_SOURCES src/verilog/*.y)

#Make the flex and bison targets
flex_target(VerilogLexer ${LEXER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/verilog_flex.c)
bison_target(VerilogParser ${PARSER_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/verilog_bison.c COMPILE_FLAGS "--verbose")
add_flex_bison_dependency(VerilogLexer VerilogParser)

#Get the include directories for the generated headers
files_to_dirs(BISON_VerilogParser_OUTPUT_HEADER PARSER_INCLUDE_DIRS)

include(CheckCXXSymbolExists)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)

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

target_link_libraries(libodin_ii
                        pthread
                        libvtrutil
                        librtlnumber
                        libarchfpga
                        libargparse
                        ${CMAKE_DL_LIBS})

#Create the executable
add_executable(odin_ii ${EXEC_SOURCES})

target_link_libraries(odin_ii
                        libodin_ii)

#################################
# INCLUDE EXTRA ODIN FLAGS IS EXECUTED HERE
#
#  include the flags one at a time in case some are not supported and report the error
#

foreach(ODIN_FLAG ${ODIN_EXTRA_FLAGS})

    CHECK_CXX_COMPILER_FLAG(${ODIN_FLAG} FLAG_TEST)

    if(FLAG_TEST)
        message("-- Performing Test CXX_SUPPORT_FLAG${ODIN_FLAG} - Success")
        target_compile_options(libodin_ii PRIVATE ${ODIN_FLAG})
        target_compile_options(odin_ii PRIVATE ${ODIN_FLAG})
    else()
        message("-- Performing Test CXX_SUPPORT_FLAG${ODIN_FLAG} - Failure")
    endif()

endforeach()

#add extra link flags for odin
foreach(ODIN_LINK_FLAG ${ODIN_EXTRA_LINK_FLAGS})
        target_link_libraries(libodin_ii ${ODIN_LINK_FLAG})
        target_link_libraries(odin_ii ${ODIN_LINK_FLAG})
endforeach()

install(TARGETS odin_ii libodin_ii DESTINATION bin)
install(FILES ${LIB_HEADERS} DESTINATION include/libodin_ii)
