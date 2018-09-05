cmake_minimum_required(VERSION 2.8.12)

project("blifexplorer")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5Widgets)
find_package(Qt5Gui)
find_package(Qt5Core)

#Check dependencies
if(NOT Qt5Widgets_FOUND)
    message(FATAL_ERROR "Failed to find Qt5Widgets required for blifexplorer (on debian/ubuntu try 'sudo apt install qt5-default')")
endif()

if(NOT Qt5Gui_FOUND)
    message(FATAL_ERROR "Failed to find Qt5Gui required for blifexplorer (on debian/ubuntu try 'sudo apt install qt5-default')")
endif()

if(NOT Qt5Core_FOUND)
    message(FATAL_ERROR "Failed to find Qt5Core required for blifexplorer (on debian/ubuntu try 'sudo apt install qt5-default')")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#Collect the source files
set(QRC_EXPLORERRES src/explorerres.qrc)

set(UI_CLOCKCONFIG src/clockconfig.ui)

file(GLOB_RECURSE EXEC_SOURCES src/*.cpp)

file(GLOB_RECURSE EXEC_HEADERS src/*.h)
files_to_dirs(EXEC_HEADERS EXEC_INCLUDE_DIRS)

#The generated header files appear in the CMAKE_CURRENT_BINARY_DIR, so ensure
#it is in the include directories
include_directories(${EXEC_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

#Create the executable
add_executable(blifexplorer 
                ${UI_CLOCKCONFIG}
                ${QRC_EXPLORERRES}
                ${EXEC_SOURCES} 
                ${EXEC_HEADERS}
)

target_link_libraries(blifexplorer
                        liblog
                        libarchfpga
                        libvtrutil
                        libodin_ii
                        libargparse
                        Qt5::Widgets
                        Qt5::Core
                        Qt5::Gui
                        ${CMAKE_DL_LIBS}
)

install(TARGETS blifexplorer DESTINATION bin)
