#include <limits>

#include "catch.hpp"

#include "vtr_math.h"

TEST_CASE("Nearest Integer", "[vtr_math]") {
    REQUIRE(vtr::nint(0.) == 0);
    REQUIRE(vtr::nint(0.1) == 0);
    REQUIRE(vtr::nint(0.5) == 1);
    REQUIRE(vtr::nint(0.9) == 1);

    REQUIRE(vtr::nint(1.) == 1);
    REQUIRE(vtr::nint(1.1) == 1);
    REQUIRE(vtr::nint(1.5) == 2);
    REQUIRE(vtr::nint(1.9) == 2);

    REQUIRE(vtr::nint(42.) == 42);
    REQUIRE(vtr::nint(42.1) == 42);
    REQUIRE(vtr::nint(42.5) == 43);
    REQUIRE(vtr::nint(42.9) == 43);
}

TEST_CASE("Safe Ratio", "[vtr_math]") {
    REQUIRE(vtr::safe_ratio(1., 1.) == Approx(1.));
    REQUIRE(vtr::safe_ratio(1., 2.) == Approx(0.5));
    REQUIRE(vtr::safe_ratio(50., 0.) == Approx(0.));
}

TEST_CASE("Is Close", "[vtr_math]") {
    //double NAN = std::numeric_limits<double>::quiet_NaN();
    double INF = std::numeric_limits<double>::infinity();

    double num = 32.4;

    double num_close = num - vtr::DEFAULT_REL_TOL * num / 2;
    double num_not_quite_close = num - 2 * vtr::DEFAULT_REL_TOL * num;
    double num_far = 2 * num;

    REQUIRE(vtr::isclose(-1., -1.));
    REQUIRE(vtr::isclose(1., 1.));
    REQUIRE(vtr::isclose(0., 0.));
    REQUIRE(vtr::isclose(num, num));
    REQUIRE(vtr::isclose(num, num_close));
    REQUIRE(!vtr::isclose(num, num_not_quite_close));
    REQUIRE(!vtr::isclose(num, num_far));

    REQUIRE(vtr::isclose(INF, INF));
    REQUIRE(!vtr::isclose(-INF, INF));
    REQUIRE(!vtr::isclose(NAN, NAN));

    //Absolute tolerance tests
    REQUIRE(vtr::isclose(32.2, 32.4, 1e-9, 0.2));
    REQUIRE(!vtr::isclose(32.2, 32.4, 1e-9, 0.1));
}
