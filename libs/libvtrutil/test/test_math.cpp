#include "catch.hpp"

#include "vtr_math.h"

TEST_CASE("Nearest Integer", "[vtr_math]") {
    REQUIRE( vtr::nint(0.) == 0);
    REQUIRE( vtr::nint(0.1) == 0);
    REQUIRE( vtr::nint(0.5) == 1);
    REQUIRE( vtr::nint(0.9) == 1);

    REQUIRE( vtr::nint(1.) == 1);
    REQUIRE( vtr::nint(1.1) == 1);
    REQUIRE( vtr::nint(1.5) == 2);
    REQUIRE( vtr::nint(1.9) == 2);

    REQUIRE( vtr::nint(42.) == 42);
    REQUIRE( vtr::nint(42.1) == 42);
    REQUIRE( vtr::nint(42.5) == 43);
    REQUIRE( vtr::nint(42.9) == 43);
}

TEST_CASE("Safe Ratio", "[vtr_math]") {
    REQUIRE( vtr::safe_ratio(1., 1.) == Approx(1.));
    REQUIRE( vtr::safe_ratio(1., 2.) == Approx(0.5));
    REQUIRE( vtr::safe_ratio(50., 0.) == Approx(0.));
}
