#include "catch2/catch_test_macros.hpp"

#include "vtr_strong_id.h"
#include "vtr_strong_id_range.h"

struct t_test_tag;
using TestStrongId = vtr::StrongId<t_test_tag>;

TEST_CASE("StrongId", "[StrongId/StrongId]") {
    TestStrongId a;
    TestStrongId b;
    TestStrongId c(0);
    TestStrongId d(0);
    TestStrongId e(1);
    TestStrongId f(2);

    REQUIRE(!bool(a));
    REQUIRE(!bool(b));
    REQUIRE(bool(c));
    REQUIRE(bool(d));
    REQUIRE(bool(e));
    REQUIRE(bool(f));

    REQUIRE(a == b);
    REQUIRE(a == TestStrongId::INVALID());

    REQUIRE(c == d);
    REQUIRE(c != a);
    REQUIRE(c != TestStrongId::INVALID());
    REQUIRE(d != TestStrongId::INVALID());

    REQUIRE(c != e);
    REQUIRE(c != f);
    REQUIRE(e != f);

    REQUIRE(c < e);
    REQUIRE(c < f);
    REQUIRE(e < f);
    REQUIRE(!(e < c));
    REQUIRE(!(f < c));
    REQUIRE(!(f < e));
}

TEST_CASE("StrongIdIterator", "[StrongId/StrongIdIterator]") {
    TestStrongId a(0);
    TestStrongId b(1);
    TestStrongId c(5);
    TestStrongId d(5);

    vtr::StrongIdIterator<TestStrongId> a_iter(a);
    vtr::StrongIdIterator<TestStrongId> b_iter(b);
    vtr::StrongIdIterator<TestStrongId> c_iter(c);
    vtr::StrongIdIterator<TestStrongId> d_iter(d);

    REQUIRE(*a_iter == a);
    REQUIRE(*b_iter == b);
    REQUIRE(*c_iter == c);
    REQUIRE(*c_iter == d);
    REQUIRE(*d_iter == c);
    REQUIRE(*d_iter == d);

    REQUIRE(a_iter != b_iter);
    REQUIRE(a_iter != c_iter);
    REQUIRE(a_iter != d_iter);

    REQUIRE(c_iter == d_iter);
    REQUIRE(c_iter != a_iter);
    REQUIRE(c_iter != b_iter);

    REQUIRE(std::distance(a_iter, b_iter) == 1);
    REQUIRE(std::distance(c_iter, d_iter) == 0);
    REQUIRE(std::distance(d_iter, a_iter) == -5);

    REQUIRE(a_iter < b_iter);
    REQUIRE(b_iter < c_iter);
    REQUIRE(!(c_iter < b_iter));

    REQUIRE(a_iter[0] == a);
    REQUIRE(a_iter[1] == b);
    REQUIRE(a_iter[5] == c);
    REQUIRE(c_iter[0] == c);
    REQUIRE(c_iter[-4] == b);
    REQUIRE(c_iter[-5] == a);

    REQUIRE((a_iter + 5) == c_iter);
    REQUIRE(a_iter == (c_iter - 5));
    a_iter += 5;
    REQUIRE(a_iter == c_iter);
    a_iter -= 4;
    REQUIRE(a_iter == b_iter);
}

TEST_CASE("StrongIdRange", "[StrongId/StrongIdRange]") {
    TestStrongId a(0);
    TestStrongId b(0);
    TestStrongId c(5);
    TestStrongId d(1);

    vtr::StrongIdRange<TestStrongId> r1(a, b);
    REQUIRE(r1.size() == 0);
    REQUIRE(r1.empty());

    vtr::StrongIdRange<TestStrongId> r2(a, c);
    REQUIRE(r2.size() == 5);
    REQUIRE(!r2.empty());

    vtr::StrongIdRange<TestStrongId> r3(d, c);
    REQUIRE(r3.size() == 4);
    REQUIRE(!r3.empty());

    int count = 0;
    for (TestStrongId id : r1) {
        (void)id;
        count += 1;
    }
    REQUIRE(count == 0);

    for (TestStrongId id : r2) {
        REQUIRE(TestStrongId(count) == id);
        count += 1;
    }
    REQUIRE(count == 5);

    count = 0;
    for (TestStrongId id : r3) {
        REQUIRE(TestStrongId(count + 1) == id);
        count += 1;
    }
    REQUIRE(count == 4);
}
