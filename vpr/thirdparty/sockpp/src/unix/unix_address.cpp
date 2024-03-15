// unix_address.cpp
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2017 Frank Pagliughi
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

#include "sockpp/unix_address.h"
#include <cstring>
#include <stdexcept>

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

constexpr sa_family_t unix_address::ADDRESS_FAMILY;
constexpr size_t unix_address::MAX_PATH_NAME;

// --------------------------------------------------------------------------

unix_address::unix_address(const string& path)
{
	addr_.sun_family = ADDRESS_FAMILY;
	::strncpy(addr_.sun_path, path.c_str(), MAX_PATH_NAME);
}

unix_address::unix_address(const sockaddr& addr) : addr_{}
{
	if (addr.sa_family != ADDRESS_FAMILY)
        throw std::invalid_argument("Not a UNIX-domain address");

    std::memcpy(&addr_, &addr, sizeof(sockaddr));
}

unix_address::unix_address(const sockaddr_un& addr) : addr_(addr)
{
    if (addr.sun_family != ADDRESS_FAMILY)
        throw std::invalid_argument("Not a UNIX-domain address");
}

// --------------------------------------------------------------------------

ostream& operator<<(ostream& os, const unix_address& addr)
{
	os << "unix:" << addr.path();
	return os;
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}
