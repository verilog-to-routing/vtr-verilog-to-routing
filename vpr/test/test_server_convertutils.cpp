#include "convertutils.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

TEST_CASE("test_server_convert_utils_to_int", "[vpr]")
{
    REQUIRE(std::optional<int>{-2} == try_convert_to_int("-2"));
    REQUIRE(std::optional<int>{0} == try_convert_to_int("0"));
    REQUIRE(std::optional<int>{2} == try_convert_to_int("2"));
    REQUIRE(std::nullopt == try_convert_to_int("2."));
    REQUIRE(std::nullopt == try_convert_to_int("2.0"));
    REQUIRE(std::nullopt == try_convert_to_int("two"));
    REQUIRE(std::nullopt == try_convert_to_int("2k"));
    REQUIRE(std::nullopt == try_convert_to_int("k2"));
}


