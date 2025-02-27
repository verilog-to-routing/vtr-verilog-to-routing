/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Test cases for the Prefix Sum class in vtr_util.
 */

#include "catch2/catch_test_macros.hpp"

#include "vtr_ndmatrix.h"
#include "vtr_prefix_sum.h"

using namespace Catch;

TEST_CASE("PrefixSum1D", "[vtr_prefix_sum/PrefixSum1D]") {
    // Construct a 1D array to compute the prefix sum over.
    std::vector<float> vals = {1.f, 7.f, 2.f, 2.f, 5.f, 6.f, 1.f, 9.f, 1.f, 3.f};

    // Construct the Prefix Sum.
    vtr::PrefixSum1D<float> prefix_sum(vals);

    // Check that the sum of each length 1 region is the original value.
    SECTION("construction") {
        for (size_t x = 0; x < vals.size(); x++) {
            float sum_val = prefix_sum.get_sum(x, x);
            REQUIRE(sum_val == vals[x]);
        }
    }

    float sum_of_all_vals = 0.f;
    for (size_t x = 0; x < vals.size(); x++) {
        sum_of_all_vals += vals[x];
    }

    // Check that get_sum is working on some testcases.
    SECTION("get_sum") {
        REQUIRE(prefix_sum.get_sum(0, vals.size() - 1) == sum_of_all_vals);
        REQUIRE(prefix_sum.get_sum(0, 2) == 10.f);
        REQUIRE(prefix_sum.get_sum(7, 9) == 13.f);
        REQUIRE(prefix_sum.get_sum(2, 5) == 15.f);
    }
}

TEST_CASE("PrefixSum2D", "[vtr_prefix_sum/PrefixSum2D]") {
    // Construct a 2D grid to compute the prefix sum over.
    vtr::NdMatrix<float, 2> vals({4, 4});
    /*
     * [ 1 3 9 2 ]
     * [ 2 4 0 8 ]
     * [ 3 7 1 3 ]
     * [ 5 6 9 2 ]
     */
    vals[0][0] = 5.f;
    vals[1][0] = 6.f;
    vals[2][0] = 9.f;
    vals[3][0] = 2.f;
    vals[0][1] = 3.f;
    vals[1][1] = 7.f;
    vals[2][1] = 1.f;
    vals[3][1] = 3.f;
    vals[0][2] = 2.f;
    vals[1][2] = 4.f;
    vals[2][2] = 0.f;
    vals[3][2] = 8.f;
    vals[0][3] = 1.f;
    vals[1][3] = 3.f;
    vals[2][3] = 9.f;
    vals[3][3] = 2.f;

    // Construct the Prefix Sum.
    vtr::PrefixSum2D<float> prefix_sum(vals);

    // Check that the sum of each 1x1 region is the original value.
    SECTION("construction") {
        for (size_t x = 0; x < 4; x++) {
            for (size_t y = 0; y < 4; y++) {
                float sum_val = prefix_sum.get_sum(x, y, x, y);
                REQUIRE(sum_val == vals[x][y]);
            }
        }
    }

    float sum_of_all_vals = 0;
    for (size_t x = 0; x < 4; x++) {
        for (size_t y = 0; y < 4; y++) {
            sum_of_all_vals += vals[x][y];
        }
    }

    // Check that get_sum is working on some testcases.
    SECTION("get_sum") {
        REQUIRE(prefix_sum.get_sum(0, 0, 3, 3) == sum_of_all_vals);
        REQUIRE(prefix_sum.get_sum(1, 1, 2, 2) == 12.f);
        REQUIRE(prefix_sum.get_sum(0, 0, 3, 0) == 22.f);
        REQUIRE(prefix_sum.get_sum(0, 0, 0, 3) == 11.f);
        REQUIRE(prefix_sum.get_sum(1, 2, 2, 3) == 16.f);
    }
}

