if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "In-tree builds are not supported. Instead, run:\ncmake . -B build <options> && cmake --build build")
endif()

cmake_minimum_required(VERSION 3.20...3.29)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(wildebeest CXX)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if (APPLE)
    # On Linux, the shared libraries can refer to symbols from the executable by default.
    # On macOS, this is not the default behavior and needs to be enabled explicitly.
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

set(OUTPUT_NAME "wildebeest")

find_package(Yosys)

string(FIND ${YOSYS_VERSION} "+" YOSYS_CHECK)
if (${YOSYS_CHECK} EQUAL "-1")
else()
    message(WARNING "Only official yosys versions are supported")
endif()

set(YOSYS_SHARE_DIR ${YOSYS_DATDIR}/plugins/${OUTPUT_NAME})

add_subdirectory(src)
add_subdirectory(architecture)

# Add back once testing is possible
enable_testing()
add_subdirectory(tests)

# code formatting target
find_program(CLANG_FORMAT_EXE clang-format)

file(GLOB_RECURSE ALL_CPP_FILES
    "*.cc"
    "*.h"
)

if(CLANG_FORMAT_EXE)
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${ALL_CPP_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting C++ source files with clang-format..."
    )
endif()