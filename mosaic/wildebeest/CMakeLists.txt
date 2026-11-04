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
# yosys_plugin_api (libs/EXTERNAL) supplies the installed yosys headers, -D_YOSYS_,
# and the Tcl include path. YOSYS_ENABLE_TCL now comes from yosys' generated
# kernel/yosys_config.h rather than being defined here.
target_link_libraries(wildebeest PUBLIC yosys_plugin_api)
# the headers only exist once the yosys external project has been installed
add_dependencies(wildebeest yosys)
