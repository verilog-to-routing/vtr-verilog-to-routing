cmake_minimum_required(VERSION 2.8)

project("Tatum")


message(STATUS "CMAKE_CONFIGURATION_TYPES: ${CMAKE_CONFIGURATION_TYPES}")
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

#We want assertions to stay on in release.
#Remove the DNDEBUG macro that cmake defines by default for release types.
set(NDEBUG_REGEX "[/-]D *NDEBUG")
foreach(flags_to_scrub CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO MAKE_CXX_FLAGS_MINSIZEREL)
    string(REGEX REPLACE ${NDEBUG_REGEX} "" "${flags_to_scrub}" "${${flags_to_scrub}}")
endforeach()

#Compiler flag configuration checks
include(CheckCXXCompilerFlag)

#
# We require c++11 support
#
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11_FLAG)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X_FLAG)
if(COMPILER_SUPPORTS_CXX11_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(WARNING "Could not determine C++11 support for compiler ${CMAKE_CXX_COMPILER}. C++11 support is required.")
endif()

#
# Determine if the compiler has Cilk Plus support
#  This is required for the analyzer to run in parallel mode
#  As of April 2015 the following compilers have full support
#  for Cilk Plus:
#    * GCC 5+ (Note: GCC 4.9 has only partial support, it does support the cilk_for keyword)
#    * Intel
#
CHECK_CXX_COMPILER_FLAG("-fcilkplus" COMPILER_SUPPORTS_CILK_FLAG)
CHECK_CXX_COMPILER_FLAG("-lcilkrts" COMPILER_SUPPORTS_CILK_RTS_FLAG)
if(COMPILER_SUPPORTS_CILK_FLAG AND COMPILER_SUPPORTS_CILK_RTS_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus -lcilkrts")
    #Check for cilk_for support, since some compilers like gcc 4.9
    #supports cilk but not cilk_for
    CHECK_CXX_SOURCE_COMPILES("#include <cilk/cilk.h>
                               int main(int argc, char** argv) {
                                   int cnt = 0;
                                   cilk_for(int i = 0; i < 10; i++) {
                                       cnt++;
                                   }
                               }" COMPILER_SUPPORTS_CILK_FOR)
    if(COMPILER_SUPPORTS_CILK_FOR)
        message(STATUS "Build will support parallel execution. The compiler ${CMAKE_CXX_COMPILER} has Cilk Plus support (including cilk_for).")
    else()
        #Remove the cilk flags since we aren't using it
        string(REPLACE "-fcilkplus" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        string(REPLACE "-lcilkrts" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        message(STATUS "Build will support only serial execution. The compiler ${CMAKE_CXX_COMPILER} has Cilk Plus support, but NO cilk_for keyword. ")
    endif()
else()
    message(STATUS "Build will support only serial execution. The compiler ${CMAKE_CXX_COMPILER} has no Cilk Plus support.")
endif()
#message(STATUS "CMAKE_CXX_FLAGS: " ${CMAKE_CXX_FLAGS})

#
# Debug/Opt flags
#
#Support for optimization that don't interfere with debugging?
CHECK_CXX_COMPILER_FLAG("-Og" COMPILER_SUPPORTS_-Og)
if(COMPILER_SUPPORTS_-Og)
    #Currently disable, Og still causes some variables to be optimized away
    #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
endif()

#Support for including non-inlined versions of inlined functions (useful for interactive debugging)
CHECK_CXX_COMPILER_FLAG("-fkeep-inline-functions" COMPILER_SUPPORTS_-fkeep-inline-functions)
if(COMPILER_SUPPORTS_-fkeep-inline-functions)
    #Can cause bugs with iostream_category and make_error_code in g++5
    #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fkeep-inline-functions")
    #set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fkeep-inline-functions")
endif()

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -fno-inline")

#
# Warning flags
#
set(WARN_FLAGS "") #The actual warning flags to be applied
set(WARN_FLAGS_TO_CHECK  #The flags to check if the compiler supports
    #GCC-like
    "-Wall"                     #Most warnings, typically good
    "-Wextra"                   #Extra warning, usually good
    "-pedantic"                 #Ensure ISO compliance (i.e. no non-standard extensions)
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
    "-Wno-unused-parameter"     #Don't worry about unused function parameters
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

#Apply the warning flags to all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS}")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


if(NOT TATUM_USE_SYSTEM_BOOST)
    #Unless overriden default to the local copy of boost
    set(BOOST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/lib/boost_minimal)
    set(Boost_NO_SYSTEM_PATHS ON)
endif()

add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(doc)
