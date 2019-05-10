cmake_minimum_required(VERSION 2.8.12)

if (${CMAKE_VERSION} VERSION_GREATER "3.8")
    #For cmake >= 3.9 INTERPROCEDURAL_OPTIMIZATION behaviour we need to explicitly
    #set the cmake policy version number
    cmake_policy(VERSION 3.9) 

    # If we are using verison < 3.9 then setting INTERPROCEDURAL_OPTIMIZATION
    # has no effect unless an Intel compiler is used
endif()

project("vpr")

#
# VPR Build Options
#

set(VPR_EXECUTION_ENGINE "auto" CACHE STRING "Specify the framework for (potential) parallel execution")
set_property(CACHE VPR_EXECUTION_ENGINE PROPERTY STRINGS auto serial tbb)

option(VPR_USE_SIGNAL_HANDLER "Should VPR use a signal handler to intercept signals (e.g. SIGINT)?" OFF)

option(VPR_IPO_BUILD "Should VPR be compiled with an interprocedural optimizations?" ON)

set(VPR_PGO_CONFIG "none" CACHE STRING "Configure VPR Profile-Guided Optimization (PGO). prof_gen: built executable will produce profiling info, prof_use: built executable will be optimized based on generated profiling info, none: disable pgo")
set_property(CACHE VPR_PGO_CONFIG PROPERTY STRINGS prof_gen prof_use none)

#
# Build Configuration
#
include(CheckCXXSymbolExists)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

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
                        libblifparse
                        libeasygl
                        libtatum
                        libargparse
                        libpugixml)

#Create the executable
add_executable(vpr ${EXEC_SOURCES})
target_link_libraries(vpr
                        libvpr)

if (VPR_IPO_BUILD)
    message(STATUS "Building VPR with IPO")
    set_property(TARGET libvpr PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET vpr PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()


#
# Profile Guilded Optimization Configuration
#
set(PROF_GEN_FLAGS_TO_CHECK 
    #GCC-like
    "-fprofile-generate" #Build will generate profiling information
    )
set(PROF_USE_FLAGS_TO_CHECK
    #GCC-like
    "-fprofile-use"     #Build will use previously generated profiling information to guide code optimization
    "-Wmissing-profile" #Warn if the used profiling information doesn't match the source code or is missing
    )

if (VPR_PGO_CONFIG STREQUAL "prof_gen")
    message(STATUS "Building VPR to generate profiling data for PGO")
    foreach(flag ${PROF_GEN_FLAGS_TO_CHECK})
        CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
        if(CXX_COMPILER_SUPPORTS_${flag})
            target_compile_options(libvpr PUBLIC ${flag})
            target_compile_options(vpr PUBLIC ${flag})
            target_link_libraries(vpr ${flag})
        endif()
    endforeach()
elseif (VPR_PGO_CONFIG STREQUAL "prof_use")
    message(STATUS "Building VPR using generated profiling data for PGO")
    foreach(flag ${PROF_USE_FLAGS_TO_CHECK})
        CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
        if(CXX_COMPILER_SUPPORTS_${flag})
            target_compile_options(libvpr PUBLIC ${flag})
            target_compile_options(vpr PUBLIC ${flag})
            target_link_libraries(vpr ${flag})
        endif()
    endforeach()
elseif (VPR_PGO_CONFIG STREQUAL "none")
    #Pass
else()
    message(ERROR "Unsupported VPR_PGO_CONFIG '${VPR_PGO_CONFIG}'")
endif()

    find_package(TBB)

#
# Execution Engine Configuration
#

#Figure out which engine to use
if (VPR_EXECUTION_ENGINE STREQUAL "serial")
    set(VPR_USE_EXECUTION_ENGINE "serial")
else()
    find_package(TBB)

    if (VPR_EXECUTION_ENGINE STREQUAL "auto")
        if (TBB_FOUND)
            set(VPR_USE_EXECUTION_ENGINE "tbb") 
        else()
            set(VPR_USE_EXECUTION_ENGINE "serial") 
        endif()
    elseif(VPR_EXECUTION_ENGINE STREQUAL "tbb")
        if (TBB_FOUND)
            set(VPR_USE_EXECUTION_ENGINE "tbb") 
        else()
            message(FATAL_ERROR "VPR: TBB requested but not found (on debian/ubuntu try 'sudo apt install libtbb-dev'")
        endif()
    endif()
endif()

#Configure the build to use the selected engine
if (VPR_USE_EXECUTION_ENGINE STREQUAL "tbb")
    target_compile_definitions(libvpr PRIVATE VPR_USE_TBB)
    target_link_libraries(libvpr tbb)
    target_link_libraries(libvpr tbbmalloc_proxy) #Use the scalable memory allocator
    message(STATUS "VPR: will support parallel execution using '${VPR_USE_EXECUTION_ENGINE}'")
elseif(VPR_USE_EXECUTION_ENGINE STREQUAL "serial")
    message(STATUS "VPR: will only support serial execution")
else()
    message(FATAL_ERROR "VPR: Unrecognized execution engine '${VPR_USE_EXECUTION_ENGINE}'")
endif()

#
# Signal handler configuration
#
if (VPR_USE_SIGNAL_HANDLER)
    #Check wheter VPR can use sigaction to handle signals (only supported by POSIX)
    CHECK_CXX_SYMBOL_EXISTS(sigaction csignal HAVE_SIGACTION)
    if(HAVE_SIGACTION)
        target_compile_definitions(libvpr PRIVATE VPR_USE_SIGACTION)
    endif()
endif()

install(TARGETS vpr libvpr DESTINATION bin)

#
# Unit Tests
#
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
add_executable(test_vpr ${TEST_SOURCES})
target_link_libraries(test_vpr
                        libcatch
                        libvpr)

add_test(NAME test_vpr
    COMMAND test_vpr --use-colour=yes
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
