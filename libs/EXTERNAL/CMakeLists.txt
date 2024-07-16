include(ExternalProject)

#Manually synchronized external libraries
add_subdirectory(libpugixml)

#External libraries synchronized with 'git subtree'
add_subdirectory(libargparse)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
add_subdirectory(libtatum)
add_subdirectory(libcatch2)
#add_subdirectory(parmys)

#VPR_USE_SERVER is initialized in the root CMakeLists
#compile sockpp only if server mode is enabled
if (VPR_USE_SERVER)
    set(SOCKPP_BUILD_SHARED OFF CACHE BOOL "Override default value" FORCE)
    set(SOCKPP_BUILD_STATIC ON CACHE BOOL "Override default value" FORCE)
    add_subdirectory(sockpp)
endif()

#VPR_USE_EZGL is initialized in the root CMakeLists.
#compile libezgl only if the user asks for or has its dependencies installed.
if (VPR_USE_EZGL STREQUAL "on")
    add_subdirectory(libezgl)
endif ()

# The VTR root CMakeFile initializes the WITH_PARMYS
if (${WITH_PARMYS})
    # building Surelog and UHDM in a custom target to avoid any target conflict with VTR targets
    if (${YOSYS_F4PGA_PLUGINS})
        set(SURELOG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Surelog)
        set(SURELOG_BINARY_DIR ${SURELOG_SOURCE_DIR}/build)
        set(YOSYS_F4PGA_PLUGINS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yosys-f4pga-plugins)

        ExternalProject_Add(surelog
                # root directory for Surelog project
                PREFIX "surelog"

                GIT_REPOSITORY https://github.com/chipsalliance/Surelog.git
                GIT_TAG v1.71
                GIT_PROGRESS TRUE
                GIT_SHALLOW TRUE

                # setting source, build and install directories
                SOURCE_DIR "${SURELOG_SOURCE_DIR}"
                BUILD_IN_SOURCE FALSE
                INSTALL_DIR "${SURELOG_BINARY_DIR}"

                # define Surelog cache values
                CMAKE_CACHE_ARGS
                "-DCMAKE_BUILD_TYPE:STRING=Release"
                "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}"
                "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
                "-DCMAKE_WARN_DEPRECATED:BOOL=OFF"

                # redirect logs to a logfile
                LOG_BUILD ON
                LOG_UPDATE ON
                LOG_INSTALL ON
                LOG_CONFIGURE OFF
                LOG_OUTPUT_ON_FAILURE ON

                # dependency
                DEPENDS yosys
                )

        ExternalProject_Add(f4pga-plugins
                # root directory for the Yosys-F4PGA-Plugins project
                PREFIX "f4pga-plugins"

                GIT_REPOSITORY https://github.com/chipsalliance/yosys-f4pga-plugins.git
                GIT_TAG v1.20230808
                GIT_PROGRESS TRUE
                GIT_SHALLOW TRUE

                # setting source, build and install directories
                SOURCE_DIR "${YOSYS_F4PGA_PLUGINS_SOURCE_DIR}"
                BUILD_IN_SOURCE FALSE
                INSTALL_DIR ""

                INSTALL_COMMAND ""
                CONFIGURE_COMMAND ""
                BUILD_COMMAND ${MAKE_PROGRAM} -C ${YOSYS_F4PGA_PLUGINS_SOURCE_DIR}
                PATH=${CMAKE_BINARY_DIR}/bin/:$ENV{PATH}
                UHDM_INSTALL_DIR=${CMAKE_BINARY_DIR}
                BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/f4pga-plugins/$(NAME) # NAME will be resolved by yosys-f4pga-plugins Make
                YOSYS_PATH=${CMAKE_BINARY_DIR}
                install -j${CUSTOM_BUILD_PARALLEL_LEVEL}

                # redirect logs to a logfile
                LOG_BUILD ON
                LOG_UPDATE ON
                LOG_INSTALL ON
                LOG_CONFIGURE OFF
                LOG_OUTPUT_ON_FAILURE ON

                # dependency
                DEPENDS surelog yosys
                )
    endif ()

endif ()

if (${VTR_ENABLE_CAPNPROTO})
    # Override default policy for capnproto (CMake policy version 3.1)
    # Enable new IPO variables
    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

    # Enable option overrides via variables
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

    # Re-enable CXX extensions for capnproto.
    set(CMAKE_CXX_EXTENSIONS ON)

    # Disable capnproto tests
    set(BUILD_TESTING OFF)

    #Since capnproto is an externally developed library/tool, we suppress all compiler warnings
    CHECK_CXX_COMPILER_FLAG("-w" CXX_COMPILER_SUPPORTS_-w)
    if (CXX_COMPILER_SUPPORTS_-w)
        add_compile_options("-w")
    endif ()

    add_subdirectory(capnproto EXCLUDE_FROM_ALL)

    #Some capnproto kj headers (e.g. filesystem.h) generate warnings, treat them as system headers to suppress warnings
    #We suppress them here since we include the capnproto sub-tree as is and do not modify its CMakeLists.txts
    target_include_directories(kj SYSTEM INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/capnproto/c++/src>
            $<INSTALL_INTERFACE:include>
            )
endif ()

# Some catch2 headers generate warnings, so treat them as system headers to suppress warnings
target_include_directories(Catch2
        SYSTEM
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libcatch2/src>
        $<INSTALL_INTERFACE:include>
        )

# Some sockpp headers generate warnings, so treat them as system headers to suppress warnings
if (VPR_USE_SERVER)
    target_include_directories(sockpp-static
            SYSTEM
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sockpp/include>
            $<INSTALL_INTERFACE:include>
            )
endif()