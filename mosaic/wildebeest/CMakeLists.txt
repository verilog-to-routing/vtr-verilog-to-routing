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

add_subdirectory(src)
