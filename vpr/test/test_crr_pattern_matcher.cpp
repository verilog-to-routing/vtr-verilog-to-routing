#include "catch2/catch_test_macros.hpp"

#include "crr_pattern_matcher.h"
#include "vpr_error.h"

using crrgenerator::CRRPatternMatcher;

TEST_CASE("CRR pattern matcher accepts blanks inside brackets", "[vpr][crr][pattern_matcher]") {
    CRRPatternMatcher matcher;
    matcher.register_pattern("SB_[4, 14]__1_");
    matcher.register_pattern("SB_[ 7 ]__[2 : 8 : 3]_");
    matcher.register_pattern("SB_[4,\t14]__*_");

    REQUIRE(matcher.matches(0, 4, 1));
    REQUIRE(matcher.matches(0, 14, 1));
    REQUIRE_FALSE(matcher.matches(0, 5, 1));
    REQUIRE_FALSE(matcher.matches(0, 4, 2));

    REQUIRE(matcher.matches(1, 7, 5));
    REQUIRE_FALSE(matcher.matches(1, 7, 6));
    REQUIRE_FALSE(matcher.matches(1, 8, 5));

    REQUIRE(matcher.matches(2, 14, 9));
    REQUIRE_FALSE(matcher.matches(2, 9, 9));
}

TEST_CASE("CRR pattern matcher rejects malformed brackets", "[vpr][crr][pattern_matcher]") {
    CRRPatternMatcher matcher;
    REQUIRE_THROWS_AS(matcher.register_pattern("SB_[4,,14]__1_"), VprError);
    REQUIRE_THROWS_AS(matcher.register_pattern("SB_[4, x]__1_"), VprError);
    REQUIRE_THROWS_AS(matcher.register_pattern("SB_[ ]__1_"), VprError);
}
