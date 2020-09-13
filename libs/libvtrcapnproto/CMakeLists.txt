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

install(FILES ${CAPNP_DEFS} DESTINATION capnp)

add_library(libvtrcapnproto STATIC
            ${CAPNP_SRCS}
            mmap_file.h
            mmap_file.cpp
            serdes_utils.h
            serdes_utils.cpp
            )
target_include_directories(libvtrcapnproto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/gen
    )
target_link_libraries(libvtrcapnproto
    libvtrutil
    CapnProto::capnp
    )
