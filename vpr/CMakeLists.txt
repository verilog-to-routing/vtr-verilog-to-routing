cmake_minimum_required(VERSION 3.9)

project("vpr")

set(VPR_EXECUTION_ENGINE "auto" CACHE STRING "Specify the framework for (potential) parallel execution")
set_property(CACHE VPR_EXECUTION_ENGINE PROPERTY STRINGS auto serial tbb)

option(VPR_USE_SIGNAL_HANDLER "Should VPR use a signal handler to intercept signals (e.g. SIGINT)?" OFF)

set(VPR_PGO_CONFIG "none" CACHE STRING "Configure VPR Profile-Guided Optimization (PGO). prof_gen: built executable will produce profiling info, prof_use: built executable will be optimized based on generated profiling info, none: disable pgo")
set_property(CACHE VPR_PGO_CONFIG PROPERTY STRINGS prof_gen prof_use none)

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

#
# Build Configuration
#
include(CheckCXXSymbolExists)

#Collect the source files
file(GLOB_RECURSE EXEC_SOURCES src/main.cpp)
file(GLOB_RECURSE LIB_SOURCES src/*/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

if(${VTR_ENABLE_CAPNPROTO})
    add_definitions("-DVTR_ENABLE_CAPNPROTO")
endif()

#Create the library
add_library(libvpr STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES}
)

target_include_directories(libvpr PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libvpr PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependancies
target_link_libraries(libvpr
                        libvtrutil
                        libarchfpga
                        libsdcparse
                        libblifparse
                        libtatum
                        libargparse
                        libpugixml)

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

target_compile_definitions(libvpr PUBLIC ${GRAPHICS_DEFINES})

if(${VTR_ENABLE_CAPNPROTO})
    target_link_libraries(libvpr libvtrcapnproto)
endif()

add_executable(vpr ${EXEC_SOURCES})

target_link_libraries(vpr libvpr)


#Supress IPO link warnings if IPO is enabled
get_target_property(VPR_USES_IPO vpr INTERPROCEDURAL_OPTIMIZATION)
if (VPR_USES_IPO)
    set_target_properties(vpr PROPERTIES LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()



#
# VPR compiler options
#
set(VPR_COMPILE_OPTIONS_FLAGS
    #GCC-like
    "-Werror"
    )

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
    message(STATUS "VPR: building to generate profiling data for PGO")
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

if (VTR_COMPILE_OPTIONS STREQUAL "strict")
    message(STATUS "VPR: building with strict flags")
    foreach(flag ${VPR_COMPILE_OPTIONS_FLAGS})
        message(STATUS "\tAdding CXX flag: ${flag}")
        target_compile_options(libvpr PRIVATE ${flag})
        target_compile_options(vpr PRIVATE ${flag})
        target_link_libraries(vpr ${flag})
    endforeach()
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

#Supress IPO link warnings if IPO is enabled
get_target_property(TEST_VPR_USES_IPO vpr INTERPROCEDURAL_OPTIMIZATION)
if (TEST_VPR_USES_IPO)
    set_target_properties(test_vpr PROPERTIES LINK_FLAGS ${IPO_LINK_WARN_SUPRESS_FLAGS})
endif()

add_test(NAME test_vpr
    COMMAND test_vpr --use-colour=yes
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
