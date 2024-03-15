// udpecho.cpp
//
// Simple UDP v4 echo client
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
#include <string>
#include "sockpp/udp_socket.h"
#include "sockpp/version.h"

using namespace std;

// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	cout << "Sample UDP echo client for 'sockpp' "
		<< sockpp::SOCKPP_VERSION << '\n' << endl;

	string host = (argc > 1) ? argv[1] : "localhost";
	in_port_t port = (argc > 2) ? atoi(argv[2]) : 12345;

	sockpp::initialize();

	sockpp::udp_socket sock;

	if (!sock.connect(sockpp::inet_address(host, port))) {
		cerr << "Error connecting to server at " << host << ":" << port 
			<< "\n\t" << sock.last_error_str() << endl;
		return 1;
	}

	cout << "Created UDP socket at: " << sock.address() << endl;

	string s, sret;
	while (getline(cin, s) && !s.empty()) {
		if (sock.send(s) != ssize_t(s.length())) {
			cerr << "Error writing to the UDP socket: "
				<< sock.last_error_str() << endl;
			break;
		}

		sret.resize(s.length());
		ssize_t n = sock.recv(&sret[0], s.length());

		if (n != ssize_t(s.length())) {
			cerr << "Error reading from UDP socket: "
				<< sock.last_error_str() << endl;
			break;
		}

		cout << sret << endl;
	}

	return (!sock) ? 1 : 0;
}
