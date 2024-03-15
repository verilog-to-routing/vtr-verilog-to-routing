// undgramechosvr.cpp
//
// A simple multi-threaded TCP/IP UDP echo server for sockpp library.
//
// This runs a UDP echo server for both IPv4 and IPv6, each in a separate
// thread. They both use the same port number, either as provided by the user
// on the command line, or defaulting to 12345.
//
// USAGE:
//  	undgramechosvr [port]
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
#include "sockpp/unix_dgram_socket.h"
#include "sockpp/version.h"

using namespace std;

// --------------------------------------------------------------------------
// The main thread creates the UDP socket, and then starts them running the
// echo service in a loop.

int main(int argc, char* argv[])
{
	cout << "Sample Unix-domain datagram echo server for 'sockpp' "
		<< sockpp::SOCKPP_VERSION << '\n' << endl;

	sockpp::initialize();

	sockpp::unix_dgram_socket sock;
	if (!sock) {
		cerr << "Error creating the socket: " << sock.last_error_str() << endl;
		return 1;
	}

	if (!sock.bind(sockpp::unix_address("/tmp/undgramechosvr.sock"))) {
		cerr << "Error binding the socket: " << sock.last_error_str() << endl;
		return 1;
	}

	// Run the socket in this thread.
	ssize_t n;
	char buf[512];

	sockpp::unix_address srcAddr;

	cout << "Awaiting packets on: '" << sock.address() << "'" << endl;

	// Read some data, also getting the address of the sender,
	// then just send it back.
	while ((n = sock.recv_from(buf, sizeof(buf), &srcAddr)) > 0)
		sock.send_to(buf, n, srcAddr);

	return 0;
}

