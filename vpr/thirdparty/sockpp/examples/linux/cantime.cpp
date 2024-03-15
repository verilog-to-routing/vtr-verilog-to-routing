// cantime.cpp
//
// Linux SoxketCAN writer example.
//
// This writes the 1-sec, 32-bit, Linux time_t value to the CAN bus each
// time it ticks. This is a simple (though not overly precise) way to
// synchronize the time for nodes on the bus
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2021 Frank Pagliughi
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
#include <thread>
#include "sockpp/can_socket.h"
#include "sockpp/can_frame.h"
#include "sockpp/version.h"

#include <net/if.h>
#include <sys/ioctl.h>

using namespace std;

// The clock to use to get time and pace the app.
using sysclock = chrono::system_clock;

// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	cout << "Sample SocketCAN writer for 'sockpp' "
		<< sockpp::SOCKPP_VERSION << endl;

	string canIface = (argc > 1) ? argv[1] : "can0";
	canid_t canID = (argc > 2) ? atoi(argv[2]) : 0x20;

	sockpp::initialize();

	sockpp::can_address addr(canIface);
	sockpp::can_socket sock(addr);

	if (!sock) {
		cerr << "Error binding to the CAN interface " << canIface << "\n\t"
			<< sock.last_error_str() << endl;
		return 1;
	}

	cout << "Created CAN socket on " << sock.address() << endl;
	time_t t = sysclock::to_time_t(sysclock::now());

	while (true) {
		// Sleep until the clock ticks to the next second
		this_thread::sleep_until(sysclock::from_time_t(t+1));

		// Re-read the time in case we fell behind
		t = sysclock::to_time_t(sysclock::now());

		// Write the time to the CAN bus as a 32-bit int
		auto nt = uint32_t(t);

		sockpp::can_frame frame { canID, &nt, sizeof(nt) };
		sock.send(frame);
	}

	return (!sock) ? 1 : 0;
}
