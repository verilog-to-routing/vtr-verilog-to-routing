// datagram_socket.cpp
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

#include "sockpp/datagram_socket.h"
#include "sockpp/exception.h"
#include <algorithm>

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////
//								datagram_socket
/////////////////////////////////////////////////////////////////////////////

datagram_socket::datagram_socket(const sock_address& addr)
{
	auto domain = addr.family();
	socket_t h = create_handle(domain);

	if (check_socket_bool(h)) {
		reset(h);
		// TODO: If the bind fails, should we close the socket and fail completely?
		bind(addr);
	}
}

// --------------------------------------------------------------------------

ssize_t datagram_socket::recv_from(void* buf, size_t n, int flags,
								   sock_address* srcAddr /*=nullptr*/)
{
	sockaddr* p = srcAddr ? srcAddr->sockaddr_ptr() : nullptr;
    socklen_t len = srcAddr ? srcAddr->size() : 0;

	// TODO: Check returned length
    #if defined(_WIN32)
        return check_ret(::recvfrom(handle(), reinterpret_cast<char*>(buf),
                                    int(n), flags, p, &len));
    #else
        return check_ret(::recvfrom(handle(), buf, n, flags, p, &len));
    #endif
}

/////////////////////////////////////////////////////////////////////////////
// End namespace sockpp
}

