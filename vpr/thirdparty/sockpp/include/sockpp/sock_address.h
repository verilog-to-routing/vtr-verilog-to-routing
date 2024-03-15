/**
 * @file sock_address.h
 *
 * Generic address class for sockpp.
 *
 * @author	Frank Pagliughi
 * @author	SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date	June 2017
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

#ifndef __sockpp_sock_address_h
#define __sockpp_sock_address_h

#include "sockpp/platform.h"
#include <cstring>
#include <stdexcept>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Generic socket address.
 * Abstract base class for socket addresses. The underlying C socket
 * functions typically take or return an address as a `sockaddr` pointer and
 * length. So derived classes that wrap the
 */
class sock_address
{
public:
	/**
	 * Virtual destructor.
	 */
	virtual ~sock_address() {}
	/**
	 * Gets the size of this structure.
	 * This is equivalent to sizeof(this) but more convenient in some
	 * places.
	 * @return The size of this structure.
	 */
	virtual socklen_t size() const =0;
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	virtual sockaddr* sockaddr_ptr() =0;
	/**
	 * Gets a pointer to this object cast to a @em sockaddr.
	 * @return A pointer to this object cast to a @em sockaddr.
	 */
	virtual const sockaddr* sockaddr_ptr() const =0;
	/**
	 * Gets the network family of the address.
	 * @return The network family of the address (AF_INET, etc). If the
	 *  	   address is not known, returns AF_UNSPEC.
	 */
	virtual sa_family_t family() const {
		auto p = sockaddr_ptr();
		return p ? p->sa_family : AF_UNSPEC;
	}
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Generic socket address.
 *
 * This is a wrapper around `sockaddr_storage` which can hold any family
 * address. This should have enough memory to contain any address struct for
 * the system on which it is compiled.
 */
class sock_address_any : public sock_address
{
    /** Storage for any kind of socket address */
    sockaddr_storage addr_;
	/** Length of the address (in bytes) */
	socklen_t sz_;

	/** The maximum size of an address, in bytes */
	static constexpr size_t MAX_SZ = sizeof(sockaddr_storage);

public:
	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	sock_address_any() : addr_{}, sz_{MAX_SZ} {}
	/**
	 * Constructs an address.
	 * @param addr Pointer to a buffer holding the address.
	 * @param n The number of valid bytes in the address
	 * @throws std::length_error if `n` is greater than the maximum size of
	 *  		  an address.
	 */
	sock_address_any(const sockaddr* addr, socklen_t n) {
		if (n > MAX_SZ)
			throw std::length_error("Address length out of range");
        std::memcpy(&addr_, addr, sz_ = n);
    }
	/**
	 * Constructs an address.
	 * @param addr The buffer holding the address.
	 * @param n The number of valid bytes in the address
	 * @throws std::length_error if `n` is greater than the maximum size of
	 *  		  an address.
	 */
	sock_address_any(const sockaddr_storage& addr, socklen_t n) {
		if (n > MAX_SZ)
			throw std::length_error("Address length out of range");
        std::memcpy(&addr_, &addr, sz_ = n);
    }
	/**
	 * Copies another address to this one.
	 * @param addr The other address to copy into this one.
	 */
	sock_address_any(const sock_address& addr)
		: sock_address_any(addr.sockaddr_ptr(), addr.size()) {}
	/**
	 * Gets the size of the address.
	 * @return The size of the address. This is the number of bytes that are a
	 *  	   valid part of the address.
	 */
	socklen_t size() const override { return sz_; }
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
};

/**
 * Determines if the two objects refer to the same address.
 * @param lhs A socket address
 * @param rhs A socket address
 * @return @em true if `lhs` and `rhs` refer to the same address, @em false
 *  	   otherwise.
 */
inline bool operator==(const sock_address& lhs, const sock_address& rhs) {
    return lhs.size() == rhs.size() &&
        std::memcmp(lhs.sockaddr_ptr(), rhs.sockaddr_ptr(), lhs.size()) == 0;
}

/**
 * Determines if the two objects refer to the different address.
 * @param lhs A socket address
 * @param rhs A socket address
 * @return @em true if `lhs` and `rhs` refer to different address, @em false
 *  	   if they refer to the same address.
 */
inline bool operator!=(const sock_address& lhs, const sock_address& rhs) {
	return !operator==(lhs, rhs);
}

/////////////////////////////////////////////////////////////////////////////
// end namespace 'sockpp'
}

#endif		// __sockpp_sock_address_h
