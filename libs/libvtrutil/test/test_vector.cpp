#include "catch.hpp"

#include "vtr_vector.h"
#include "vtr_strong_id.h"

struct test_tag;
typedef vtr::StrongId<test_tag> TestId;

TEST_CASE("Basic Ops", "[vtr_vector]") {

    vtr::vector<TestId,int> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);


    REQUIRE(vec.size() == 3);
    REQUIRE(vec[TestId(0)] == 1);
    REQUIRE(vec[TestId(1)] == 2);
    REQUIRE(vec[TestId(2)] == 3);

    vec.emplace_back(4);

    REQUIRE(vec.size() == 4);
    REQUIRE(vec[TestId(3)] == 4);

    REQUIRE(vec.front() == 1);
    REQUIRE(vec.back() == 4);
}
