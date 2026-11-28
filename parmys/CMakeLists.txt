cmake_minimum_required(VERSION 3.16)

project("parmys")

# Parmys is a Yosys plugin (partial mapper) that is built and used only from the VTR tree.
# Yosys itself is built in libs/EXTERNAL; its headers are taken from that build.

set(PARMYS_SOURCES
    parmys-plugin/parmys.cc
    parmys-plugin/parmys_arch.cc
    parmys-plugin/parmys_resolve.cc
    parmys-plugin/parmys_update.cc
    parmys-plugin/parmys_utils.cc
    parmys-plugin/core/adder.cc
    parmys-plugin/core/block_memory.cc
    parmys-plugin/core/hard_block.cc
    parmys-plugin/core/memory.cc
    parmys-plugin/core/multiplier.cc
    parmys-plugin/core/subtractor.cc
    parmys-plugin/mapping/hard_soft_logic_mixer.cc
    parmys-plugin/mapping/mixing_optimization.cc
    parmys-plugin/mapping/odin_ii.cc
    parmys-plugin/mapping/partial_map.cc
    parmys-plugin/netlist/netlist_cleanup.cc
    parmys-plugin/netlist/netlist_statistic.cc
    parmys-plugin/netlist/netlist_utils.cc
    parmys-plugin/netlist/netlist_visualizer.cc
    parmys-plugin/netlist/node_utils.cc
    parmys-plugin/utils/ast_util.cc
    parmys-plugin/utils/enum_str.cc
    parmys-plugin/utils/hash_table.cc
    parmys-plugin/utils/odin_error.cc
    parmys-plugin/utils/odin_util.cc
    parmys-plugin/utils/read_xml_config_file.cc
    parmys-plugin/utils/string_cache.cc
)

file(GLOB_RECURSE PARMYS_HEADERS CONFIGURE_DEPENDS parmys-plugin/*.h)

add_library(parmys SHARED ${PARMYS_SOURCES} ${PARMYS_HEADERS})

target_include_directories(parmys PUBLIC
    parmys-plugin
    parmys-plugin/core
    parmys-plugin/mapping
    parmys-plugin/netlist
    parmys-plugin/utils
)

target_include_directories(parmys PUBLIC ${VTR_SOURCE_DIR}/libs/EXTERNAL/yosys/share/include)
target_compile_definitions(parmys PUBLIC _YOSYS_)

add_dependencies(parmys yosys)
target_link_libraries(parmys
    liblog
    libarchfpga
    libvtrutil
    librtlnumber
    libpugixml
    libpugiutil
    pthread
    ${CMAKE_DL_LIBS}
)
# MacOS require specific linker settings
if(APPLE)
  target_link_options(parmys PRIVATE -undefined dynamic_lookup)
endif()


# Yosys loads plugins by name from <datdir>/plugins/<name>.so (also on macOS)
set_target_properties(parmys PROPERTIES PREFIX "" SUFFIX ".so")

# Install into the Yosys data directory of this build tree:
#   share/yosys/plugins/parmys.so        the plugin (`plugin -i parmys`)
#   share/yosys/parmys/*.v               techmap libraries and the VTR primitive models used by
#                                        vtr_flow/misc/yosys/synthesis.tcl (`read_verilog +/parmys/vtr_primitives.v`)
set(PARMYS_PLUGIN_DIR ${CMAKE_BINARY_DIR}/share/yosys/plugins)
set(PARMYS_DATA_DIR ${CMAKE_BINARY_DIR}/share/yosys/parmys)
file(GLOB PARMYS_TECHLIBS CONFIGURE_DEPENDS parmys-plugin/techlibs/*.v)

add_custom_command(TARGET parmys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PARMYS_PLUGIN_DIR} ${PARMYS_DATA_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:parmys> ${PARMYS_PLUGIN_DIR}/parmys.so
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PARMYS_TECHLIBS} ${PARMYS_DATA_DIR}
    COMMENT "Installing the parmys plugin and its techlibs into ${CMAKE_BINARY_DIR}/share/yosys"
    VERBATIM
)

# Kept for scripts and habits that build the plugin by this name.
add_custom_target(parmys-plugin ALL DEPENDS parmys)
