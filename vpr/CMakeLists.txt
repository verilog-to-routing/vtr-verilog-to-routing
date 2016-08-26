project("vpr")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules") #For FindFontconfig

#Handle graphics
find_package(X11 COMPONENTS Xft)
find_package(Freetype)
find_package(Fontconfig)
include_directories(${X11_INCLUDE_DIRS})
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${FONTCONFIG_INCLUDE_DIRS})

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES SRC/main.c)
file(GLOB_RECURSE LIB_SOURCES SRC/*/*.c)
file(GLOB_RECURSE LIB_HEADERS SRC/*/*.h)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)

add_definitions(-DBUILD_VERSION="0.0.0")
add_definitions(-DBUILD_DATE="0-0-0")

#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} ${EXEC_SOURCES} PROPERTIES LANGUAGE CXX)

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
                        ${X11_LIBRARIES}
                        ${X11_Xft_LIB}
                        ${FREETYPE_LIBRARIES}
                        ${FONTCONFIG_LIBRARIES})

#Create the executable
add_executable(vpr ${EXEC_SOURCES})
target_link_libraries(vpr 
                        libvpr)
