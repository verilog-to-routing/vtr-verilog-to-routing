cmake_minimum_required(VERSION 3.9)

project("vqm2blif")

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Create the library
add_library(libvqm2blif STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libvqm2blif PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libvqm2blif PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libvqm2blif
                        libarchfpga
                        libvqm)

#Create the executable
add_executable(vqm2blif ${EXEC_SOURCES})
target_link_libraries(vqm2blif 
                        libvqm2blif)
