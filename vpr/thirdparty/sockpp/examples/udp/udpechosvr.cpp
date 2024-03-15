// udpechosvr.cpp
//
// A simple multi-threaded TCP/IP UDP echo server for sockpp library.
//
// This runs a UDP echo server for both IPv4 and IPv6, each in a separate
// thread. They both use the same port number, either as provided by the user
// on the command line, or defaulting to 12345.
//
// USAGE:
//  	uspechosvr [port]
//
// You can test with a netcat client, like:
// 		$ nc -u localhost 12345		# IPv4
// 		$ nc -6u localhost 12345	# IPv6
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

#include <iostream>
#include <thread>
#include "sockpp/udp_socket.h"
#include "sockpp/udp6_socket.h"
#include "sockpp/version.h"

using namespace std;

// --------------------------------------------------------------------------
// The thread function. This is run in a separate thread for each socket.
// Ownership of the socket object is transferred to the thread, so when this
// function exits, the socket is automatically closed.

template <typename UDPSOCK>
void run_echo(UDPSOCK sock)
{
	ssize_t n;
	char buf[512];

	// Each UDP socket type knows its address type as `addr_t`
	typename UDPSOCK::addr_t srcAddr;

	// Read some data, also getting the address of the sender,
	// then just send it back.
	while ((n = sock.recv_from(buf, sizeof(buf), &srcAddr)) > 0)
		sock.send_to(buf, n, srcAddr);
}

// --------------------------------------------------------------------------
// The main thread creates the two UDP sockets (one each for IPv4 and IPv6),
// and then starts them running the echo function each in a separate thread.

int main(int argc, char* argv[])
{
	cout << "Sample UDP echo server for 'sockpp' "
		<< sockpp::SOCKPP_VERSION << '\n' << endl;

	in_port_t port = (argc > 1) ? atoi(argv[1]) : 12345;

	sockpp::initialize();

	sockpp::udp_socket	udpsock;
	if (!udpsock) {
		cerr << "Error creating the UDP v4 socket: " << udpsock.last_error_str() << endl;
		return 1;
	}

	if (!udpsock.bind(sockpp::inet_address("localhost", port))) {
		cerr << "Error binding the UDP v4 socket: " << udpsock.last_error_str() << endl;
		return 1;
	}

	sockpp::udp6_socket	udp6sock;
	if (!udp6sock) {
		cerr << "Error creating the UDP v6 socket: " << udp6sock.last_error_str() << endl;
		return 1;
	}

	if (!udp6sock.bind(sockpp::inet6_address("localhost", port))) {
		cerr << "Error binding the UDP v6 socket: " << udp6sock.last_error_str() << endl;
		return 1;
	}

	// Spin up a thread to run the IPv4 socket.
	thread thr(run_echo<sockpp::udp_socket>, std::move(udpsock));
	thr.detach();

	// Run the IPv6 socket in this thread. (Call doesn't return)
	run_echo(std::move(udp6sock));
	return 0;
}

