cmake_minimum_required(VERSION 2.8.12)

project("VTR")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds not allowed. Use the Makefile wrapper (e.g. make), or create a new build directory and call cmake manually from there (e.g. mkdir -p build && cd build && cmake .. && make). You may need to 'rm -rf CMakeCache.txt CMakeFiles' first.")
endif()

#We install to the source directory by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "default install path" FORCE)
endif()

#Allow the user to configure how much assertion checking should occur
set(VTR_ASSERT_LEVEL "2" CACHE STRING "VTR assertion checking level. 0: no assertions, 1: fast assertions, 2: regular assertions, 3: all assertions (including those with significant run-time cost)")
set_property(CACHE VTR_ASSERT_LEVEL PROPERTY STRINGS 0 1 2 3)

option(VTR_ENABLE_SANITIZE "Enable address/leak/undefined-behaviour sanitizers (i.e. run-time error checking)" OFF)
option(VTR_ENABLE_PROFILING "Enable performance profiler (gprof)" OFF)
option(VTR_ENABLE_COVERAGE "Enable code coverage tracking (gcov)" OFF)

set(VTR_VERSION_MAJOR 8)
set(VTR_VERSION_MINOR 0)
set(VTR_VERSION_PATCH 0)
set(VTR_VERSION_PRERELEASE "dev")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(FilesToDirs)

#
#
# Determine compiler configuration
#
#

#Set the default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build: None, Debug, Release, RelWithDebInfo, MinSizeRel"
        FORCE)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

#
#Set the assertion level
#
add_definitions("-DVTR_ASSERT_LEVEL=${VTR_ASSERT_LEVEL}")

#Compiler flag configuration checks
include(CheckCXXCompilerFlag)

