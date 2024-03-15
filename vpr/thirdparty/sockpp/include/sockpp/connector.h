/**
 * @file connector.h
 *
 * Class for creating client-side streaming connections.
 *
 * @author	Frank Pagliughi
 * @author	SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date	December 2014
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2019 Frank Pagliughi
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

#ifndef __sockpp_connector_h
#define __sockpp_connector_h

#include "sockpp/stream_socket.h"
#include "sockpp/sock_address.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class to create a client stream connection.
 * This is a base class for creating active, streaming sockets that initiate
 * connections to a server. It can be used to derive classes that implement
 * TCP on IPv4 or IPv6.
 */
class connector : public stream_socket
{
    /** The base class */
	using base = stream_socket;

	// Non-copyable
	connector(const connector&) =delete;
	connector& operator=(const connector&) =delete;

	/** Recreate the socket with a new handle, closing any old one. */
    bool recreate(const sock_address& addr);

public:
	/**
	 * Creates an unconnected connector.
	 */
	connector() {}
	/**
	 * Creates the connector and attempts to connect to the specified
	 * address.
	 * @param addr The remote server address.
	 */
	connector(const sock_address& addr) { connect(addr); }
	/**
	 * Creates the connector and attempts to connect to the specified
	 * server, with a timeout.
	 * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
	 */
	template <class Rep, class Period>
	connector(const sock_address& addr, const std::chrono::duration<Rep, Period>& relTime) {
		connect(addr, std::chrono::microseconds(relTime));
	}
	/**
	 * Creates the connector and attempts to connect to the specified
	 * address, with a timeout.
     * If the operation times out, the \ref last_error will be set to ETIMEOUT.
	 * @param addr The remote server address.
     * @param t The duration after which to give up. Zero means never.
	 */
	connector(const sock_address& addr, std::chrono::milliseconds t) { connect(addr, t); }
	/**
	 * Move constructor.
	 * Creates a connector by moving the other connector to this one.
	 * @param conn Another connector.
	 */
	connector(connector&& conn) : base(std::move(conn)) {}
	/**
	 * Move assignment.
	 * @param rhs The other connector to move into this one.
	 * @return A reference to this object.
	 */
	connector& operator=(connector&& rhs) {
		base::operator=(std::move(rhs));
		return *this;
	}
	/**
	 * Determines if the socket connected to a remote host.
	 * Note that this is not a reliable determination if the socket is
	 * currently connected, but rather that an initial connection was
	 * established.
	 * @return @em true If the socket connected to a remote host,
	 *  	   @em false if not.
	 */
	bool is_connected() const { return is_open(); }
	/**
     * Attempts to connect to the specified server.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
	 * @param addr The remote server address.
	 * @return @em true on success, @em false on error
	 */
	bool connect(const sock_address& addr);
	/**
     * Attempts to connect to the specified server, with a timeout.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
	 * If the operation times out, the @ref last_error will be set to
	 * ETIMEDOUT.
	 * @param addr The remote server address.
     * @param timeout The duration after which to give up. Zero means never.
	 * @return @em true on success, @em false on error
	 */
	bool connect(const sock_address& addr, std::chrono::microseconds timeout);
	/**
     * Attempts to connect to the specified server, with a timeout.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
	 * If the operation times out, the @ref last_error will be set to
	 * ETIMEDOUT.
	 * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
	 * @return @em true on success, @em false on error
	 */
	template <class Rep, class Period>
	bool connect(const sock_address& addr, const std::chrono::duration<Rep, Period>& relTime) {
		return connect(addr, std::chrono::microseconds(relTime));
	}
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Class to create a client TCP connection.
 */
template <typename STREAM_SOCK, typename ADDR=typename STREAM_SOCK::addr_t>
class connector_tmpl : public connector
{
    /** The base class */
	using base = connector;

	// Non-copyable
	connector_tmpl(const connector_tmpl&) =delete;
	connector_tmpl& operator=(const connector_tmpl&) =delete;

public:
	/** The type of streaming socket from the acceptor. */
	using stream_sock_t = STREAM_SOCK;
	/** The type of address for the connector. */
	using addr_t = ADDR;

	/**
	 * Creates an unconnected connector.
	 */
	connector_tmpl() {}
	/**
	 * Creates the connector and attempts to connect to the specified
	 * address.
	 * @param addr The remote server address.
	 */
	connector_tmpl(const addr_t& addr) : base(addr) {}
	/**
	 * Creates the connector and attempts to connect to the specified
	 * server, with a timeout.
	 * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
	 */
	template <class Rep, class Period>
	connector_tmpl(const addr_t& addr, const std::chrono::duration<Rep, Period>& relTime)
		: base(addr, relTime)
	{}
	/**
	 * Move constructor.
	 * Creates a connector by moving the other connector to this one.
	 * @param rhs Another connector.
	 */
	connector_tmpl(connector_tmpl&& rhs) : base(std::move(rhs)) {}
	/**
	 * Move assignment.
	 * @param rhs The other connector to move into this one.
	 * @return A reference to this object.
	 */
	connector_tmpl& operator=(connector_tmpl&& rhs) {
		base::operator=(std::move(rhs));
		return *this;
	}
	/**
	 * Gets the local address to which the socket is bound.
	 * @return The local address to which the socket is bound.
	 * @throw sys_error on error
	 */
	addr_t address() const { return addr_t(base::address()); }
	/**
	 * Gets the address of the remote peer, if this socket is connected.
	 * @return The address of the remote peer, if this socket is connected.
	 * @throw sys_error on error
	 */
	addr_t peer_address() const { return addr_t(base::peer_address()); }
	/**
	 * Binds the socket to the specified address.
	 * This call is optional for a client connector, though it is rarely
	 * used.
	 * @param addr The address to which we get bound.
	 * @return @em true on success, @em false on error
	 */
	bool bind(const addr_t& addr) { return base::bind(addr); }
	/**
	 * Attempts to connects to the specified server.
	 * If the socket is currently connected, this will close the current
	 * connection and open the new one.
	 * @param addr The remote server address.
	 * @return @em true on success, @em false on error
	 */
	bool connect(const addr_t& addr) { return base::connect(addr); }
	/**
     * Attempts to connect to the specified server, with a timeout.
     * If the socket is currently connected, this will close the current
     * connection and open the new one.
	 * If the operation times out, the @ref last_error will be set to
	 * ETIMEDOUT.
	 * @param addr The remote server address.
     * @param relTime The duration after which to give up. Zero means never.
	 * @return @em true on success, @em false on error
	 */
	template <class Rep, class Period>
	bool connect(const sock_address& addr, const std::chrono::duration<Rep, Period>& relTime) {
		return base::connect(addr, std::chrono::microseconds(relTime));
	}
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_connector_h

