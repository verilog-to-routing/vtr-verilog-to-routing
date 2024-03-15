/**
 * @file exception.h
 *
 * Exception classes for the sockpp library.
 *
 * @author	Frank Pagliughi
 * @author  SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date	October 2016
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

#ifndef __sockpp_exception_h
#define __sockpp_exception_h

#include <cerrno>
#include <stdexcept>
#include <string>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * System error.
 * These are errors that are resulted from system socket calls. The error
 * codes are platform 'errno' values (or similar), and the messages are
 * typically derived from the system.
 */
class sys_error : public std::runtime_error
{
	/** The system error number (errno) */
	int errno_;

public:
	/**
	 * Creates an error using the current system 'errno' value.
	 */
	sys_error() : sys_error(errno) {}
	/**
	 * Constructs an error with the specified system errno.
	 * @param err The error number. This is the system errno value.
	 */
	explicit sys_error(int err);
	/**
	 * Get the error number.
	 * @return The system error number.
	 */
	int error() const { return errno_; }
    /**
     * Gets a string describing the specified error.
     * This is typically the returned message from the system strerror().
     * @param err The system error number.
     * @return A string describing the specified error.
     */
    static std::string error_str(int err);
};

/**
 * Errors from getaddrinfo.
 * These are errors relating to DNS lookup, returned from the getaddrinfo system call.
 * Their codes are declared in <netdb.h>.
 */
class getaddrinfo_error : public std::runtime_error
{
	/** The error number, as returned by getaddrinfo. */
	int error_;
    /** The hostname being resolved. */
    std::string hostname_;

public:
	/**
	 * Constructs an error with the specified getaddrinfo error code.
	 * @param err The error number, as returned by getaddrinfo.
     * @param hostname The DNS name being resolved that triggered the error.
	 */
	getaddrinfo_error(int err, const std::string &hostname);
	/**
	 * Get the error number.
	 * @return The error number returned by getaddrinfo.
	 */
	int error() const { return error_; }
    /**
     * Get the hostname that triggered the error.
     * @return The hostname that getaddrinfo failed to resolve.
     */
    const std::string &hostname() const { return hostname_; }
};

/////////////////////////////////////////////////////////////////////////////
// end namespace 'sockpp'
}

#endif		// __sockpp_exception_h

