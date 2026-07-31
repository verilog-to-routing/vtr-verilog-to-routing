find_package(Yosys)

# mosaic-only sources live next to the wildebeest tree
# (mosaic/src), not inside wildebeest/src.
set(MOSAIC_SRC_DIR "${CMAKE_SOURCE_DIR}/../src")

# pugixml is source-compiled into the plugin from vtr's vendored copy, so the
# arch xml reader uses a real xml parser without adding any link dependency.
set(PUGIXML_SRC_DIR "${CMAKE_SOURCE_DIR}/../../libs/EXTERNAL/libpugixml/src"
    CACHE PATH "path to the vendored pugixml sources")

add_library(wildebeest SHARED
    # wildebeest-originated (modified for -vtr_arch)
    clk_domains.cc
    # mosaic-only
    ${MOSAIC_SRC_DIR}/arch_rule_gen.cc
    ${MOSAIC_SRC_DIR}/vtr_arch_clocks.cc
    ${MOSAIC_SRC_DIR}/vtr_arch_info.cc
    ${MOSAIC_SRC_DIR}/vtr_arch_rules.cc
    ${PUGIXML_SRC_DIR}/pugixml.cpp
)
target_include_directories(wildebeest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MOSAIC_SRC_DIR}
    ${PUGIXML_SRC_DIR}
)
target_link_libraries(wildebeest
    PRIVATE
        yosys::yosys
)

set_target_properties(wildebeest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    # Yosys assumes `.so` to be the suffix on all platforms, so the following is required to make
    # `yosys -m wildebeest` to work after installation on any platform.
    PREFIX ""
    OUTPUT_NAME ${OUTPUT_NAME}
    SUFFIX ".so"
)

if (WIN32)
    # install .dll only
    install(TARGETS wildebeest RUNTIME DESTINATION ${YOSYS_DATDIR}/plugins)
else()
    # install .so/.dylib only
    install(TARGETS wildebeest LIBRARY DESTINATION ${YOSYS_DATDIR}/plugins)
endif()

add_custom_target(
    uninstall
    COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
)
