/// @file result.h
///
/// Type(s) for return values that can indicate a success or failure.
///
/// @date	January 2023

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023 Frank Pagliughi
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

#ifndef __sockpp_result_h
#define __sockpp_result_h

#include "sockpp/platform.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Result of a thread-safe I/O operation.
 *
 * Most I/O operations in the OS will return >=0 on sccess and -1 on error.
 * In the case of an error, the calling thread must read an `errno` variable
 * immediately, before any other system calls, to get the cause of an error
 * as an integer value defined by the constants ENOENT, EINTR, EBUSY, etc.
 *
 * Most sockpp calls retain this known behavior, but will cache this errno
 * value to be read later by a call to @ref socket::last_error() or similar.
 * But this makes the @ref socket classes themselves non-thread-safe since
 * the cached errno variable can't be shared across threads.
 *
 * Several new "thread-safe" variants of I/O functions simply retrieve the
 * errno immediately on a failed I/O call and instead of caching the value,
 * return it immediately in this ioresult.
 * See: @ref stream_socket::read_r, @ref stream_socket::read_n_r, @ref
 * stream_socket::write_r, etc.
 */
class ioresult {
	/** Byte count, or 0 on error or EOF */
	size_t cnt_ = 0;
	/** errno value (0 if no error or EOF) */
	int err_ = 0;

	/**
	 * OS-specific means to retrieve the last error from an operation.
	 * This should be called after a failed system call to get the caue of
	 * the error.
	 */
	static int get_last_error();

	friend class socket;

public:
	/** Creates an empty result */
	ioresult() = default;
	/**
	 * Creates a result from the return value of a low-level I/O function.
	 * @param n The number of bytes read or written. If <0, then an error is
	 *  		assumed and obtained from socket::get_last_error().
	 *
	 */
	explicit inline ioresult(ssize_t n) {
		if (n < 0)
			err_ = get_last_error();
		else
			cnt_ = size_t(n);
	}

	/** Sets the error value */
	void set_error(int e) { err_ = e; }

	/** Increments the count */
	void incr(size_t n) { cnt_ += n; }

	/** Determines if the result is OK (not an error) */
	bool is_ok() const { return err_ == 0; }
	/** Determines if the result is an error */
	bool is_err() const { return err_ != 0; }

	/** Determines if the result is OK */
	explicit operator bool() const { return is_ok(); }

	/** Gets the count */
	size_t count() const { return cnt_; }

	/** Gets the error */
	int error() const { return err_; }
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_result_h

