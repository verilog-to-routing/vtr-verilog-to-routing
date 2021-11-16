#include "catch2/catch_test_macros.hpp"

#include "vtr_string_view.h"
#include "vtr_string_interning.h"

TEST_CASE("String view", "[vtr_string_view/string_view]") {
    vtr::string_view a("test");
    vtr::string_view b("test");
    vtr::string_view c("tes");
    vtr::string_view d("est");
    vtr::string_view e("es");

    REQUIRE(a.size() == 4);
    REQUIRE(b.size() == 4);
    REQUIRE(c.size() == 3);
    REQUIRE(d.size() == 3);
    REQUIRE(e.size() == 2);

    REQUIRE(a[0] == 't');
    REQUIRE(a[1] == 'e');
    REQUIRE(a[2] == 's');
    REQUIRE(a[3] == 't');

    auto itr = a.begin();
    REQUIRE(*itr++ == 't');
    REQUIRE(*itr++ == 'e');
    REQUIRE(*itr++ == 's');
    REQUIRE(*itr++ == 't');
    REQUIRE(itr == a.end());

    REQUIRE(a.front() == 't');
    REQUIRE(c.front() == 't');
    REQUIRE(c.back() == 's');

    REQUIRE(a == b);
    REQUIRE(a <= b);
    REQUIRE(a >= b);
    REQUIRE(a != c);

    REQUIRE(c < a);
    REQUIRE(a >= c);

    REQUIRE(a > c);
    REQUIRE(c <= a);

    REQUIRE(c != d);
    REQUIRE(a.substr(0, 3) == c);
    REQUIRE(a.substr(1, 3) == d);
    REQUIRE(a.substr(1) == d);
    REQUIRE(a.substr(1, 2) == e);
    REQUIRE(std::hash<vtr::string_view>()(a) == std::hash<vtr::string_view>()(b));
    REQUIRE(std::hash<vtr::string_view>()(a) != std::hash<vtr::string_view>()(c));

    vtr::string_view f = a;
    REQUIRE(b == f);

    f = e;
    REQUIRE(b != f);
    REQUIRE(e == f);

    std::swap(a, f);
    REQUIRE(a == e);
    REQUIRE(f == b);
}

TEST_CASE("Basic string internment", "[vtr_string_interning/string_internment") {
    vtr::string_internment internment;

    vtr::interned_string a = internment.intern_string(vtr::string_view("test"));
    vtr::interned_string b = internment.intern_string(vtr::string_view("test"));
    vtr::interned_string c = internment.intern_string(vtr::string_view("tes"));
    vtr::interned_string d = internment.intern_string(vtr::string_view("est"));
    vtr::interned_string e = internment.intern_string(vtr::string_view("es"));

    auto itr = a.begin(&internment);
    REQUIRE(*itr++ == 't');
    REQUIRE(*itr++ == 'e');
    REQUIRE(*itr++ == 's');
    REQUIRE(*itr++ == 't');
    REQUIRE(itr == a.end());

    itr = a.begin(&internment);
    REQUIRE(*itr == 't');
    ++itr;
    REQUIRE(*itr == 'e');
    ++itr;
    REQUIRE(*itr == 's');
    ++itr;
    REQUIRE(*itr == 't');
    ++itr;
    REQUIRE(itr == a.end());

    REQUIRE(a == b);
    REQUIRE(a.bind(&internment) <= b.bind(&internment));
    REQUIRE(a.bind(&internment) >= b.bind(&internment));
    REQUIRE(a != c);

    REQUIRE(c.bind(&internment) < a.bind(&internment));
    REQUIRE(a.bind(&internment) >= c.bind(&internment));

    REQUIRE(a.bind(&internment) > c.bind(&internment));
    REQUIRE(c.bind(&internment) <= a.bind(&internment));

    REQUIRE(c != d);
    REQUIRE(std::hash<vtr::interned_string>()(a) == std::hash<vtr::interned_string>()(b));
    REQUIRE(std::hash<vtr::interned_string>()(a) != std::hash<vtr::interned_string>()(c));

    std::string g;
    a.get(&internment, &g);
    REQUIRE(g == "test");
    c.get(&internment, &g);
    REQUIRE(g == "tes");
    d.get(&internment, &g);
    REQUIRE(g == "est");

    vtr::interned_string f = a;
    REQUIRE(b == f);

    f = e;
    REQUIRE(b != f);
    REQUIRE(e == f);

    std::swap(a, f);
    REQUIRE(a == e);
    REQUIRE(f == b);
}

static void test_internment_retreval(const vtr::string_internment* internment, vtr::interned_string str, const char* expect) {
    std::string copy;
    str.get(internment, &copy);
    REQUIRE(copy == expect);
    copy.clear();
    std::copy(str.begin(internment), str.end(), std::back_inserter(copy));
    REQUIRE(copy == expect);
}

TEST_CASE("Split string internment", "[vtr_string_interning/string_internment") {
    vtr::string_internment internment;

    size_t unique_strings = 0;

    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string a = internment.intern_string(vtr::string_view("test"));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string b = internment.intern_string(vtr::string_view("test.test"));
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string c = internment.intern_string(vtr::string_view("test.test.test"));
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string d = internment.intern_string(vtr::string_view("test.test.test.test"));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);

    test_internment_retreval(&internment, a, "test");
    test_internment_retreval(&internment, b, "test.test");
    test_internment_retreval(&internment, c, "test.test.test");
    test_internment_retreval(&internment, d, "test.test.test.test");

    vtr::interned_string f = internment.intern_string(vtr::string_view("a"));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string g = internment.intern_string(vtr::string_view("b.c"));
    unique_strings += 2;
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string h = internment.intern_string(vtr::string_view("d.e.f"));
    unique_strings += 3;
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string i = internment.intern_string(vtr::string_view("g.h.i.j"));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);

    test_internment_retreval(&internment, f, "a");
    test_internment_retreval(&internment, g, "b.c");
    test_internment_retreval(&internment, h, "d.e.f");
    test_internment_retreval(&internment, i, "g.h.i.j");

    vtr::interned_string j = internment.intern_string(vtr::string_view("."));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string k = internment.intern_string(vtr::string_view(".."));
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string l = internment.intern_string(vtr::string_view("..."));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string m = internment.intern_string(vtr::string_view("...."));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);

    test_internment_retreval(&internment, j, ".");
    test_internment_retreval(&internment, k, "..");
    test_internment_retreval(&internment, l, "...");
    test_internment_retreval(&internment, m, "....");

    vtr::interned_string n = internment.intern_string(vtr::string_view(".q"));
    unique_strings += 1;
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string o = internment.intern_string(vtr::string_view(".a."));
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string p = internment.intern_string(vtr::string_view("b.c.d"));
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string q = internment.intern_string(vtr::string_view("e..f"));
    REQUIRE(internment.unique_strings() == unique_strings);
    vtr::interned_string r = internment.intern_string(vtr::string_view("e."));
    REQUIRE(internment.unique_strings() == unique_strings);

    test_internment_retreval(&internment, n, ".q");
    test_internment_retreval(&internment, o, ".a.");
    test_internment_retreval(&internment, p, "b.c.d");
    test_internment_retreval(&internment, q, "e..f");
    test_internment_retreval(&internment, r, "e.");
}
