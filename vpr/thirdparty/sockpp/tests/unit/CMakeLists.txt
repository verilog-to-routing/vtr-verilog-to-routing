# CMakeLists.txt
#
# CMake file for the unit tests in the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2018 Frank Pagliughi
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
# --------------------------------------------------------------------------

set(UNIT_TEST_DIR ${PROJECT_SOURCE_DIR})
find_package(Catch2 REQUIRED)

message(STATUS "Catch2 v${Catch2_VERSION}")
if (Catch2_VERSION VERSION_LESS "3.0")
    add_definitions(-DCATCH2_V2)
    # Put this in if/when deprecating v2.
    #message(DEPRECATION "Catch2 version 2.x is deprecated")
endif()

# --- For apps that use threads ---

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- Executables ---

add_executable(unit_tests 
	unit_tests.cpp
	test_inet_address.cpp
	test_socket.cpp
	test_stream_socket.cpp
	test_tcp_socket.cpp
	test_datagram_socket.cpp
	test_acceptor.cpp
	test_connector.cpp
)

if(UNIX)
	target_sources(unit_tests 
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/test_unix_address.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/test_unix_stream_socket.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/test_unix_dgram_socket.cpp
	)
endif()

target_include_directories(unit_tests
	PUBLIC
		${SOCKPP_INCLUDE_DIR}
	PRIVATE 
		${SOCKPP_GENERATED_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(unit_tests PROPERTIES
  CXX_EXTENSIONS OFF
)

# --- Link for executables ---

target_link_libraries(unit_tests 
	${SOCKPP_LIB} 
	Catch2::Catch2
	Threads::Threads
)

include(CTest)
include(Catch)

catch_discover_tests(unit_tests)

