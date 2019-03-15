cmake_minimum_required(VERSION 2.8.12)

if (${CMAKE_VERSION} VERSION_GREATER "3.8")
    #For cmake >= 3.9 INTERPROCEDURAL_OPTIMIZATION behaviour we need to explicitly
    #set the cmake policy version number
    cmake_policy(VERSION 3.9) 

    # If we are using verison < 3.9 then setting INTERPROCEDURAL_OPTIMIZATION
    # has no effect unless an Intel compiler is used
endif()

project("vpr")

option(VPR_USE_SIGNAL_HANDLER "Should VPR use a signal handler to intercept signals (e.g. SIGINT)?" OFF)
option(VPR_IPO_BUILD "Should VPR be compiled with an interprocedural optimizations?" ON)

set(VPR_PGO_CONFIG "none" CACHE STRING "Configure VPR Profile-Guided Optimization (PGO). prof_gen: built executable will produce profiling info, prof_use: built executable will be optimized based on generated profiling info, none: disable pgo")
set_property(CACHE VPR_PGO_CONFIG PROPERTY STRINGS prof_gen prof_use none)

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

if (VPR_USE_SIGNAL_HANDLER)
    #Check wheter VPR can use sigaction to handle signals (only supported by POSIX)
    CHECK_CXX_SYMBOL_EXISTS(sigaction csignal HAVE_SIGACTION)
    if(HAVE_SIGACTION)
        target_compile_definitions(libvpr PRIVATE VPR_USE_SIGACTION)    
    endif()
endif()

install(TARGETS vpr libvpr DESTINATION bin)
