// tcpechotest.cpp
//
// TCP echo timing client.
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2020 Frank Pagliughi
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
#include <chrono>
#include <random>
#include "sockpp/tcp_connector.h"
#include "sockpp/version.h"

using namespace std;
using namespace std::chrono;

const size_t DFLT_N = 100000;
const size_t DFLT_SZ = 512;

using fpsec = std::chrono::duration<double, std::chrono::seconds::period>;

// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	cout << "Unix-domain echo timing test client for 'sockpp' "
		<< sockpp::SOCKPP_VERSION << '\n' << endl;

	string host = (argc > 1) ? argv[1] : "localhost";
	in_port_t port = (argc > 2) ? atoi(argv[2]) : 12345;

	size_t n  = (argc > 3) ? size_t(atoll(argv[3])) : DFLT_N;
	size_t sz = (argc > 4) ? size_t(atoll(argv[4])) : DFLT_SZ;

	sockpp::initialize();

	auto t_start = high_resolution_clock::now();

	sockpp::tcp_connector conn({host, port});
	if (!conn) {
		cerr << "Error connecting to server at "
			<< sockpp::inet_address(host, port)
			<< "\n\t" << conn.last_error_str() << endl;
		return 1;
	}

	cout << "Created a connection from " << conn.address() << endl;
	cout << "Created a connection to " << conn.peer_address() << endl;

    // Set a timeout for the responses
    if (!conn.read_timeout(seconds(2))) {
        cerr << "Error setting timeout on TCP stream: "
                << conn.last_error_str() << endl;
    }
	string s, sret;

    random_device rd;
    mt19937 reng(rd());
    uniform_int_distribution<> dist(0, 25);

	for (size_t i=0; i<sz; ++i)
		s.push_back('a' + static_cast<char>(dist(reng)));

	auto t_start_tx = high_resolution_clock::now();

	for (size_t i=0; i<n; ++i) {
		if (conn.write(s) != (ssize_t) s.length()) {
			cerr << "Error writing to the UNIX stream" << endl;
			break;
		}

		sret.resize(s.length());
		ssize_t n = conn.read_n(&sret[0], s.length());

		if (n != (ssize_t) s.length()) {
			cerr << "Error reading from UNIX stream" << endl;
			break;
		}
	}

	auto t_end = high_resolution_clock::now();
	cout << "Total time: " << fpsec(t_end - t_start).count() << "s" << endl;

	auto t_tx = fpsec(t_end - t_start_tx).count();
	auto rate = size_t(n / t_tx);
	cout << "Transfer time: " << t_tx << "s\n    "
		<< rate << " msg/s" << endl;

	return (!conn) ? 1 : 0;
}
