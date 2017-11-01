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
