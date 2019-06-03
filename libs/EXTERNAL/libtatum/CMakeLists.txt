cmake_minimum_required(VERSION 3.9)

project("tatum")

set(TATUM_EXECUTION_ENGINE "auto" CACHE STRING "Specify the framework for (potential) parallel execution")
set_property(CACHE TATUM_EXECUTION_ENGINE PROPERTY STRINGS auto serial tbb)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    #Set the default build type if not specified
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build: None, Debug, Release, RelWithDebInfo, MinSizeRel"
            FORCE)
    endif()
    message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")


    #Only set compiler flags if not a sub-project
    set(WARN_FLAGS -Wall -Wextra -Wpedantic -Wcast-qual -Wcast-align -Wshadow -Wformat=2 -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wswitch-default -Wundef -Wunused-variable -Wdisabled-optimization -Wnoexcept -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor)

    add_compile_options(${WARN_FLAGS})
    add_compile_options(-std=c++14)

    set(FLEX_BISON_WARN_SUPPRESS_FLAGS -Wno-switch-default -Wno-unused-parameter -Wno-sign-compare -Wno-missing-declarations)
endif()

add_subdirectory(libtatum)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    #Only build the parser, test executable and docs if not a sub-project
    add_subdirectory(tatum_test)
    add_subdirectory(libtatumparse)
    add_subdirectory(tatumparse_test)
    add_subdirectory(doc)
endif()
