// can_address.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2021 Frank Pagliughi
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

#include "sockpp/can_address.h"
#include "sockpp/socket.h"
#include <cstring>
#include <stdexcept>
#include <sys/ioctl.h>
#include <net/if.h>

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

constexpr sa_family_t can_address::ADDRESS_FAMILY;

// --------------------------------------------------------------------------

can_address::can_address(unsigned ifindex) : addr_{}
{
	addr_.can_family = AF_CAN;
	addr_.can_ifindex = ifindex;
}

can_address::can_address(const string& iface) : addr_{}
{
	unsigned idx = if_nametoindex(iface.c_str());

	if (idx != 0) {
		addr_.can_family = AF_CAN;
		addr_.can_ifindex = idx;
	}
}

can_address::can_address(const sockaddr& addr)
{
    auto domain = addr.sa_family;
    if (domain != AF_CAN)
        throw std::invalid_argument("Not a SocketCAN address");

    std::memcpy(&addr_, &addr, sizeof(sockaddr));
}

string can_address::iface() const
{
	if (addr_.can_family == AF_UNSPEC)
		return string("none");

	if (addr_.can_ifindex == 0)
		return string("any");

	char buf[IF_NAMESIZE];
	const char* iface = if_indextoname(addr_.can_ifindex, buf);

	return string(iface ? iface : "unknown");
}


// --------------------------------------------------------------------------

ostream& operator<<(ostream& os, const can_address& addr)
{
	os << "can:" << addr.iface();
	return os;
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}
