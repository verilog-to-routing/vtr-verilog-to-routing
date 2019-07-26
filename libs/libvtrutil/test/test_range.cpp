#include "catch.hpp"

#include "vtr_range.h"
#include <vector>

TEST_CASE("Range Ops", "[vtr_range]") {
    std::vector<int> vec = {1, 2, 3};

    {
        //From iterator pair
        auto range = vtr::make_range(vec.begin(), vec.end());
        REQUIRE(range.size() == vec.size());

        size_t i = 0;
        for (auto elem : range) {
            REQUIRE(elem == vec[i]);
            i++;
        }
        REQUIRE(i == vec.size());
    }

    {
        //From container
        auto range = vtr::make_range(vec);
        REQUIRE(range.size() == vec.size());

        size_t i = 0;
        for (auto elem : range) {
            REQUIRE(elem == vec[i]);
            i++;
        }
        REQUIRE(i == vec.size());
    }

    {
        //Empty
        auto range = vtr::make_range(vec.begin(), vec.begin());
        REQUIRE(range.size() == 0);
        REQUIRE(range.empty());
    }
}
