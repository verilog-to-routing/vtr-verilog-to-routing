/**
 * @file can_socket.h
 *
 * Class (typedef) for Linux SocketCAN socket.
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

#ifndef __sockpp_can_socket_h
#define __sockpp_can_socket_h

#include "sockpp/datagram_socket.h"
#include "sockpp/can_address.h"
#include "sockpp/can_frame.h"

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

/**
 * Socket type for Linux SocketCAN.
 *
 * Note that technically these are RAW sockets, not DGRAM. We can/should
 * organize the underlying hierarch to properly indicate this, but for
 * practical usge, it doesn't matter too MUCH.
 * The BCM CAN sockets are DGRAM sockets, but those aren't implemented yet.
 * It wouldn't take too much to add them, though.
 */
class can_socket : public datagram_socket
{
	/** The base class */
	using base = datagram_socket;

	// Non-copyable
	can_socket(const can_socket&) =delete;
	can_socket& operator=(const can_socket&) =delete;

	/**
	 * We can't connect to a raw CAN socket;
	 * we can only bind the address/iface.
	 */
	bool connect(const sock_address&) =delete;

protected:
	static socket_t create_handle(int type, int protocol) {
		return socket_t(::socket(PROTOCOL_FAMILY, type, protocol));
	}

public:
	/**
	 *  The SocketCAN protocol family.
	 *  Note that AF_CAN == PF_CAN, which is used in many of the CAN
	 *  examples.
	 */
	static const int PROTOCOL_FAMILY = AF_CAN;

	/** The socket 'type' for communications semantics. */
	static constexpr int COMM_TYPE = SOCK_RAW;

	/**
	 * Creates an uninitialized CAN socket.
	 */
	can_socket() {}
	/**
     * Creates a CAN socket from an existing OS socket handle and
     * claims ownership of the handle.
	 * @param handle A socket handle from the operating system.
	 */
	explicit can_socket(socket_t handle) : base(handle) {}
	/**
	 * Creates a CAN socket and binds it to the address.
	 * @param addr The address to bind.
	 */
	explicit can_socket(const can_address& addr);
	/**
	 * Move constructor.
	 * @param other The other socket to move to this one
	 */
	can_socket(can_socket&& other) : base(std::move(other)) {}
	/**
	 * Move assignment.
	 * @param rhs The other socket to move into this one.
	 * @return A reference to this object.
	 */
	can_socket& operator=(can_socket&& rhs) {
		base::operator=(std::move(rhs));
		return *this;
	}

	/**
	 * Gets the system time of the last frame read from the socket.
	 * @return The system time of the last frame read from the socket with
	 *  	   microsecond precision.
	 */
	std::chrono::system_clock::time_point last_frame_time();
	/**
	 * Gets a floating point timestamp of the last frame read from the
	 * socket.
	 * This is the number of seconds since the Unix epoch (time_t), with
	 * floating-point, microsecond precision.
	 * @return A floating-point timestamp with microsecond precision.
	 */
	double last_frame_timestamp();

	// ----- I/O -----

	/**
	 * Sends a frame to the CAN interfacce at the specified address.
	 * @param frame The CAN frame to send.
	 * @param flags The flags. See send(2).
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	ssize_t send_to(const can_frame& frame, int flags, const can_address& addr) {
		return check_ret(
			::sendto(handle(), &frame, sizeof(can_frame), flags,
					 addr.sockaddr_ptr(), addr.size())
	    );
	}

	/**
	 * Sends a frame to the CAN interface at the specified address.
	 * @param frame The CAN frame to send.
	 * @param addr The remote destination of the data.
	 * @return the number of bytes sent on success or, @em -1 on failure.
	 */
	ssize_t send_to(const can_frame& frame, const can_address& addr) {
		return check_ret(
			::sendto(handle(), &frame, sizeof(can_frame), 0,
					 addr.sockaddr_ptr(), addr.size())
	    );
	}
	/**
	 * Sends a frame to the CAN bus.
	 * The socket should be bound before calling this.
	 * @param frame The CAN frame to send.
	 * @param flags The option bit flags. See send(2).
	 * @return @em zero on success, @em -1 on failure.
	 */
	ssize_t send(const can_frame& frame, int flags=0) {
		return check_ret(::send(handle(), &frame, sizeof(can_frame), flags));
	}
	/**
	 * Receives a message from the CAN interface at the specified address.
	 * @param frame CAN frame to get the incoming data.
	 * @param flags The option bit flags. See send(2).
	 * @param srcAddr Receives the address of the peer that sent the
	 *  			   message
	 * @return The number of bytes read or @em -1 on error.
	 */
	ssize_t recv_from(can_frame* frame, int flags, can_address* srcAddr=nullptr);
	/**
	 * Receives a message on the socket.
	 * @param frame CAN frame to get the incoming data.
	 * @param flags The option bit flags. See send(2).
	 * @return The number of bytes read or @em -1 on error.
	 */
	ssize_t recv(can_frame* frame, int flags=0) {
		return check_ret(::recv(handle(), frame, sizeof(can_frame), flags));
	}
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_can_socket_h