#
# We require c++14 support
#
CHECK_CXX_COMPILER_FLAG("-std=c++14" CXX_COMPILER_SUPPORTS_CXX14_FLAG)
if(CXX_COMPILER_SUPPORTS_CXX14_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
else()
    message(FATAL_ERROR "Could not determine C++14 support for compiler ${CMAKE_CXX_COMPILER}. C++14 support is required.")
endif()

#
# Warning flags
#
set(WARN_FLAGS "") #The actual warning flags to be applied

if(MSVC)
    #Visual studio warnings
    # Note that we don't use /Wall since it generates warnings about standard library headers
    set(WARN_FLAGS_TO_CHECK  #The flags to check if the compiler supports
        "/W4" #Most warnings
        )
else()
    set(WARN_FLAGS_TO_CHECK  #The flags to check if the compiler supports
        #GCC-like
        "-Wall"                         #Most warnings, typically good
        "-Wextra"                       #Extra warning, usually good
        "-Wpedantic"                    #Ensure ISO compliance (i.e. no non-standard extensions)
        "-Wcast-qual"                   #Warn if cast removes qualifier (e.g. const char* -> char*)
        "-Wcast-align"                  #Warn if a cast causes memory alignment changes
        "-Wshadow"                      #Warn if local variable shadows another variable
        "-Wformat=2"                    #Sanity checks for printf-like formatting
        "-Wno-format-nonliteral"        # But don't worry about non-literal formtting (i.e. run-time printf format strings)
        "-Wlogical-op"                  #Checks for logical op when bit-wise expected
        "-Wmissing-declarations"        #Warn if a global function is defined with no delcaration
        "-Wmissing-include-dirs"        #Warn if a user include directory is missing
        "-Wredundant-decls"             #Warn if there are overlapping declarations
        "-Wswitch-default"              #Warn if a switch has no default
        "-Wundef"                       #Warn if #if() preprocessor refers to an undefined directive
        "-Wunused-variable"             #Warn about variables that are not used
        "-Wdisabled-optimization"       #Warn when optimizations are skipped (usually due to large/complex code)
        "-Wnoexcept"                    #Warn when functions should be noexcept (i.e. compiler know it doesn't throw)
        "-Woverloaded-virtual"          #Warn when a function declaration overrides a virtual method
        "-Wctor-dtor-privacy"           #Warn about inaccessible constructors/destructors
        "-Wnon-virtual-dtor"            #Warn about missing virtual destructors
        #GCC-like optional
        #"-Wold-style-cast"              #Warn about using c-style casts
        #"-Wconversion"                  #Warn when type conversions may change value
        #"-Wsign-conversion"             #Warn if a conversion may change the sign
        #"-Wpadded"                      #Will warn if additional padding is introduced to a struct/class. Turn on if optimizing class memory layouts
        #"-Wstrict-overflow=2"           #Warn if the compiler optimizes assuming signed overflow does not occur
        #"-Wfloat-equal"                 #Warn about using direct floating point equality
        #"-Wunsafe-loop-optimizations"   #Warn when loops can't be optimized
        #"-Wswitch-enum"                 #Warn about uncovered enumeration values in a switch (even if there is a default)
        #"-Wsign-promo"                  #Warn when overload resolution converts an unsigned type to signed when an unsigned overload exists
        )
endif()

#Check and see if the compiler supports the various warning flags,
#and add valid flags
foreach(flag ${WARN_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
    if(CXX_COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(WARN_FLAGS "${WARN_FLAGS} ${flag}")
    endif()
endforeach()

#The flex/bison code is not warning clean so we need to suppress some warnings
set(FLEX_BISON_WARN_SUPPRESS_FLAGS "")
set(FLEX_BISON_WARN_SUPPRESS_FLAGS_TO_CHECK
    "-Wno-redundant-decls"  #Flex/bison generate code with redundant declarations
    "-Wno-switch-default"   #Flex/bison generate switch statments w/o default cases
    "-Wno-unused-parameter" #Flex produces functions with unused params in re-entrant mode
    "-Wno-missing-declarations" #Flex misses some declarations in re-entrant mode
    "-Wimplicit-fallthrough=0" #Bison produces some cases with explicit
    "-Wno-sign-compare" #Flex generates code which performs some signed/unsigned comparison
    )
foreach(flag ${FLEX_BISON_WARN_SUPPRESS_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
    if(CXX_COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(FLEX_BISON_WARN_SUPPRESS_FLAGS "${FLEX_BISON_WARN_SUPPRESS_FLAGS} ${flag}")
    endif()
endforeach()

#
# Sanitizer flags
#

set(SANITIZE_FLAGS "")
if(VTR_ENABLE_SANITIZE)
    #Enable sanitizers
    # -fuse-ld=gold force the gold linker to be used (required for sanitizers, but not enabled by default on some systems)
    set(SANITIZE_FLAGS "-g -fsanitize=address -fsanitize=leak -fsanitize=undefined -fuse-ld=gold")
    message(STATUS "SANTIIZE_FLAGS: ${SANITIZE_FLAGS}")
endif()

#
# Profiling flags
#

#Support for gprof
set(PROFILING_FLAGS "")
if(VTR_ENABLE_PROFILING)
    #Enable gprof
    set(PROFILING_FLAGS "-g -pg")
    message(STATUS "Profiling Flags: ${PROFILING_FLAGS}")
endif()

#
# Code coverage flags
#

#Support for gcov
set(COVERAGE_FLAGS "")
if(VTR_ENABLE_COVERAGE)
    #Enable gcov
    set(COVERAGE_FLAGS "-g -ftest-coverage -fprofile-arcs")
    message(STATUS "Coverage Flags: ${COVERAGE_FLAGS}")
endif()

#
# Set final flags
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS} ${SANITIZE_FLAGS} ${PROFILING_FLAGS} ${COVERAGE_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


#
# Titan Benchmarks
#
add_custom_target(get_titan_benchmarks
    COMMAND ./vtr_flow/scripts/download_titan.py --vtr_flow_dir ./vtr_flow
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Downloading (~1GB) and extracting Titan benchmarks (~10GB) into VTR source tree.")

#
# Sub-projects
#

#Add the various sub-projects
add_subdirectory(liblog)
add_subdirectory(libpugixml)
add_subdirectory(libpugiutil)
add_subdirectory(libvtrutil)
add_subdirectory(libargparse)
add_subdirectory(libarchfpga)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
add_subdirectory(libeasygl)
add_subdirectory(libtatum)
add_subdirectory(vpr)
add_subdirectory(abc_with_bb_support)
add_subdirectory(ODIN_II)
add_subdirectory(ace2)

#Add extra compilation flags to suppress warnings from some libraries
# Note that target_compile_options() *appends* to the current compilation options of
# the specified target

#PugiXml has some deliberate switch fallthrough cases (as indicated by comments), but they
#are tagged as warnings with g++-7 (the comments don't match g++-7's suppression regexes). 
#Since we don't want to change PugiXml (it is developed externally), we relax the warning 
#level so no fallthrough warnings are generated
CHECK_CXX_COMPILER_FLAG("-Wimplicit-fallthrough=0" CXX_COMPILER_SUPPORTS_-Wimplicit-fallthrough=0)
if(CXX_COMPILER_SUPPORTS_-Wimplicit-fallthrough=0)
    target_compile_options(libpugixml PRIVATE "-Wimplicit-fallthrough=0")
endif()


#Set output locations to be in the main source tree under the relevant folder
set_target_properties(liblog test_log
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/liblog"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/liblog"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/liblog")
set_target_properties(libpugixml
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libpugixml"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libpugixml"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libpugixml")
set_target_properties(libpugiutil
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libpugiutil"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libpugiutil"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libpugiutil")
set_target_properties(libvtrutil
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libvtrutil"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libvtrutil"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libvtrutil")
set_target_properties(libarchfpga read_arch
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libarchfpga"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libarchfpga"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libarchfpga")
set_target_properties(libsdcparse sdcparse_test
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libsdcparse"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libsdcparse"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libsdcparse")
set_target_properties(libblifparse blifparse_test
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libblifparse"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libblifparse"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libblifparse")
set_target_properties(libeasygl
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libeasygl"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libeasygl"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libeasygl")
set_target_properties(libtatum
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libtatum"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libtatum"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libtatum")
set_target_properties(libabc abc 
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/abc_with_bb_support"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/abc_with_bb_support"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/abc_with_bb_support")
set_target_properties(libvpr vpr
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vpr"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vpr"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vpr")
set_target_properties(ace
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ace2"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ace2"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ace2")
set_target_properties(odin_II
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ODIN_II"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ODIN_II"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ODIN_II")
