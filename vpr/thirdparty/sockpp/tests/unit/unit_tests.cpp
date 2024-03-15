// unit_tests.cpp
//
// Main for unit tests.
//

// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2018 Frank Pagliughi
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

// Normally, we would just tell Catch2 to define main() like this...
// This tells Catch to provide a main() - only do this in one cpp file
//#define CATCH_CONFIG_MAIN

// ...but we need to run the sockpp global initialization before running 
// any of the tests. Defining a main() is described here:
// https://github.com/catchorg/Catch2/blob/master/docs/own-main.md 
//  

#include "sockpp/socket.h"

// This seems to be required, at least for MSVS 2015 on Win7, 
// using Catch2 v2.9.2
#if defined(_WIN32)
    #define CATCH_CONFIG_DISABLE_EXCEPTIONS
#endif

#define CATCH_CONFIG_RUNNER
#include "catch2_version.h"

int main(int argc, char* argv[]) 
{
    // global setup...
	sockpp::initialize();

    int result = Catch::Session().run(argc, argv);

    // global clean-up...

    return result;
}
