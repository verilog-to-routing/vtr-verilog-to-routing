cmake_minimum_required(VERSION 3.16)

project("libvtrutil")

#Version info
set(VTR_VERSION_FILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/src/vtr_version.cpp.in)
set(VTR_VERSION_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/vtr_version.cpp)

#Compiler info
set(VTR_COMPILER_INFO "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} on ${CMAKE_SYSTEM} ${CMAKE_SYSTEM_PROCESSOR}")

#Set default version numbers in case not specified
if(NOT DEFINED VTR_VERSION_MAJOR)
    set(VTR_VERSION_MAJOR 0)
endif()

if(NOT DEFINED VTR_VERSION_MINOR)
    set(VTR_VERSION_MINOR 0)
endif()

if(NOT DEFINED VTR_VERSION_PATCH)
    set(VTR_VERSION_PATCH 0)
endif()

if(NOT DEFINED VTR_VERSION_TAG_REF)
    set(VTR_VERSION_TAG_REF "*")
endif()

set(VTR_BUILD_INFO "${CMAKE_BUILD_TYPE}")
if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(VTR_BUILD_INFO "${VTR_BUILD_INFO} IPO")
endif()

if (VPR_PGO_CONFIG STREQUAL "prof_use")
    set(VTR_BUILD_INFO "${VTR_BUILD_INFO} PGO")
elseif (VPR_PGO_CONFIG STREQUAL "prof_gen")
    set(VTR_BUILD_INFO "${VTR_BUILD_INFO} PGOgen")
endif()

set(VTR_BUILD_INFO "${VTR_BUILD_INFO} VTR_ASSERT_LEVEL=${VTR_ASSERT_LEVEL}")

if (VTR_ENABLE_SANITIZE)
    set(VTR_BUILD_INFO "${VTR_BUILD_INFO} sanitizers")
endif()
if (VTR_ENABLE_PROFILING)
    set(VTR_BUILD_INFO "${VTR_BUILD_INFO} gprof")
endif()

if (VTR_ENABLE_COVERAGE)
    set(VTR_BUILD_INFO "${VTR_BUILD_INFO} gcov")
endif()

if (VTR_ENABLE_DEBUG_LOGGING)
    set(VTR_BUILD_INFO "${VTR_BUILD_INFO} debug_logging")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(GetGitRevisionDescription)

# Get the VCS revision.
git_describe_working_tree(VTR_VCS_REVISION --match=${VTR_VERSION_TAG_REF} --tags --always --long)
string(FIND ${VTR_VCS_REVISION} "NOTFOUND" GIT_DESCRIBE_VTR_REVISION_NOTFOUND)
if (NOT ${GIT_DESCRIBE_VTR_REVISION_NOTFOUND} EQUAL -1)
    # Git describe failed, usually this means we
    # aren't in a git repo -- so don't set a VCS
    # revision
    set(VTR_VCS_REVISION "unknown")
endif()

# Get the short VCS revision
git_describe_working_tree(VTR_VCS_REVISION_SHORT --match=${VTR_VERSION_TAG_REF} --tags --always --long --exclude '*')
string(FIND "${VTR_VCS_REVISION_SHORT}" "NOTFOUND" GIT_DESCRIBE_VTR_REVISION_SHORT_NOTFOUND)
if (NOT ${GIT_DESCRIBE_VTR_REVISION_SHORT_NOTFOUND} EQUAL -1)
    # Git describe failed, usually this means we
    # aren't in a git repo -- so don't set a VCS
    # revision
    set(VTR_VCS_REVISION_SHORT "unknown")
endif()

# Set the version according to semver.org
set(VTR_VERSION "${VTR_VERSION_MAJOR}.${VTR_VERSION_MINOR}.${VTR_VERSION_PATCH}")
if(VTR_VERSION_PRERELEASE)
    set(VTR_VERSION "${VTR_VERSION}-${VTR_VERSION_PRERELEASE}")
endif()
set(VTR_VERSION_SHORT ${VTR_VERSION})
if(VTR_VCS_REVISION)
    set(VTR_VERSION "${VTR_VERSION}+${VTR_VCS_REVISION_SHORT}")
endif()

# Other build meta-data
string(TIMESTAMP VTR_BUILD_TIMESTAMP)
set(VTR_BUILD_TIMESTAMP "${VTR_BUILD_TIMESTAMP}")
set(VTR_BUILD_INFO "${VTR_BUILD_INFO}")

message(STATUS "VTR Version: ${VTR_VERSION}")
message(STATUS "VTR Revision: ${VTR_VCS_REVISION}")
message(STATUS "VTR Short Revision: ${VTR_VCS_REVISION_SHORT}")

configure_file(${VTR_VERSION_FILE_IN} ${VTR_VERSION_FILE_OUT})

#
# Source files and library
#
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.hpp src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Add the version file to the sources
list(APPEND LIB_SOURCES ${VTR_VERSION_FILE_OUT})

#Create the library
add_library(libvtrutil STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libvtrutil PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libvtrutil PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Specify link-time dependencies
target_link_libraries(libvtrutil
                        liblog)

# Using filesystem library requires additional compiler/linker options for GNU implementation prior to 9.1
# and LLVM implementation prior to LLVM 9.0;
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Get the compiler version string
    execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE _gcc_output)
    string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" _gcc_version ${_gcc_output})

    if(_gcc_version VERSION_LESS "9.1")
        target_link_libraries(libvtrutil stdc++fs)
    endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Get the compiler version string
    execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE _clang_output)
    string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" _clang_version ${_clang_output})

    if(_clang_version VERSION_LESS "9.0")
        target_link_libraries(libvtrutil c++fs)
    endif()
endif()

install(TARGETS libvtrutil DESTINATION bin)
install(FILES ${LIB_HEADERS} DESTINATION include/libvtrutil)

#
# Unit Tests
#
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
add_executable(test_vtrutil ${TEST_SOURCES})
target_link_libraries(test_vtrutil 
                        libvtrutil
                        Catch2::Catch2WithMain)

add_test(NAME test_vtrutil
    COMMAND test_vtrutil
    --colour-mode ansi
    )
            
