# CMakeLists.txt
#
# Top-level CMake build file for the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2017-2023 Frank Pagliughi
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ---------------------------------------------------------------------------

# --- CMake required version ---

cmake_minimum_required(VERSION 3.12)

# --- Project setup ---

project(sockpp VERSION "1.0.0")

# --- Build Options ---

option(SOCKPP_BUILD_SHARED "Build shared library" ON)
option(SOCKPP_BUILD_STATIC "Build static library" OFF)
option(SOCKPP_BUILD_EXAMPLES "Build example applications" OFF)
option(SOCKPP_BUILD_TESTS "Build unit tests" OFF)
option(SOCKPP_BUILD_DOCUMENTATION "Create Doxygen reference documentation" OFF)
option(SOCKPP_BUILD_CAN "Build the Linux SocketCAN components" OFF)

# --- Setting naming variables ---

set(SOCKPP_SHARED_LIBRARY sockpp)
set(SOCKPP_STATIC_LIBRARY sockpp-static)
set(SOCKPP_OBJECT_LIBRARY sockpp-objs)

set(SOCKPP_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SOCKPP_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# --- Generate a version header ---

configure_file(
	${PROJECT_SOURCE_DIR}/cmake/version.h.in  
	${SOCKPP_GENERATED_DIR}/include/sockpp/version.h
	@ONLY
)

# --- Common library sources, etc ---

add_subdirectory(src)

# --- System libraries ---

if(WIN32)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	set(LIBS_SYSTEM ws2_32)
endif()

# --- Collect the targets names ---

if(${SOCKPP_BUILD_SHARED})
	list(APPEND SOCKPP_TARGETS ${SOCKPP_SHARED_LIBRARY})
endif()

if(${SOCKPP_BUILD_STATIC})
	list(APPEND SOCKPP_TARGETS ${SOCKPP_STATIC_LIBRARY})
endif()

# --- Create the libraries and export them ---

if(NOT SOCKPP_TARGETS)
	message(FATAL_ERROR "No targets are specified")
endif()

if(${SOCKPP_BUILD_SHARED})
	message(STATUS "Creating shared library: ${SOCKPP_SHARED_LIBRARY}")
	add_library(${SOCKPP_SHARED_LIBRARY} SHARED $<TARGET_OBJECTS:${SOCKPP_OBJECT_LIBRARY}>)

	target_compile_features(${SOCKPP_SHARED_LIBRARY} PUBLIC cxx_std_14)

	target_include_directories(${SOCKPP_SHARED_LIBRARY}
		PUBLIC
			$<BUILD_INTERFACE:${SOCKPP_INCLUDE_DIR}>
			$<INSTALL_INTERFACE:include>
		PRIVATE
			${SOCKPP_GENERATED_DIR}/include
	)

	target_link_libraries(${SOCKPP_SHARED_LIBRARY} PUBLIC ${LIBS_SYSTEM})

	set_target_properties(${SOCKPP_SHARED_LIBRARY} PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}
		CXX_EXTENSIONS OFF
	)

	list(APPEND TARGET_FILES ${SOCKPP_SHARED_LIBRARY})
endif()

if(${SOCKPP_BUILD_STATIC})
	message(STATUS "Creating static library: ${SOCKPP_STATIC_LIBRARY}")
	add_library(${SOCKPP_STATIC_LIBRARY} STATIC $<TARGET_OBJECTS:${SOCKPP_OBJECT_LIBRARY}>)

	target_compile_features(${SOCKPP_STATIC_LIBRARY} PUBLIC cxx_std_14)

	target_include_directories(${SOCKPP_STATIC_LIBRARY}
		PUBLIC
			$<BUILD_INTERFACE:${SOCKPP_INCLUDE_DIR}>
			$<INSTALL_INTERFACE:include>
		PRIVATE
			${SOCKPP_GENERATED_DIR}/include
	)

	target_link_libraries(${SOCKPP_STATIC_LIBRARY} PUBLIC ${LIBS_SYSTEM})

	set_target_properties(${SOCKPP_STATIC_LIBRARY} PROPERTIES
		CXX_EXTENSIONS OFF
	)

  # On *nix systems, the static library can have the same base filename
  # as the shared library, thus 'libsockpp.a' for the static lib.
  # On Windows they need different names to tell the static lib from the
  # DLL import library.
	if(UNIX)
		set_target_properties(${SOCKPP_STATIC_LIBRARY} PROPERTIES 
			OUTPUT_NAME ${SOCKPP_SHARED_LIBRARY}
		)
	endif()

	list(APPEND TARGET_FILES ${SOCKPP_STATIC_LIBRARY})
endif()

# --- Install Targets ---

include(GNUInstallDirs)

install(TARGETS ${TARGET_FILES}
	EXPORT sockpp-targets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT sockpp-targets
	FILE
		sockppTargets.cmake
	NAMESPACE
		Sockpp::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${SOCKPP_GENERATED_DIR}/cmake/sockppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(DIRECTORY include/ ${SOCKPP_GENERATED_DIR}/include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	FILES
		${PROJECT_SOURCE_DIR}/cmake/sockppConfig.cmake
		${SOCKPP_GENERATED_DIR}/cmake/sockppConfigVersion.cmake
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/sockpp
)

# --- Documentation ---

if(SOCKPP_BUILD_DOCUMENTATION)
	add_subdirectory(doc)
endif()

# --- Default library for examples and unit tests ---

if(SOCKPP_BUILD_SHARED)
	set(SOCKPP_LIB ${SOCKPP_SHARED_LIBRARY})
else()
	set(SOCKPP_LIB ${SOCKPP_STATIC_LIBRARY})
endif()

# --- Example applications ---

if(SOCKPP_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# --- Unit Tests ---

if(SOCKPP_BUILD_TESTS)
	add_subdirectory(tests/unit)
endif()

