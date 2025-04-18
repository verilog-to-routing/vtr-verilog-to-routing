include(ExternalProject)
include(ProcessorCount)

#Manually synchronized external libraries
add_subdirectory(libpugixml)

#External libraries synchronized with 'git subtree'
add_subdirectory(libargparse)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
add_subdirectory(libtatum)
add_subdirectory(libcatch2)
#add_subdirectory(synlig)
#add_subdirectory(parmys)

#Proc numbers
ProcessorCount(PROCESSOR_COUNT)

if(PROCESSOR_COUNT EQUAL 0)
    # Fallback to 1 if the processor count cannot be determined
    set(PROCESSOR_COUNT 1)
endif()

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

    if (${SYNLIG_SYSTEMVERILOG})

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
                DEPENDS  yosys
                )

     # Synlig integration (manages Surelog and UHDM internally)                

        # Synlig integration (manages Surelog and UHDM internally)
        set(SYNLIG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/synlig)

        # Clone Synlig repository and ensure submodules are synced before building
        ExternalProject_Add(synlig
            PREFIX "synlig"

            # Clone the Synlig repository
            GIT_REPOSITORY https://github.com/chipsalliance/synlig.git
            GIT_TAG main
            GIT_PROGRESS TRUE
            GIT_SHALLOW TRUE

            # Set source and build directories
            SOURCE_DIR "${SYNLIG_SOURCE_DIR}"
            BUILD_IN_SOURCE FALSE
            INSTALL_DIR " "
            
            # Sync submodules after cloning
            
            
            UPDATE_COMMAND git submodule update --init --recursive third_party/surelog
            WORKING_DIRECTORY ${SYNLIG_SOURCE_DIR}
            
            BUILD_COMMAND ${MAKE_PROGRAM} -C ${SYNLIG_SOURCE_DIR} install DESTDIR=${CMAKE_BINARY_DIR}/bin/synlig_install -j${PROCESSOR_COUNT}
            INSTALL_COMMAND ""
            CONFIGURE_COMMAND ""
            
            # Pass necessary paths and set environment variables
            CMAKE_CACHE_ARGS
                "-DCMAKE_BUILD_TYPE:STRING=Release"
                "-DSURELOG_PATH=${CMAKE_BINARY_DIR}/surelog"
                "-DYOSYS_PATH=${CMAKE_BINARY_DIR}/yosys"
                "-DUHDM_INSTALL_DIR=${CMAKE_BINARY_DIR}"
                "-DBUILD_DIR=${CMAKE_BINARY_DIR}/synlig-build"
                "-DEXPORT_PATH=${CMAKE_BINARY_DIR}/synlig_install/usr/local/bin:$ENV{PATH}"
            
            LOG_BUILD ON
            LOG_UPDATE ON
            LOG_INSTALL ON
            LOG_CONFIGURE OFF
            LOG_OUTPUT_ON_FAILURE ON

            # Ensure dependencies like Yosys are built first
            DEPENDS yosys  # Ensure submodule sync runs before synlig build
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