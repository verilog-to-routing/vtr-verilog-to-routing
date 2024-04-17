#include "zlibutils.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

TEST_CASE("test_server_zlib_utils", "[vpr]")
{
    const std::string orig{"This string is going to be compressed now"};

    std::optional<std::string> compressedOpt = tryCompress(orig);
    REQUIRE(compressedOpt);
    REQUIRE(orig != compressedOpt.value());

    std::optional<std::string> decompressedOpt = tryDecompress(compressedOpt.value());
    REQUIRE(decompressedOpt);

    REQUIRE(orig == decompressedOpt.value());
}





