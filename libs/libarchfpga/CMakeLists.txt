cmake_minimum_required(VERSION 3.9)

project("libarchfpga")

file(GLOB_RECURSE EXEC_SOURCES main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Remove test executable from library
list(REMOVE_ITEM LIB_SOURCES ${EXEC_SOURCES})

#Create the library
add_library(libarchfpga STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libarchfpga PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libarchfpga PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libarchfpga
                        libvtrutil
                        libpugixml
                        libpugiutil)

#Create the test executable
add_executable(read_arch ${EXEC_SOURCES})
target_link_libraries(read_arch libarchfpga)

#Supress IPO link warnings if IPO is enabled
get_target_property(READ_ARCH_USES_IPO read_arch INTERPROCEDURAL_OPTIMIZATION)
if (READ_ARCH_USES_IPO)
    set_target_properties(read_arch PROPERTIES LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

install(TARGETS libarchfpga read_arch DESTINATION bin)
