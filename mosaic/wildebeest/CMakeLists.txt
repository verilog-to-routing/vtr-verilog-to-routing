cmake_minimum_required(VERSION 3.16)

# wildebeest sources compiled into the mosaic plugin
add_library(wildebeest OBJECT
    src/clk_domains.cc
)
set_target_properties(wildebeest PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(wildebeest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # clk_domains.cc includes mosaic headers (vtr_arch_clocks.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_include_directories(wildebeest SYSTEM PUBLIC
    ${VTR_SOURCE_DIR}/libs/EXTERNAL/yosys
    ${VTR_SOURCE_DIR}/libs/EXTERNAL/yosys/share/include
)
target_compile_definitions(wildebeest PUBLIC
    _YOSYS_
)

if(TCL_FOUND)
    target_compile_definitions(wildebeest PUBLIC YOSYS_ENABLE_TCL)
    target_include_directories(wildebeest SYSTEM PUBLIC ${TCL_INCLUDE_PATH})
endif()
