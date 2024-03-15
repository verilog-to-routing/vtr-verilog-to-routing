/**
 * @file unix_address.h
 *
 * Class for a UNIX-domain socket address.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date February 2014
 */

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

#ifndef __sockpp_unix_addr_h
#define __sockpp_unix_addr_h

#include "sockpp/platform.h"
#include "sockpp/sock_address.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sys/un.h>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a UNIX domain address.
 * This inherits from the UNIX form of a socket address, @em sockaddr_un.
 */
class unix_address : public sock_address
{
	/** The underlying C struct for unix-domain addresses  */
	sockaddr_un addr_;

	/** The size of the underlying address struct, in bytes */
	static constexpr size_t SZ = sizeof(sockaddr_un);

public:
    /** The address family for this type of address */
	static constexpr sa_family_t ADDRESS_FAMILY = AF_UNIX;

	/** The max length of the file path */
	static constexpr size_t MAX_PATH_NAME = sizeof(sockaddr_un::sun_path);

	/**
	 * Constructs an empty address.
	 * The address is initialized to all zeroes.
	 */
	unix_address() : addr_() {}
	/**
	 * Constructs an address given the specified path.
	 * @param path The path to the socket file.
	 */
	unix_address(const std::string& path);
	/**
	 * Constructs the address by copying the specified structure.
     * @param addr The generic address
     * @throws std::invalid_argument if the address is not a UNIX-domain
     *            address (i.e. family is not AF_UNIX)
	 */
	explicit unix_address(const sockaddr& addr);
	/**
	 * Constructs the address by copying the specified structure.
	 * @param addr The other address
	 */
	unix_address(const sock_address& addr) {
		std::memcpy(&addr_, addr.sockaddr_ptr(), SZ);
	}
	/**
	 * Constructs the address by copying the specified structure.
     * @param addr The other address
     * @throws std::invalid_argument if the address is not properly
     *            initialized as a UNIX-domain address (i.e. family is not
     *            AF_UNIX)
	 */
	unix_address(const sockaddr_un& addr);
	/**
	 * Constructs the address by copying the specified address.
	 * @param addr The other address
	 */
	unix_address(const unix_address& addr) : addr_(addr.addr_) {}
	/**
	 * Checks if the address is set to some value.
	 * This doesn't attempt to determine if the address is valid, simply
	 * that it's not all zero.
	 * @return @em true if the address has been set, @em false otherwise.
	 */
	bool is_set() const { return addr_.sun_path[0] != '\0'; }
	/**
	 * Gets the path to which this address refers.
	 * @return The path to which this address refers.
	 */
	std::string path() const {
		return std::string(addr_.sun_path, strnlen(addr_.sun_path, MAX_PATH_NAME));
	}
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
    //   create(path)
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
	 * Gets a const pointer to this object cast to a @em sockaddr_un.
	 * @return const sockaddr_un pointer to this object.
	 */
	const sockaddr_un* sockaddr_un_ptr() const { return &addr_; }
	/**
	 * Gets a pointer to this object cast to a @em sockaddr_un.
	 * @return sockaddr_un pointer to this object.
	 */
	sockaddr_un* sockaddr_un_ptr() { return &addr_; }
	/**
	 * Gets a printable string for the address.
	 * @return A string representation of the address in the form
	 *  	   "unix:<path>"
	 */
	std::string to_string() const {
		return std::string("unix:") + path();
	}
};

// --------------------------------------------------------------------------

/**
 * Stream inserter for the address.
 * @param os The output stream
 * @param addr The address
 * @return A reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const unix_address& addr);

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_unix_addr_h

