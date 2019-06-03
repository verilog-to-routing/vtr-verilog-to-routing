#include "catch.hpp"

#include "vtr_vector.h"
#include "vtr_strong_id.h"

#include <ostream>

struct test_tag;
typedef vtr::StrongId<test_tag> TestId;

std::ostream& operator<<(std::ostream& os, const TestId id);

std::ostream& operator<<(std::ostream& os, const TestId id) {
    os << "TestId(" << size_t(id) << ")";
    return os;
}

TEST_CASE("Basic Ops", "[vtr_vector]") {
    vtr::vector<TestId, int> vec;

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

TEST_CASE("Key Access", "[vtr_vector]") {
    vtr::vector<TestId, int> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    std::vector<TestId> expected_keys = {TestId(0), TestId(1), TestId(2), TestId(3)};

    auto keys = vec.keys();
    REQUIRE(keys.size() == vec.size());

    size_t i = 0;
    for (TestId key : keys) {
        REQUIRE(key == expected_keys[i]);
        ++i;
    }
}
