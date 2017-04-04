cmake_minimum_required(VERSION 2.8.12)

project("vpr")

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES SRC/main.cpp)
file(GLOB_RECURSE LIB_SOURCES SRC/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS SRC/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Create the library
add_library(libvpr STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libvpr PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libvpr PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libvpr
                        libvtrutil
                        libarchfpga
                        libsdcparse
                        libblifparse
                        libeasygl
                        libtatum)

#Create the executable
add_executable(vpr ${EXEC_SOURCES})
target_link_libraries(vpr 
                        libvpr)

install(TARGETS vpr libvpr DESTINATION bin)
