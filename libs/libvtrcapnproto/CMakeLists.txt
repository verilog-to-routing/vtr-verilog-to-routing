include(GNUInstallDirs)

if(NOT MSCV)
    # These flags generate noisy but non-bug warnings when using lib kj,
    # supress them.
    set(WARN_FLAGS_TO_DISABLE
        -Wno-undef
        -Wno-non-virtual-dtor
        )
    foreach(flag ${WARN_FLAGS_TO_DISABLE})
        CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
        if(CXX_COMPILER_SUPPORTS_${flag})
            #Flag supported, so enable it
            add_compile_options(${flag})
        endif()
    endforeach()
endif()

# Create generated headers from capnp schema files
#
# Each schema used should appear here.
set(CAPNP_DEFS
    place_delay_model.capnp
    matrix.capnp
    gen/rr_graph_uxsdcxx.capnp
    map_lookahead.capnp
    extended_map_lookahead.capnp
)

capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS
    ${CAPNP_DEFS}
)

if (VPR_ENABLE_INTERCHANGE)
    set(IC_DIR ${CMAKE_SOURCE_DIR}/libs/EXTERNAL/libinterchange/interchange)
    set(CAPNPC_SRC_PREFIX ${IC_DIR})

    find_program(WGET wget REQUIRED)
    find_package(ZLIB REQUIRED)

    # Add Java schema
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
        DEPENDS ${JAVA_SCHEMA}
    )

    set(CAPNPC_IMPORT_DIRS)
    list(APPEND CAPNPC_IMPORT_DIRS ${CMAKE_CURRENT_BINARY_DIR}/schema)

    set(IC_PROTOS
        LogicalNetlist.capnp
        PhysicalNetlist.capnp
        DeviceResources.capnp
        References.capnp
    )
    set(IC_SRCS)
    set(IC_HDRS)
    foreach(PROTO ${IC_PROTOS})
        capnp_generate_cpp(
            IC_SRC
            IC_HDR
            ${IC_DIR}/${PROTO}
        )
        list(APPEND IC_SRCS ${IC_SRC})
        list(APPEND IC_HDRS ${IC_HDR})
        list(APPEND CAPNP_DEFS ${IC_DIR}/${PROTO})
    endforeach()
endif()

install(FILES ${CAPNP_DEFS} DESTINATION ${CMAKE_INSTALL_DATADIR}/vtr)

add_library(libvtrcapnproto STATIC
            ${CAPNP_SRCS}
            ${IC_SRCS}
            mmap_file.h
            mmap_file.cpp
            serdes_utils.h
            serdes_utils.cpp
            )

if (VPR_ENABLE_INTERCHANGE)
    add_dependencies(libvtrcapnproto
        get_java_capnp_schema
    )
endif()

target_include_directories(libvtrcapnproto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/gen
    )
target_link_libraries(libvtrcapnproto
    libvtrutil
    CapnProto::capnp
)
