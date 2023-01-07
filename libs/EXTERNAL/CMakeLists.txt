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

    ExternalProject_Add(libyosys
    # root directory for the Yosys-F4PGA-Plugins project 
    PREFIX "libyosys" 

    GIT_REPOSITORY    https://github.com/YosysHQ/yosys.git
    GIT_TAG           yosys-0.25 # yosys-systemverilog release 3c3aaf2-2022-12-30
    GIT_PROGRESS      TRUE 
    
    # setting source, build and install directories 
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libyosys" 
    BUILD_IN_SOURCE FALSE 
    INSTALL_DIR "" 
    # BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/libyosys"
 
    CONFIGURE_COMMAND "" 
    BUILD_COMMAND ${MAKE_PROGRAM} -C ${CMAKE_CURRENT_BINARY_DIR}/libyosys
        -f ${CMAKE_CURRENT_SOURCE_DIR}/libyosys/Makefile #(out-of-tree) build directory
        -k "ENABLE_LIBYOSYS=1" "ENABLE_ABC=0"
        PREFIX=${VTR_SOURCE_DIR}/Yosys
        -j${CMAKE_BUILD_PARALLEL_LEVEL}

    INSTALL_COMMAND ${MAKE_PROGRAM} -C ${CMAKE_CURRENT_BINARY_DIR}/libyosys
        -f ${CMAKE_CURRENT_SOURCE_DIR}/libyosys/Makefile #(out-of-tree) build directory
        install
        PREFIX=${VTR_SOURCE_DIR}/Yosys
        -j${CMAKE_BUILD_PARALLEL_LEVEL}
    
    # redirect logs to a logfile
    LOG_BUILD ON
    LOG_UPDATE ON
    LOG_INSTALL ON
    LOG_CONFIGURE ON
    LOG_OUTPUT_ON_FAILURE ON
    
) 

    # add_subdirectory(libyosys)

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
            PREFIX "surelog" 

            GIT_REPOSITORY    https://github.com/chipsalliance/Surelog.git 
            GIT_TAG           900fb2499c7d3f793409999eabee63db513814b4 # yosys-systemverilog release 3c3aaf2-2022-12-30
            GIT_PROGRESS      TRUE 
            
            # setting source, build and install directories 
            SOURCE_DIR "${SURELOG_SOURCE_DIR}" 
            BUILD_IN_SOURCE FALSE 
            INSTALL_DIR "${SURELOG_BINARY_DIR}" 

            # define Surelog cache values 
            CMAKE_CACHE_ARGS 
                "-DCMAKE_BUILD_TYPE:STRING=Release" 
                "-DCMAKE_INSTALL_PREFIX:PATH=${VTR_SOURCE_DIR}/Yosys" 
                "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON" 
                "-DCMAKE_WARN_DEPRECATED:BOOL=OFF" 

            # redirect logs to a logfile
            LOG_BUILD ON
            LOG_UPDATE ON
            LOG_INSTALL ON
            LOG_CONFIGURE ON
            LOG_OUTPUT_ON_FAILURE ON
            
            # dependency
            DEPENDS libyosys 
        ) 

        ExternalProject_Add(f4pga-plugins 
            # root directory for the Yosys-F4PGA-Plugins project 
            PREFIX "f4pga-plugins" 

            GIT_REPOSITORY    https://github.com/chipsalliance/yosys-f4pga-plugins.git 
            GIT_TAG           ef5f5a2efe77e9bd1d72dbaa28ede168cf1a7465 # yosys-systemverilog release 3c3aaf2-2022-12-30
            GIT_PROGRESS      TRUE 
            
            # setting source, build and install directories 
            SOURCE_DIR "${YOSYS_F4PGA_PLUGINS_SOURCE_DIR}" 
            BUILD_IN_SOURCE FALSE 
            INSTALL_DIR "" 

            INSTALL_COMMAND "" 
            CONFIGURE_COMMAND "" 
            BUILD_COMMAND ${MAKE_PROGRAM} -C ${YOSYS_F4PGA_PLUGINS_SOURCE_DIR}
                                        PATH=${libyosys_BINARY_DIR}/bin/:$ENV{PATH}
                                        UHDM_INSTALL_DIR=${libyosys_BINARY_DIR}
                                        BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/f4pga-plugins/$(NAME) # NAME will be resolved by yosys-f4pga-plugins Make
                                        YOSYS_PATH=${VTR_SOURCE_DIR}/Yosys
                                        PLUGIN_LIST=systemverilog #fasm xdc params sdc ql-iob design_introspection integrateinv ql-qlf systemverilog uhdm dsp-ff
                                        install -j${CMAKE_BUILD_PARALLEL_LEVEL}
            
            # redirect logs to a logfile
            LOG_BUILD ON
            LOG_UPDATE ON
            LOG_INSTALL ON
            LOG_CONFIGURE ON
            LOG_OUTPUT_ON_FAILURE ON
            
            # dependency
            DEPENDS surelog libyosys 
        ) 
    endif() 

    if(${YOSYS_PARMYS_PLUGIN}) 
        set(YOSYS_DATDIR ${libyosys_BINARY_DIR}/share/yosys) 
        set(PARMYS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/parmys-plugin) 
        ExternalProject_Add(parmys-plugin 
            PREFIX "parmys-plugin" 

            GIT_REPOSITORY    https://github.com/CAS-Atlantic/parmys-plugin.git
            GIT_TAG           v2.0
            GIT_PROGRESS      TRUE 
            
            # setting source, build and install directories 
            SOURCE_DIR "${PARMYS_SOURCE_DIR}" 
            BUILD_IN_SOURCE FALSE 
            INSTALL_DIR "" 

            INSTALL_COMMAND "" 
            CONFIGURE_COMMAND "" 
            BUILD_COMMAND ${MAKE_PROGRAM} -C ${PARMYS_SOURCE_DIR} 
                                        PATH=${libyosys_BINARY_DIR}/bin/:$ENV{PATH} 
                                        VTR_INSTALL_DIR=${CMAKE_BINARY_DIR} 
                                        BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/parmys-plugin
                                        YOSYS_PATH=${VTR_SOURCE_DIR}/Yosys
                                        install -j${CMAKE_BUILD_PARALLEL_LEVEL} 
            
            # redirect logs to a logfile
            LOG_BUILD ON
            LOG_UPDATE ON
            LOG_INSTALL ON
            LOG_CONFIGURE ON
            LOG_OUTPUT_ON_FAILURE ON
            
            # dependency
            DEPENDS libyosys
        ) 
    endif()

    # In addition to libyosys in the build folder, we copy the libyosys directory
    # into a temporary folder in the VTR root, named Yosys, to have access to Yosys 
    # (plugins) execs for using in VTR scripts (similar to VPR/vpr or ODIN_II/odin_II)
    # add_custom_target(vtr-yosys ALL 
    #                   DEPENDS libyosys
    #                         COMMAND ${CMAKE_COMMAND} -E 
    #                                                 remove_directory ${VTR_SOURCE_DIR}/Yosys/                     
    #                         COMMAND ${CMAKE_COMMAND} -E 
    #                                                 make_directory ${VTR_SOURCE_DIR}/Yosys/
    #                         COMMAND ${CMAKE_COMMAND} -E 
    #                                                 copy_directory ${CMAKE_CURRENT_BINARY_DIR}/libyosys ${VTR_SOURCE_DIR}/Yosys/                     
    #                         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    if(${YOSYS_SV_UHDM_PLUGIN})
        # add_dependencies(vtr-yosys f4pga-plugins)
    endif()
    if(${YOSYS_PARMYS_PLUGIN})
        # parmys-plugin requires the binaries and headers of the vtr libraries to be installed
        add_custom_target(vtr-libs ALL 
                        DEPENDS libarchfpga libvtrutil liblog libpugixml libpugiutil librtlnumber libyosys
                            COMMAND ${CMAKE_COMMAND}
                                                    --install ${CMAKE_BINARY_DIR}/libs                    
                            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        add_dependencies(parmys-plugin vtr-libs)
        # add_dependencies(vtr-yosys parmys-plugin)
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

