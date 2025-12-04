include(ExternalProject)
include(ProcessorCount)

find_package(Git REQUIRED)

#Manually synchronized external libraries
add_subdirectory(libpugixml)

#External libraries synchronized with 'git subtree'
add_subdirectory(libargparse)
add_subdirectory(libsdcparse)
add_subdirectory(libblifparse)
add_subdirectory(libtatum)
add_subdirectory(libcatch2)

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
endif ()

# The VTR root CMakeFile initializes the WITH_PARMYS
if (${WITH_PARMYS})

	cmake_minimum_required(VERSION 3.16)

	#project(yosys)

	# Create a target out of the library compilation result
	SET(YOSYS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yosys)
	SET(YOSYS_BUILD_DIR ${CMAKE_BINARY_DIR}/bin/yosys)

    add_definitions("-D_YOSYS_")
    if(${CMAKE_GENERATOR} STREQUAL "Ninja")
        set(MAKE_PROGRAM "$$MAKE") # Escaped for Ninja
    else()
        set(MAKE_PROGRAM "$(MAKE)")
    endif()

    set(CURRENT_CPPFLAGS "$(CPPFLAGS)-w")
	if(${CMAKE_GENERATOR} STREQUAL "Ninja")
	    set(CURRENT_CPPFLAGS "-w")
	endif()

	#Initialize yosys submodules
	execute_process(
	 COMMAND ${GIT_EXECUTABLE} submodule update --init
	 WORKING_DIRECTORY ${YOSYS_SRC_DIR}
	)
	execute_process(
	  COMMAND ${GIT_EXECUTABLE} submodule foreach --recursive ${GIT_EXECUTABLE}\ submodule\ update\ --init
	  WORKING_DIRECTORY ${YOSYS_SRC_DIR}
	)

	# how to build the result of the library
	add_custom_command(OUTPUT ${YOSYS_BUILD_DIR}
		COMMAND ${MAKE_PROGRAM} -C ${YOSYS_SRC_DIR}
		    ENABLE_ABC=0
		    PREFIX=${CMAKE_BINARY_DIR}
            > /dev/null 2>&1
		 COMMAND ${MAKE_PROGRAM} 
		    -C ${YOSYS_SRC_DIR}
		    install
		    ENABLE_ABC=0
		    PREFIX=${CMAKE_BINARY_DIR}
            > /dev/null 2>&1
		WORKING_DIRECTORY ${YOSYS_SRC_DIR})

	add_custom_target(yosys ALL DEPENDS ${YOSYS_BUILD_DIR})

    if (${SLANG_SYSTEMVERILOG})
        set(SLANG_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yosys-slang)
        set(SLANG_BUILD_DIR ${CMAKE_BINARY_DIR}/yosys-slang)
        #Initialize yosys-slang submodules
        execute_process(
	        COMMAND ${GIT_EXECUTABLE} submodule update --init
	        WORKING_DIRECTORY ${SLANG_SRC_DIR}
	    )
	    execute_process(
	        COMMAND ${GIT_EXECUTABLE} submodule foreach --recursive ${GIT_EXECUTABLE}\ submodule\ update\ --init
	        WORKING_DIRECTORY ${SLANG_SRC_DIR}
	    )
        set(SLANG_FE "${SLANG_SRC_DIR}/src/slang_frontend.cc")
        ExternalProject_Add(
            yosys-slang
            SOURCE_DIR ${SLANG_SRC_DIR}
            BINARY_DIR ${SLANG_BUILD_DIR}
                
            CONFIGURE_COMMAND ""
            
            # Sets location of yosys-config required by slang and suppresses warnings
            BUILD_COMMAND 
                ${CMAKE_COMMAND} -E env
                    YOSYS_PREFIX=${CMAKE_BINARY_DIR}/bin/
                ${MAKE_PROGRAM} -C ${SLANG_SRC_DIR}
                    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-w"
                install

            INSTALL_COMMAND ""

            DEPENDS yosys

            LOG_BUILD OFF
            LOG_UPDATE ON
            LOG_INSTALL ON
            LOG_CONFIGURE OFF
            LOG_OUTPUT_ON_FAILURE ON
            
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
