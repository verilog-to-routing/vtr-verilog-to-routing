cmake_minimum_required(VERSION 3.16)

# mosaic yosys plugin, built as part of the vtr tree.

project("mosaic")

# wildebeest-originated sources (clk_domains / max_level)
add_subdirectory(wildebeest)

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
)
target_include_directories(mosaic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# yosys_plugin_api (libs/EXTERNAL) supplies the installed yosys headers and -D_YOSYS_
target_link_libraries(mosaic
    PRIVATE
        yosys_plugin_api
        wildebeest
        libpugixml
        ${CMAKE_DL_LIBS}
)
add_dependencies(mosaic yosys)

# yosys assumes `.so` on all platforms for `plugin -i mosaic`.
set_target_properties(mosaic PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mosaic"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/yosys/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/yosys/plugins
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/yosys/plugins)
install(TARGETS mosaic DESTINATION ${CMAKE_BINARY_DIR}/share/yosys/plugins)

# fallback path used by max_level when templateDir is unset:
# share/yosys/plugins/mosaic/lut_models/LUTs.v
set(MOSAIC_LUT_MODELS_SRC
    ${VTR_SOURCE_DIR}/vtr_flow/misc/mosaic/template/lut_models/LUTs.v)
set(MOSAIC_LUT_MODELS_DST
    ${CMAKE_BINARY_DIR}/share/yosys/plugins/mosaic/lut_models)
file(MAKE_DIRECTORY ${MOSAIC_LUT_MODELS_DST})
configure_file(
    ${MOSAIC_LUT_MODELS_SRC}
    ${MOSAIC_LUT_MODELS_DST}/LUTs.v
    COPYONLY
)
install(FILES ${MOSAIC_LUT_MODELS_SRC} DESTINATION ${MOSAIC_LUT_MODELS_DST})
