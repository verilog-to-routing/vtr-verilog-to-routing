// stream_socket.cpp
//
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

#include "sockpp/stream_socket.h"
#include "sockpp/exception.h"
#include <algorithm>
#include <memory>

using namespace std::chrono;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

// Creates a stream socket for the given domain/protocol.

stream_socket stream_socket::create(int domain, int protocol /*=0*/)
{
	stream_socket sock(::socket(domain, COMM_TYPE, protocol));
	if (!sock)
		sock.clear(get_last_error());
	return sock;
}

// --------------------------------------------------------------------------
// Reads from the socket. Note that we use ::recv() rather then ::read()
// because many non-*nix operating systems make a distinction.

ssize_t stream_socket::read(void *buf, size_t n)
{
	#if defined(_WIN32)
		return check_ret(::recv(handle(), reinterpret_cast<char*>(buf),
								int(n), 0));
	#else
		return check_ret(::recv(handle(), buf, n, 0));
	#endif
}

// --------------------------------------------------------------------------

ioresult stream_socket::read_r(void *buf, size_t n)
{
    #if defined(_WIN32)
        return ioresult(::recv(handle(), reinterpret_cast<char*>(buf),
                               int(n), 0));
    #else
        return ioresult(::recv(handle(), buf, n, 0));
    #endif
}

// --------------------------------------------------------------------------
// Attempts to read the requested number of bytes by repeatedly calling
// read() until it has the data or an error occurs.
//

ssize_t stream_socket::read_n(void *buf, size_t n)
{
	size_t	nr = 0;
	ssize_t	nx = 0;

	uint8_t *b = reinterpret_cast<uint8_t*>(buf);

	while (nr < n) {
		if ((nx = read(b+nr, n-nr)) < 0 && last_error() == EINTR)
			continue;

		if (nx <= 0)
			break;

		nr += nx;
	}

	return (nr == 0 && nx < 0) ? nx : ssize_t(nr);
}

// --------------------------------------------------------------------------

ioresult stream_socket::read_n_r(void *buf, size_t n)
{
    ioresult res;
	uint8_t *b = reinterpret_cast<uint8_t*>(buf);

	while (res.count() < n) {
        ioresult r = read_r(b + res.count(), n - res.count());
		if (r.is_err() && r.error() != EINTR) {
            res.set_error(r.error());
			break;
        }
		res.incr(r.count());
	}

	return res;
}

// --------------------------------------------------------------------------

ssize_t stream_socket::read(const std::vector<iovec>& ranges)
{
	if (ranges.empty())
		return 0;

	#if !defined(_WIN32)
		return check_ret(::readv(handle(), ranges.data(), int(ranges.size())));
	#else
		std::vector<WSABUF> bufs;
		for (const auto& iovec : ranges) {
			bufs.push_back({
				static_cast<ULONG>(iovec.iov_len),
				static_cast<CHAR*>(iovec.iov_base)
			});
		}

		DWORD flags = 0,
			  nread = 0,
			  nbuf = DWORD(bufs.size());

		auto ret = check_ret(::WSARecv(handle(), bufs.data(), nbuf, &nread,
									    &flags, nullptr, nullptr));
		return ssize_t(ret == SOCKET_ERROR ? ret : nread);
	#endif
}

// --------------------------------------------------------------------------

bool stream_socket::read_timeout(const microseconds& to)
{
	auto tv = 
		#if defined(_WIN32)
			DWORD(duration_cast<milliseconds>(to).count());
		#else
			to_timeval(to);
		#endif
	return set_option(SOL_SOCKET, SO_RCVTIMEO, tv);
}

// --------------------------------------------------------------------------

ssize_t stream_socket::write(const void *buf, size_t n)
{
	#if defined(_WIN32)
		return check_ret(::send(handle(), reinterpret_cast<const char*>(buf),
								int(n) , 0));
	#else
		return check_ret(::send(handle(), buf, n, 0));
	#endif
}

// --------------------------------------------------------------------------

ioresult stream_socket::write_r(const void *buf, size_t n)
{
    #if defined(_WIN32)
        return ioresult(::send(handle(), reinterpret_cast<const char*>(buf),
                               int(n) , 0));
    #else
        return ioresult(::send(handle(), buf, n, 0));
    #endif
}

// --------------------------------------------------------------------------
// Attempts to write the entire buffer by repeatedly calling write() until
// either all of the data is sent or an error occurs.

ssize_t stream_socket::write_n(const void *buf, size_t n)
{
	size_t	nw = 0;
	ssize_t	nx = 0;

	const uint8_t *b = reinterpret_cast<const uint8_t*>(buf);

	while (nw < n) {
		if ((nx = write(b+nw, n-nw)) < 0 && last_error() == EINTR)
			continue;

		if (nx <= 0)
			break;

		nw += nx;
	}

	return (nw == 0 && nx < 0) ? nx : ssize_t(nw);
}

// --------------------------------------------------------------------------

ioresult stream_socket::write_n_r(const void *buf, size_t n)
{
    ioresult res;
	const uint8_t *b = reinterpret_cast<const uint8_t*>(buf);

	while (res.count() < n) {
        ioresult r = write_r(b + res.count(), n - res.count());
		if (r.is_err() && r.error() != EINTR) {
			res.set_error(r.error());
			break;
		}
		res.incr(r.count());
	}

	return res;
}

// --------------------------------------------------------------------------

ssize_t stream_socket::write(const std::vector<iovec>& ranges)
{
	#if !defined(_WIN32)
		return check_ret(::writev(handle(), ranges.data(), int(ranges.size())));
	#else
		std::vector<WSABUF> bufs;
		for (const auto& iovec : ranges) {
			bufs.push_back({
				static_cast<ULONG>(iovec.iov_len),
				static_cast<CHAR*>(iovec.iov_base)
			});
		}

		DWORD nwritten = 0,
			  nmsg = DWORD(bufs.size());

		auto ret = check_ret(::WSASend(handle(), bufs.data(),
									   nmsg, &nwritten, 0, nullptr, nullptr));
		return ssize_t(ret == SOCKET_ERROR ? ret : nwritten);
	#endif
}

// --------------------------------------------------------------------------

bool stream_socket::write_timeout(const microseconds& to)
{
	auto tv = 
		#if defined(_WIN32)
			DWORD(duration_cast<milliseconds>(to).count());
		#else
			to_timeval(to);
		#endif

	return set_option(SOL_SOCKET, SO_SNDTIMEO, tv);
}

/////////////////////////////////////////////////////////////////////////////
// end namespace sockpp
}

