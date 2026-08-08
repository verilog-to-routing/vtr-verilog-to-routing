# mosaic yosys plugin
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "In-tree builds are not supported. Instead, run:\ncmake . -B build <options> && cmake --build build")
endif()

cmake_minimum_required(VERSION 3.20...3.29)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(mosaic CXX)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if (APPLE)
    # on linux, shared libraries can refer to symbols from the executable by
    # default. on macos that needs to be enabled explicitly.
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

find_package(Yosys)

string(FIND "${YOSYS_VERSION}" "+" yosysCheck)
if (NOT yosysCheck EQUAL -1)
    message(WARNING "Only official yosys versions are supported")
endif()

# wildebeest-originated sources (clk_domains / max_level)
add_subdirectory(wildebeest)

# pugixml is source-compiled into the plugin from vtr's vendored copy.
set(PUGIXML_SRC_DIR "${CMAKE_SOURCE_DIR}/../libs/EXTERNAL/libpugixml/src"
    CACHE PATH "path to the vendored pugixml sources")

add_library(mosaic SHARED
    src/vtr_arch_clocks.cc
    src/vtr_arch_info.cc
    src/vtr_arch_rules.cc
    src/arch_rule_gen/template_io.cc
    src/arch_rule_gen/bram_rule_gen.cc
    src/arch_rule_gen/adder_rule_gen.cc
    src/arch_rule_gen/multiply_rule_gen.cc
    src/arch_rule_gen/exotic_rule_gen.cc
    src/arch_rule_gen/hardblock_lib_gen.cc
    src/arch_rule_gen/registry.cc
    ${PUGIXML_SRC_DIR}/pugixml.cpp
)
target_include_directories(mosaic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PUGIXML_SRC_DIR}
)
target_link_libraries(mosaic
    PRIVATE
        wildebeest
        yosys::yosys
)

set_target_properties(mosaic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    # yosys assumes `.so` on all platforms for `plugin -i mosaic`.
    PREFIX ""
    OUTPUT_NAME "mosaic"
    SUFFIX ".so"
)

if (WIN32)
    install(TARGETS mosaic RUNTIME DESTINATION ${YOSYS_DATDIR}/plugins)
else()
    install(TARGETS mosaic LIBRARY DESTINATION ${YOSYS_DATDIR}/plugins)
endif()

add_custom_target(
    uninstall
    COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
)
