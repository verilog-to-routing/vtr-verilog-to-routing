cmake_minimum_required(VERSION 3.9)

project("libargparse")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    #Only set compiler settings if we are not a sub-project
    set(WARN_FLAGS "-Wall -Wextra -Wpedantic -Wcast-qual -Wcast-align -Wshadow -Wformat=2 -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wswitch-default -Wundef -Wunused-variable -Wdisabled-optimization -Wnoexcept -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14 ${WARN_FLAGS}") 
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined") 
    set(FLEX_BISON_WARN_SUPPRESS_FLAGS "-Wno-switch-default -Wno-unused-parameter -Wno-missing-declarations")
endif()

set(LIB_INCLUDE_DIRS src)
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp)

#Create the library
add_library(libargparse STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
set_target_properties(libargparse PROPERTIES PREFIX "") #Avoid extra 'lib' prefix
target_include_directories(libargparse PUBLIC ${LIB_INCLUDE_DIRS})

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    #Create the test executable
    add_executable(argparse_test argparse_test.cpp)
    target_link_libraries(argparse_test libargparse)

    #Create the example executable
    add_executable(argparse_example argparse_example.cpp)
    target_link_libraries(argparse_example libargparse)
endif()
