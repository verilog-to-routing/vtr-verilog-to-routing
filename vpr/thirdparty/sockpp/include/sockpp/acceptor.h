/// @file acceptor.h
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

#ifndef __sockpp_acceptor_h
#define __sockpp_acceptor_h

#include "sockpp/inet_address.h"
#include "sockpp/stream_socket.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class for creating a streaming server.
 * Objects of this class bind and listen on streaming ports for incoming
 * connections. Normally, a server thread creates one of these and blocks on
 * the call to accept incoming connections. The call to accept creates and
 * returns a @ref stream_socket which can then be used for the actual
 * communications.
 */
class acceptor : public socket
{
	/** The base class */
	using base = socket;

	// Non-copyable
	acceptor(const acceptor&) =delete;
	acceptor& operator=(const acceptor&) =delete;

protected:
	/** The default listener queue size. */
	static const int DFLT_QUE_SIZE = 4;

	/**
	 * Creates an underlying acceptor socket.
	 * The acceptor uses a stream socket type, but for our purposes is not
	 * classified (derived from) a streaming socket, since it doesn't
	 * support read and write to the socket.
	 * @param domain The communications domain (address family).
	 * @return An OS handle to a stream socket.
	 */
	static socket_t create_handle(int domain) {
		return stream_socket::create_handle(domain);
	}

public:
	/**
	 * Creates an unconnected acceptor.
	 */
	acceptor() {}
	/**
	 * Creates an acceptor from an existing OS socket
	 * handle and claims ownership of the handle.
	 * @param handle A socket handle from the operating system.
	 */
	explicit acceptor(socket_t handle) : base(handle) {}
	/**
	 * Creates an acceptor socket and starts it listening to the specified
	 * address.
	 * @param addr The address to which this server should be bound.
	 * @param queSize The listener queue size.
	 */
	acceptor(const sock_address& addr, int queSize=DFLT_QUE_SIZE) {
		open(addr, queSize);
	}
	/**
	 * Move constructor.
	 * Creates an acceptor by moving the other acceptor to this one.
	 * @param acc Another acceptor
	 */
	acceptor(acceptor&& acc) : base(std::move(acc)) {}
	/**
	 * Creates an unbound acceptor socket with an open OS socket handle.
	 * An application would need to manually bind and listen to this
	 * acceptor to get incoming connections.
	 * @param domain The communications domain (address family).
	 * @return An open, but unbound acceptor socket.
	 */
	static acceptor create(int domain);
	/**
	 * Move assignment.
	 * @param rhs The other socket to move into this one.
	 * @return A reference to this object.
	 */
	acceptor& operator=(acceptor&& rhs) {
		base::operator=(std::move(rhs));
		return *this;
	}
	/**
	 * Sets the socket listening on the address to which it is bound.
	 * @param queSize The listener queue size.
	 * @return @em true on success, @em false on error
	 */
	bool listen(int queSize=DFLT_QUE_SIZE) {
		return check_ret_bool(::listen(handle(), queSize));
	}
	/**
	 * Opens the acceptor socket, binds it to the specified address, and starts
	 * listening.
	 * @param addr The address to which this server should be bound.
	 * @param queSize The listener queue size.
	 * @param reuseSock Whether the SO_REUSEPORT (or SO_REUSEADDR on Win32)
	 *  				socket option should be used before binding and
	 *  				listening.
	 * @return @em true on success, @em false on error
	 */
	bool open(const sock_address& addr, int queSize=DFLT_QUE_SIZE, bool reuseSock=true);
	/**
	 * Accepts an incoming TCP connection and gets the address of the client.
	 * @param clientAddr Pointer to the variable that will get the
	 *  				 address of a client when it connects.
	 * @return A socket to the remote client.
	 */
	stream_socket accept(sock_address* clientAddr=nullptr);
};


/////////////////////////////////////////////////////////////////////////////

