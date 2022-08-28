include(ExternalProject)

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

    # building Surelog and UHDM in a custom target to avoid any target conflict with VTR targets 
    if(${YOSYS_SV_UHDM_PLUGIN}) 
        # keep record of the yosys DAT dir 
        set(YOSYS_DATDIR ${libyosys_BINARY_DIR}/share/yosys) 
        # keep record of the yosys-uhdm-plugin-integration path 
        set(SURELOG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Surelog) 
        set(SURELOG_BINARY_DIR ${SURELOG_SOURCE_DIR}/build) 
        set(YOSYS_F4PGA_PLUGINS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yosys-f4pga-plugins) 

        # Surelog 
        ExternalProject_Add(surelog 
            # root directory for Surelog project 
            PREFIX "" 

            GIT_REPOSITORY    https://github.com/chipsalliance/Surelog.git 
            GIT_TAG           8da6fcba650826219ead335a3382c52f1c1d5690 # yosys-systemverilog release c7f1ded-2022-08-10 
            GIT_PROGRESS      TRUE 
            
            # setting source, build and install directories 
            SOURCE_DIR "${SURELOG_SOURCE_DIR}" 
            BUILD_IN_SOURCE TRUE 
            INSTALL_DIR "${SURELOG_BINARY_DIR}" 

            # define Surelog cache values 
            CMAKE_CACHE_ARGS 
                "-DCMAKE_BUILD_TYPE:STRING=Release" 
                "-DCMAKE_INSTALL_PREFIX:PATH=${libyosys_BINARY_DIR}" 
                "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON" 
                "-DCMAKE_WARN_DEPRECATED:BOOL=OFF" 

            # redirect logs to a logfile
            LOG_BUILD ON
            LOG_UPDATE ON
            LOG_INSTALL ON
            LOG_CONFIGURE ON
            LOG_OUTPUT_ON_FAILURE ON
            
            # dependency
            DEPENDS yosys 
        ) 

        ExternalProject_Add(yosys-plugins 
            # root directory for the Yosys-F4PGA-Plugins project 
            PREFIX "" 

            GIT_REPOSITORY    https://github.com/chipsalliance/yosys-f4pga-plugins.git 
            GIT_TAG           3b3dcc5cc216f7a39d26a6d894c53b8aa3e10d71 # yosys-systemverilog release c7f1ded-2022-08-10 
            GIT_PROGRESS      TRUE 
            
            # setting source, build and install directories 
            SOURCE_DIR "${YOSYS_F4PGA_PLUGINS_SOURCE_DIR}" 
            BUILD_IN_SOURCE TRUE 
            INSTALL_DIR "" 

            INSTALL_COMMAND "" 
            CONFIGURE_COMMAND "" 
            BUILD_COMMAND ${MAKE_PROGRAM} -C ${YOSYS_F4PGA_PLUGINS_SOURCE_DIR} 
                                        PATH=${libyosys_BINARY_DIR}/bin/:$ENV{PATH} 
                                        UHDM_INSTALL_DIR=${libyosys_BINARY_DIR} 
                                        install -j${CMAKE_BUILD_PARALLEL_LEVEL} 
            
            # redirect logs to a logfile
            LOG_BUILD ON
            LOG_UPDATE ON
            LOG_INSTALL ON
            LOG_CONFIGURE ON
            LOG_OUTPUT_ON_FAILURE ON
            
            # dependency
            DEPENDS surelog yosys 
        ) 
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

