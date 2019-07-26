# Support ccache for faster builds.
#
# By default ccache will be used if it is found in your path.
#
# You can use `BUILD_USING_CCACHE=off` to disable it's usage.
# You can use `BUILD_USING_CCACHE=off` to make sure ccache is used.
#
set(BUILD_USING_CCACHE "auto")
if(NOT "$ENV{BUILD_USING_CCACHE}" STREQUAL "")
    set(BUILD_USING_CCACHE $ENV{BUILD_USING_CCACHE})
endif()
find_program(CCACHE_PROGRAM ccache)
if (BUILD_USING_CCACHE STREQUAL "on")
    if(NOT CCACHE_PROGRAM)
        message(FATAL_ERROR "BUILD_USING_CCACHE set to on but ccache binary not found!")
    endif()
endif()
if (NOT BUILD_USING_CCACHE STREQUAL "off")
    if(CCACHE_PROGRAM)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
        message(STATUS "Using ccache binary found @ ${CCACHE_PROGRAM}")
    endif()
else()
    if(CCACHE_PROGRAM)
        message(STATUS "**Not** using ccache binary found @ ${CCACHE_PROGRAM} (as BUILD_USING_CCACHE is off)")
    endif()
endif()
