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
target_include_directories(parmys PUBLIC ${VTR_SOURCE_DIR}/yosys/share/include)

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

add_custom_target(parmys-plugin ALL
		DEPENDS parmys
		COMMAND ${CMAKE_COMMAND}
		--install ${CMAKE_BINARY_DIR}/parmys
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/yosys/plugins)
install(TARGETS parmys DESTINATION ${CMAKE_BINARY_DIR}/share/yosys/plugins)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/yosys/parmys)
install(FILES ${LIB_VERILOGS} DESTINATION ${CMAKE_BINARY_DIR}/share/yosys/parmys)
install(FILES "third_party/vtr_flow/primitives.v" DESTINATION ${CMAKE_BINARY_DIR}/share/yosys/parmys RENAME vtr_primitives.v)
