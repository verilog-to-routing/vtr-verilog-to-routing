#include "catch2/catch_test_macros.hpp"

#include "router_lookahead_map.h"

extern t_wire_cost_map f_wire_cost_map;

namespace {

#ifdef VTR_ENABLE_CAPNPROTO
static constexpr const char kMapLookaheadBin[] = "test_map_lookahead.bin";

TEST_CASE("round_trip_map_lookahead", "[vpr]") {
    constexpr size_t num_layers = 1;
    constexpr std::array<size_t, 6> kDim({num_layers, 10, 12, num_layers, 15, 16});

    f_wire_cost_map.resize(kDim);
    for (size_t from_layer = 0; from_layer < kDim[0]; from_layer++) {
        for (size_t x = 0; x < kDim[1]; ++x) {
            for (size_t y = 0; y < kDim[2]; ++y) {
                for (size_t to_layer = 0; to_layer < kDim[3]; to_layer++) {
                    for (size_t z = 0; z < kDim[4]; ++z) {
                        for (size_t w = 0; w < kDim[5]; ++w) {
                            f_wire_cost_map[from_layer][x][y][to_layer][z][w].delay = (x + 1) * (y + 1) * (z + 1) * (w + 1);
                            f_wire_cost_map[from_layer][x][y][to_layer][z][w].congestion = 2 * (x + 1) * (y + 1) * (z + 1) * (w + 1);
                        }
                    }
                }
            }
        }
    }

    write_router_lookahead(kMapLookaheadBin);

    for (size_t from_layer = 0; from_layer < kDim[0]; from_layer++) {
        for (size_t x = 0; x < kDim[1]; ++x) {
            for (size_t y = 0; y < kDim[2]; ++y) {
                for (size_t to_layer = 0; to_layer < kDim[3]; to_layer++) {
                    for (size_t z = 0; z < kDim[4]; ++z) {
                        for (size_t w = 0; w < kDim[5]; ++w) {
                            f_wire_cost_map[from_layer][x][y][to_layer][z][w].delay = 0.f;
                            f_wire_cost_map[from_layer][x][y][to_layer][z][w].congestion = 0.f;
                        }
                    }
                }
            }
        }
    }

    f_wire_cost_map.resize({0, 0, 0, 0, 0, 0});

    read_router_lookahead(kMapLookaheadBin);

    for (size_t i = 0; i < kDim.size(); ++i) {
        REQUIRE(f_wire_cost_map.dim_size(i) == kDim[i]);
    }

    for (size_t from_layer = 0; from_layer < kDim[0]; from_layer++) {
        for (size_t x = 0; x < kDim[1]; ++x) {
            for (size_t y = 0; y < kDim[2]; ++y) {
                for (size_t to_layer = 0; to_layer < kDim[3]; to_layer++) {
                    for (size_t z = 0; z < kDim[4]; ++z) {
                        for (size_t w = 0; w < kDim[5]; ++w) {
                            REQUIRE(f_wire_cost_map[from_layer][x][y][to_layer][z][w].delay == (x + 1) * (y + 1) * (z + 1) * (w + 1));
                            REQUIRE(f_wire_cost_map[from_layer][x][y][to_layer][z][w].congestion == 2 * (x + 1) * (y + 1) * (z + 1) * (w + 1));
                        }
                    }
                }
            }
        }
    }
}

#endif

} // namespace
