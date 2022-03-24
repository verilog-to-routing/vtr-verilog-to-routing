#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "route_samplers.h"

namespace {

TEST_CASE("test_convex_hull", "[vpr]") {
    /* Smoke test for the convex hull algorithm in the sampler */
    std::vector<SinkPoint> points1 {
        {0, 0, 0}, {0, 1, 0}, {1, 1, 0}
    };
    std::vector<SinkPoint> expected_hull1(points1);
    std::vector<SinkPoint> hull1 = quickhull(points1);
    REQUIRE_THAT(hull1, Catch::Matchers::UnorderedEquals(expected_hull1));

    std::vector<SinkPoint> points2 {
        {113,148,0}, {113,143,0}, {113,145,0}, {114,146,0}, {111,138,0}, {110,139,0},
        {112,138,0}, {108,146,0}, {111,145,0}, {103,142,0}, {103,148,0}, {116,142,0},
        {116,141,0}, {110,148,0}, {106,146,0}
    }; 
    std::vector<SinkPoint> expected_hull2 {
        {111,138,0}, {116,141,0}, {112,138,0}, {103,148,0}, {103,142,0}, {116,142,0},
         {113,148,0}
    };
    std::vector<SinkPoint> hull2 = quickhull(points2);
    REQUIRE_THAT(hull2, Catch::Matchers::UnorderedEquals(expected_hull2));
}

} // namespace
