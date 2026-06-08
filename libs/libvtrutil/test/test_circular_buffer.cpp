#include "catch2/catch_test_macros.hpp"

#include "vtr_circular_buffer.h"

#include <stdexcept>
#include <vector>

namespace {

std::vector<int> to_vector(const vtr::circular_buffer<int>& buffer) {
    return std::vector<int>(buffer.begin(), buffer.end());
}

struct MoveOnlyValue {
    explicit MoveOnlyValue(int init_value) noexcept
        : value(init_value) {}

    MoveOnlyValue(const MoveOnlyValue&) = delete;
    MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;
    MoveOnlyValue(MoveOnlyValue&&) = default;
    MoveOnlyValue& operator=(MoveOnlyValue&&) = default;

    int value = 0;
};

} // namespace

TEST_CASE("Circular buffer overwrites oldest values", "[vtr_circular_buffer]") {
    vtr::circular_buffer<int> buffer(3);

    REQUIRE(buffer.empty());
    REQUIRE_FALSE(buffer.full());
    REQUIRE(buffer.capacity() == 3);

    buffer.push_back(1);
    buffer.push_back(2);
    buffer.push_back(3);

    REQUIRE(buffer.full());
    REQUIRE(buffer.size() == 3);
    REQUIRE(to_vector(buffer) == std::vector<int>{1, 2, 3});
    REQUIRE(buffer.front() == 1);
    REQUIRE(buffer.back() == 3);

    buffer.push_back(4);

    REQUIRE(buffer.full());
    REQUIRE(to_vector(buffer) == std::vector<int>{2, 3, 4});
    REQUIRE(buffer[0] == 2);
    REQUIRE(buffer[1] == 3);
    REQUIRE(buffer[2] == 4);
    REQUIRE(buffer.front() == 2);
    REQUIRE(buffer.back() == 4);

    buffer.emplace_back(5);

    REQUIRE(to_vector(buffer) == std::vector<int>{3, 4, 5});
}

TEST_CASE("Circular buffer supports wrapped iteration and popping", "[vtr_circular_buffer]") {
    vtr::circular_buffer<int> buffer(3, {1, 2, 3});

    buffer.push_back(4);
    REQUIRE(to_vector(buffer) == std::vector<int>{2, 3, 4});

    buffer.pop_front();
    REQUIRE(to_vector(buffer) == std::vector<int>{3, 4});
    REQUIRE(buffer.front() == 3);
    REQUIRE(buffer.back() == 4);

    buffer.push_back(5);
    REQUIRE(to_vector(buffer) == std::vector<int>{3, 4, 5});

    const vtr::circular_buffer<int>& const_buffer = buffer;
    REQUIRE(std::vector<int>(const_buffer.rbegin(), const_buffer.rend()) == std::vector<int>{5, 4, 3});

    buffer.pop_back();
    REQUIRE(to_vector(buffer) == std::vector<int>{3, 4});

    buffer.clear();
    REQUIRE(buffer.empty());
    REQUIRE(buffer.capacity() == 3);
}

TEST_CASE("Circular buffer checks bounds", "[vtr_circular_buffer]") {
    vtr::circular_buffer<int> buffer(2);

    buffer.push_back(1);

    REQUIRE(buffer.at(0) == 1);
    REQUIRE_THROWS_AS(buffer.at(1), std::out_of_range);
}

TEST_CASE("Circular buffer handles zero capacity", "[vtr_circular_buffer]") {
    vtr::circular_buffer<int> buffer;

    REQUIRE(buffer.capacity() == 0);
    REQUIRE(buffer.empty());
    REQUIRE(buffer.full());

    buffer.push_back(1);
    buffer.emplace_back(2);

    REQUIRE(buffer.empty());
    REQUIRE(buffer.begin() == buffer.end());
}

TEST_CASE("Circular buffer supports non-default-constructible move-only values", "[vtr_circular_buffer]") {
    vtr::circular_buffer<MoveOnlyValue> buffer(2);

    buffer.emplace_back(1);
    buffer.emplace_back(2);
    buffer.emplace_back(3);

    REQUIRE(buffer.size() == 2);
    REQUIRE(buffer.front().value == 2);
    REQUIRE(buffer.back().value == 3);
}
