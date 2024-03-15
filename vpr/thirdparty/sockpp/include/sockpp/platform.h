/**
 * @file platform.h
 *
 * Platform-specific definitions for the sockpp library.
 *
 * @author	Frank Pagliughi
 * @author	SoRo Systems, Inc.
 * @author  www.sorosys.com
 *
 * @date	December 2014
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

#ifndef __sockpp_platform_h
#define __sockpp_platform_h

#include <cstdint>

#if defined(_WIN32)
	//#pragma warning(4 : 4996)	// Deprecated functions (CRT & all)
	//#pragma warning(4 : 4250)	// Inheritance via dominance

	#if !defined(WIN32_LEAN_AND_MEAN)
		#define WIN32_LEAN_AND_MEAN
	#endif

	#if !defined(_CRT_SECURE_NO_DEPRECATE)
		#define _CRT_SECURE_NO_DEPRECATE
	#endif

	#include <winsock2.h>
	#include <ws2tcpip.h>

	#define SOCKPP_SOCKET_T_DEFINED
	using socket_t = SOCKET;

	using socklen_t = int;
	using in_port_t = uint16_t;
	using in_addr_t = uint32_t;

	using sa_family_t = u_short;

	#ifndef _SSIZE_T_DEFINED 
		#define _SSIZE_T_DEFINED 
		#undef ssize_t
		using ssize_t = SSIZE_T;
	#endif // _SSIZE_T_DEFINED

	#ifndef _SUSECONDS_T
		#define _SUSECONDS_T
		typedef long suseconds_t;	// signed # of microseconds in timeval
	#endif	// _SUSECONDS_T
 
	#define SHUT_RD SD_RECEIVE
	#define SHUT_WR SD_SEND
	#define SHUT_RDWR SD_BOTH

	struct iovec
	{
		void* iov_base;
		size_t iov_len;
	};

#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <sys/uio.h>
	#include <arpa/inet.h>
	#ifdef __FreeBSD__
		#include <netinet/in.h>
	#endif
	#include <netdb.h>
	#include <signal.h>
	#include <cerrno>
#endif

#endif

