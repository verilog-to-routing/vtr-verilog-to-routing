find_package(CapnProto REQUIRED)
set(PROTOS LogicalNetlist.capnp PhysicalNetlist.capnp DeviceResources.capnp References.capnp)

set(INTERCHANGE_SCHEMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../interchange)

find_program(WGET wget REQUIRED)

set(JAVA_SCHEMA ${CMAKE_CURRENT_BINARY_DIR}/schema/capnp/java.capnp)
add_custom_command(
    OUTPUT ${JAVA_SCHEMA}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/schema/capnp/
    COMMAND ${WGET}
        https://raw.githubusercontent.com/capnproto/capnproto-java/master/compiler/src/main/schema/capnp/java.capnp
        -O ${JAVA_SCHEMA}
    )
add_custom_target(
    get_java_capnp_schema
    DEPENDS ${JAVA_SCHEMA})

set(CAPNPC_IMPORT_DIRS)
list(APPEND CAPNPC_IMPORT_DIRS ${CMAKE_CURRENT_BINARY_DIR}/schema)

set(CAPNPC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/../../interchange")

add_custom_command(
    OUTPUT ${CAPNPC_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CAPNPC_OUTPUT_DIR}
    )
add_custom_target(
    fpga_interchange_capnp_output_directory
    DEPENDS ${CAPNPC_OUTPUT_DIR})

set(CAPNPC_SRC_PREFIX ${INTERCHANGE_SCHEMA_DIR})
set(CAPNP_SRCS)
set(CAPNP_HDRS)
foreach (proto ${PROTOS})
    capnp_generate_cpp(CAPNP_SRC CAPNP_HDR ${INTERCHANGE_SCHEMA_DIR}/${proto})
    list(APPEND CAPNP_HDRS ${CAPNP_HDR})
    list(APPEND CAPNP_SRCS ${CAPNP_SRC})
endforeach()
add_library(fpga_interchange_capnp STATIC ${CAPNP_SRCS})
add_dependencies(fpga_interchange_capnp
    get_java_capnp_schema
    fpga_interchange_capnp_output_directory)
target_link_libraries(fpga_interchange_capnp PRIVATE CapnProto::capnp)

get_filename_component(FPGA_INTERCHANGE_CAPNP_INCLUDE_DIR "${CAPNPC_OUTPUT_DIR}" ABSOLUTE)
target_include_directories(fpga_interchange_capnp INTERFACE ${FPGA_INTERCHANGE_CAPNP_INCLUDE_DIR})
