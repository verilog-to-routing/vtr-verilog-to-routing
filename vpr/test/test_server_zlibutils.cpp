#include "zlibutils.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

TEST_CASE("test_server_zlib_utils", "[vpr]")
{
    const std::string orig{"This string is going to be compressed now"};

    std::string compressed = tryCompress(orig);
    std::string decompressed = tryDecompress(compressed);

    REQUIRE(orig != compressed);
    REQUIRE(orig == decompressed);
}





