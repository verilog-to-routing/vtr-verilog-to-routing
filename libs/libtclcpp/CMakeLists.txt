cmake_minimum_required(VERSION 3.9)

project("libtclcpp")

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

add_library(libtclcpp STATIC
    ${LIB_HEADERS}
    ${LIB_SOURCES}
)

target_include_directories(libtclcpp PUBLIC ${LIB_INCLUDE_DIRS})

set_target_properties(libtclcpp PROPERTIES PREFIX "")

find_package(TCL REQUIRED)
target_link_libraries(libtclcpp tcl)

install(TARGETS libtclcpp DESTINATION bin)
