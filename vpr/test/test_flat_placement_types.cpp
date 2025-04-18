/**
 * @file
 * @author  Alex Singer
 * @date    March 2025
 * @brief   Unit tests for flat placement types
 */

#include <catch2/catch_approx.hpp>
#include "catch2/catch_test_macros.hpp"
#include "flat_placement_types.h"

namespace {

TEST_CASE("test_t_flat_pl_loc", "[vpr_flat_pl_types]") {
    SECTION("Test addition operator") {
        t_flat_pl_loc loc1{1.0f, 2.0f, 3.0f};
        t_flat_pl_loc loc2{4.0f, 5.0f, 6.0f};

        loc1 += loc2;

        REQUIRE(loc1.x == Catch::Approx(5.0f));
        REQUIRE(loc1.y == Catch::Approx(7.0f));
        REQUIRE(loc1.layer == Catch::Approx(9.0f));
    }

    SECTION("Test division operator") {
        t_flat_pl_loc loc{10.0f, 20.0f, 30.0f};

        loc /= 2.0f;

        REQUIRE(loc.x == Catch::Approx(5.0f));
        REQUIRE(loc.y == Catch::Approx(10.0f));
        REQUIRE(loc.layer == Catch::Approx(15.0f));
    }

    SECTION("Test addition and division operators combined") {
        t_flat_pl_loc loc1{1.0f, 2.0f, 3.0f};
        t_flat_pl_loc loc2{4.0f, 5.0f, 6.0f};

        loc1 += loc2;
        loc1 /= 2.0f;

        REQUIRE(loc1.x == Catch::Approx(2.5f));
        REQUIRE(loc1.y == Catch::Approx(3.5f));
        REQUIRE(loc1.layer == Catch::Approx(4.5f));
    }
}

} // namespace
