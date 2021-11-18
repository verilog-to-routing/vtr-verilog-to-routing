#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "fasm_utils.h"

namespace fasm {
namespace {

using Catch::Matchers::Equals;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("parse_names", "[fasm]") {
    std::string name;
    int index;

    parse_name_with_optional_index("A", &name, &index);
    CHECK_THAT(name, Equals("A"));
    CHECK(index == 0);

    parse_name_with_optional_index("ABCD[500]", &name, &index);
    CHECK_THAT(name, Equals("ABCD"));
    CHECK(index == 500);
}

TEST_CASE("split_fasm_entry", "[fasm]") {
    auto parts = split_fasm_entry("A B C", " ", "");
    REQUIRE(parts.size() == 3);
    CHECK_THAT(parts[0], Equals("A"));
    CHECK_THAT(parts[1], Equals("B"));
    CHECK_THAT(parts[2], Equals("C"));

    parts = split_fasm_entry("A \nB\n\tC ", "\n", "\t ");

    REQUIRE(parts.size() == 3);
    CHECK_THAT(parts[0], Equals("A"));
    CHECK_THAT(parts[1], Equals("B"));
    CHECK_THAT(parts[2], Equals("C"));
}

} // namespace
} // namespace fasm
