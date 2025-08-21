#include <cmath>
#include <limits>

#include "catch2/catch_test_macros.hpp"
#include "catch2/catch_approx.hpp"

#include "vtr_math.h"

using namespace Catch;

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

TEST_CASE("Median", "[vtr_math]") {
    // Check the median of a vector with a singular value.
    std::vector<double> single_vec = {1.0};
    REQUIRE(vtr::median(single_vec) == 1.0);

    // Check the median of a vector with two elements.
    std::vector<double> double_vec = {1.0, 2.0};
    REQUIRE(vtr::median(double_vec) == 1.5);

    // Check the median of a vector with an odd length.
    std::vector<double> odd_vec = {1.0, 2.0, 3.0, 4.0, 5.0};
    REQUIRE(vtr::median(odd_vec) == 3.0);

    // Check the median of a vector with a median length.
    std::vector<double> even_vec = {1.0, 2.0, 3.0, 4.0};
    REQUIRE(vtr::median(even_vec) == 2.5);

    // Check the median of a negative, odd-lengthed vector works.
    std::vector<double> negative_odd_vec = {-1.0, -2.0, -3.0, -4.0, -5.0};
    REQUIRE(vtr::median(negative_odd_vec) == -3.0);

    // Check the median of a negative, even-lengthed vector works.
    std::vector<double> negative_even_vec = {-1.0, -2.0, -3.0, -4.0};
    REQUIRE(vtr::median(negative_even_vec) == -2.5);

    // Check the median of an fp vector with an odd length.
    std::vector<float> fp_odd_vec = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    REQUIRE(vtr::median(fp_odd_vec) == 3.0);
    REQUIRE(vtr::median<float>(fp_odd_vec) == 3.0f);

    // Check the median of an fp vector with a median length.
    std::vector<float> fp_even_vec = {1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE(vtr::median(fp_even_vec) == 2.5);
    REQUIRE(vtr::median<float>(fp_even_vec) == 2.5f);

    // Check the median of an integral, odd-lengthed vecor.
    std::vector<int> int_vec_odd = {1, 2, 3, 4, 5};
    REQUIRE(vtr::median(int_vec_odd) == 3.0);

    // Check the median of an integral, even-lengthed vecor.
    std::vector<int> int_vec_even = {1, 2, 3, 4};
    REQUIRE(vtr::median(int_vec_even) == 2.5);

    // Check that trying to get the median of an empty vector returns a NaN.
    std::vector<double> empty_vec;
    REQUIRE(std::isnan(vtr::median(empty_vec)));
}
