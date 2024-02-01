#include "convertutils.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

TEST_CASE("test_server_convert_utils_to_int", "[vpr]") {
    REQUIRE(std::optional<int>{-2} == tryConvertToInt("-2"));
    REQUIRE(std::optional<int>{0} == tryConvertToInt("0"));
    REQUIRE(std::optional<int>{2} == tryConvertToInt("2"));
    REQUIRE(std::nullopt == tryConvertToInt("2."));
    REQUIRE(std::nullopt == tryConvertToInt("2.0"));
    REQUIRE(std::nullopt == tryConvertToInt("two"));
    REQUIRE(std::nullopt == tryConvertToInt("2k"));
    REQUIRE(std::nullopt == tryConvertToInt("k2"));
}
