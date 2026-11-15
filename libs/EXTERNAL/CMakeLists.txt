include(ExternalProject)
include(ProcessorCount)

find_package(Git REQUIRED)

# Eigen is header-only. Expose the pinned submodule without configuring its
# standalone project, which would add Eigen's own tests and install rules.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/eigen/Eigen/Core")
    message(FATAL_ERROR "Eigen submodule is missing. Run: git submodule update")
endif()
add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
target_include_directories(Eigen3::Eigen SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/eigen")

#Manually synchronized external libraries
add_subdirectory(libpugixml)

#External libraries synchronized with 'git subtree'
add_subdirectory(libargparse)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
# Build Tatum's unit tests so they run with the rest of VTR's unit tests
set(TATUM_BUILD_UNIT_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory(libtatum)
add_subdirectory(libcatch2)

# Don't build yaml-cpp command line tools since we don't use them
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
if(MSVC)
    # yaml-cpp defaults to the static MSVC runtime (/MT) for static builds; match the rest of VTR (/MD)
    set(YAML_MSVC_SHARED_RT ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(yaml-cpp)
# Treat yaml-cpp headers as system headers to suppress warnings
if(TARGET yaml-cpp)
    get_target_property(YAML_INCLUDE_DIRS yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(yaml-cpp PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${YAML_INCLUDE_DIRS}"
    )
endif()

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

    # DEF-013 fix — propagate coverage flags into the ezgl submodule so the
    # gcovr `-f libs/EXTERNAL/libezgl/src` filter picks up real instrumentation
    # data. We can't `add_compile_options(${ADDITIONAL_FLAGS})` at this
    # directory level because libezgl is a git submodule and we don't want
    # to edit its CMakeLists.txt; per-target flags applied after
    # `add_subdirectory` are scoped to the upstream target only.
    if(VTR_ENABLE_COVERAGE AND TARGET ezgl)
        target_compile_options(ezgl PRIVATE ${COVERAGE_FLAGS_LIST})
        target_link_options(ezgl PRIVATE ${COVERAGE_FLAGS_LIST})
    endif()
endif ()

# The VTR root CMakeFile sets WITH_YOSYS when parmys and/or mosaic is enabled
if (${WITH_YOSYS})

    # Yosys (>= 0.67) is a CMake project. Build it as an external project and
    # install it into the VTR build tree so the layout matches what the flow
    # scripts and the old Makefile-based build expected:
    #   bin/yosys, bin/yosys-abc, share/yosys/{include,plugins,...}
    set(YOSYS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yosys)
    set(YOSYS_BINARY_DIR ${CMAKE_BINARY_DIR}/yosys-build)
    set(YOSYS_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
    set(YOSYS_DATDIR ${YOSYS_INSTALL_PREFIX}/share/yosys)

    if(NOT EXISTS "${YOSYS_SRC_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "Yosys submodule is missing. Run: git submodule update --init --recursive")
    endif()

    # Yosys carries its own submodules (abc, slang, sv-elab, fmt, ...).
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${YOSYS_SRC_DIR}
    )

    # Since 0.67 the slang SystemVerilog frontend (read_slang) is built into
    # Yosys, so SLANG_SYSTEMVERILOG now toggles that instead of a separate plugin.
    if(${SLANG_SYSTEMVERILOG})
        set(YOSYS_WITHOUT_SLANG OFF)
    else()
        set(YOSYS_WITHOUT_SLANG ON)
    endif()

    # Yosys' bundled abc (yosys-abc) is built by default so the mosaic synthesis
    # template can run `abc` inside yosys. VTR's standalone abc binary is unaffected.
    ExternalProject_Add(yosys
        SOURCE_DIR ${YOSYS_SRC_DIR}
        BINARY_DIR ${YOSYS_BINARY_DIR}
        INSTALL_DIR ${YOSYS_INSTALL_PREFIX}
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            # Yosys is not our code; don't surface its warnings in VTR builds.
            -DCMAKE_CXX_FLAGS=-w
            -DCMAKE_C_FLAGS=-w
            -DYOSYS_USE_BUNDLED_LIBS=ON
            -DYOSYS_ENABLE_UNIT_TESTS=OFF
            -DYOSYS_WITHOUT_SLANG=${YOSYS_WITHOUT_SLANG}
        # Yosys' own build is incremental, so re-running it is cheap and keeps
        # the installed copy in sync with the submodule checkout.
        BUILD_ALWAYS ON
        USES_TERMINAL_BUILD ON
        LOG_CONFIGURE ON
        LOG_INSTALL ON
        LOG_OUTPUT_ON_FAILURE ON
    )

    # Compile/link interface for Yosys plugins (parmys, mosaic). Equivalent to
    # what `yosys-config --cxxflags --linkflags` reports: the installed headers
    # (including the generated kernel/yosys_config.h), -D_YOSYS_, and Tcl
    # headers since kernel/yosys.h includes <tcl.h> when Yosys has Tcl support.
    add_library(yosys_plugin_api INTERFACE)
    target_include_directories(yosys_plugin_api SYSTEM INTERFACE ${YOSYS_DATDIR}/include)
    target_compile_definitions(yosys_plugin_api INTERFACE _YOSYS_)

    # Plugins must compile against the same Tcl that yosys links, or Tcl API
    # calls resolve to the wrong (or no) symbol at plugin load time. Yosys finds
    # Tcl through pkg-config, so do the same here and only fall back to CMake's
    # FindTCL when pkg-config has nothing. Like yosys, let pkg-config see
    # Homebrew's keg-only packages (tcl-tk, zlib) on macOS.
    if(APPLE)
        file(GLOB homebrew_pkg_roots /opt/homebrew/opt/* /usr/local/opt/*)
        list(APPEND CMAKE_PREFIX_PATH ${homebrew_pkg_roots})
    endif()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(YOSYS_TCL QUIET tcl)
    endif()
    if(YOSYS_TCL_FOUND)
        message(STATUS "Yosys plugins: using Tcl ${YOSYS_TCL_VERSION} from pkg-config")
        target_include_directories(yosys_plugin_api SYSTEM INTERFACE ${YOSYS_TCL_INCLUDE_DIRS})
        target_compile_options(yosys_plugin_api INTERFACE ${YOSYS_TCL_CFLAGS_OTHER})
    else()
        find_package(TCL)
        if(TCL_FOUND)
            message(STATUS "Yosys plugins: using Tcl headers from ${TCL_INCLUDE_PATH}")
            target_include_directories(yosys_plugin_api SYSTEM INTERFACE ${TCL_INCLUDE_PATH})
        endif()
    endif()
    if(APPLE)
        # Plugin symbols resolve against the yosys executable at dlopen time.
        target_link_options(yosys_plugin_api INTERFACE -undefined dynamic_lookup)
    endif()
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
