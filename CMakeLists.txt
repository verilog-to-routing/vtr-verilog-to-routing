cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# create the project
project(
  ezgl
  VERSION 1.0.1
  LANGUAGES CXX
)

# we rely on GTK3 for the GUI, so make sure the system has it
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 QUIET gtk+-3.0)
pkg_check_modules(X11 QUIET x11)

if(NOT GTK3_FOUND)
  message(WARNING "EZGL: Failed to find required GTK3 library (on debian/ubuntu try 'sudo apt-get install libgtk-3-dev' to install)")
endif()

# we also rely on glib to compile the GTK resource files
# a set of macros has been developed by Makman2 on GitHub to help with this
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gcr-cmake/macros)

#Is ezgl the root cmake project?
set(IS_ROOT_PROJECT TRUE)
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    set(IS_ROOT_PROJECT FALSE)
endif()

# include the configuration/compile time options for this library
include(options.cmake)

# create a library that can be linked by executables
add_library(
  ${PROJECT_NAME}
  include/ezgl/application.hpp
  include/ezgl/camera.hpp
  include/ezgl/canvas.hpp
  include/ezgl/color.hpp
  include/ezgl/control.hpp
  include/ezgl/callback.hpp
  include/ezgl/graphics.hpp
  include/ezgl/point.hpp
  include/ezgl/rectangle.hpp
  src/application.cpp
  src/camera.cpp
  src/canvas.cpp
  src/control.cpp
  src/callback.cpp
  src/graphics.cpp
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC include
)

#Treat GTK/X11 headers as system headers so they
#do not generate compilation warnings
target_include_directories(
  ${PROJECT_NAME}
  SYSTEM
  PUBLIC ${GTK3_INCLUDE_DIRS}
  PUBLIC ${X11_INCLUDE_DIRS}
)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC ${GTK3_LIBRARIES}
  PUBLIC ${X11_LIBRARIES}
)

# add_compile_options does not seem to be working on the UG machines,
# and we cannot set target properties in version 3.0.2
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(EZGL_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(EZGL_BUILD_DOCS)
  add_subdirectory(doc)
endif()
