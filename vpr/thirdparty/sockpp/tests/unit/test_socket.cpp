// test_socket.cpp
//
// Unit tests for the base `socket` class.
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

#include "sockpp/socket.h"
#include "sockpp/inet_address.h"
#include "catch2_version.h"
#include <string>

using namespace sockpp;
using namespace std::chrono;

/////////////////////////////////////////////////////////////////////////////
// Aux functions

TEST_CASE("test to_timeval", "aux") {
	SECTION("concrete function") {
		timeval tv = to_timeval(microseconds(500));
		REQUIRE(tv.tv_sec == 0);
		REQUIRE(tv.tv_usec == 500);

		tv = to_timeval(microseconds(2500000));
		REQUIRE(tv.tv_sec == 2);
		REQUIRE(tv.tv_usec == 500000);
	}

	SECTION("template") {
		timeval tv = to_timeval(milliseconds(1));
		REQUIRE(tv.tv_sec == 0);
		REQUIRE(tv.tv_usec == 1000);

		tv = to_timeval(milliseconds(2500));
		REQUIRE(tv.tv_sec == 2);
		REQUIRE(tv.tv_usec == 500000);

		tv = to_timeval(seconds(5));
		REQUIRE(tv.tv_sec == 5);
		REQUIRE(tv.tv_usec == 0);
	}
}

/////////////////////////////////////////////////////////////////////////////
// socket class

constexpr in_port_t INET_TEST_PORT = 12346;

TEST_CASE("socket constructors", "[socket]") {
	SECTION("default constructor") {
		sockpp::socket sock;

		REQUIRE(!sock);
		REQUIRE(!sock.is_open());
		REQUIRE(sock.handle() == INVALID_SOCKET);
		REQUIRE(sock.last_error() == 0);
	}

	SECTION("handle constructor") {
		constexpr auto HANDLE = socket_t(3);
		sockpp::socket sock(HANDLE);

		REQUIRE(sock);
		REQUIRE(sock.is_open());
		REQUIRE(sock.handle() == HANDLE);
		REQUIRE(sock.last_error() == 0);
	}


	SECTION("move constructor") {
		constexpr auto HANDLE = socket_t(3);
		sockpp::socket org_sock(HANDLE);

		sockpp::socket sock(std::move(org_sock));

		// Make sure the new socket got the handle
		REQUIRE(sock);
		REQUIRE(sock.handle() == HANDLE);
		REQUIRE(sock.last_error() == 0);

		// Make sure the handle was moved out of the org_sock
		REQUIRE(!org_sock);
		REQUIRE(org_sock.handle() == INVALID_SOCKET);
	}
}

// Test the socket error behavior
TEST_CASE("socket errors", "[socket]") {
	SECTION("basic errors") {
		sockpp::socket sock;

		// Operations on an unopened socket should give an error
		int reuse = 1;
		socklen_t len = sizeof(int);
		bool ok = sock.get_option(SOL_SOCKET, SO_REUSEADDR, &reuse, &len);

		// Socket should be in error state
		REQUIRE(!ok);
		REQUIRE(!sock);

		int err = sock.last_error();
		REQUIRE(err != 0);

		// last_error() is sticky, unlike `errno`
		REQUIRE(sock.last_error() == err);

		// We can clear the error
		sock.clear();
		REQUIRE(sock.last_error() == 0);

		// Test arbitrary clear value
		sock.clear(42);
		REQUIRE(sock.last_error() == 42);
		REQUIRE(!sock);
	}

	SECTION("clear error") {
		auto sock = sockpp::socket::create(AF_INET, SOCK_STREAM);
		REQUIRE(sock);

		sock.clear(42);
		REQUIRE(!sock);

		sock.clear();
		REQUIRE(sock);
	}
}

TEST_CASE("socket handles", "[socket]") {

	constexpr auto HANDLE = socket_t(3);

	SECTION("test release") {
		sockpp::socket sock(HANDLE);

		REQUIRE(sock.handle() == HANDLE);
		REQUIRE(sock.release() == HANDLE);

		// Make sure the handle was moved out of the sock
		REQUIRE(!sock);
		REQUIRE(sock.handle() == INVALID_SOCKET);
	}

	SECTION("test reset") {
		sockpp::socket sock(HANDLE);
		REQUIRE(sock.handle() == HANDLE);

		sock.reset();	// Default reset acts like release w/o return

		// Make sure the handle was moved out of the sock
		REQUIRE(!sock);
		REQUIRE(sock.handle() == INVALID_SOCKET);

		// Now reset with a "valid" handle
		sock.reset(HANDLE);
		REQUIRE(sock);
		REQUIRE(sock.handle() == HANDLE);
	}
}

