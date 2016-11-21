if(TATUM_ENABLE_PARALLEL_ANALYSIS)
    #
    # Determine if the compiler has Cilk Plus support
    #  This is required for the analyzer to run in parallel mode
    #  As of April 2015 the following compilers have full support
    #  for Cilk Plus:
    #    * GCC 5+ (Note: GCC 4.9 has only partial support, it does support the cilk_for keyword)
    #    * Intel
    #
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

    if(COMPILER_SUPPORTS_CILK_PLUS)
        message(STATUS "Tatum will support parallel analysis")
    else()
        message(STATUS "Tatum will NOT support parallel analysis (compiler does not support Cilk Plus)")
    endif()
endif()

#
#
# Build files configuration
#
#

#Source files for the library
file(GLOB_RECURSE LIB_TATUM_SOURCES */*.cpp)
file(GLOB_RECURSE LIB_TATUM_HEADERS */*.hpp)

#Include directories
foreach(header ${LIB_TATUM_HEADERS})
    get_filename_component(incl_dir ${header} DIRECTORY)
    list(APPEND LIB_TATUM_INCLUDE_DIRS ${incl_dir})
endforeach()
#Remove duplicate include directories
list(REMOVE_DUPLICATES LIB_TATUM_INCLUDE_DIRS)

#
#
# Define the actual build targets
#
#

#Define the library
add_library(libtatum STATIC ${LIB_TATUM_SOURCES} ${LIB_TATUM_HEADERS})
set_target_properties(libtatum PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

if(TATUM_ENABLE_PARALLEL_ANALYSIS AND COMPILER_SUPPORTS_CILK_PLUS)
    #Compile with parallel support
    # Since we the analyzer's are C++ templates, clients must compile
    # with cilk to enable parallel analysis
    target_compile_options(libtatum PUBLIC "-fcilkplus")

    #Link to the cilk run-time
    target_link_libraries(libtatum cilkrts)
endif()

#Export library headers
target_include_directories(libtatum PUBLIC ${LIB_TATUM_INCLUDE_DIRS})
