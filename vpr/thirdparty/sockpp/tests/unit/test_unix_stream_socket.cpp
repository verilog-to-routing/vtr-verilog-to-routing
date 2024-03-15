// test_unix_stream_socket.cpp
//
// Unit tests for the `unix_stream_socket` class.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2019 Frank Pagliughi
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------
//

#include "sockpp/unix_stream_socket.h"
#include "catch2_version.h"
#include <string>

using namespace sockpp;

// Test that we can create a Unix-domain stream socket pair and send
// data from one of the sockets to the other.
TEST_CASE("unix stream socket pair", "[unix_stream_socket]") {
	unix_stream_socket sock1, sock2;
	std::tie(sock1, sock2) = std::move(unix_stream_socket::pair());

	REQUIRE(sock1);
	REQUIRE(sock2);

	REQUIRE(sock1.is_open());
	REQUIRE(sock2.is_open());

	const std::string MSG { "Hello there!" };
	const size_t N = MSG.length();

	char buf[512];

	REQUIRE(sock1.write(MSG) == N);
	REQUIRE(sock2.read_n(buf, N) == N);

	std::string msg { buf, buf+N };
	REQUIRE(msg == MSG);
}