/**
 * Base template class for streaming servers of specific address families.
 * This is a base for creating socket acceptor classes for an individual
 * family. In most cases, all that is needed is a type definition specifying
 * which addresses type should be used to receive incoming connections,
 * like:
 *     using tcp_acceptor = acceptor_tmpl<tcp_socket>;
 */
template <typename STREAM_SOCK, typename ADDR=typename STREAM_SOCK::addr_t>
class acceptor_tmpl : public acceptor
{
	/** The base class */
	using base = acceptor;

	// Non-copyable
	acceptor_tmpl(const acceptor_tmpl&) =delete;
	acceptor_tmpl& operator=(const acceptor_tmpl&) =delete;

public:
	/** The type of streaming socket from the acceptor. */
	using stream_sock_t = STREAM_SOCK;
	/** The type of address for the acceptor and streams. */
	using addr_t = ADDR;

	/**
	 * Creates an unconnected acceptor.
	 */
	acceptor_tmpl() {}
	/**
	 * Creates a acceptor and starts it listening on the specified address.
	 * @param addr The TCP address on which to listen.
	 * @param queSize The listener queue size.
	 */
	acceptor_tmpl(const addr_t& addr, int queSize=DFLT_QUE_SIZE) {
		open(addr, queSize);
	}
	/**
	 * Creates a acceptor and starts it listening on the specified port.
	 * The acceptor binds to the specified port for any address on the local
	 * host.
	 * @param port The TCP port on which to listen.
	 * @param queSize The listener queue size.
	 */
	acceptor_tmpl(in_port_t port, int queSize=DFLT_QUE_SIZE) {
		open(port, queSize);
	}
	/**
	 * Move constructor.
	 * Creates an acceptor by moving the other acceptor to this one.
	 * @param acc Another acceptor
	 */
	acceptor_tmpl(acceptor_tmpl&& acc) : base(std::move(acc)) {}
	/**
	 * Creates an unbound acceptor socket with an open OS socket handle.
	 * An application would need to manually bind and listen to this
	 * acceptor to get incoming connections.
	 * @return An open, but unbound acceptor socket.
	 */
	static acceptor_tmpl create() {
		return base::create(addr_t::ADDRESS_FAMILY);
	}
	/**
	 * Move assignment.
	 * @param rhs The other socket to move into this one.
	 * @return A reference to this object.
	 */
	acceptor_tmpl& operator=(acceptor_tmpl&& rhs) {
		base::operator=(std::move(rhs));
		return *this;
	}
	/**
	 * Gets the local address to which we are bound.
	 * @return The local address to which we are bound.
	 */
	addr_t address() const { return addr_t(base::address()); }
	/**
	 * Binds the socket to the specified address.
	 * @param addr The address to which we get bound.
	 * @return @em true on success, @em false on error
	 */
	bool bind(const addr_t& addr) { return base::bind(addr); }
	/**
	 * Opens the acceptor socket, binds it to the specified address, and starts
	 * listening.
	 * @param addr The address to which this server should be bound.
	 * @param queSize The listener queue size.
	 * @return @em true on success, @em false on error
	 */
	bool open(const addr_t& addr, int queSize=DFLT_QUE_SIZE) {
		return base::open(addr, queSize);
	}
	/**
	 * Opens the acceptor socket, binds the socket to all adapters and starts it
	 * listening.
	 * @param port The TCP port on which to listen.
	 * @param queSize The listener queue size.
	 * @return @em true on success, @em false on error
	 */
	bool open(in_port_t port, int queSize=DFLT_QUE_SIZE) {
		return open(addr_t(port), queSize);
	}
	/**
	 * Accepts an incoming connection and gets the address of the client.
	 * @param clientAddr Pointer to the variable that will get the
	 *  				 address of a client when it connects.
	 * @return A tcp_socket to the remote client.
	 */
	stream_sock_t accept(addr_t* clientAddr=nullptr) {
		return stream_sock_t(base::accept(clientAddr));
	}
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_acceptor_h

