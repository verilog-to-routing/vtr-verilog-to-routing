find_package(Yosys)

add_library(wildebeest SHARED
    arch_rule_gen.cc
    clk_domains.cc
    vtr_arch_clocks.cc
    vtr_arch_info.cc
    vtr_arch_rules.cc
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
