cmake_minimum_required(VERSION 3.16)

project("parmys")

file(GLOB_RECURSE LIB_SOURCES parmys-plugin/*.cc)
file(GLOB_RECURSE LIB_HEADERS parmys-plugin/*.h)
file(GLOB_RECURSE LIB_SOURCES_O parmys-plugin/*/*.cc)
file(GLOB_RECURSE LIB_HEADERS_O parmys-plugin/*/*.h)
file(GLOB_RECURSE LIB_VERILOGS parmys-plugin/techlibs/*2dff.v)

files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)
files_to_dirs(LIB_HEADERS_O LIB_INCLUDE_DIRS_O)

add_library(parmys SHARED
    ${LIB_HEADERS}
    ${LIB_SOURCES}
    ${LIB_HEADERS_O}
    ${LIB_SOURCES_O}
)
target_include_directories(parmys PUBLIC ${LIB_INCLUDE_DIRS})
target_include_directories(parmys PUBLIC ${LIB_INCLUDE_DIRS_O})
target_include_directories(parmys PUBLIC ${VTR_SOURCE_DIR}/libs/EXTERNAL/yosys/share/include)

add_definitions("-D_YOSYS_")
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

set_target_properties(parmys PROPERTIES PREFIX "") # Avoid extra 'lib' prefix
set_target_properties(parmys PROPERTIES SUFFIX ".so") # Yosys expects the suffix to be '.so' even on macOS

# Stage the plugin and its data files into the yosys data directory in the
# build tree (yosys is built with PREFIX=${CMAKE_BINARY_DIR}). The install
# rules use relative destinations (absolute ones are an error under
# -Werror=dev with CMake 4.x) and are installed here with --prefix pointing at
# the build tree. They are excluded from the default install since yosys itself
# is not installed to CMAKE_INSTALL_PREFIX.
add_custom_target(parmys-plugin ALL
		DEPENDS parmys
		COMMAND ${CMAKE_COMMAND}
		--install ${CMAKE_BINARY_DIR}/parmys
		--prefix ${CMAKE_BINARY_DIR}
		--component parmys_plugin
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/yosys/plugins)
install(TARGETS parmys DESTINATION share/yosys/plugins COMPONENT parmys_plugin EXCLUDE_FROM_ALL)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/yosys/parmys)
install(FILES ${LIB_VERILOGS} DESTINATION share/yosys/parmys COMPONENT parmys_plugin EXCLUDE_FROM_ALL)
install(FILES "third_party/vtr_flow/primitives.v" DESTINATION share/yosys/parmys RENAME vtr_primitives.v COMPONENT parmys_plugin EXCLUDE_FROM_ALL)
