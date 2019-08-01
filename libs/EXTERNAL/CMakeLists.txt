#Manually synchronized external libraries
add_subdirectory(libpugixml)
add_subdirectory(libcatch)

#External libraries synchronized with 'git subtree'
add_subdirectory(libargparse)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
add_subdirectory(libtatum)

# Override default policy for capnproto and libezgl (CMake policy version 3.1)
# Enable new IPO variables
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

add_subdirectory(libezgl)

if(${VTR_ENABLE_CAPNPROTO})
    # Enable option overrides via variables
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    # Re-enable CXX extensions for capnproto.
    set(CMAKE_CXX_EXTENSIONS ON)

    # Disable capnproto tests
    set(BUILD_TESTING OFF)

    add_subdirectory(capnproto EXCLUDE_FROM_ALL)
endif()
