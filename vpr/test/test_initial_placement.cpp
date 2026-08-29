#include "catch2/catch_test_macros.hpp"

#include "initial_placement.h"

TEST_CASE("initial placement prefers scarce compatible locations on score ties", "[vpr_place]") {
    t_block_score flexible;
    flexible.num_compatible_locations = 100;

    t_block_score scarce;
    scarce.num_compatible_locations = 10;

    CHECK(initial_placement_block_score_less(flexible, scarce));
    CHECK_FALSE(initial_placement_block_score_less(scarce, flexible));

    SECTION("the existing difficulty score retains precedence") {
        flexible.macro_size = 2;
        CHECK_FALSE(initial_placement_block_score_less(flexible, scarce));
        CHECK(initial_placement_block_score_less(scarce, flexible));
    }

    SECTION("equivalent scores are equivalent in the heap ordering") {
        CHECK_FALSE(initial_placement_block_score_less(flexible, flexible));
        CHECK_FALSE(initial_placement_block_score_less(scarce, scarce));
    }
}
