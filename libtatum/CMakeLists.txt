#
# Compiler flags come from parent
#

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

#Export library headers
target_include_directories(libtatum PUBLIC ${LIB_TATUM_INCLUDE_DIRS})
