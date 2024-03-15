# CMakeLists.txt
#
# Source CMake build file (src/) for the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2017-2018 Frank Pagliughi
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

add_library(sockpp-objs OBJECT
	acceptor.cpp
	connector.cpp
	datagram_socket.cpp
	exception.cpp
	inet_address.cpp
	inet6_address.cpp
  result.cpp
	socket.cpp
	stream_socket.cpp
)

if(UNIX)
	target_sources(sockpp-objs PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/unix/unix_address.cpp
	)
	if(SOCKPP_BUILD_CAN)
		target_sources(sockpp-objs PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/linux/can_address.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/linux/can_socket.cpp
		)
	endif()
endif()

target_compile_features(sockpp-objs PUBLIC cxx_std_14)

# Position indepence is only necessary for older compilers, but doesn't hurt
set_target_properties(sockpp-objs 
	PROPERTIES 
		POSITION_INDEPENDENT_CODE 1
		CXX_EXTENSIONS OFF
)

target_include_directories(sockpp-objs
	PUBLIC
		$<BUILD_INTERFACE:${SOCKPP_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:include>
	PRIVATE 
		${SOCKPP_GENERATED_DIR}/include
		${CMAKE_SOURCE_DIR}/src
)

# --- Warnings ---

target_compile_options(sockpp-objs PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic -Wdocumentation>
  $<$<NOT:$<OR:$<CXX_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:Clang>>>:-Wall -Wextra -Wpedantic>
)

