cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(SupportCcache)

project("VTR")

#
# VTR Build Options
#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds not allowed. Use the Makefile wrapper (e.g. make), or create a new build directory and call cmake manually from there (e.g. mkdir -p build && cd build && cmake .. && make). You may need to 'rm -rf CMakeCache.txt CMakeFiles' first.")
endif()

#We install to the source directory by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "default install path" FORCE)
endif()

set(VTR_IPO_BUILD "auto" CACHE STRING "Should VTR be compiled with interprocedural compiler optimizations?")
set_property(CACHE VTR_IPO_BUILD PROPERTY STRINGS auto on off)

#Allow the user to configure how much assertion checking should occur
set(VTR_ASSERT_LEVEL "2" CACHE STRING "VTR assertion checking level. 0: no assertions, 1: fast assertions, 2: regular assertions, 3: additional assertions with noticeable run-time overhead, 4: all assertions (including those with significant run-time cost)")
set_property(CACHE VTR_ASSERT_LEVEL PROPERTY STRINGS 0 1 2 3 4)

option(VTR_ENABLE_SANITIZE "Enable address/leak/undefined-behaviour sanitizers (i.e. run-time error checking)" OFF)
option(VTR_ENABLE_PROFILING "Enable performance profiler (gprof)" OFF)
option(VTR_ENABLE_COVERAGE "Enable code coverage tracking (gcov)" OFF)
option(VTR_ENABLE_DEBUG_LOGGING "Enable debug logging" OFF)
option(VTR_ENABLE_VERBOSE "Enable increased debug verbosity" OFF)
option(SPEC_CPU "Enable SPEC CPU v8 support" OFF)

#Allow the user to decide whether to compile the graphics library
set(VPR_USE_EZGL "auto" CACHE STRING "Specify whether vpr uses the graphics library")
set_property(CACHE VPR_USE_EZGL PROPERTY STRINGS auto off on)
option(VTR_ENABLE_CAPNPROTO "Enable capnproto binary serialization support in VPR." ON)

#Allow the user to decide whether to compile the server module
option(VPR_USE_SERVER "Specify whether vpr enables the server mode" ON)

option(VPR_ENABLE_INTERCHANGE "Enable FPGA interchange." ON)
option(VPR_ENABLE_NOC_SAT_ROUTING "Enable NoC SAT routing." OFF)

option(WITH_BLIFEXPLORER "Enable build with blifexplorer" OFF)

option(WITH_ABC "Enable building abc" ON)
option(WITH_ODIN "Enable building odin" OFF)
option(ODIN_DEBUG "Enable building odin with debug flags" OFF)
option(ODIN_WARN "Enable building odin with extra warning flags" OFF)
option(ODIN_COVERAGE "Enable building odin with coverage flags" OFF)
option(ODIN_TIDY "Enable building odin with clang tidy" OFF)
option(ODIN_SANITIZE "Enable building odin with sanitize flags" OFF)

# Allow the user to enable building Yosys
option(WITH_PARMYS "Enable Yosys as elaborator and parmys-plugin as partial mapper" ON)
option(SLANG_SYSTEMVERILOG "Enable building and installing Yosys-Slang plugin for parsing SystemVerilog" ON)

set(VTR_VERSION_MAJOR 9)
set(VTR_VERSION_MINOR 0)
set(VTR_VERSION_PATCH 0)
set(VTR_VERSION_PRERELEASE "dev")

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
# We require c++20 support
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #No compiler specific extensions

#
# Interprocedural optimization
#
#   Note that we manually clear the INTERPROCEDURAL_OPTIMIZATION flag on ABC later
#   to avoid cmake warnings
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)
if (VTR_IPO_BUILD STREQUAL "on")
    if (IPO_SUPPORTED)
        message(STATUS "Building with IPO: on")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(ERROR "Building with IPO unsupported with this compiler!")
    endif()
elseif(VTR_IPO_BUILD STREQUAL "auto")
    if (IPO_SUPPORTED AND NOT CMAKE_BUILD_TYPE STREQUAL "debug")
        message(STATUS "Building with IPO: on (auto)")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    else()
        message(STATUS "Building with IPO: off (auto)")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fPIC")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC")
    endif()
else()
    message(STATUS "Building with IPO: off")
endif()

#
# Build type flags
#

