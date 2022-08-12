cmake_minimum_required(VERSION 3.9)

project("librrgraph")

# Source file and library
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

if(${VTR_ENABLE_CAPNPROTO})
    add_definitions("-DVTR_ENABLE_CAPNPROTO")
endif()

#Create the library
add_library(librrgraph STATIC
    ${LIB_HEADERS}
    ${LIB_SOURCES}
)

target_include_directories(librrgraph PUBLIC ${LIB_INCLUDE_DIRS})

set_target_properties(librrgraph PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(librrgraph
                      libvtrutil
                      libarchfpga
)

if(${VTR_ENABLE_CAPNPROTO})
    target_link_libraries(librrgraph libvtrcapnproto)
endif()

target_compile_definitions(librrgraph PUBLIC ${INTERCHANGE_SCHEMA_HEADERS})

# Unit tests
#file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
#add_executable(test_rr_graph ${TEST_SOURCES})
#target_link_libraries(test_rr_graph
#                      librrgraph)

# Run unit tests: comment out for now
#add_test(NAME test_rr_graph COMMAND test_rr_graph --use-colour=yes)

add_custom_target(
    generate_rr_graph_serializers
    COMMAND ${CMAKE_COMMAND} -E remove_directory rr_graph_generate
    COMMAND ${CMAKE_COMMAND} -E make_directory rr_graph_generate
    COMMAND ${CMAKE_COMMAND} -E chdir rr_graph_generate git clone https://github.com/duck2/uxsdcxx
    COMMAND python3 -mpip install --user -r rr_graph_generate/uxsdcxx/requirements.txt
    COMMAND ${CMAKE_COMMAND} -E chdir rr_graph_generate python3 uxsdcxx/uxsdcxx.py ${CMAKE_CURRENT_SOURCE_DIR}/src/base/rr_graph.xsd
    COMMAND ${CMAKE_COMMAND} -E chdir rr_graph_generate python3 uxsdcxx/uxsdcap.py ${CMAKE_CURRENT_SOURCE_DIR}/src/base/rr_graph.xsd
    COMMAND ${CMAKE_COMMAND} -E copy
        rr_graph_generate/rr_graph_uxsdcxx.h
        rr_graph_generate/rr_graph_uxsdcxx_capnp.h
        rr_graph_generate/rr_graph_uxsdcxx_interface.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/base/gen
    COMMAND ${CMAKE_COMMAND} -E copy rr_graph_generate/rr_graph_uxsdcxx.capnp ${CMAKE_CURRENT_SOURCE_DIR}/../libvtrcapnproto/gen
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/base/rr_graph.xsd
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )