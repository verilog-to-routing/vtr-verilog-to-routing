include(GitRevision)

find_package(Yosys)

git_rev_parse(YOSYS_SYN_REVISION ${CMAKE_SOURCE_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

add_library(wildebeest SHARED
    clk_domains.cc
    load_models.cc
    time_chrono.cc
    report_stat.cc
    zeroasic_dsp.cc
    cp.cc
    obs_clean.cc
    synth_fpga.cc
    zopt_dff.cc
    zopt_const_dff.cc
    zqcsat.cc
    zsimplemap.cc
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)
target_include_directories(wildebeest
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(wildebeest
    PRIVATE
        yosys::yosys
        zeroasic_dsp
)
target_compile_definitions(wildebeest
    PRIVATE
        SYN_SHARE_DIR=\"plugins/${OUTPUT_NAME}\"
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

set(SHARE_DIRECTORIES
    ff_models
    ram_models
    lut_models
    abc_scripts
    ../architecture
)

if (WIN32)
    # install .dll only
    install(TARGETS wildebeest RUNTIME DESTINATION ${YOSYS_DATDIR}/plugins)
else()
    # install .so/.dylib only
    install(TARGETS wildebeest LIBRARY DESTINATION ${YOSYS_DATDIR}/plugins)
endif()
install(DIRECTORY ${SHARE_DIRECTORIES} DESTINATION ${YOSYS_SHARE_DIR})

add_custom_target(
    uninstall
    COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
)