if(NOT MSVC)
    # for GCC and Clang
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

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
        "-Wno-format-nonliteral"        # But don't worry about non-literal formatting (i.e. run-time printf format strings)
        "-Wlogical-op"                  #Checks for logical op when bit-wise expected
        "-Wmissing-declarations"        #Warn if a global function is defined with no declaration
        "-Wmissing-include-dirs"        #Warn if a user include directory is missing
        "-Wredundant-decls"             #Warn if there are overlapping declarations
        "-Wswitch-default"              #Warn if a switch has no default
        "-Wundef"                       #Warn if #if() preprocessor refers to an undefined directive
        "-Wunused"                      #Warn about unused variables/parameters
        "-Wunused-variable"             #Warn about variables that are not used
        "-Wunused-parameter"            #Warn about function parameters which are unused
        "-Wdisabled-optimization"       #Warn when optimizations are skipped (usually due to large/complex code)
        "-Wnoexcept"                    #Warn when functions should be noexcept (i.e. compiler know it doesn't throw)
        "-Woverloaded-virtual"          #Warn when a function declaration overrides a virtual method
        "-Wctor-dtor-privacy"           #Warn about inaccessible constructors/destructors
        "-Wnon-virtual-dtor"            #Warn about missing virtual destructors
        "-Wduplicated-cond"             #Warn about identical conditions in if-else chains
        "-Wduplicated-branches"         #Warn when different branches of an if-else chain are equivalent
        "-Wnull-dereference"            #Warn about null pointer dereference execution paths
        "-Wuninitialized"               #Warn about uninitialized values
        "-Winit-self"                   #Warn about self-initialization
        "-Wcatch-value=3"               #Warn when catch statements don't catch by reference
        "-Wextra-semi"                  #Warn about redundant semicolons
        "-Wimplicit-fallthrough=3"      #Warn about case fallthroughs, but allow 'fallthrough' comments to suppress warnings
        #GCC-like optional
        #"-Wsuggest-final-types"         #Suggest where 'final' would help if specified on a type methods
        #"-Wsuggest-final-methods"       #Suggest where 'final' would help if specified on methods
        #"-Wsuggest-override"            #Suggest where 'override' should be specified
        #"-Wold-style-cast"              #Warn about using c-style casts
        #"-Wconversion"                  #Warn when type conversions may change value
        #"-Wsign-conversion"             #Warn if a conversion may change the sign
        #"-Wpadded"                      #Will warn if additional padding is introduced to a struct/class. Turn on if optimizing class memory layouts
        #"-Wstrict-overflow=2"           #Warn if the compiler optimizes assuming signed overflow does not occur
        #"-Wfloat-equal"                 #Warn about using direct floating point equality
        #"-Wunsafe-loop-optimizations"   #Warn when loops can't be optimized
        #"-Wswitch-enum"                 #Warn about uncovered enumeration values in a switch (even if there is a default)
        #"-Wsign-promo"                  #Warn when overload resolution converts an unsigned type to signed when an unsigned overload exists
        #"-Wdouble-promotion"            #Warn when float is implicitly propted to double
        #"-Wuseless-cast"                #Warn about casts to the same type
        #"-Wzero-as-null-pointer-constant" #Warn about using '0' instead of nullptr
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
    "-Wno-switch-default"   #Flex/bison generate switch statements w/o default cases
    "-Wno-unused-parameter" #Flex produces functions with unused params in re-entrant mode
    "-Wno-missing-declarations" #Flex misses some declarations in re-entrant mode
    "-Wimplicit-fallthrough=0" #Bison produces some cases with explicit
    "-Wno-sign-compare" #Flex generates code which performs some signed/unsigned comparison
    "-Wno-null-dereference" #Bison produces some cases with potential null derefs
    )
