cmake_minimum_required(VERSION 3.9)

project("libeasygl")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules") #For FindFontconfig

option(EASYGL_ENABLE_GRAPHICS "Enables easygl graphics" ON)

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} PROPERTIES LANGUAGE CXX)

#Handle graphics setup
set(GRAPHICS_LIBRARIES "")
set(GRAPHICS_INCLUDES "")
set(GRAPHICS_DEFINES "")

if(EASYGL_ENABLE_GRAPHICS)
    #Check for dependancies
    set(FOUND_EASYGL_DEPENDANCIES true)

    if(WIN32 OR CYGWIN) #CMake in CYGWIN does not define WIN32
        #Windows
        message(STATUS "EasyGL: Windows detected")
        list(APPEND GRAPHICS_DEFINES "-DWIN32")
        if(CYGWIN)
            #CYGWIN requires explicitly linking to the windows platform libraries
            list(APPEND GRAPHICS_LIBRARIES gdi32 User32)
        endif()
    else()
        #Assume unix-like

        #Look for X11, font and cairo libraries
        message(STATUS "EasyGL: UNIX detected, looking for X11 and font libraries")
        find_package(Fontconfig)
        find_package(X11 COMPONENTS X11 Xft)
        find_package(Freetype)
        find_package(Cairo)

        if(NOT FONTCONFIG_FOUND)
            set(FOUND_EASYGL_DEPENDANCIES  false)
            message(WARNING "EasyGL: Failed to find required fontconfig library (on debian/ubuntu try 'sudo apt-get install fontconfig' to install)")
        endif()

        if(NOT X11_FOUND)
            set(FOUND_EASYGL_DEPENDANCIES  false)
            message(WARNING "EasyGL: Failed to find required X11 library (on debian/ubuntu try 'sudo apt-get install libx11-dev' to install)")
        endif()

        if(NOT FREETYPE_FOUND)
            set(FOUND_EASYGL_DEPENDANCIES  false)
            message(WARNING "EasyGL: Failed to find required Xft library (on debian/ubuntu try 'sudo apt-get install libxft-dev' to install)")
        endif()

        if(NOT CAIRO_FOUND)
            set(FOUND_EASYGL_DEPENDANCIES  false)
            message(WARNING "EasyGL: Failed to find required cairo library (on debian/ubuntu try 'sudo apt-get install libcairo2-dev' to install)")
        endif()

        if(FOUND_EASYGL_DEPENDANCIES)
            #Got all libraries, enable
            list(APPEND GRAPHICS_LIBRARIES ${X11_X11_LIB} ${X11_Xft_LIB} ${FREETYPE_LIBRARIES} ${FONTCONFIG_LIBRARIES} ${CAIRO_LIBRARIES})
            list(APPEND GRAPHICS_INCLUDES ${X11_X11_INCLUDE_PATH} ${X11_Xft_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS})
        else()
            #Disable
            set(EASYGL_ENABLE_GRAPHICS false)
        endif()
    endif()
endif()

if(EASYGL_ENABLE_GRAPHICS)
    message(STATUS "EasyGL: graphics enabled")
else()
    list(APPEND GRAPHICS_DEFINES "-DNO_GRAPHICS")
    message(STATUS "EasyGL: graphics disabled")
endif()

#Create the library
add_library(libeasygl STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libeasygl PUBLIC ${LIB_INCLUDE_DIRS} ${GRAPHICS_INCLUDES})
target_compile_definitions(libeasygl PUBLIC ${GRAPHICS_DEFINES})
set_target_properties(libeasygl PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

target_link_libraries(libeasygl 
                        ${GRAPHICS_LIBRARIES})

install(TARGETS libeasygl DESTINATION bin)
