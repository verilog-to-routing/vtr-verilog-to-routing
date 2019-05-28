#include "catch.hpp"

#include "vtr_map_util.h"
#include "vtr_range.h"

#include <map>

TEST_CASE("Iterate Map Keys Values", "[vtr_map_util]") {
    std::vector<int> keys = {0, 1, 2, 3};
    std::vector<char> values = {'a', 'b', 'c', 'd'};

    //Initialize map
    std::map<int, char> map;
    for (size_t i = 0; i < keys.size(); ++i) {
        map[keys[i]] = values[i];
    }

    //Check key iteration
    auto key_range = vtr::make_key_range(map);

    std::vector<int> seen_keys;
    for (int key : key_range) {
        seen_keys.push_back(key);
    }
    REQUIRE(seen_keys == keys);

    //Check value iteration
    auto value_range = vtr::make_value_range(map);

    std::vector<char> seen_values;
    for (char value : value_range) {
        seen_values.push_back(value);
    }
    REQUIRE(seen_values == values);
}