foreach(flag ${FLEX_BISON_WARN_SUPPRESS_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
    if(CXX_COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(FLEX_BISON_WARN_SUPPRESS_FLAGS "${FLEX_BISON_WARN_SUPPRESS_FLAGS} ${flag}")
    endif()
endforeach()

# Suppress IPO link warnings.
# When IPO is turned on, it sometimes leads to false positives for warnings
# since it checks for warnings after some of the source files have been compiled.
# We globally suppress these warnings here. Any CMake executable which is added
# after this line will have these warnings suppressed at link time.
if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(STATUS "IPO: Suppressing known VTR warnings.")
    add_link_options(-Wno-alloc-size-larger-than  # libarchfpga allocates C-style arrays using integers.
                     -Wno-stringop-overflow       # EXTERNAL/capnproto has some string overflow warnings.
                     )
endif()

#
# Sanitizer flags
#

set(SANITIZE_FLAGS "")
if(VTR_ENABLE_SANITIZE)
    #Enable sanitizers
    # -fuse-ld=gold force the gold linker to be used (required for sanitizers, but not enabled by default on some systems)
    set(SANITIZE_FLAGS "-g -fsanitize=address -fsanitize=leak -fuse-ld=gold -static-libasan")
    message(STATUS "SANITIZE_FLAGS: ${SANITIZE_FLAGS}")
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
# Debug logging
#
set(LOGGING_FLAGS "")
if(VTR_ENABLE_DEBUG_LOGGING)
    #Enable gcov
    set(LOGGING_FLAGS "-DVTR_ENABLE_DEBUG_LOGGING")
    message(STATUS "Logging Flags: ${LOGGING_FLAGS}")
endif()

#
# Increased debugging vebosity
#
set(EXTRA_FLAGS "")
if(VTR_ENABLE_VERBOSE)
    set(EXTRA_FLAGS "${EXTRA_FLAGS} -DVTR_ENABLE_DEBUG_LOGGING")
    message(STATUS "Enabling increased debugging verbosity")
endif()

#
# Build for SPEC CPU Benchmark v8
#
set(SPEC_CPU_FLAGS "")
if(SPEC_CPU)
    # Enable SPEC CPU flag - Maximizes portability of code and minimizing the variation in how much work we do on different platforms
    set(SPEC_CPU_FLAGS "-DSPEC_CPU")
    message(STATUS "SPEC CPU FLAGS: ${SPEC_CPU_FLAGS}")
endif()

if (CMAKE_MAKE_PROGRAM EQUAL "ninja" )
    #Only for coloured output for ninja, it may be desired
    #to not force colours with other make programs (e.g. if
    #being parsed by an IDE).
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(COLORED_COMPILE "-fdiagnostics-color=always")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(COLORED_COMPILE "-fcolor-diagnostics")
    else()
        set(COLORED_COMPILE "")
    endif ()
endif()

#
# Set final flags
#
separate_arguments(
    ADDITIONAL_FLAGS UNIX_COMMAND "${SANITIZE_FLAGS} ${PROFILING_FLAGS} ${COVERAGE_FLAGS} ${LOGGING_FLAGS} ${COLORED_COMPILE} ${EXTRA_FLAGS} ${SPEC_CPU_FLAGS}"
    )
separate_arguments(
    WARN_FLAGS UNIX_COMMAND "${WARN_FLAGS}"
    )


#
# Titan Benchmarks
#
add_custom_target(get_titan_benchmarks
    COMMAND ./vtr_flow/scripts/download_titan.py --vtr_flow_dir ./vtr_flow
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Downloading (~1GB) and extracting Titan benchmarks (~10GB) into VTR source tree.")

#
# NoC MLP Benchmarks
#
add_custom_target(get_noc_mlp_benchmarks
        COMMAND ./vtr_flow/scripts/download_noc_mlp.py --vtr_flow_dir ./vtr_flow
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Downloading (~100MB) and extracting NoC MLP benchmarks (~3.2GB) into VTR source tree.")

#
# ISPD Benchmarks
#
add_custom_target(get_ispd_benchmarks
    COMMAND ./vtr_flow/scripts/download_ispd.py --vtr_flow_dir ./vtr_flow
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Downloading (~50MB) and extracting Titan benchmarks (~0.5GB) into VTR source tree.")

#
# SymbiFlow Benchmarks
#
add_custom_target(get_symbiflow_benchmarks
    COMMAND ./vtr_flow/scripts/download_symbiflow.py --vtr_flow_dir ./vtr_flow
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Downloading (~100MB) and extracting SymbiFlow architectures (~2.7GB) into VTR source tree.")

#
# Zero ASIC RR Graph Files
#
add_custom_target(get_zeroasic_rr_graphs
    COMMAND ./vtr_flow/scripts/get_zeroasic_rr_graphs.py --vtr_flow_dir ./vtr_flow
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Downloading (~2MB) and extracting Zero ASIC RR graphs (~0.1GB) into VTR source tree.")

#
# Unit Testing
#
enable_testing()

#
# Sub-projects
#

#Explicitly specify that libreadline has not been found
# This must be explicitly set for ABC to not depend on
# libreadline. Note that because the ABC CMakeLists uses MATCHES
# in its check the value must be explicitly 'FALSE' (in upper case).
#
# TODO: Ideally ABC's CMakeLists.txt should handle this itself,
#       or we should really search for readline correctly.
#       For now just disable readline.
set(READLINE_FOUND FALSE)


# set VPR_USE_EZGL in the root of VTR to decide whether to add
# subdirectory of graphics library, which prevents users
# without gtk/x11 libraries installed to build. VPR_USE_EZGL is
# being used in both the vpr CMakeLists and libs/EXTERNAL CMakeLists.
#
# check if GTK and X11 are installed and turn on/off graphics
if (VPR_USE_EZGL STREQUAL "auto")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 QUIET gtk+-3.0)
    pkg_check_modules(X11 QUIET x11)

    if(GTK3_FOUND AND X11_FOUND)
        set(VPR_USE_EZGL "on")
        message(STATUS "VPR Graphics: Enabled")
    else()
        set(VPR_USE_EZGL "off")
        message(STATUS "VPR Graphics: Disabled (required libraries missing, on debian/ubuntu try: sudo apt install libgtk-3-dev libx11-dev")
    endif()
endif()

#Add the various sub-projects
if(${WITH_ABC})
    add_subdirectory(abc)
endif()

add_subdirectory(libs) #libs/CMakeLists.txt handles adding warnings flags to non-external libraries

if(${WITH_PARMYS})
    add_subdirectory(parmys)
endif()

#Add the various tools
add_compile_options(${WARN_FLAGS}) #Add warn flags for VTR tools
add_compile_options(${ADDITIONAL_FLAGS})
link_libraries(${ADDITIONAL_FLAGS})
add_subdirectory(vpr)
if(${WITH_ABC})
    add_subdirectory(ace2)
endif()
add_subdirectory(utils)

if(${WITH_ODIN})
    add_subdirectory(odin_ii)
    # blifexplorer depends on odin
    if(${WITH_BLIFEXPLORER})
        add_subdirectory(blifexplorer)
    endif()
endif()

# handle cmake params to compile yosys-slang plugin for Yosys
if(${SLANG_SYSTEMVERILOG})
    # avoid compiling yosys-slang plugin in case the Parmys frontend is not active
    if(NOT ${WITH_PARMYS})
        message(WARNING "Parmys is not enabled, disabling Yosys-Slang as well.")
        set(SLANG_SYSTEMVERILOG OFF)
    endif()
endif()

#Add extra compilation flags to suppress warnings from some libraries/tools
# Note that target_compile_options() *appends* to the current compilation options of
# the specified target

if(${WITH_ABC})
    #Since ABC is an externally developed tool, we suppress all compiler warnings
    CHECK_CXX_COMPILER_FLAG("-w" CXX_COMPILER_SUPPORTS_-w)
    if(CXX_COMPILER_SUPPORTS_-w)
        target_compile_options(libabc PRIVATE "-w")
        target_compile_options(abc PRIVATE "-w")
    endif()

    #Some ABC headers generate warnings, treat them as system headers to suppress warnings
    get_property(ABC_INCLUDE_DIRS TARGET libabc PROPERTY INCLUDE_DIRECTORIES)
    target_include_directories(libabc SYSTEM INTERFACE ${ABC_INCLUDE_DIRS})

    #We don't control the ABC cmake file, so we need to manually unset the IPO flags
    #to avoid cmake dev warnings due to it's old cmake version policy
    set_target_properties(abc libabc libabc-pic PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

#PugiXml has some deliberate switch fallthrough cases (as indicated by comments), but they
#are tagged as warnings with g++-7 (the comments don't match g++-7's suppression regexes).
#Since we don't want to change PugiXml (it is developed externally), we relax the warning
#level so no fallthrough warnings are generated
CHECK_CXX_COMPILER_FLAG("-Wimplicit-fallthrough=0" CXX_COMPILER_SUPPORTS_-Wimplicit-fallthrough=0)
if(CXX_COMPILER_SUPPORTS_-Wimplicit-fallthrough=0)
    target_compile_options(libpugixml PRIVATE "-Wimplicit-fallthrough=0")
endif()

#
#
# Code Autoformatting
#
#
list(APPEND DIRS_TO_FORMAT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/vpr")
list(APPEND DIRS_TO_FORMAT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/libs/libarchfpga")
list(APPEND DIRS_TO_FORMAT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/libs/libvtrutil")
list(APPEND DIRS_TO_FORMAT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/libs/libpugiutil")
list(APPEND DIRS_TO_FORMAT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/libs/liblog")
list(APPEND DIRS_TO_FORMAT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/libs/librtlnumber")
list(APPEND DIRS_TO_FORMAT_CPP "${CMAKE_CURRENT_SOURCE_DIR}/odin_ii")

include(AutoClangFormat)

list(APPEND DIRS_TO_FORMAT_PY "${CMAKE_CURRENT_SOURCE_DIR}/odin_ii")
list(APPEND DIRS_TO_FORMAT_PY "${CMAKE_CURRENT_SOURCE_DIR}/ace2")
list(APPEND DIRS_TO_FORMAT_PY "${CMAKE_CURRENT_SOURCE_DIR}/dev")
list(APPEND DIRS_TO_FORMAT_PY "${CMAKE_CURRENT_SOURCE_DIR}/doc")
list(APPEND DIRS_TO_FORMAT_PY "${CMAKE_CURRENT_SOURCE_DIR}/vpr")
list(APPEND DIRS_TO_FORMAT_PY "${CMAKE_CURRENT_SOURCE_DIR}/vtr_flow")

include(AutoPyFormat)


#
# Python Environment setup
#
include(PyEnv)
