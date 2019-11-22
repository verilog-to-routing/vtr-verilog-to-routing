#include "catch.hpp"

#include "vtr_geometry.h"

TEST_CASE("Point", "[vtr_geometry/Point]") {
    vtr::Point<int> p1(5, 3);
    vtr::Point<float> p2(5.3, 3.9);
    SECTION("location") {
        REQUIRE(p1.x() == 5);
        REQUIRE(p1.y() == 3);

        REQUIRE(p2.x() == Approx(5.3));
        REQUIRE(p2.y() == Approx(3.9));
    }

    SECTION("equality") {
        REQUIRE(p1 == p1);
        REQUIRE(p2 == p2);
    }
}

TEST_CASE("Rect", "[vtr_geometry/Rect]") {
    vtr::Point<int> pi_1(5, 3);
    vtr::Point<int> pi_2(10, 11);

    vtr::Rect<int> r1(pi_1.x(), pi_1.y(), pi_2.x(), pi_2.y());
    vtr::Rect<int> r2(pi_1, pi_2);

    SECTION("equality") {
        REQUIRE(r1 == r2);
    }

    SECTION("location") {
        REQUIRE(r1.xmin() == pi_1.x());
        REQUIRE(r1.xmax() == pi_2.x());
        REQUIRE(r1.ymin() == pi_1.y());
        REQUIRE(r1.ymax() == pi_2.y());
    }

    SECTION("point_accessors") {
        REQUIRE(r1.bottom_left() == pi_1);
        REQUIRE(r1.top_right() == pi_2);
        REQUIRE(r2.bottom_left() == pi_1);
        REQUIRE(r2.top_right() == pi_2);
    }

    SECTION("dimensions") {
        REQUIRE(r1.width() == 5);
        REQUIRE(r1.height() == 8);
        REQUIRE(r2.width() == 5);
        REQUIRE(r2.height() == 8);
    }

    SECTION("contains_int") {
        REQUIRE(r2.contains(pi_1));
        REQUIRE(r2.contains({6, 4}));
        REQUIRE_FALSE(r2.contains({100, 4}));
        REQUIRE_FALSE(r2.contains(pi_2));
    }

    SECTION("strictly_contains_int") {
        REQUIRE_FALSE(r2.strictly_contains(pi_1));
        REQUIRE(r2.strictly_contains({6, 4}));
        REQUIRE_FALSE(r2.strictly_contains({100, 4}));
        REQUIRE_FALSE(r2.strictly_contains(pi_2));
    }

    SECTION("coincident_int") {
        REQUIRE(r2.coincident(pi_1));
        REQUIRE(r2.coincident({6, 4}));
        REQUIRE_FALSE(r2.coincident({100, 4}));
        REQUIRE(r2.coincident(pi_2));
    }

    vtr::Point<float> pf_1(5.3, 3.9);
    vtr::Point<float> pf_2(10.5, 11.1);

    vtr::Rect<float> r3(pf_1.x(), pf_1.y(), pf_2.x(), pf_2.y());
    vtr::Rect<float> r4(pf_1, pf_2);

    SECTION("equality_float") {
        REQUIRE(r3 == r4);
    }

    SECTION("location_float") {
        REQUIRE(r3.xmin() == pf_1.x());
        REQUIRE(r3.xmax() == pf_2.x());
        REQUIRE(r3.ymin() == pf_1.y());
        REQUIRE(r3.ymax() == pf_2.y());
    }

    SECTION("point_accessors_float") {
        REQUIRE(r3.bottom_left() == pf_1);
        REQUIRE(r3.top_right() == pf_2);
        REQUIRE(r4.bottom_left() == pf_1);
        REQUIRE(r4.top_right() == pf_2);
    }

    SECTION("dimensions") {
        REQUIRE(r3.width() == Approx(5.2));
        REQUIRE(r3.height() == Approx(7.2));
        REQUIRE(r4.width() == Approx(5.2));
        REQUIRE(r4.height() == Approx(7.2));
    }

    SECTION("contains_float") {
        REQUIRE(r4.contains(pf_1));
        REQUIRE(r4.contains({6, 4}));
        REQUIRE_FALSE(r4.contains({100, 4}));
        REQUIRE_FALSE(r4.contains(pf_2));
    }

    SECTION("strictly_contains_float") {
        REQUIRE_FALSE(r4.strictly_contains(pf_1));
        REQUIRE(r4.strictly_contains({6, 4}));
        REQUIRE_FALSE(r4.strictly_contains({100, 4}));
        REQUIRE_FALSE(r4.strictly_contains(pf_2));
    }

    SECTION("coincident_float") {
        REQUIRE(r4.coincident(pf_1));
        REQUIRE(r4.coincident({6, 4}));
        REQUIRE_FALSE(r4.coincident({100, 4}));
        REQUIRE(r4.coincident(pf_2));
    }
}

TEST_CASE("Line", "[vtr_geometry/Line]") {
    std::vector<vtr::Point<int>> points = {{0, 0},
                                           {0, 2},
                                           {1, 0},
                                           {1, -2}};

    vtr::Line<int> line(points);

    SECTION("points") {
        auto line_points = line.points();

        REQUIRE(line_points.size() == points.size());

        int i = 0;
        for (auto point : line_points) {
            REQUIRE(points[i] == point);
            ++i;
        }
    }

    SECTION("bounding_box") {
        auto bb = line.bounding_box();

        REQUIRE(bb.xmin() == 0);
        REQUIRE(bb.xmax() == 1);
        REQUIRE(bb.ymin() == -2);
        REQUIRE(bb.ymax() == 2);
    }
}

TEST_CASE("RectUnion", "[vtr_geometry/RectUnion]") {
    std::vector<vtr::Rect<int>> rects = {{0, 0, 2, 2},
                                         {1, 1, 3, 3}};

    vtr::RectUnion<int> rect_union(rects);

    SECTION("rects") {
        auto union_rects = rect_union.rects();

        REQUIRE(union_rects.size() == rects.size());

        int i = 0;
        for (auto rect : union_rects) {
            REQUIRE(rects[i] == rect);
            ++i;
        }
    }
    SECTION("bounding_box") {
        auto bb = rect_union.bounding_box();

        REQUIRE(bb.xmin() == 0);
        REQUIRE(bb.xmax() == 3);
        REQUIRE(bb.ymin() == 0);
        REQUIRE(bb.ymax() == 3);
    }

    SECTION("contains") {
        REQUIRE(rect_union.contains({0, 0}));
        REQUIRE(rect_union.contains({1, 1}));
        REQUIRE(rect_union.contains({2, 2}));
        REQUIRE_FALSE(rect_union.contains({3, 3}));
    }

    SECTION("strictly_contains") {
        REQUIRE_FALSE(rect_union.strictly_contains({0, 0}));
        REQUIRE(rect_union.strictly_contains({1, 1}));
        REQUIRE(rect_union.strictly_contains({2, 2}));
        REQUIRE_FALSE(rect_union.strictly_contains({3, 3}));
    }

    SECTION("coincident") {
        REQUIRE(rect_union.coincident({0, 0}));
        REQUIRE(rect_union.coincident({1, 1}));
        REQUIRE(rect_union.coincident({2, 2}));
        REQUIRE(rect_union.coincident({3, 3}));
    }
}
