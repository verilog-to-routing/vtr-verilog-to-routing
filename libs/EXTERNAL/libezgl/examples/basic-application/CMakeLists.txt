cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(
  basic-application
  VERSION 0.0.1
  LANGUAGES CXX
)

# create the resouce list
set(
  RESOURCE_LIST
  # Strip all the whitespace characters from ui file
  STRIPBLANKS main.ui
)

# include the macros used to generate/compile the resources
include(GlibCompileResourcesSupport)

# compile the resources---the generated files will be in the build directory
compile_gresources(
  # input: the name of our resources
  RESOURCE_FILE
  # output: the filename of the generated XML file
  XML_OUT
  # generate C code to be compiled with our program
  TYPE
    EMBED_C
  # specify the name of the C file that is generated
  TARGET
    resources.C
  # specify the resource prefix (used in the code)
  PREFIX
    /ezgl
  # input: specify the list of files to compile into resources
  RESOURCES
    ${RESOURCE_LIST}
)

# make sure that the resources are compiled when the project is built
add_custom_target(
  resource-basic ALL
  DEPENDS
    ${RESOURCE_FILE}
)

add_executable(
  ${PROJECT_NAME}
  basic_application.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/resources.C
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE ezgl
)