TEST_CASE("socket family", "[socket]") {
	SECTION("uninitialized socket") {
		// Uninitialized socket should have unspecified family
		sockpp::socket sock;
		REQUIRE(sock.family() == AF_UNSPEC);
	}

	SECTION("unbound socket") {
		// Unbound socket should have creation family
		auto sock = socket::create(AF_INET, SOCK_STREAM);

		// Windows and *nix behave differently
		#if defined(_WIN32)
			REQUIRE(sock.family() == AF_UNSPEC);
		#else
			REQUIRE(sock.family() == AF_INET);
		#endif
	}

	SECTION("bound socket") {
		// Bound socket should have same family as
		// address to which it's bound
		auto sock = socket::create(AF_INET, SOCK_STREAM);
		inet_address addr(INET_TEST_PORT);

		int reuse = 1;
		REQUIRE(sock.set_option(SOL_SOCKET, SO_REUSEADDR, reuse));
		REQUIRE(sock.bind(addr));
		REQUIRE(sock.family() == addr.family());
	}
}

TEST_CASE("socket address", "[socket]") {
	SECTION("uninitialized socket") {
		// Uninitialized socket should have empty address
		sockpp::socket sock;
		REQUIRE(sock.address() == sock_address_any{});
	}

	// The address has the specified family but all zeros
	SECTION("unbound socket") {
		auto sock = socket::create(AF_INET, SOCK_STREAM);
		auto addr = inet_address(sock.address());

		// Windows and *nix behave differently for family
		#if defined(_WIN32)
			REQUIRE(sock.family() == AF_UNSPEC);
		#else
			REQUIRE(sock.family() == AF_INET);
		#endif

		REQUIRE(addr.address() == 0);
		REQUIRE(addr.port() == 0);
	}

	SECTION("bound socket") {
		// Bound socket should have same family as
		// address to which it's bound
		auto sock = socket::create(AF_INET, SOCK_STREAM);
		const inet_address ADDR(INET_TEST_PORT);

		int reuse = 1;
		REQUIRE(sock.set_option(SOL_SOCKET, SO_REUSEADDR, reuse));

		REQUIRE(sock.bind(ADDR));
		REQUIRE(sock.address() == ADDR);
	}
}

// Socket pair shouldn't work for TCP sockets on any known platform.
// So this should fail, but fail gracefully and retain the error
// in both sockets.
TEST_CASE("failed socket pair", "[socket]") {
	sockpp::socket sock1, sock2;
	std::tie(sock1, sock2) = std::move(socket::pair(AF_INET, SOCK_STREAM));

	REQUIRE(!sock1);
	REQUIRE(!sock2);

	REQUIRE(sock1.last_error() != 0);
	REQUIRE(sock1.last_error() == sock2.last_error());
}

// Test putting the socket into and out of non-blocking mode
TEST_CASE("socket non-blocking mode", "[socket]") {
	auto sock = socket::create(AF_INET, SOCK_STREAM);

	#if !defined(_WIN32)
		REQUIRE(!sock.is_non_blocking());
	#endif

	REQUIRE(sock.set_non_blocking());
	#if !defined(_WIN32)
		REQUIRE(sock.is_non_blocking());
	#endif

	REQUIRE(sock.set_non_blocking(false));
	#if !defined(_WIN32)
		REQUIRE(!sock.is_non_blocking());
	#endif
}

// --------------------------------------------------------------------------

// Test that the "last error" call to a socket gives the proper result
// for the current thread.
// Here we share a socket across two threads, force an error in one
// thread, and then check to make sure that the error did not propagate
// to the other thread.
//
#if 0
TEST_CASE("thread-safe last error", "[socket]") {
	sockpp::socket sock;

	int state = 0;
	std::mutex m;
	std::condition_variable cv;

	std::thread thr([&] {
		// Test #1
		REQUIRE(sock.last_error() == 0);
		{
			// Wait for Test #2
			std::unique_lock<std::mutex> lk(m);
			state = 1;
			cv.notify_one();
			cv.wait(lk, [&state]{return state >= 2;});
		}

		// Test #3
		REQUIRE(sock.last_error() == 0);
	});

	{
		// Wait for Test #1
		std::unique_lock<std::mutex> lk(m);
		cv.wait(lk, [&state]{return state >= 1;});
	}

	// Test #2
	// Setting options on an un-opened socket should generate an error
	int reuse = 1;
	socklen_t len = sizeof(int);
	bool ok = sock.get_option(SOL_SOCKET, SO_REUSEADDR, &reuse, &len);

	REQUIRE(!ok);
	REQUIRE(sock.last_error() != 0);

	{
		std::unique_lock<std::mutex> lk(m);
		state = 2;
		cv.notify_one();
	}
	thr.join();
}
#endif

