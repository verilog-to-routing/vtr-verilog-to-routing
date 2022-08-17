cmake_minimum_required(VERSION 3.9)

project("libarchfpga")

file(GLOB_RECURSE READ_ARCH_EXEC_SRC src/main.cpp)
file(GLOB_RECURSE WRITE_ARCH_BB_EXEC_SRC src/write_arch_bb.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${READ_ARCH_EXEC_SRC} ${WRITE_ARCH_BB_EXEC_SRC})

#Create the library
add_library(libarchfpga STATIC
    ${LIB_HEADERS}
    ${LIB_SOURCES}
)

target_include_directories(libarchfpga PUBLIC ${LIB_INCLUDE_DIRS})

set_target_properties(libarchfpga PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libarchfpga
                        libvtrutil
                        libpugixml
                        libpugiutil
                        libvtrcapnproto
)

target_compile_definitions(libarchfpga PUBLIC ${INTERCHANGE_SCHEMA_HEADERS})

#Create the test executable
add_executable(read_arch ${READ_ARCH_EXEC_SRC})
add_executable(write_arch_bb ${WRITE_ARCH_BB_EXEC_SRC})
target_link_libraries(read_arch libarchfpga)
target_link_libraries(write_arch_bb libarchfpga)

#Supress IPO link warnings if IPO is enabled
get_target_property(READ_ARCH_USES_IPO read_arch INTERPROCEDURAL_OPTIMIZATION)
get_target_property(WRITE_ARCH_BB_USES_IPO write_arch_bb INTERPROCEDURAL_OPTIMIZATION)
if (READ_ARCH_USES_IPO)
    set_property(TARGET read_arch APPEND PROPERTY LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()
if (WRITE_ARCH_BB_USES_IPO)
    set_property(TARGET write_arch_bb APPEND PROPERTY LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

install(TARGETS libarchfpga read_arch write_arch_bb DESTINATION bin)

#
# install executables in the VTR source root directory
# to utilize them for scripts running VTR flow
#
set(ARCHFPGA_DIR ${VTR_SOURCE_DIR}/ArchFPGA)
# making a new custom target out of libarchfpga to automatically perform
# the house keeping for end users by deleting the ArchFPGA dir in the
# VTR root dir if it exists and holds expired execs
add_custom_target(archfpga-execs ALL 
                    DEPENDS read_arch write_arch_bb
                    COMMAND ${CMAKE_COMMAND} -E 
                                            remove_directory ${ARCHFPGA_DIR}                     
                    COMMAND ${CMAKE_COMMAND} -E 
                                            make_directory ${ARCHFPGA_DIR}
                    COMMAND ${CMAKE_COMMAND} -E 
                                            copy_directory ${CMAKE_CURRENT_BINARY_DIR} ${ARCHFPGA_DIR}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 
# Unit Tests
#
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
add_executable(test_archfpga ${TEST_SOURCES})
target_link_libraries(test_archfpga Catch2::Catch2WithMain libarchfpga)
