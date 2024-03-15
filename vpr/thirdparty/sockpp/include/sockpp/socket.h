/**
 * @file socket.h
 *
 * Classes for TCP & UDP socket.
 *
 * @author Frank Pagliughi
 * @author SoRo Systems, Inc.
 * @author www.sorosys.com
 *
 * @date December 2014
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

#ifndef __sockpp_socket_h
#define __sockpp_socket_h

#include "sockpp/sock_address.h"
#include "sockpp/result.h"
#include <chrono>
#include <string>
#include <tuple>

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

#if !defined(SOCKPP_SOCKET_T_DEFINED)
	typedef int socket_t;				///< The OS socket handle
	const socket_t INVALID_SOCKET = -1;	///< Invalid socket descriptor
	#define SOCKPP_SOCKET_T_DEFINED
#endif

/**
 * Converts a number of microseconds to a relative timeval.
 * @param dur A chrono duration of microseconds.
 * @return A timeval
 */
timeval to_timeval(const std::chrono::microseconds& dur);

/**
 * Converts a chrono duration to a relative timeval.
 * @param dur A chrono duration.
 * @return A timeval.
 */
template<class Rep, class Period>
timeval to_timeval(const std::chrono::duration<Rep,Period>& dur) {
	return to_timeval(std::chrono::duration_cast<std::chrono::microseconds>(dur));
}

/**
 * Converts a relative timeval to a chrono duration.
 * @param tv A timeval.
 * @return A chrono duration.
 */
inline std::chrono::microseconds to_duration(const timeval& tv)
{
    auto dur = std::chrono::seconds{tv.tv_sec}
				+ std::chrono::microseconds{tv.tv_usec};
    return std::chrono::duration_cast<std::chrono::microseconds>(dur);
}

/**
 * Converts an absolute timeval to a chrono time_point.
 * @param tv A timeval.
 * @return A chrono time_point.
 */
inline std::chrono::system_clock::time_point to_timepoint(const timeval& tv)
{
	return std::chrono::system_clock::time_point {
        std::chrono::duration_cast<std::chrono::system_clock::duration>(to_duration(tv))
	};
}

/////////////////////////////////////////////////////////////////////////////
//							socket_initializer
/////////////////////////////////////////////////////////////////////////////

/**
 * RAII singleton class to initialize and then shut down the library.
 *
 * The singleton object of this class should be created before any other
 * classes in the library are used.
 *
 * This is only required on some platforms, particularly Windows, but is
 * harmless on other platforms. On some, such as POSIX, the initializer sets
 * optional parameters for the library, and the destructor does nothing.
 */
class socket_initializer
{
	/** Initializes the sockpp library */
	socket_initializer();

	socket_initializer(const socket_initializer&) =delete;
	socket_initializer& operator=(const socket_initializer&) =delete;

	friend class socket;

public:
	/**
	 * Creates the initializer singleton on the first call as a static
	 * object which will get destructed on program termination with the
	 * other static objects in reverse order as they were created,
	 */
	static void initialize() {
		static socket_initializer sock_init;
	}
	/**
	 * Destructor shuts down the sockpp library.
	 */
	~socket_initializer();
};

/**
 * Initializes the sockpp library.
 *
 * This initializes the library by creating a static singleton
 * RAII object which does any platform-specific initialization the first
 * time it's call in a process, and then performs cleanup automatically
 * when the object is destroyed at program termination.
 *
 * This should be call at least once before any other sockpp objects are
 * created or used. It can be called repeatedly with subsequent calls
 * having no effect
 *
 * This is primarily required for Win32, to startup the WinSock DLL.
 *
 * On Unix-style platforms it disables SIGPIPE signals. Errors are handled
 * by functon return values and exceptions.
 */
void initialize();

/////////////////////////////////////////////////////////////////////////////
//									socket
/////////////////////////////////////////////////////////////////////////////

/**
 * Base class for socket objects.
 *
 * This class wraps an OS socket handle and maintains strict ownership
 * semantics. If a socket object has a valid, open socket, then it owns the
 * handle and will close it when the object is destroyed.
 *
 * Objects of this class are not copyable, but they are moveable.
 */
class socket
{
	/** The OS integer socket handle */
	socket_t handle_;
	/** Cache of the last error (errno) */
	mutable int lastErr_;
	/**
	 * The OS-specific function to close a socket handle/
	 * @param h The OS socket handle.
	 * @return @em true if the sock is closed, @em false on error.
	 */
	bool close(socket_t h);

