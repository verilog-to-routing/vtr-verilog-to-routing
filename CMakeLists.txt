cmake_minimum_required(VERSION 2.8.12)

project("VTR")

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds not allowed. Use the Makefile wrapper (e.g. make -f Makefile), or create a new build directory and call cmake manually from there (e.g. mkdir -p build && cd build && cmake .. && make). You will need to 'rm -rf CMakeCache.txt CMakeFiles' first.")
endif()

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
# Warning flags
#
set(WARN_FLAGS "") #The actual warning flags to be applied
set(WARN_FLAGS_TO_CHECK  #The flags to check if the compiler supports
    #GCC-like
    #"-Wall"                         #Most warnings, typically good
    #"-Wextra"                       #Extra warning, usually good
    #"-Wpedantic"                    #Ensure ISO compliance (i.e. no non-standard extensions)
    #"-Wcast-qual"                   #Warn if cast removes qualifier (e.g. const char* -> char*)
    #"-Wcast-align"                  #Warn if a cast causes memory alignment changes
    #"-Wshadow"                      #Warn if local variable shadows another variable
    #"-Wformat=2"                    #Sanity checks for printf-like formatting
    #"-Wlogical-op"                  #Checks for logical op when bit-wise expected
    #"-Wmissing-declarations"        #Warn if a global function is defined with no delcaration
    #"-Wmissing-include-dirs"        #Warn if a user include directory is missing
    #"-Wredundant-decls"             #Warn if there are overlapping declarations
    #"-Wswitch-default"              #Warn if a switch has no default
    #"-Wundef"                       #Warn if #if() preprocessor refers to an undefined directive
    #"-Wunused-variable"             #Warn about variables that are not used
    #"-Wdisabled-optimization"       #Warn when optimizations are skipped (usually due to large/complex code)
    #"-Wnoexcept"                    #Warn when functions should be noexcept (i.e. compiler know it doesn't throw)
    #"-Woverloaded-virtual"          #Warn when a function declaration overrides a virtual method
    #"-Wctor-dtor-privacy"           #Warn about inaccessible constructors/destructors
    #"-Wnon-virtual-dtor"            #Warn about missing virtual destructors
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
    "-Wno-redundant-decls"  #Flex/bison generate code with redundant declarations
    "-Wno-switch-default"   #Flex/bison generate switch statments w/o default cases
    "-Wno-unused-parameter" #Flex produces functions with unused params in re-entrant mode
    "-Wno-missing-declarations" #Flex misses some declarations in re-entrant mode
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

#
# Set final flags
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS} ${SANITIZE_FLAGS} ${PROFILING_FLAGS} ${COVERAGE_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


#
# Sub-projects
#

#Add the various sub-projects
add_subdirectory(src)
add_subdirectory(libs)

#Set output locations to be in the main source tree under the relevant folder
set_target_properties(libvqm2blif vqm2blif
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
