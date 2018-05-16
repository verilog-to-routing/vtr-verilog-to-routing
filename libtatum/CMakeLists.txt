project("libtatum")


#
#
#Check for parallel execution framework support
#
#
set(CILK_PLUS_SUPPORTED FALSE)
set(TBB_SUPPORTED FALSE)

# Determine if the compiler has Cilk Plus support
#  This is required for the analyzer to run in parallel mode
#  As of April 2015 the following compilers have full support
#  for Cilk Plus:
#    * GCC 5+ (Note: GCC 4.9 has only partial support, it does support the cilk_for keyword)
#    * Intel
#
# Note that Cilk Plus is considered deprecated as of GCC 7, and Intel 2018
include(CheckCXXCompilerFlag)

set(CMAKE_REQUIRED_FLAGS "-fcilkplus -lcilkrts")
#Check for cilk_for support, since some compilers like gcc 4.9
#supports cilk but not cilk_for
CHECK_CXX_SOURCE_COMPILES("#include <cilk/cilk.h>
                           int main(int argc, char** argv) {
                               int cnt = 0;
                               cilk_for(int i = 0; i < 10; i++) {
                                   cnt++;
                               }
                           }" COMPILER_SUPPORTS_CILK_PLUS)

if (COMPILER_SUPPORTS_CILK_PLUS)
    set(CILK_PLUS_SUPPORTED TRUE)
endif()

#Check for Thread Building Blocks support
find_package(TBB)

if (TBB_FOUND)
    set(TBB_SUPPORTED TRUE)
endif()

#
#
# Determine parallel execution framework
#
#
set(TATUM_USE_EXECUTION_ENGINE "") #The actual execution engine to use (based on what is available)

if (TATUM_EXECUTION_ENGINE STREQUAL "auto")
    #Pick the best supported execution engine
    if (TBB_SUPPORTED)
        set(TATUM_USE_EXECUTION_ENGINE "tbb")
    elseif (CILK_PLUS_SUPPORTED)
        set(TATUM_USE_EXECUTION_ENGINE "cilk")
    else()
        set(TATUM_USE_EXECUTION_ENGINE "serial")
    endif()
else()
    #The user requested a specific execution engine
    if (TATUM_EXECUTION_ENGINE STREQUAL "cilk")
        if (NOT CILK_PLUS_SUPPORTED)
            message(FATAL_ERROR "Tatum: Requested execution engine '${TATUM_EXECUTION_ENGINE}' not found")
        endif()
    elseif (TATUM_EXECUTION_ENGINE STREQUAL "tbb")
        if (NOT TBB_SUPPORTED)
            message(FATAL_ERROR "Tatum: Requested execution engine '${TATUM_EXECUTION_ENGINE}' not found")
        endif()
    elseif (TATUM_EXECUTION_ENGINE STREQUAL "serial")
        #Pass
    else()
        message(FATAL_ERROR "Tatum: Unrecognized execution engine '${TATUM_EXECUTION_ENGINE}'")
    endif()
    #Set the engine to use (it must be valid or we would have errored out)
    set(TATUM_USE_EXECUTION_ENGINE "${TATUM_EXECUTION_ENGINE}")
endif()

#
#
# Build files configuration
#
#

#Source files for the library
file(GLOB_RECURSE LIB_TATUM_SOURCES *.cpp)
file(GLOB_RECURSE LIB_TATUM_HEADERS *.hpp)

#Include directories
set(LIB_TATUM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

#
#
# Define the actual build targets
#
#

#Define the library
add_library(libtatum STATIC ${LIB_TATUM_SOURCES} ${LIB_TATUM_HEADERS})
set_target_properties(libtatum PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Export library headers
target_include_directories(libtatum PUBLIC ${LIB_TATUM_INCLUDE_DIRS})

#Setup parallel execution
if (TATUM_USE_EXECUTION_ENGINE STREQUAL "cilk")
    message(STATUS "Tatum: will support parallel execution using '${TATUM_USE_EXECUTION_ENGINE}'")

    message(WARNING "Tatum: Cilk Plus support is deprecated and will be removed in the future")

    #Compile with parallel support
    # Since we the analyzer's are C++ templates, clients must compile
    # with cilk to enable parallel analysis
    target_compile_options(libtatum PUBLIC "-fcilkplus")

    target_compile_definitions(libtatum PUBLIC TATUM_USE_CILK)
    if (TBB_SUPPORTED)
        target_link_libraries(libtatum tbbmalloc_proxy) #Use the scalable memory allocator
    endif()

    #Link to the cilk run-time
    target_link_libraries(libtatum cilkrts)

elseif (TATUM_USE_EXECUTION_ENGINE STREQUAL "tbb")
    message(STATUS "Tatum: will support parallel execution using '${TATUM_USE_EXECUTION_ENGINE}'")

    target_compile_definitions(libtatum PUBLIC TATUM_USE_TBB)
    target_link_libraries(libtatum tbb)
    target_link_libraries(libtatum tbbmalloc_proxy) #Use the scalable memory allocator

elseif (TATUM_USE_EXECUTION_ENGINE STREQUAL "serial")
    #Nothing to do
    message(STATUS "Tatum: will support only serial execution")
else()
    message(FATAL_ERROR "Tatum: Unrecognized concrete execution engine '${TATUM_USE_EXECUTION_ENGINE}'")
endif()