	// Non-copyable.
	socket(const socket&) =delete;
	socket& operator=(const socket&) =delete;

    friend class ioresult;

protected:
	/**
	 * Closes the socket without checking for errors or updating the last
	 * error.
	 * This is used in internal open/connect type functions that fail after
	 * creating the socket, but want to preserve the previous failure
	 * condition.
	 * Assumes that the socket handle is valid.
	 * @return Always returns @em false.
	 */
	bool close_on_err() {
		close(release());
		return false;
	}
	/**
	 * OS-specific means to retrieve the last error from an operation.
	 * This should be called after a failed system call to set the
	 * lastErr_ member variable. Normally this would be called from
	 * @ref check_ret.
	 */
	static int get_last_error() {
		return ioresult::get_last_error();
	}
	/**
	 * Cache the last system error code into this object.
	 * This should be called after a failed system call to store the error
	 * value.
	 */
	void set_last_error() {
		lastErr_ = get_last_error();
	}
	/**
	 * Checks the value and if less than zero, sets last error.
     * @tparam T A signed integer type of any size
	 * @param ret The return value from a library or system call.
	 * @return Returns the value sent to it, `ret`.
	 */
    template <typename T>
	T check_ret(T ret) const{
		lastErr_ = (ret < 0) ? get_last_error() : 0;
		return ret;
	}
	/**
     * Checks the value and if less than zero, sets last error. 
     * @tparam T A signed integer type of any size
	 * @param ret The return value from a library or system call.
	 * @return @em true if the value is a typical system success value (>=0)
	 *  	   or @em false is is an error (<0)
	 */
    template <typename T>
	bool check_ret_bool(T ret) const{
		lastErr_ = (ret < 0) ? get_last_error() : 0;
		return ret >= 0;
	}
	/**
	 * Checks the value and if it is not a valid socket, sets last error.
	 * This is specifically required for Windows which uses an unsigned type
	 * for its SOCKET.
	 * @param ret The return value from a library or system call that returns
	 *  		  a socket, such as socket() or accept().
	 * @return Returns the value sent to it, `ret`.
	 */
	socket_t check_socket(socket_t ret) const {
		lastErr_ = (ret == INVALID_SOCKET) ? get_last_error() : 0;
		return ret;
	}
    /**
     * Checks the value and if it is INVALID_SOCKET, sets last error. 
	 * This is specifically required for Windows which uses an unsigned type
	 * for its SOCKET.
	 * @param ret The return value from a library or system call that returns
	 *  		  a socket such as socket() or accept().
	 * @return @em true if the value is a valid socket (not INVALID_SOCKET)
	 *  	   or @em false is is an error (INVALID_SOCKET)
	 */
	bool check_socket_bool(socket_t ret) const{
		lastErr_ = (ret == INVALID_SOCKET) ? get_last_error() : 0;
		return ret != INVALID_SOCKET;
	}

