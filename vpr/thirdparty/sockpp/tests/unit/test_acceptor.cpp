// test_acceptor.cpp
//
// Unit tests for the `acceptor` class(es).
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

#include "sockpp/acceptor.h"
#include "sockpp/inet_address.h"
#include "catch2_version.h"
#include <string>

using namespace sockpp;

TEST_CASE("acceptor default constructor", "[acceptor]") {
	acceptor sock;
	REQUIRE(!sock);
	REQUIRE(!sock.is_open());
}

TEST_CASE("acceptor handle constructor", "[acceptor]") {
	constexpr auto HANDLE = socket_t(3);

	SECTION("valid handle") {
		acceptor sock(HANDLE);
		REQUIRE(sock);
		REQUIRE(sock.is_open());
	}

	SECTION("invalid handle") {
		acceptor sock(INVALID_SOCKET);
		REQUIRE(!sock);
		REQUIRE(!sock.is_open());
		// TODO: Should this set an error?
		REQUIRE(sock.last_error() == 0);
	}
}

TEST_CASE("acceptor address constructor", "[acceptor]") {
	SECTION("valid address") {
		const auto ADDR = inet_address("localhost", 12345);

		acceptor sock(ADDR);
		REQUIRE(sock);
		REQUIRE(sock.is_open());
		REQUIRE(sock.last_error() == 0);
		REQUIRE(sock.address() == ADDR);
	}

	SECTION("invalid address") {
		const auto ADDR = sock_address_any{};

		acceptor sock(ADDR);
		REQUIRE(!sock);
		REQUIRE(!sock.is_open());

        // Windows returns a different error code than *nix
        #if defined(_WIN32)
            REQUIRE(sock.last_error() == WSAEINVAL);
        #else
            REQUIRE(sock.last_error() == EAFNOSUPPORT);
        #endif
	}
}

TEST_CASE("acceptor create", "[acceptor]") {
	SECTION("valid domain") {
		auto sock = acceptor::create(AF_INET);

		REQUIRE(sock);
		REQUIRE(sock.is_open());
		REQUIRE(sock.last_error() == 0);

        // Windows returns unknown family for unbound socket
        // Windows returns a different error code than *nix
        #if defined(_WIN32)
            REQUIRE(sock.family() == AF_UNSPEC);
        #else
            REQUIRE(sock.family() == AF_INET);
        #endif
	}

	SECTION("invalid domain") {
		auto sock = acceptor::create(AF_UNSPEC);

		REQUIRE(!sock);
		REQUIRE(!sock.is_open());

        // Windows returns a different error code than *nix
        #if defined(_WIN32)
            REQUIRE(sock.last_error() == WSAEINVAL);
        #else
            REQUIRE(sock.last_error() == EAFNOSUPPORT);
        #endif
	}
}

