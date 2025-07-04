cmake_minimum_required(VERSION 3.16)

project("vpr")

set(VPR_EXECUTION_ENGINE "auto" CACHE STRING "Specify the framework for (potential) parallel execution")
set_property(CACHE VPR_EXECUTION_ENGINE PROPERTY STRINGS auto serial tbb)

option(VPR_USE_SIGNAL_HANDLER "Should VPR use a signal handler to intercept signals (e.g. SIGINT)?" OFF)

set(VPR_PGO_CONFIG "none" CACHE STRING "Configure VPR Profile-Guided Optimization (PGO). prof_gen: built executable will produce profiling info, prof_use: built executable will be optimized based on generated profiling info, none: disable pgo")
set_property(CACHE VPR_PGO_CONFIG PROPERTY STRINGS prof_gen prof_use none)

set(VPR_PGO_DATA_DIR "." CACHE PATH "Where to store and retrieve PGO data")

set(VPR_ENABLE_OPEN_MP "on" CACHE STRING "Enable OpenMP when compiling VPR")

#Handle graphics setup
set(GRAPHICS_DEFINES "")

if (VPR_USE_EZGL STREQUAL "on")
    message(STATUS "EZGL: graphics enabled")
    set(
        RESOURCE_LIST
        # Strip all the whitespace characters from ui file
        STRIPBLANKS main.ui
    )

    list(APPEND CMAKE_MODULE_PATH
         ${CMAKE_CURRENT_SOURCE_DIR}/../libs/EXTERNAL/libezgl/gcr-cmake/macros)

    include(GlibCompileResourcesSupport)
else()
    list(APPEND GRAPHICS_DEFINES "-DNO_GRAPHICS")
    message(STATUS "EZGL: graphics disabled")
endif()


#Handle server setup
set(SERVER_DEFINES "")

set(SERVER_DISABILED_REASON "")
if (VPR_USE_SERVER)
    if (VPR_USE_EZGL STREQUAL "off")
        set(SERVER_DISABILED_REASON ", due to EZGL being disabled")
        set(VPR_USE_SERVER OFF)
    endif()
endif()

if (VPR_USE_SERVER)
    message(STATUS "Server mode is enabled")
else()
    list(APPEND SERVER_DEFINES "-DNO_SERVER")
    message(STATUS "Server mode is disabled${SERVER_DISABILED_REASON}")
endif()

#
# Build Configuration
#
include(CheckCXXSymbolExists)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

if(${VPR_DEBUG_PARTITION_TREE})
    message(STATUS "VPR: Partition tree debug logs: enabled")
    add_definitions("-DVPR_DEBUG_PARTITION_TREE")
endif()

#Create the library
add_library(libvpr STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
)


target_include_directories(libvpr PUBLIC ${LIB_INCLUDE_DIRS})


# Find if Eigen is installed. Eigen is used within the Analytical Solver of the
# Analytical Placement flow. If Eigen is not installed, certain solvers cannot
# be used.
find_package(Eigen3 3.3 NO_MODULE)
if (TARGET Eigen3::Eigen)
    target_link_libraries (libvpr Eigen3::Eigen)
    target_compile_definitions(libvpr PUBLIC -DEIGEN_INSTALLED)
    message(STATUS "Eigen3: Found")
else ()
    message(STATUS "Eigen3: Not Found. Some features may be disabled.")
endif (TARGET Eigen3::Eigen)

if (${VPR_ENABLE_NOC_SAT_ROUTING})
    message(STATUS "VPR NoC SAT Routing: Requested")
    find_package(ortools CONFIG REQUIRED)
    if (TARGET ortools::ortools)
        message(STATUS "VPR NoC SAT Routing dependency (or-tools): Found")
        message(STATUS "VPR NoC SAT Routing: Enabled")
        target_link_libraries(libvpr ortools::ortools)
        target_compile_definitions(libvpr PUBLIC -DENABLE_NOC_SAT_ROUTING)
    else ()
        message(STATUS "VPR NoC SAT Routing dependency (or-tools): Not Found (You may need to set CMAKE_PREFIX_PATH in order for CMake to find your OR-Tools installation)")
        message(STATUS "VPR NoC SAT Routing: Disabled")
    endif (TARGET ortools::ortools)
endif ()

