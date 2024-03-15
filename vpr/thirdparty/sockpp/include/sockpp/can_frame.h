/**
 * @file can_frame.h
 *
 * Class for the Linux SocketCAN frames.
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
// Copyright (c) 2021 Frank Pagliughi
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

#ifndef __sockpp_can_frame_h
#define __sockpp_can_frame_h

#include "sockpp/platform.h"
#include <string>
#include <cstring>
//#include <sys/un.h>
#include <linux/can.h>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Class that represents a Linux SocketCAN frame.
 * This inherits from the Linux CAN frame struct, just providing easier
   construction.
 */
class can_frame : public ::can_frame
{
	using base = ::can_frame;

	/** The size of the underlying address struct, in bytes */
	static constexpr size_t SZ = sizeof(::can_frame);

public:
	/**
	 * Constructs an empty frame.
	 * The frame is initialized to all zeroes.
	 */
	can_frame() : base{} {}
	/**
	 * Constructs a frame with the specified ID and data.
	 * @param canID The CAN identifier for the frame
	 * @param data The data field for the frame
	 */
	can_frame(canid_t canID, const std::string& data)
		: can_frame{ canID, data.data(), data.length() } {}
	/**
	 * Constructs a frame with the specified ID and data.
	 * @param canID The CAN identifier for the frame
	 * @param data The data field for the frame
	 * @param n The number of bytes in the data field
	 */
	can_frame(canid_t canID, const void* data, size_t n) : base{} {
		this->can_id = canID;
		this->can_dlc = n;
		::memcpy(&this->data, data, n);
	}
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_can_frame_h

