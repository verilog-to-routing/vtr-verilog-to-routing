/**
 * @file inet6_address.h
 *
 * Class for a TCP/IP IPv6 socket address.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date March 2019
 */

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2019 Frank Pagliughi All rights reserved.
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

#ifndef __sockpp_inet6_addr_h
#define __sockpp_inet6_addr_h

#include "sockpp/platform.h"
#include "sockpp/sock_address.h"
#include <iostream>
#include <string>
#include <cstring>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////


/**
 * Class that represents an internet (IPv6) address. This inherits from the
 * IP-specific form of a socket address, @em sockaddr_in.
 */
class inet6_address : public sock_address
{
	/** The underlying C IPv6 struct  */
	sockaddr_in6 addr_;

	/** The size of the underlying address struct, in bytes */
	static constexpr size_t SZ = sizeof(sockaddr_in6);

public:
    /** The address family for this type of address */
	static constexpr sa_family_t ADDRESS_FAMILY = AF_INET6;

	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	inet6_address() : addr_() {}
	/**
	 * Constructs an address for any iface using the specified port.
	 * This is a convenient way for a server to specify an address that will
	 * bind to all interfaces.
	 * @param port The port number in native/host byte order.
	 */
	explicit inet6_address(in_port_t port) {
		const in6_addr ANY IN6ADDR_ANY_INIT;
		create(ANY, port);
	}
	/**
	 * Constructs an address using the name of the host and the specified
	 * port. This attempts to resolve the host name to an address.
	 *
	 * @param saddr The name of the host.
	 * @param port The port number in native/host byte order.
	 */
	inet6_address(const std::string& saddr, in_port_t port) {
		create(saddr, port);
	}
	/**
     * Constructs the address by copying the specified structure.
     * TODO: Do we actually need a conversion from something that's
     * cast to a sockaddr, but is really a sockaddr_in6?
	 * @param addr The other address
	 */
	inet6_address(const sockaddr_storage& addr) {
		std::memcpy(&addr_, &addr, SZ);
	}
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	inet6_address(const sock_address& addr) {
		std::memcpy(&addr_, addr.sockaddr_ptr(), SZ);
	}
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	inet6_address(const sockaddr_in6& addr) {
		std::memcpy(&addr_, &addr, SZ);
	}
	/**
	 * Constructs the address by copying the specified address.
	 * @param addr The other address
	 */
	inet6_address(const inet6_address& addr) : addr_(addr.addr_) {}
    /**
     * Creates an address on the loopback (localhost) interface.
     * @param port The port number (in native/host byte order).
     * @return The full address on the loopback interface.
     */
    static inet6_address loopback(in_port_t port) {
		const in6_addr LOOPBACK IN6ADDR_LOOPBACK_INIT;
        inet6_address addr;
        addr.create(LOOPBACK, port);
        return addr;
	}
	/**
	 * Checks if the address is set to some value.
	 * This doesn't attempt to determine if the address is valid, simply
	 * that it's not all zero.
	 * @return bool
	 */
	bool is_set() const;
	/**
	 * Attempts to resolve the host name into a 32-bit internet address.
	 * @param saddr The string host name.
	 * @return The internet address in network byte order.
	 */
	static in6_addr resolve_name(const std::string& saddr);
	/**
	 * Creates the socket address using the specified host address and port
	 * number.
	 * @param addr The host address (16-byte IPv6 address).
	 * @param port The host port number.
	 */
    void create(const in6_addr& addr, in_port_t port);
	/**
	 * Creates the socket address using the specified host name and port
	 * number.
	 * @param saddr The string host name.
	 * @param port The port number in native/host byte order.
	 */
	void create(const std::string& saddr, in_port_t port);
	/**
     * Gets 128-bit IPv6 address.
     * The address is usually stored in network byte order.
	 * @return The IPv6 address.
	 */
	in6_addr address() const { return addr_.sin6_addr; }
	/**
     * Gets a byte of the 128-bit IPv6 Address.
     * Note that the address is normally stored in network byte
     * order.
	 * @param i The byte to read (0-7)
{
	return addr_ != sockaddr_in6{};
	 * @return The specified byte in the 128-bit IPv6 Address
	 */
	uint8_t operator[](int i) const {
		return addr_.sin6_addr.s6_addr[i];
	}
	/**
	 * Gets the port number.
	 * @return The port number in native/host byte order.
	 */
	in_port_t port() const { return ntohs(addr_.sin6_port); }
	/**
	 * Gets the size of this structure.
	 * This is equivalent to sizeof(this) but more convenient in some
	 * places.
	 * @return The size of this structure.
	 */
	socklen_t size() const override { return socklen_t(SZ); }
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	const sockaddr* sockaddr_ptr() const override {
		return reinterpret_cast<const sockaddr*>(&addr_);
	}
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	sockaddr* sockaddr_ptr() override {
		return reinterpret_cast<sockaddr*>(&addr_);
	}
	/**
     * Gets a const pointer to this object cast to a @em
     * sockaddr_in6.
	 * @return const sockaddr_in6 pointer to this object.
	 */
	const sockaddr_in6* sockaddr_in6_ptr() const { return &addr_; }
	/**
	 * Gets a pointer to this object cast to a @em sockaddr_in6.
	 * @return sockaddr_in6 pointer to this object.
	 */
	sockaddr_in6* sockaddr_in6_ptr() { return &addr_; }
	/**
	 * Gets a printable string for the address.
     * This gets the address in the printable form "[addr]:port"
     * using inet_ntop(). It does not attempt to find the host name
     * using a lookup.
	 * @return A string representation of the address in the form 
	 *  	   '[address]:port'
	 */
	std::string to_string() const;
};

// --------------------------------------------------------------------------

/**
 * Stream inserter for the address. 
 * This uses the simple dot notation of the address as returned from 
 * inet_ntop(). It does not attempt a host lookup.
 * @param os The output stream
 * @param addr The address
 * @return A reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const inet6_address& addr);

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_inet6_addr_h

