// test_result.cpp
//
// Unit tests for the result class.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2023 Frank Pagliughi All rights reserved.
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

#include "sockpp/result.h"
#include "catch2_version.h"

using namespace sockpp;

TEST_CASE("test result success", "[result]") {
	const int VAL = 42;
	auto res = result<int>(VAL);

	REQUIRE(res);
	REQUIRE(res.is_ok());
	REQUIRE(!res.is_error());
	REQUIRE(res.value() == VAL);
	REQUIRE(res.error() == error_code{});
	REQUIRE(res.error().value() == 0);
	REQUIRE(res == success(VAL));
	REQUIRE(res == error_code{});
}

TEST_CASE("test result error", "[result]") {
	const auto ERR = errc::interrupted;
	auto res = result<int>::from_error(ERR);

	REQUIRE(!res);
	REQUIRE(!res.is_ok());
	REQUIRE(res.is_error());
	REQUIRE(res.error() == ERR);
	REQUIRE(res == ERR);
	REQUIRE(res == std::make_error_code(ERR));
	REQUIRE(res == error<int>(ERR));
}

