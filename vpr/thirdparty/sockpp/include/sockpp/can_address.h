/**
 * @file can_address.h
 *
 * Class for the Linux SocketCAN socket address.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date March 2021
 */

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

#ifndef __sockpp_can_addr_h
#define __sockpp_can_addr_h

#include "sockpp/platform.h"
#include "sockpp/sock_address.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sys/un.h>
#include <linux/can.h>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a Linux SocketCAN address.
 * This inherits from the CAN form of a socket address, @em sockaddr_can.
 */
class can_address : public sock_address
{
	/** The underlying C struct for SocketCAN addresses  */
	sockaddr_can addr_;

	/** The size of the underlying address struct, in bytes */
	static constexpr size_t SZ = sizeof(sockaddr_can);

public:
    /** The address family for this type of address */
	static constexpr sa_family_t ADDRESS_FAMILY = AF_CAN;

	/** Iface to use to indicate binding to all interfaces */
	static const unsigned ALL_IFACE = 0;

	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	can_address() : addr_() {}
	/**
	 * Creates an address for binding to a specific CAN interface
	 * @param ifindex The interface index to use. This must, obviously, be
	 *  			  an index to a CAN interface.
	 */
	explicit can_address(unsigned ifindex);
	/**
	 * Constructs an address for the specified CAN interface.
	 * The interface might be "can0", "can1", "vcan0", etc.
	 * @param iface The name of the CAN interface
	 */
	can_address(const std::string& iface);
	/**
	 * Constructs the address by copying the specified structure.
     * @param addr The generic address
     * @throws std::invalid_argument if the address is not a SocketCAN
     *            address (i.e. family is not AF_CAN)
	 */
	explicit can_address(const sockaddr& addr);
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	can_address(const sock_address& addr) {
		std::memcpy(&addr_, addr.sockaddr_ptr(), SZ);
	}
	/**
	 * Constructs the address by copying the specified structure.
     * @param addr The other address
     * @throws std::invalid_argument if the address is not properly
     *            initialized as a SocketCAN address (i.e. family is not
     *            AF_CAN)
	 */
	can_address(const sockaddr_can& addr) : addr_(addr) {}
	/**
	 * Constructs the address by copying the specified address.
	 * @param addr The other address
	 */
	can_address(const can_address& addr) : addr_(addr.addr_) {}
	/**
	 * Checks if the address is set to some value.
	 * This doesn't attempt to determine if the address is valid, simply
	 * that it's not all zero.
	 * @return @em true if the address has been set, @em false otherwise.
	 */
	bool is_set() const { return family() != AF_UNSPEC; }
	/**
	 * Gets the name of the CAN interface to which this address refers.
	 * @return The name of the CAN interface to which this address refers.
	 */
	std::string iface() const;
	/**
	 * Gets the size of the address structure.
	 * Note: In this implementation, this should return sizeof(this) but
	 * more convenient in some places, and the implementation might change
	 * in the future, so it might be more compatible with future revisions
	 * to use this call.
	 * @return The size of the address structure.
	 */
	socklen_t size() const override { return socklen_t(SZ); }

    // TODO: Do we need a:
    //   create(iface)
    // to mimic the inet_address behavior?

    /**
	 * Gets a pointer to this object cast to a const @em sockaddr.
	 * @return A pointer to this object cast to a const @em sockaddr.
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
	 * Gets a const pointer to this object cast to a @em sockaddr_can.
	 * @return const sockaddr_can pointer to this object.
	 */
	const sockaddr_can* sockaddr_can_ptr() const { return &addr_; }
	/**
	 * Gets a pointer to this object cast to a @em sockaddr_can.
	 * @return sockaddr_can pointer to this object.
	 */
	sockaddr_can* sockaddr_can_ptr() { return &addr_; }
	/**
	 * Gets a printable string for the address.
	 * @return A string representation of the address in the form
	 *  	   "unix:<path>"
	 */
	std::string to_string() const {
		return std::string("can:") + iface();
	}
};

// --------------------------------------------------------------------------

/**
 * Stream inserter for the address.
 * @param os The output stream
 * @param addr The address
 * @return A reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const can_address& addr);

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_can_addr_h

