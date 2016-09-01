project("libvtrutil")

#Version info
#
# We add a custom target 'version' which is always out of date.
# This ensures it will be called every time the project is built, ensuring
# the git revision and dirty status are up to date.
#
# That target calls the configure_version.cmake script to generate
# an up-to-date vtr_version.cpp
set(VTR_VERSION_FILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/src/vtr_version.cpp.in)
set(VTR_VERSION_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/vtr_version.cpp)
set(VTR_COMPILER_INFO "${CMAKE_CXX_COMPILER_ID} on ${CMAKE_SYSTEM}")
add_custom_target(
    version
    ${CMAKE_COMMAND} -D IN_FILE=${VTR_VERSION_FILE_IN}
                     -D OUT_FILE=${VTR_VERSION_FILE_OUT}
                     -D VTR_VERSION_MAJOR=${VTR_VERSION_MAJOR}
                     -D VTR_VERSION_MINOR=${VTR_VERSION_MINOR}
                     -D VTR_VERSION_PATCH=${VTR_VERSION_PATCH}
                     -D VTR_VERSION_PRERELEASE=${VTR_VERSION_PRERELEASE}
                     -D VTR_COMPILER_INFO=${VTR_COMPILER_INFO}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/configure_version.cmake)


#
# Source files and library
#
file(GLOB_RECURSE LIB_SOURCES src/*.cpp ${VTR_VERSION_FILE_OUT})
file(GLOB_RECURSE LIB_HEADERS src/*.hpp src/*.h)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)

message("VTR UTIL SOURCES HEADERS: ${LIB_SOURCES} ${LIB_HEADERS}")

#Create the library
add_library(libvtrutil STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libvtrutil PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libvtrutil PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Ensure version is always up to date by requiring version to be run first
add_dependencies(libvtrutil version)

#Specify link-time dependancies
target_link_libraries(libvtrutil
                        liblog)

install(TARGETS libvtrutil DESTINATION bin)