	#if !defined(_WIN32)
		/** Gets the flags on the socket handle. */
		int get_flags() const;
		/** Sets the flags on the socket handle. */
		bool set_flags(int flags);
		/** Sets a single flag on or off */
		bool set_flag(int flag, bool on=true);
	#endif

public:
	/**
	 * Creates an unconnected (invalid) socket
	 */
	socket() : handle_(INVALID_SOCKET), lastErr_(0) {}
	/**
	 * Creates a socket from an existing OS socket handle.
	 * The object takes ownership of the handle and will close it when
	 * destroyed.
	 * @param h An OS socket handle.
	 */
	explicit socket(socket_t h) : handle_(h), lastErr_(0) {}
	/**
	 * Move constructor.
	 * This takes ownership of the underlying handle in sock.
	 * @param sock An rvalue reference to a socket object.
	 */
	socket(socket&& sock) noexcept
			: handle_(sock.handle_), lastErr_(sock.lastErr_) {
		sock.handle_ = INVALID_SOCKET;
	}
	/**
	 * Destructor closes the socket.
	 */
	virtual ~socket() { close(); }
	/**
	 * Creates an OS handle to a socket.
	 *
	 * This is normally only required for internal or diagnostics code.
	 *
	 * @param domain The communications domain for the sockets (i.e. the
	 *  			 address family)
	 * @param type The communication semantics for the sockets (SOCK_STREAM,
	 *  		   SOCK_DGRAM, etc).
	 * @param protocol The particular protocol to be used with the sockets
	 *
	 * @return An OS socket handle with the requested communications
	 *  	   characteristics, or INVALID_SOCKET on failure.
	 */
	static socket_t create_handle(int domain, int type, int protocol=0) {
		return socket_t(::socket(domain, type, protocol));
	}
	/**
	 * Creates a socket with the specified communications characterics.
	 * Not that this is not normally how a socket is creates in the sockpp
	 * library. Applications would typically create a connector (client) or
	 * acceptor (server) socket which would take care of the details.
	 *
	 * This is included for completeness or for creating different types of
	 * sockets than are supported by the library.
	 *
	 * @param domain The communications domain for the sockets (i.e. the
	 *  			 address family)
	 * @param type The communication semantics for the sockets (SOCK_STREAM,
	 *  		   SOCK_DGRAM, etc).
	 * @param protocol The particular protocol to be used with the sockets
	 *
	 * @return A socket with the requested communications characteristics.
	 */
	static socket create(int domain, int type, int protocol=0);
	/**
	 * Determines if the socket is open (valid).
	 * @return @em true if the socket is open, @em false otherwise.
	 */
	bool is_open() const { return handle_ != INVALID_SOCKET; }
	/**
	 * Determines if the socket is closed or in an error state.
	 * @return @em true if the socket is closed or in an error state, @em
	 *  	   false otherwise.
	 */
	bool operator!() const {
		return handle_ == INVALID_SOCKET || lastErr_ != 0;
	}
	/**
	 * Determines if the socket is open and in an error-free state.
	 * @return @em true if the socket is open and in an error-free state,
	 *  	   @em false otherwise.
	 */
	explicit operator bool() const {
		return handle_ != INVALID_SOCKET && lastErr_ == 0;
	}
	/**
	 * Get the underlying OS socket handle.
	 * @return The underlying OS socket handle.
	 */
	socket_t handle() const { return handle_; }
	/**
	 * Gets the network family of the address to which the socket is bound.
	 * @return The network family of the address (AF_INET, etc) to which the
	 *  	   socket is bound. If the socket is not bound, or the address
	 *  	   is not known, returns AF_UNSPEC.
	 */
	virtual sa_family_t family() const {
		return address().family();
	}
	/**
	 * Creates a new socket that refers to this one.
	 * This creates a new object with an independent lifetime, but refers
	 * back to this same socket. On most systems, this duplicates the file
	 * handle using the dup() call.
	 * A typical use of this is to have separate threads for reading and
	 * writing the socket. One thread would get the original socket and the
	 * other would get the cloned one.
	 * @return A new socket object that refers to the same socket as this
	 *  	   one.
	 */
	socket clone() const;
	/**
	 * Creates a pair of connected sockets.
	 *
	 * Whether this will work at all is highly system and domain dependent.
	 * Currently it is only known to work for Unix-domain sockets on *nix
	 * systems.
	 *
	 * Note that applications would normally call this from a derived socket
	 * class which would return the properly type-cast sockets to match the
	 * `domain` and `type`.
	 *
	 * @param domain The communications domain for the sockets (i.e. the
	 *  			 address family)
	 * @param type The communication semantics for the sockets (SOCK_STREAM,
	 *  		   SOCK_DGRAM, etc).
	 * @param protocol The particular protocol to be used with the sockets
	 *
	 * @return A std::tuple of sockets. On error both sockets will be in an
	 *  	   error state with the
	 */
	static std::tuple<socket, socket> pair(int domain, int type, int protocol=0);
	/**
	 * Clears the error flag for the object.
	 * @param val The value to set the flag, normally zero.
	 */
	void clear(int val=0) { lastErr_ = val; }
	/**
	 * Releases ownership of the underlying socket object.
	 * @return The OS socket handle.
	 */
	socket_t release() {
		socket_t h = handle_;
		handle_ = INVALID_SOCKET;
		return h;
	}
	/**
	 * Replaces the underlying managed socket object.
	 * @param h The new socket handle to manage.
	 */
	void reset(socket_t h=INVALID_SOCKET);
	/**
	 * Move assignment.
	 * This assigns ownership of the socket from the other object to this
	 * one.
	 * @return A reference to this object.
	 */
	socket& operator=(socket&& sock) noexcept {
		// Give our handle to the other to close.
		std::swap(handle_, sock.handle_);
		lastErr_ = sock.lastErr_;
		return *this;
	}
	/**
	 * Binds the socket to the specified address.
	 * @param addr The address to which we get bound.
	 * @return @em true on success, @em false on error
	 */
	bool bind(const sock_address& addr);
	/**
	 * Gets the local address to which the socket is bound.
	 * @return The local address to which the socket is bound.
	 */
	sock_address_any address() const;
	/**
	 * Gets the address of the remote peer, if this socket is connected.
	 * @return The address of the remote peer, if this socket is connected.
	 */
	sock_address_any peer_address() const;
    /**
     * Gets the value of a socket option.
     *
     * This is a thin wrapper for the system `getsockopt`.
     *
     * @param level The protocol level at which the option resides, such as
     *              SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param optval The buffer for the value to retrieve
     * @param optlen Initially contains the lenth of the buffer, and on return
     *               contains the length of the value retrieved.
     *
     * @return bool @em true if the value was retrieved, @em false if an error
     *         occurred.
     */
    bool get_option(int level, int optname, void* optval, socklen_t* optlen) const;
    /**
     * Gets the value of a socket option.
     *
     * @param level The protocol level at which the option resides, such as
     *              SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param val The value to retrieve
     * @return bool @em true if the value was retrieved, @em false if an error
     *         occurred.
     */
	template <typename T>
    bool get_option(int level, int optname, T* val) const {
		socklen_t len = sizeof(T);
		return get_option(level, optname, (void*) val, &len);
	}
    /**
     * Sets the value of a socket option.
     *
     * This is a thin wrapper for the system `setsockopt`.
     *
     * @param level The protocol level at which the option resides, such as
     *              SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param optval The buffer with the value to set.
     * @param optlen Contains the lenth of the value buffer.
     *
     * @return bool @em true if the value was set, @em false if an error
     *         occurred.
     */
    bool set_option(int level, int optname, const void* optval, socklen_t optlen);
    /**
     * Sets the value of a socket option.
     *
     * @param level The protocol level at which the option resides, such as
     *              SOL_SOCKET.
     * @param optname The option passed directly to the protocol module.
     * @param val The value to set.
     *
     * @return bool @em true if the value was set, @em false if an error
     *         occurred.
     */
	template <typename T>
    bool set_option(int level, int optname, const T& val) {
		return set_option(level, optname, (void*) &val, sizeof(T));
	}
	/**
	 * Places the socket into or out of non-blocking mode.
	 * When in non-blocking mode, a call that is not immediately ready to
	 * complete (read, write, accept, etc) will return immediately with the
	 * error EWOULDBLOCK.
	 * @param on Whether to turn non-blocking mode on or off.
	 * @return @em true on success, @em false on failure.
	 */
	bool set_non_blocking(bool on=true);
	#if !defined(_WIN32)
		/**
		 * Determines if the socket is non-blocking
		 */
		bool is_non_blocking() const;
	#endif
    /**
     * Gets a string describing the specified error.
     * This is typically the returned message from the system strerror().
     * @param errNum The error number.
     * @return A string describing the specified error.
     */
    static std::string error_str(int errNum);
	/**
	 * Gets the code for the last errror.
	 * This is typically the code from the underlying OS operation.
	 * @return The code for the last errror.
	 */
	int last_error() const { return lastErr_; }
	/**
	 * Gets a string describing the last errror.
	 * This is typically the returned message from the system strerror().
	 * @return A string describing the last errror.
	 */
	std::string last_error_str() const {
        return error_str(lastErr_);
    }
	/**
	 * Shuts down all or part of the full-duplex connection.
	 * @param how Which part of the connection should be shut:
	 *  	@li SHUT_RD   (0) Further reads disallowed.
	 *  	@li SHUT_WR   (1) Further writes disallowed
	 *  	@li SHUT_RDWR (2) Further reads and writes disallowed.
	 * @return @em true on success, @em false on error.
	 */
	bool shutdown(int how=SHUT_RDWR);
	/**
	 * Closes the socket.
	 * After closing the socket, the handle is @em invalid, and can not be
	 * used again until reassigned.
	 * @return @em true if the sock is closed, @em false on error.
	 */
	virtual bool close();
};

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

#endif		// __sockpp_socket_h

