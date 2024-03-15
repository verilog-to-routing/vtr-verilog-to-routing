// test_tcp_socket.cpp
//
// Unit tests for the `tcp_socket` class(es).
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

#include "sockpp/tcp_socket.h"
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "catch2_version.h"
#include <string>

using namespace sockpp;

static const in_port_t TEST_PORT = 12345;

TEST_CASE("tcp_socket default constructor", "[tcp_socket]") {
	tcp_socket sock;
	REQUIRE(!sock);
	REQUIRE(!sock.is_open());

	//REQUIRE(sock.family() == AF_INET);
}

TEST_CASE("tcp_socket handle constructor", "[tcp_socket]") {
	constexpr auto HANDLE = socket_t(3);

	SECTION("valid handle") {
		tcp_socket sock(HANDLE);
		REQUIRE(sock);
		REQUIRE(sock.is_open());

		//REQUIRE(sock.family() == AF_INET);
	}

	SECTION("invalid handle") {
		tcp_socket sock(INVALID_SOCKET);
		REQUIRE(!sock);
		REQUIRE(!sock.is_open());
		// TODO: Should this set an error?
		REQUIRE(sock.last_error() == 0);

		//REQUIRE(sock.family() == AF_INET);
	}
}

// --------------------------------------------------------------------------
// Connected tests

TEST_CASE("tcp_socket read/write", "[stream_socket]") {
	const std::string STR { "This is a test. This is only a test." };
	const size_t N = STR.length();

	inet_address addr { "localhost", TEST_PORT };
	tcp_acceptor asock{ addr };

	tcp_connector csock;
	csock.set_non_blocking();

	REQUIRE(csock.connect(addr));

	auto ssock = asock.accept();
	REQUIRE(ssock);

	SECTION("read_n/write_n") {
		char buf[512];  // N

		REQUIRE(csock.write_n(STR.data(), N) == N);
		REQUIRE(ssock.read_n(buf, N) == N);

		std::string str { buf, buf+N };
		REQUIRE(str == STR);

		char buf2[512]; // N

		// string write is a write_n()
		REQUIRE(csock.write(STR) == N);
		REQUIRE(ssock.read_n(buf2, N) == N);

		std::string str2 { buf2, buf2+N };
		REQUIRE(str2 == STR);
	}

	SECTION("scatter/gather") {
		const std::string HEADER { "<start>" },
						  FOOTER { "<end>" };

		const size_t N_HEADER = HEADER.length(),
					 N_FOOTER = FOOTER.length(),
					 N_TOT = N_HEADER + N + N_FOOTER;

		std::vector<iovec> outv {
			iovec { (void*) HEADER.data(), N_HEADER },
			iovec { (void*) STR.data(), N },
			iovec { (void*) FOOTER.data(), N_FOOTER }
		};

		char hbuf[512], 	// N_HEADER
			 buf[512],  	// N
			 fbuf[512]; 	// N_FOOTER

		std::vector<iovec> inv {
			iovec { (void*) hbuf, N_HEADER },
			iovec { (void*) buf, N },
			iovec { (void*) fbuf, N_FOOTER }
		};

		REQUIRE(csock.write(outv) == N_TOT);
		REQUIRE(csock.write(outv) == N_TOT);
		REQUIRE(csock.write(outv) == N_TOT);

		REQUIRE(ssock.read(inv) == N_TOT);

		REQUIRE(std::string(hbuf, N_HEADER) == HEADER);
		REQUIRE(std::string(buf, N) == STR);
		REQUIRE(std::string(fbuf, N_FOOTER) == FOOTER);
	}
}


