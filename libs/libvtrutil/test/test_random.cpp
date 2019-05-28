#include "catch.hpp"

#include "vtr_random.h"

#include <vector>
#include <iostream>

TEST_CASE("shuffle", "[vtr_random/shuffle]") {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    vtr::RandState rand_state = 1;
    vtr::shuffle(numbers.begin(), numbers.end(), rand_state);

    std::vector<int> numbers_shuffled_1 = {5, 2, 4, 1, 3};
    REQUIRE(numbers == numbers_shuffled_1);
}
