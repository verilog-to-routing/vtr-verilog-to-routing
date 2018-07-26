cmake_minimum_required(VERSION 2.8.12)

project("blifexplorer")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Core REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

#Collect the source files
file(GLOB_RECURSE QRC_EXPLORERRES src/explorerres.qrc)
file(GLOB_RECURSE UI_CLOCKCONFIG src/clockconfig.ui)
file(GLOB_RECURSE EXEC_SOURCES src/*.cpp)
file(GLOB_RECURSE EXEC_HEADERS src/*.h)
files_to_dirs(EXEC_HEADERS EXEC_INCLUDE_DIRS)

include_directories(${EXEC_INCLUDE_DIRS})

#Create the executable
add_executable(blifexplorer 
                ${UI_CLOCKCONFIG}
                ${QRC_EXPLORERRES}
                ${EXEC_SOURCES} 
                ${EXEC_HEADERS})

target_link_libraries(blifexplorer
                        libodin_ii
                        Qt5::Widgets
                        Qt5::Core
                        Qt5::Gui
                        ${CMAKE_DL_LIBS})

install(TARGETS blifexplorer DESTINATION bin)
