cmake_minimum_required(VERSION 2.8.12)

project("VTR")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds not allowed. Use the Makefile-cmake wrapper (e.g. make -f Makefile-cmake), or create a new build directory and call cmake manually from there (e.g. mkdir -p build && cd build && cmake .. && make). You will need to 'rm -rf CMakeCache.txt CMakeFiles' first.")
endif()

#We install to the source directory by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "default install path" FORCE)
endif()

#Allow the user to configure how much assertion checking should occure
set(VTR_ASSERT_LEVEL "2" CACHE STRING "VTR assertion checking level. 0: no assertions, 1, 2, 3: all assertions (including those with significant run-time cost)")
set_property(CACHE VTR_ASSERT_LEVEL PROPERTY STRINGS 0 1 2 3)
add_definitions("-DVTR_ASSERT_LEVEL=${VTR_ASSERT_LEVEL}") #Set assertion level

set(VTR_VERSION_MAJOR 8)
set(VTR_VERSION_MINOR 0)
set(VTR_VERSION_PATCH 0)
set(VTR_VERSION_PRERELEASE "dev")

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

#Compiler flag configuration checks
include(CheckCXXCompilerFlag)

#
# We require c++11 support
#
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14_FLAG)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11_FLAG)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X_FLAG)
if(COMPILER_SUPPORTS_CXX14_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif(COMPILER_SUPPORTS_CXX11_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(WARNING "Could not determine C++11 support for compiler ${CMAKE_CXX_COMPILER}. C++11 support is required.")
endif()

#
# Debug/Opt flags
#
#Support for optimization that don't interfere with debugging?
#CHECK_CXX_COMPILER_FLAG("-Og" COMPILER_SUPPORTS_-Og)
#if(COMPILER_SUPPORTS_-Og)
    ##Currently disable, Og still causes some variables to be optimized away
    ##set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
#endif()

#Support for including non-inlined versions of inlined functions (useful for interactive debugging)
#CHECK_CXX_COMPILER_FLAG("-fkeep-inline-functions" COMPILER_SUPPORTS_-fkeep-inline-functions)
#if(COMPILER_SUPPORTS_-fkeep-inline-functions)
    ##set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fkeep-inline-functions")
    ##set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fkeep-inline-functions")
#endif()

#Support for gprof
#CHECK_CXX_COMPILER_FLAG("-pg" COMPILER_SUPPORTS_-pg)
#if(COMPILER_SUPPORTS_-fkeep-inline-functions)
    ##set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fkeep-inline-functions")
    #set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -pg")
#endif()

#
# Warning flags
#
set(WARN_FLAGS "") #The actual warning flags to be applied
set(WARN_FLAGS_TO_CHECK  #The flags to check if the compiler supports
    #GCC-like
    "-Wall"                     #Most warnings, typically good
    "-Wextra"                   #Extra warning, usually good
    "-Wpedantic"                 #Ensure ISO compliance (i.e. no non-standard extensions)
    "-Wcast-qual"               #Warn if cast removes qualifier (e.g. const char* -> char*)
    "-Wcast-align"              #Warn if a cast causes memory alignment changes
    "-Wshadow"                  #Warn if local variable shadows another variable
    "-Wformat=2"                #Sanity checks for printf-like formatting
    "-Wlogical-op"              #Checks for logical op when bit-wise expected
    "-Wmissing-declarations"    #Warn if a global function is defined with no delcaration in a header file
    "-Wmissing-include-dirs"    #Warn if a user include directory is missing
    "-Wredundant-decls"         #Warn if there are overlapping declarations
    "-Wswitch-default"          #Warn if a switch has no default
    "-Wundef"                   #Warn if #if() preprocessor refers to an undefined directive
    "-Wunused-variable"         #Warn about variables that are not used
    #GCC-like optional
    #"-Wsign-conversion"         #Warn if a conversion may change the sign
    #"-Wpadded"                  #Will warn if additional padding is introduced to a struct/class. Turn on if optimizing class memory layouts
    )

#Check and see if the compiler supports the various warning flags,
#and add valid flags
foreach(flag ${WARN_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${flag})
    if(COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(WARN_FLAGS "${WARN_FLAGS} ${flag}")
    endif()
endforeach()

#The flex/bison code is not warning clean so we need to suppress some warnings
set(FLEX_BISON_WARN_SUPPRESS_FLAGS "")
set(FLEX_BISON_WARN_SUPPRESS_FLAGS_TO_CHECK
    #"-Wno-redundant-decls"  #Flex/bison generate code with redundant declarations
    "-Wno-switch-default"   #Flex/bison generate switch statments w/o default cases
    )
foreach(flag ${FLEX_BISON_WARN_SUPPRESS_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${flag})
    if(COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(FLEX_BISON_WARN_SUPPRESS_FLAGS "${FLEX_BISON_WARN_SUPPRESS_FLAGS} ${flag}")
    endif()
endforeach()

#We sometimes want to suppress format-nonliteral checks
set(WARN_NON_LITERAL_SUPPRESS_FLAGS "")
set(WARN_NON_LITERAL_SUPPRESS_FLAGS_TO_CHECK
    "-Wno-format-nonliteral" #Disable format check warnings on non-literal strings
    )
foreach(flag ${WARN_NON_LITERAL_SUPPRESS_FLAGS_TO_CHECK})
    CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${flag})
    if(COMPILER_SUPPORTS_${flag})
        #Flag supported, so enable it
        set(WARN_NON_LITERAL_SUPPRESS_FLAGS "${WARN_NON_LITERAL_SUPPRESS_FLAGS} ${flag}")
    endif()
endforeach()


#Apply the warning flags to all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS}")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

function(get_include_directories header_file_list include_directory_list)
    #Extract the directories for each header file
    foreach(header ${${header_file_list}})
        get_filename_component(incl_dir ${header} DIRECTORY)
        list(APPEND dir_list ${incl_dir})
    endforeach()

    #Remove any duplicates
    list(LENGTH "${dir_list}" length)
    if(${length} GREATER 1) #Avoid error with zero-length lists
        list(REMOVE_DUPLICATES ${dir_list})
    endif()

    #Set the second argument in the caller's scope
    set(${include_directory_list} ${dir_list} PARENT_SCOPE)
endfunction(get_include_directories)

#ABC as external
set(ABC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/abc_with_bb_support)
include(ExternalProject)
ExternalProject_Add(abc_external
    SOURCE_DIR ${ABC_SOURCE_DIR}
    CONFIGURE_COMMAND ""
    #Use $(MAKE) to enable parallel build
    BUILD_COMMAND $(MAKE) abc libabc.a
    BUILD_IN_SOURCE 1
    LOG_BUILD 0
    INSTALL_COMMAND ""
    TEST_COMMAND ""
)

add_executable(abc IMPORTED GLOBAL)
add_library(libabc STATIC IMPORTED GLOBAL)
set_property(TARGET libabc PROPERTY IMPORTED_LOCATION ${ABC_SOURCE_DIR}/libabc.a)

include_directories(
                    #For ODIN
                    ${ABC_SOURCE_DIR}/src/bdd/cudd 
                    ${ABC_SOURCE_DIR}/src/bdd/epd 
                    ${ABC_SOURCE_DIR}/src/bdd/mtr 
                    ${ABC_SOURCE_DIR}/src/misc/st
                    #For ACE
                    ${ABC_SOURCE_DIR}/src/base/io 
                    ${ABC_SOURCE_DIR}/src/base/abc 
                    ${ABC_SOURCE_DIR}/src/base/main 
                    ${ABC_SOURCE_DIR}/src/base/cmd 
                    ${ABC_SOURCE_DIR}/src/misc/vec 
                    ${ABC_SOURCE_DIR}/src/misc/util 
                    ${ABC_SOURCE_DIR}/src/misc/st 
                    ${ABC_SOURCE_DIR}/src/misc/extra 
                    ${ABC_SOURCE_DIR}/src/misc/espresso 
                    ${ABC_SOURCE_DIR}/src/misc/nm 
                    ${ABC_SOURCE_DIR}/src/aig/aig 
                    ${ABC_SOURCE_DIR}/src/aig/hop 
                    ${ABC_SOURCE_DIR}/src/aig/mem 
                    ${ABC_SOURCE_DIR}/src/aig/fra 
                    ${ABC_SOURCE_DIR}/src/aig/dar 
                    ${ABC_SOURCE_DIR}/src/bdd/cudd 
                    ${ABC_SOURCE_DIR}/src/bdd/mtr 
                    ${ABC_SOURCE_DIR}/src/bdd/epd 
                    ${ABC_SOURCE_DIR}/src/map/if 
                    ${ABC_SOURCE_DIR}/src/sat/bsat)

set_property(TARGET abc PROPERTY IMPORTED_LOCATION ${ABC_SOURCE_DIR}/abc)

install(FILES ${ABC_SOURCE_DIR}/libabc.a DESTINATION bin)
install(PROGRAMS ${ABC_SOURCE_DIR}/abc DESTINATION bin)

add_subdirectory(liblog)
add_subdirectory(libpugixml)
add_subdirectory(libvtrutil)
add_subdirectory(libarchfpga)
add_subdirectory(libsdcparse)
add_subdirectory(libeasygl)
add_subdirectory(vpr)
add_subdirectory(ODIN_II)
add_subdirectory(ace2)

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
set_target_properties(libeasygl
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libeasygl"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libeasygl"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libeasygl")
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
set_target_properties(odin_II.exe
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ODIN_II"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ODIN_II"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ODIN_II")
