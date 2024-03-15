/// @file unix_acceptor.h
///
/// Class for a TCP server to accept incoming connections.
///
/// @author	Frank Pagliughi
///	@author	SoRo Systems, Inc.
///	@author	www.sorosys.com
///
/// @date	December 2014

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

#ifndef __sockpp_unix_acceptor_h
#define __sockpp_unix_acceptor_h

#include "sockpp/acceptor.h"
#include "sockpp/unix_stream_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/// Class for creating a Unix-domain server.
/// Objects of this class bind and listen on Unix-domain ports for
/// connections. Normally, a server thread creates one of these and blocks
/// on the call to accept incoming connections. The call to accept creates
/// and returns a @ref unix_stream_socket which can then be used for the
/// actual communications.

class unix_acceptor : public acceptor
{
    /** The base class */
    using base = acceptor;

	// Non-copyable
	unix_acceptor(const unix_acceptor&) =delete;
	unix_acceptor& operator=(const unix_acceptor&) =delete;

public:
	/**
	 * Creates an unconnected acceptor.
	 */
	unix_acceptor() {}
	/**
	 * Creates a acceptor and starts it listening on the specified address.
	 * @param addr The TCP address on which to listen.
	 * @param queSize The listener queue size.
	 */
	unix_acceptor(const unix_address& addr, int queSize=DFLT_QUE_SIZE) {
		open(addr, queSize);
	}
	/**
	 * Gets the local address to which we are bound.
	 * @return The local address to which we are bound.
	 */
	unix_address address() const { return unix_address(base::address()); }
    /**
     * Base open call also work.
     */
    using base::open;
	/**
	 * Opens the acceptor socket and binds it to the specified address.
	 * @param addr The address to which this server should be bound.
	 * @param queSize The listener queue size.
	 * @return @em true on success, @em false on error
	 */
	bool open(const unix_address& addr, int queSize=DFLT_QUE_SIZE) {
		return base::open(addr, queSize);
	}
	/**
     * Accepts an incoming UNIX connection and gets the address of the
     * client.
	 * @return A unix_socket to the client.
	 */
	unix_socket accept() { return unix_socket(base::accept()); }
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
};

#endif		// __sockpp_unix_acceptor_h

