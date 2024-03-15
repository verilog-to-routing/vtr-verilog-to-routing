// exception.cpp
//
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
//

#include "sockpp/exception.h"
#include "sockpp/platform.h"
#include <cstring>

// Used to explicitly ignore the returned value of a function call.
#define ignore_result(x) if (x) {}

using namespace std;

namespace sockpp {

/////////////////////////////////////////////////////////////////////////////

sys_error::sys_error(int err) : runtime_error(error_str(err)), errno_(err)
{
}

// --------------------------------------------------------------------------
// Get a string description of the specified system error.

string sys_error::error_str(int err)
{
	char buf[1024];
	buf[0] = '\x0';

	#if defined(_WIN32)
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buf, sizeof(buf), NULL);
    #else
    	#ifdef _GNU_SOURCE
			#if !defined(__GLIBC__)
			// use the XSI standard behavior.
				int e = strerror_r(err, buf, sizeof(buf));
				auto s = strerror(e);
				return s ? std::string(s) : std::string();
			#else
			// assume GNU exception
				auto s = strerror_r(err, buf, sizeof(buf));
				return s ? std::string(s) : std::string();
			#endif
        #else
            ignore_result(strerror_r(err, buf, sizeof(buf)));
        #endif
    #endif
	return std::string(buf);
}

/////////////////////////////////////////////////////////////////////////////

getaddrinfo_error::getaddrinfo_error(int err, const string& hostname)
    : runtime_error(gai_strerror(err)), error_(err), hostname_(hostname)
{
}

/////////////////////////////////////////////////////////////////////////////
// end namespace 'sockpp'
}

