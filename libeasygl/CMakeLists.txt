project("libeasygl")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules") #For FindFontconfig

option(ENABLE_GRAPHICS "Enables easygl graphics" ON)

file(GLOB_RECURSE LIB_SOURCES src/*.c)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
get_include_directories(LIB_HEADERS LIB_INCLUDE_DIRS)

#Treat .c as CXX
set_source_files_properties(${LIB_SOURCES} ${LIB_HEADERS} PROPERTIES LANGUAGE CXX)

#Handle graphics setup
set(GRAPHICS_LIBRARIES "")
set(GRAPHICS_INCLUDES "")
set(GRAPHICS_DEFINES "")

if(ENABLE_GRAPHICS)
    if(WIN32)
        #Windows
        list(GRAPHICS_DEFINES APPEND "-DWIN32")
    else()
        #Assume unix-like

        #Look for X11 and font libraries
        message(STATUS "EasyGL: Looking for X11 and font libraries")
        find_package(X11 COMPONENTS X11 Xft)
        find_package(Freetype)
        find_package(Fontconfig)

        set(FOUND_EASYGL_DEPENDANCIES true)

        if(NOT X11_FOUND)
            set(FOUND_EASYGL_DEPENDANCIES  false)
            message(WARNING "EasyGL: Failed to find required X11 library (on debian/ubuntu try 'sudo apt-get install libx11-dev' to install)")
        endif()

        if(NOT FREETYPE_FOUND)
            set(FOUND_EASYGL_DEPENDANCIES  false)
            message(WARNING "EasyGL: Failed to find required Xft library (on debian/ubuntu try 'sudo apt-get install libxft-dev' to install)")
        endif()

        if(NOT FONTCONFIG_FOUND)
            set(FOUND_EASYGL_DEPENDANCIES  false)
            message(WARNING "EasyGL: Failed to find required fontconfig library (on debian/ubuntu try 'sudo apt-get install fontconfig' to install)")
        endif()

        if(FOUND_EASYGL_DEPENDANCIES)
            #Got all libraries, enable
            list(APPEND GRAPHICS_LIBRARIES ${X11_X11_LIB} ${X11_Xft_LIB} ${FREETYPE_LIBRARIES} ${FONTCONFIG_LIBRARIES})
            list(APPEND GRAPHICS_INCLUDES ${X11_X11_INCLUDE_PATH} ${X11_Xft_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS})
        else()
            #Disable
            set(ENABLE_GRAPHICS false)
        endif()
    endif()
endif()

if(ENABLE_GRAPHICS)
    message(STATUS "EasyGL: enabled")
else()
    list(APPEND GRAPHICS_DEFINES "-DNO_GRAPHICS")
    message(STATUS "EasyGL: disabled")
endif()

#Specify defines and includes
add_definitions(${GRAPHICS_DEFINES})
include_directories(${GRAPHICS_INCLUDES})

#Create the library
add_library(libeasygl STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libeasygl PUBLIC ${LIB_INCLUDE_DIRS} ${GRAPHICS_INCLUDES})
set_target_properties(libeasygl PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

target_link_libraries(libeasygl 
                        ${GRAPHICS_LIBRARIES})
