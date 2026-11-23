#ifdef __GNUC__
// GCC 13 emits a spurious -Wnull-dereference inside <bits/stl_construct.h>
// when inlining std::vector::_M_realloc_insert at begin() under -O3.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

#include <memory>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "catch2/catch_test_macros.hpp"

#include "vtr_flat_map.h"

#include <string>

TEST_CASE("Insert and Emplace", "[vtr_flat_map]") {
    vtr::flat_map<int, std::string> map;

    // Insert into empty map (position == end(), size() == 0)
    auto [it1, inserted1] = map.insert({10, "ten"});
    REQUIRE(inserted1);
    REQUIRE(it1->first == 10);
    REQUIRE(it1->second == "ten");

    // Insert larger key at end() when non-empty
    auto [it2, inserted2] = map.insert({30, "thirty"});
    REQUIRE(inserted2);
    REQUIRE(it2->first == 30);
    REQUIRE(it2->second == "thirty");

    // Insert smaller key at begin()
    auto [it3, inserted3] = map.insert({5, "five"});
    REQUIRE(inserted3);
    REQUIRE(it3->first == 5);
    REQUIRE(it3->second == "five");

    // Insert middle key (including at end() - 1)
    auto [it4, inserted4] = map.insert({20, "twenty"});
    REQUIRE(inserted4);
    REQUIRE(it4->first == 20);
    REQUIRE(it4->second == "twenty");

    // Duplicate key does not overwrite
    auto [it_dup, inserted_dup] = map.insert({20, "other"});
    REQUIRE(!inserted_dup);
    REQUIRE(it_dup->second == "twenty");

    // Emplace larger key at end() when non-empty
    auto [it5, inserted5] = map.emplace({40, "forty"});
    REQUIRE(inserted5);
    REQUIRE(it5->first == 40);
    REQUIRE(it5->second == "forty");

    // Emplace smaller key at begin() and middle key
    auto [it6, inserted6] = map.emplace({1, "one"});
    REQUIRE(inserted6);
    REQUIRE(it6->first == 1);

    auto [it7, inserted7] = map.emplace({25, "twenty-five"});
    REQUIRE(inserted7);
    REQUIRE(it7->first == 25);

    // Emplace duplicate key does not overwrite
    auto [it_edup, inserted_edup] = map.emplace({40, "other"});
    REQUIRE(!inserted_edup);
    REQUIRE(it_edup->second == "forty");

    REQUIRE(map.size() == 7);
    REQUIRE(map.at(1) == "one");
    REQUIRE(map.at(5) == "five");
    REQUIRE(map.at(10) == "ten");
    REQUIRE(map.at(20) == "twenty");
    REQUIRE(map.at(25) == "twenty-five");
    REQUIRE(map.at(30) == "thirty");
    REQUIRE(map.at(40) == "forty");
}

TEST_CASE("Hinted Insert and Emplace", "[vtr_flat_map]") {
    vtr::flat_map<int, int> map;

    auto it = map.insert(map.end(), {10, 100});
    REQUIRE(it->first == 10);

    it = map.insert(map.end(), {30, 300});
    REQUIRE(it->first == 30);

    it = map.insert(map.begin(), {5, 50});
    REQUIRE(it->first == 5);

    it = map.insert(map.lower_bound(20), {20, 200});
    REQUIRE(it->first == 20);

    it = map.emplace(map.end(), {40, 400});
    REQUIRE(it->first == 40);

    it = map.emplace(map.begin(), {1, 10});
    REQUIRE(it->first == 1);

    it = map.emplace(map.lower_bound(25), {25, 250});
    REQUIRE(it->first == 25);

    REQUIRE(map.size() == 7);
}
