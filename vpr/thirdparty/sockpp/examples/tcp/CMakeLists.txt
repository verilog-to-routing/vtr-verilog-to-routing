# CMakeLists.txt
#
# CMake file for the TCP v4 socket example applications
# in the 'sockpp' library.
#
# ---------------------------------------------------------------------------
# This file is part of the "sockpp" C++ socket library.
#
# Copyright (c) 2017-2022 Frank Pagliughi
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

# --- For apps that use threads ---

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- Executables ---

set(THREADED_EXECUTABLES
  tcpechosvr
  tcpechomt
  tcp6echosvr
)

set(EXECUTABLES
	tcpecho
	tcpechotest
	tcp6echo
  ${THREADED_EXECUTABLES}
)

foreach(EXECUTABLE ${EXECUTABLES})
	add_executable(${EXECUTABLE} ${EXECUTABLE}.cpp)
	
	target_include_directories(${EXECUTABLE}
		PUBLIC
			${SOCKPP_INCLUDE_DIR}
		PRIVATE
			${SOCKPP_GENERATED_DIR}/include
	)
	
  set_target_properties(${EXECUTABLE} PROPERTIES
    CXX_EXTENSIONS OFF
  )

	target_link_libraries(${EXECUTABLE} ${SOCKPP_LIB})
endforeach()

# --- Additional linkage to Threads ---

foreach(EXECUTABLE ${THREADED_EXECUTABLES})
  target_link_libraries(${EXECUTABLE} Threads::Threads)
endforeach()

# --- Install examples ---

install(TARGETS ${EXECUTABLES} RUNTIME DESTINATION bin)

