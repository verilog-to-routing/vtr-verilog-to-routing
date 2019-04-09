project("libtatum")


#
#
#Check for parallel execution framework support
#
#
set(TBB_SUPPORTED FALSE)

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
    else()
        set(TATUM_USE_EXECUTION_ENGINE "serial")
    endif()
else()
    #The user requested a specific execution engine
    if (TATUM_EXECUTION_ENGINE STREQUAL "tbb")
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
if (TATUM_USE_EXECUTION_ENGINE STREQUAL "tbb")
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

