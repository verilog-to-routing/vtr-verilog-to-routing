cmake_minimum_required(VERSION 3.9)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED True)

project(testbench)

#include(../../../libs/CMakeLists.txt)
#

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Create the library
add_library(libtb STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
)
target_include_directories(libtb PUBLIC ${LIB_INCLUDE_DIRS})

# #link_libraries(${ADDITIONAL_FLAGS})

# set_target_properties(libtb PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

target_link_libraries(libtb
                        libvtrutil
                        libarchfpga
                        libsdcparse
                        libblifparse
                        libtatum
                        libargparse
                        libpugixml
                        libvpr
)


add_executable(tb_build_switchblocks main.cpp)
#add_executable(main main.cpp)

target_link_libraries(tb_build_switchblocks
                        libtb
)