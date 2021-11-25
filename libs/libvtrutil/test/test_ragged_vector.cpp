#include "catch2/catch_test_macros.hpp"

#include "vtr_ragged_matrix.h"

#include <numeric>

TEST_CASE("Construction", "[vtr_ragged_matrix]") {
    vtr::FlatRaggedMatrix<float> empty;
    REQUIRE(empty.size() == 0);
    REQUIRE(empty.empty());

    std::vector<size_t> row_sizes = {1, 5, 3, 10};
    size_t nelem = std::accumulate(row_sizes.begin(), row_sizes.end(), 0u);

    //Construct from container of row sizes
    vtr::FlatRaggedMatrix<float> ones_container(row_sizes, 1.0);
    REQUIRE(ones_container.size() == nelem);
    REQUIRE(!ones_container.empty());

    //Construct from row size callback
    auto row_size_callback = [&](size_t irow) {
        return row_sizes[irow];
    };
    vtr::FlatRaggedMatrix<float> ones_callback(row_sizes.size(), row_size_callback, 1.0);
    REQUIRE(ones_callback.size() == nelem);
    REQUIRE(!ones_callback.empty());

    //Construct from row size iterators
    vtr::FlatRaggedMatrix<float> ones_iterator(row_sizes.begin(), row_sizes.end(), 1.0);
    REQUIRE(ones_iterator.size() == nelem);
    REQUIRE(!ones_iterator.empty());

    //Clear
    ones_container.clear();
    REQUIRE(ones_container.size() == 0);

    ones_callback.clear();
    REQUIRE(ones_callback.size() == 0);

    ones_iterator.clear();
    REQUIRE(ones_iterator.size() == 0);
}

TEST_CASE("Iteration", "[vtr_ragged_matrix]") {
    std::vector<size_t> row_sizes = {1, 5, 3, 10};
    vtr::FlatRaggedMatrix<float> ones(row_sizes, 1.0);

    float expected_sum = std::accumulate(row_sizes.begin(), row_sizes.end(), 0.f);

    //Iteration by indices
    float index_iteration_sum = 0.;
    for (size_t irow = 0; irow < row_sizes.size(); ++irow) {
        for (size_t icol = 0; icol < row_sizes[irow]; ++icol) {
            index_iteration_sum += ones[irow][icol];
        }
    }
    REQUIRE(index_iteration_sum == expected_sum);

    //Iteration by first index + proxy
    float row_for_iteration_sum = 0.;
    for (size_t irow = 0; irow < row_sizes.size(); ++irow) {
        REQUIRE(ones[irow].size() == row_sizes[irow]);

        for (float val : ones[irow]) {
            row_for_iteration_sum += val;
        }
    }
    REQUIRE(row_for_iteration_sum == expected_sum);

    //Iteration by range
    float for_iteration_sum = 0.;
    for (float val : ones) {
        for_iteration_sum += val;
    }
    REQUIRE(for_iteration_sum == expected_sum);
}

TEST_CASE("Modification", "[vtr_ragged_matrix]") {
    std::vector<size_t> row_sizes = {1, 5, 3, 10};
    vtr::FlatRaggedMatrix<float> ones(row_sizes, 1.0);

    float base_sum = std::accumulate(row_sizes.begin(), row_sizes.end(), 0.f);

    //Index based modification
    size_t irow = 3;
    for (size_t icol = 0; icol < row_sizes[irow]; ++icol) {
        ones[irow][icol] = 2.;
    }
    base_sum += row_sizes[irow];
    REQUIRE(std::accumulate(ones.begin(), ones.end(), 0.f) == base_sum);

    //Range for row modification
    irow = 2;
    for (float& val : ones[irow]) {
        val = 2.;
    }
    base_sum += row_sizes[irow];
    REQUIRE(std::accumulate(ones.begin(), ones.end(), 0.f) == base_sum);

    //Single element modification
    ones[0][0] = 3.;
    base_sum += 2.;
    REQUIRE(std::accumulate(ones.begin(), ones.end(), 0.f) == base_sum);
}
