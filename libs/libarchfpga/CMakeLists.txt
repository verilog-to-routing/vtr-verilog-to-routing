cmake_minimum_required(VERSION 3.16)

project("libarchfpga")

file(GLOB_RECURSE READ_ARCH_EXEC_SRC src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Create the library
add_library(libarchfpga STATIC
    ${LIB_HEADERS}
    ${LIB_SOURCES}
)

target_include_directories(libarchfpga PUBLIC ${LIB_INCLUDE_DIRS})

set_target_properties(libarchfpga PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependencies
target_link_libraries(libarchfpga
                        libvtrutil
                        libpugixml
                        libpugiutil
)

if(${VTR_ENABLE_CAPNPROTO})
    target_link_libraries(libarchfpga libvtrcapnproto)
    target_compile_definitions(libarchfpga PRIVATE VTR_ENABLE_CAPNPROTO)
endif()

# Using filesystem library requires additional compiler/linker options for GNU implementation prior to 9.1
# and LLVM implementation prior to LLVM 9.0;
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Get the compiler version string
    execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE _gcc_output)
    string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" _gcc_version ${_gcc_output})

    if(_gcc_version VERSION_LESS "9.1")
        target_link_libraries(libarchfpga stdc++fs)
    endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Get the compiler version string
    execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE _clang_output)
    string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" _clang_version ${_clang_output})

    if(_clang_version VERSION_LESS "9.0")
        target_link_libraries(libarchfpga c++fs)
    endif()
endif()

target_compile_definitions(libarchfpga PUBLIC ${INTERCHANGE_SCHEMA_HEADERS})

#Create the test executable
add_executable(read_arch ${READ_ARCH_EXEC_SRC})
target_link_libraries(read_arch libarchfpga)

#Supress IPO link warnings if IPO is enabled
get_target_property(READ_ARCH_USES_IPO read_arch INTERPROCEDURAL_OPTIMIZATION)
if(READ_ARCH_USES_IPO)
    set_property(TARGET read_arch APPEND PROPERTY LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

install(TARGETS libarchfpga DESTINATION bin)
install(FILES ${LIB_HEADERS} DESTINATION include/libarchfpga)

# 
# Unit Tests
#
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
add_executable(test_archfpga ${TEST_SOURCES})
target_link_libraries(test_archfpga Catch2::Catch2WithMain libarchfpga)

add_test(NAME test_archfpga
    COMMAND test_archfpga
    --colour-mode ansi
    )
