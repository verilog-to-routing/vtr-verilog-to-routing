// inet6_address.cpp
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

#include "sockpp/inet6_address.h"
#include "sockpp/exception.h"

using namespace std;

namespace sockpp {

// --------------------------------------------------------------------------

bool inet6_address::is_set() const
{
	static const auto EMPTY_ADDR = sockaddr_in6{};
	return std::memcmp(&addr_, &EMPTY_ADDR, SZ) != 0;
}

// --------------------------------------------------------------------------

in6_addr inet6_address::resolve_name(const string& saddr)
{
	#if !defined(_WIN32)
		in6_addr ia;
		if (::inet_pton(ADDRESS_FAMILY, saddr.c_str(), &ia) == 1)
			return ia;
	#endif

    addrinfo *res, hints = addrinfo{};
    hints.ai_family = ADDRESS_FAMILY;
    hints.ai_socktype = SOCK_STREAM;

    int gai_err = ::getaddrinfo(saddr.c_str(), NULL, &hints, &res);

    #if !defined(_WIN32)
        if (gai_err == EAI_SYSTEM)
            throw sys_error();
    #endif

    if (gai_err != 0)
        throw getaddrinfo_error(gai_err, saddr);


    auto ipv6 = reinterpret_cast<sockaddr_in6*>(res->ai_addr);
    auto addr = ipv6->sin6_addr;
    freeaddrinfo(res);
    return addr;
}

// --------------------------------------------------------------------------

void inet6_address::create(const in6_addr& addr, in_port_t port)
{
	addr_ = sockaddr_in6{};
    addr_.sin6_family = AF_INET6;
    addr_.sin6_flowinfo = 0;
    addr_.sin6_addr = addr;
    addr_.sin6_port = htons(port);
	#if defined(__APPLE__) || defined(BSD)
		addr_.sin6_len = (uint8_t) SZ;
	#endif

}

// --------------------------------------------------------------------------

void inet6_address::create(const string& saddr, in_port_t port)
{
	addr_ = sockaddr_in6{};
	addr_.sin6_family = AF_INET6;
    addr_.sin6_flowinfo = 0;
	addr_.sin6_addr = resolve_name(saddr.c_str());
	addr_.sin6_port = htons(port);
	#if defined(__APPLE__) || defined(BSD)
		addr_.sin6_len = (uint8_t) SZ;
	#endif
}

// --------------------------------------------------------------------------

string inet6_address::to_string() const
{
    char buf[INET6_ADDRSTRLEN];
    auto str = inet_ntop(AF_INET6, (void*) &(addr_.sin6_addr),
						 buf, INET6_ADDRSTRLEN);
    return std::string("[") + std::string(str ? str : "<unknown>")
        + "]:" + std::to_string(unsigned(port()));
}

/////////////////////////////////////////////////////////////////////////////

ostream& operator<<(ostream& os, const inet6_address& addr)
{
	char buf[INET6_ADDRSTRLEN];
	auto str = inet_ntop(AF_INET6, (void*) &(addr.sockaddr_in6_ptr()->sin6_addr),
						 buf, INET6_ADDRSTRLEN);
	os << "[" << (str ? str : "<unknown>") << "]:" << unsigned(addr.port());
	return os;
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}