set_target_properties(libvpr PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependencies
find_package(ZLIB)
target_link_libraries(libvpr
                      libvtrutil
                      libarchfpga
                      libsdcparse
                      libblifparse
                      libtatum
                      libargparse
                      libpugixml
                      librrgraph
                      ZLIB::ZLIB
)

if(VPR_USE_SERVER)
    target_link_libraries(libvpr
                          sockpp-static
    )
endif()

#link graphics library only when graphics set to on
if (VPR_USE_EZGL STREQUAL "on")
    target_link_libraries(libvpr
			     ezgl)

    compile_gresources(
      # input: the name of our resources
      RESOURCE_FILE
      # output: the filename of the generated XML file
      XML_OUT
      # generate C code to be compiled with our program
      TYPE
        EMBED_C
      # specify the name of the C file that is generated
      TARGET
        resources.C
      # specify the resource prefix (used in the code)
      PREFIX
        /ezgl
      # input: specify the list of files to compile into resources
      RESOURCES
        ${RESOURCE_LIST}
    )


    add_custom_target(
      resource ALL
      DEPENDS
        ${RESOURCE_FILE}
    )

    #Create the executable with resources
    list(APPEND EXEC_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/resources.C)

endif()

target_compile_definitions(libvpr PUBLIC ${GRAPHICS_DEFINES} ${SERVER_DEFINES})

if(${VTR_ENABLE_CAPNPROTO})
    target_link_libraries(libvpr libvtrcapnproto)
    target_compile_definitions(libvpr PRIVATE VTR_ENABLE_CAPNPROTO)
endif()

add_executable(vpr ${EXEC_SOURCES})

target_link_libraries(vpr libvpr)


#
# Profile Guilded Optimization Configuration
#
set(PROF_GEN_FLAGS_TO_CHECK
    #GCC-like
    "-fprofile-generate=${VPR_PGO_DATA_DIR}" #Build will generate profiling information
    )
set(PROF_USE_FLAGS_TO_CHECK
    #GCC-like
    "-fprofile-use=${VPR_PGO_DATA_DIR}"     #Build will use previously generated profiling information to guide code optimization
    "-Wmissing-profile" #Warn if the used profiling information doesn't match the source code or is missing
    )

if (VPR_PGO_CONFIG STREQUAL "prof_gen")
    message(STATUS "VPR: building to generate profiling data for PGO")
    target_link_libraries(libvpr gcov)

    # This is to provide the -lgcov flag required for -fprofile-use to succeed in CHECK_CXX_COMPILER_FLAG below.
    set(CMAKE_REQUIRED_LIBRARIES gcov)

    foreach(flag ${PROF_GEN_FLAGS_TO_CHECK})
        CHECK_CXX_COMPILER_FLAG(${flag} CXX_COMPILER_SUPPORTS_${flag})
        if(CXX_COMPILER_SUPPORTS_${flag})
            target_compile_options(libvpr PUBLIC ${flag})
            target_compile_options(vpr PUBLIC ${flag})
            target_link_libraries(vpr ${flag})
        endif()
    endforeach()
elseif (VPR_PGO_CONFIG STREQUAL "prof_use")
    message(STATUS "VPR: using generated profiling data for PGO")
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
    target_link_libraries(libvpr ${TBB_tbbmalloc_proxy_LIBRARY}) #Use the scalable memory allocator
    message(STATUS "VPR: will support parallel execution using '${VPR_USE_EXECUTION_ENGINE}'")
elseif(VPR_USE_EXECUTION_ENGINE STREQUAL "serial")
    message(STATUS "VPR: will only support serial execution")
else()
    message(FATAL_ERROR "VPR: Unrecognized execution engine '${VPR_USE_EXECUTION_ENGINE}'")
endif()

#
# OpenMP configuration
#
if (VPR_ENABLE_OPEN_MP STREQUAL "on")
    find_package(OpenMP)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(libvpr OpenMP::OpenMP_CXX)
        message(STATUS "OpenMP: Enabled")
    else()
        message(STATUS "OpenMP: Disabled (requested but not found)")
    endif()
else()
    message(STATUS "OpenMP: Disabled")
endif()

#
# Signal handler configuration
#
if (VPR_USE_SIGNAL_HANDLER)
    #Check whether VPR can use sigaction to handle signals (only supported by POSIX)
    CHECK_CXX_SYMBOL_EXISTS(sigaction csignal HAVE_SIGACTION)
    if(HAVE_SIGACTION)
        target_compile_definitions(libvpr PRIVATE VPR_USE_SIGACTION)
    endif()
endif()

install(TARGETS vpr libvpr DESTINATION bin)
install(FILES ${LIB_HEADERS} DESTINATION include/libvpr)


#
# Unit Tests
#
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
add_executable(test_vpr ${TEST_SOURCES})
target_link_libraries(test_vpr
                        Catch2::Catch2WithMain
                        libvpr)

add_test(NAME test_vpr
    COMMAND test_vpr --colour-mode ansi
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
    )

