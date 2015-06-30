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


#set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/base/TimingTags.cpp
                            #PROPERTIES COMPILE_FLAGS "-fno-inline")

#
# External library dependancies
#
find_package(Boost 1.54 REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS}) #Treat boost as a system header to suppress warnings
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

#
#
# Define the actual build targets
#
#

#Define the library
add_library(tatum STATIC ${LIB_TATUM_SOURCES} ${LIB_TATUM_HEADERS})

#Export library headers
target_include_directories(tatum PUBLIC ${LIB_TATUM_INCLUDE_DIRS})

