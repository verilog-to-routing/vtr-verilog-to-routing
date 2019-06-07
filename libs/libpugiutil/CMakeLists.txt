cmake_minimum_required(VERSION 3.9)

project("libpugiutil")

#
# Source files and library
#
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Create the library
add_library(libpugiutil STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libpugiutil PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libpugiutil PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

target_link_libraries(libpugiutil
                        libpugixml)

install(TARGETS libpugiutil DESTINATION bin)
