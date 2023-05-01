#include "catch2/catch_test_macros.hpp"

#include "router_lookahead_map.h"

extern t_wire_cost_map f_wire_cost_map;

namespace {

#ifdef VTR_ENABLE_CAPNPROTO
static constexpr const char kMapLookaheadBin[] = "test_map_lookahead.bin";

TEST_CASE("round_trip_map_lookahead", "[vpr]") {
    constexpr std::array<size_t, 5> kDim({1, 10, 12, 15, 16});

    f_wire_cost_map.resize(kDim);
    for (size_t layer = 0; layer < kDim[0]; layer++) {
        for (size_t x = 0; x < kDim[1]; ++x) {
            for (size_t y = 0; y < kDim[2]; ++y) {
                for (size_t z = 0; z < kDim[3]; ++z) {
                    for (size_t w = 0; w < kDim[4]; ++w) {
                        f_wire_cost_map[layer][x][y][z][w].delay = (x + 1) * (y + 1) * (z + 1) * (w + 1);
                        f_wire_cost_map[layer][x][y][z][w].congestion = 2 * (x + 1) * (y + 1) * (z + 1) * (w + 1);
                    }
                }
            }
        }
    }

    write_router_lookahead(kMapLookaheadBin);

    for (size_t layer = 0; layer < kDim[0]; layer++) {
        for (size_t x = 0; x < kDim[1]; ++x) {
            for (size_t y = 0; y < kDim[2]; ++y) {
                for (size_t z = 0; z < kDim[3]; ++z) {
                    for (size_t w = 0; w < kDim[4]; ++w) {
                        f_wire_cost_map[layer][x][y][z][w].delay = 0.f;
                        f_wire_cost_map[layer][x][y][z][w].congestion = 0.f;
                    }
                }
            }
        }
    }

    f_wire_cost_map.resize({0, 0, 0, 0, 0});

    read_router_lookahead(kMapLookaheadBin);

    for (size_t i = 0; i < kDim.size(); ++i) {
        REQUIRE(f_wire_cost_map.dim_size(i) == kDim[i]);
    }

    for (size_t layer = 0; layer < kDim[0]; layer++) {
        for (size_t x = 0; x < kDim[1]; ++x) {
            for (size_t y = 0; y < kDim[2]; ++y) {
                for (size_t z = 0; z < kDim[3]; ++z) {
                    for (size_t w = 0; w < kDim[4]; ++w) {
                        REQUIRE(f_wire_cost_map[layer][x][y][z][w].delay == (x + 1) * (y + 1) * (z + 1) * (w + 1));
                        REQUIRE(f_wire_cost_map[layer][x][y][z][w].congestion == 2 * (x + 1) * (y + 1) * (z + 1) * (w + 1));
                    }
                }
            }
        }
    }
}

#endif

} // namespace
