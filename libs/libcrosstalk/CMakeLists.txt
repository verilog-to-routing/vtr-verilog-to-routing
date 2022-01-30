cmake_minimum_required(VERSION 3.9)

project("libcrosstalk")

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Create the library
add_library(libcrosstalk STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libcrosstalk PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libcrosstalk PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libcrosstalk
					  libvtrutil)