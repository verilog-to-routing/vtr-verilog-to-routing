// test_inet6_address.cpp
//
// Unit tests for the `inet6_address` class.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2018-2023 Frank Pagliughi
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

#include "sockpp/inet6_address.h"
#include "catch2_version.h"
#include <string>

using namespace sockpp;

const in6_addr      ANY_ADDR        IN6ADDR_ANY_INIT;         // Any iface
const in6_addr      LOCALHOST_ADDR  IN6ADDR_LOOPBACK_INIT;    // Localhost 0x7F000001
const in_port_t     PORT            { 12345 };

TEST_CASE("inet6_address default constructor", "[address]") {
    inet6_address addr;
	in6_addr iaddr{};

    REQUIRE(!addr);
    REQUIRE(!addr.is_set());
    REQUIRE(memcmp(iaddr.s6_addr, addr.address().s6_addr, 16) == 0);
    REQUIRE(0 == addr.port());
    REQUIRE(sizeof(sockaddr_in6) == addr.size());
}

// When created using only a port number this should use the
// "any" address to bind to all interfaces (typ for server)
TEST_CASE("inet6_address port-only constructor", "[address]") {
    inet6_address addr(PORT);

    REQUIRE(addr);
    REQUIRE(addr.is_set());
    REQUIRE(memcmp(ANY_ADDR.s6_addr, addr.address().s6_addr, 16) == 0);
    REQUIRE(PORT == addr.port());

    // Check the low-level struct
    REQUIRE(AF_INET6 == addr.sockaddr_in6_ptr()->sin6_family);
}

/*
TEST_CASE("inet_address int32_t constructor", "[address]") {
    inet_address addr(LOCALHOST_ADDR, PORT);

	REQUIRE(addr);
    REQUIRE(addr.is_set());
    REQUIRE(LOCALHOST_ADDR == addr.address());
    REQUIRE(PORT == addr.port());

    REQUIRE(uint8_t((LOCALHOST_ADDR >>  0) &0xFF) == addr[0]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >>  8) &0xFF) == addr[1]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 16) &0xFF) == addr[2]);
    REQUIRE(uint8_t((LOCALHOST_ADDR >> 24) &0xFF) == addr[3]);

    // Check the low-level struct
    REQUIRE(AF_INET == addr.sockaddr_in_ptr()->sin_family);
    REQUIRE(LOCALHOST_ADDR == ntohl(addr.sockaddr_in_ptr()->sin_addr.s_addr));
    REQUIRE(PORT == ntohs(addr.sockaddr_in_ptr()->sin_port));
}
*/

TEST_CASE("inet6_address name constructor", "[address]") {
    inet6_address addr{"FD80:CD00:0000:0CDE:1257:0000:211E:729C", PORT};

	REQUIRE(addr);
    REQUIRE(addr.is_set());
    REQUIRE(0xFD == addr[0]);
    REQUIRE(0x9C == addr[15]);
    REQUIRE(PORT == addr.port());

    // Check the low-level struct
    REQUIRE(AF_INET6 == addr.sockaddr_in6_ptr()->sin6_family);
    REQUIRE(PORT == ntohs(addr.sockaddr_in6_ptr()->sin6_port));
}

TEST_CASE("IPv6 resolve address", "[address]") {
	SECTION("local address") {
		auto addr_res = inet6_address::resolve_name("::1");
		REQUIRE(addr_res.is_ok());
		auto addr = addr_res.value();

		REQUIRE(memcmp(LOCALHOST_ADDR.s6_addr, addr.s6_addr, 16) == 0);
	}

	SECTION("any address") {
		auto addr_res = inet6_address::resolve_name("::");
		REQUIRE(addr_res.is_ok());
		auto addr = addr_res.value();
		REQUIRE(memcmp(ANY_ADDR.s6_addr, addr.s6_addr, 16) == 0);
	}

	// According to RFC6761, "invalid." domain should not resolve
	SECTION("resolve failure", "[address]") {
		#if defined(SOCKPP_WITH_EXCEPTIONS)
			REQUIRE_THROWS(inet6_address::resolve_name("invalid"));
		#else
			auto res = inet6_address::resolve_name("invalid");
			REQUIRE(!res);
			REQUIRE(res.is_error());
		#endif
	}
}

TEST_CASE("IPv6 create address", "[address]") {
	SECTION("local address") {
		auto res = inet6_address::create("::1", PORT);
		REQUIRE(res.is_ok());

		auto addr = res.value();

		REQUIRE(LOCALHOST_ADDR == addr.address());
		REQUIRE(PORT == addr.port());
		REQUIRE(inet6_address{LOCALHOST_ADDR, PORT} == addr);
	}

	SECTION("any address") {
		auto res = inet6_address::create("::", PORT);
		REQUIRE(res.is_ok());

		auto addr = res.value();

		REQUIRE(ANY_ADDR == addr.address());
		REQUIRE(PORT == addr.port());
		REQUIRE(inet6_address{ANY_ADDR, PORT} == addr);
	}

	// According to RFC6761, "invalid." domain should not resolve
	SECTION("create failure", "[address]") {
		#if defined(SOCKPP_WITH_EXCEPTIONS)
			REQUIRE_THROWS(inet6_address::create("invalid", PORT));
		#else
			auto res = inet6_address::create("invalid", PORT);
			REQUIRE(!res);
			REQUIRE(res.is_error());
		#endif
	}
}

