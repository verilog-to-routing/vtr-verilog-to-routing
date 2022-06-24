#Manually synchronized external libraries
add_subdirectory(libpugixml)

#External libraries synchronized with 'git subtree'
add_subdirectory(libargparse)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
add_subdirectory(libtatum)
add_subdirectory(libcatch2)

#VPR_USE_EZGL is initialized in the root CMakeLists.
#compile libezgl only if the user asks for or has its dependencies installed.
if(VPR_USE_EZGL STREQUAL "on")
    add_subdirectory(libezgl)
endif()

# The VTR root CMakeFile initializes the ODIN_USE_YOSYS or WITH_YOSYS
# Yosys is compiled only if the user ask for it
if(${ODIN_USE_YOSYS} OR ${WITH_YOSYS})
    # handle make program with both ninja and unix style
    set(MAKE_PROGRAM "$(MAKE)")
    if(${CMAKE_GENERATOR} STREQUAL "Ninja")
        set(MAKE_PROGRAM "make")
    endif()

    add_subdirectory(libyosys)

    # adding a custom target to download (submodules), and compile Yosys plugins
    if(${YOSYS_SV_UHDM_PLUGIN})
        # keep record of the yosys DAT dir
        set(YOSYS_DATDIR ${libyosys_BINARY_DIR}/share/yosys)
        # keep record of the yosys-uhdm-plugin-integration path
        set(SURELOG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Surelog)
        set(YOSYS_F4PGA_PLUGINS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yosys-f4pga-plugins)

        # building Surelog and UHDM in a custom command to avoid any target conflict with VTR targets
        add_custom_command( OUTPUT ${libyosys_BINARY_DIR}/bin/surelog ${libyosys_BINARY_DIR}/bin/uhdm-dump ${libyosys_BINARY_DIR}/bin/uhdm-hier  DEPENDS yosys 
                            # Surelog
                            COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release
                                                     -DCMAKE_INSTALL_PREFIX=${libyosys_BINARY_DIR}
                                                     -DCMAKE_POSITION_INDEPENDENT_CODE=ON 
                                                     -S ${SURELOG_SOURCE_DIR}
                                                     -B ${SURELOG_SOURCE_DIR}/build
                                                     -Wno-deprecated
                                                     > /dev/null 2>&1
                            COMMAND ${CMAKE_COMMAND} --build ${SURELOG_SOURCE_DIR}/build
                                                     -j${CMAKE_BUILD_PARALLEL_LEVEL}
                                                     > /dev/null 2>&1
                            COMMAND ${CMAKE_COMMAND} --install ${SURELOG_SOURCE_DIR}/build
                                                     > /dev/null 2>&1
                            # UHDM
                            COMMAND ${MAKE_PROGRAM}  -C ${YOSYS_F4PGA_PLUGINS_SOURCE_DIR}
                                                     PATH=${libyosys_BINARY_DIR}/bin/:$ENV{PATH}
                                                     UHDM_INSTALL_DIR=${libyosys_BINARY_DIR}
                                                     install
                                                     -j${CMAKE_BUILD_PARALLEL_LEVEL}
                                                     > /dev/null 2>&1

                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        # create a custom target out of the plugins compilation result
        add_custom_target(yosys-plugins ALL 
                          DEPENDS 
                          ${libyosys_BINARY_DIR}/bin/surelog 
                          ${libyosys_BINARY_DIR}/bin/uhdm-dump
                          ${libyosys_BINARY_DIR}/bin/uhdm-hier)
    endif()

    # In addition to libyosys in the build folder, we copy the libyosys directory
    # into a temporary folder in the VTR root, named Yosys, to have access to Yosys 
    # (plugins) execs for using in VTR scripts (similar to VPR/vpr or ODIN_II/odin_II)
    add_custom_target(vtr-yosys ALL 
                      DEPENDS yosys
                            COMMAND ${CMAKE_COMMAND} -E 
                                                    remove_directory ${VTR_SOURCE_DIR}/Yosys/                     
                            COMMAND ${CMAKE_COMMAND} -E 
                                                    make_directory ${VTR_SOURCE_DIR}/Yosys/
                            COMMAND ${CMAKE_COMMAND} -E 
                                                    copy_directory ${CMAKE_CURRENT_BINARY_DIR}/libyosys ${VTR_SOURCE_DIR}/Yosys/                     
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    if(${YOSYS_SV_UHDM_PLUGIN})
        add_dependencies(vtr-yosys yosys-plugins)
    endif()

endif()

if(${VTR_ENABLE_CAPNPROTO})
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
    if(CXX_COMPILER_SUPPORTS_-w)
        add_compile_options("-w")
    endif()

    add_subdirectory(capnproto EXCLUDE_FROM_ALL)

    #Some capnproto kj headers (e.g. filesystem.h) generate warnings, treat them as system headers to suppress warnings
    #We suppress them here since we include the capnproto sub-tree as is and do not modify its CMakeLists.txts
    target_include_directories(kj SYSTEM INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/capnproto/c++/src>
      $<INSTALL_INTERFACE:include>
    )
endif()

# Some catch2 headers generate warnings, so treat them as system headers to suppress warnings
target_include_directories(Catch2
  SYSTEM
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libcatch2/src>
    $<INSTALL_INTERFACE:include>
)

