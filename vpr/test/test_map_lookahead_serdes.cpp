#include "catch2/catch_test_macros.hpp"

#include "router_lookahead_map.h"

extern t_wire_cost_map f_wire_cost_map;

namespace {

#ifdef VTR_ENABLE_CAPNPROTO
static constexpr const char kMapLookaheadBin[] = "test_map_lookahead.bin";

TEST_CASE("round_trip_map_lookahead", "[vpr]") {
    constexpr size_t num_layers = 1;
    constexpr std::array<size_t, 6> kDim({num_layers, num_layers, 10, 12, 15, 16});

    f_wire_cost_map.resize(kDim);
    for (size_t from_layer = 0; from_layer < kDim[0]; from_layer++) {
        for (size_t to_layer = 0; to_layer < kDim[1]; to_layer++) {
            for (size_t z = 0; z < kDim[2]; ++z) {
                for (size_t w = 0; w < kDim[3]; ++w) {
                    for (size_t x = 0; x < kDim[4]; ++x) {
                        for (size_t y = 0; y < kDim[5]; ++y) {
                            f_wire_cost_map[from_layer][to_layer][z][w][x][y].delay = (x + 1) * (y + 1) * (z + 1) * (w + 1);
                            f_wire_cost_map[from_layer][to_layer][z][w][x][y].congestion = 2 * (x + 1) * (y + 1) * (z + 1) * (w + 1);
                        }
                    }
                }
            }
        }
    }

    write_router_lookahead(kMapLookaheadBin);

    for (size_t from_layer = 0; from_layer < kDim[0]; from_layer++) {
        for (size_t to_layer = 0; to_layer < kDim[1]; to_layer++) {
            for (size_t z = 0; z < kDim[2]; ++z) {
                for (size_t w = 0; w < kDim[3]; ++w) {
                    for (size_t x = 0; x < kDim[4]; ++x) {
                        for (size_t y = 0; y < kDim[5]; ++y) {
                            f_wire_cost_map[from_layer][to_layer][z][w][x][y].delay = 0.f;
                            f_wire_cost_map[from_layer][to_layer][z][w][x][y].congestion = 0.f;
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
        for (size_t to_layer = 0; to_layer < kDim[1]; to_layer++) {
            for (size_t z = 0; z < kDim[2]; ++z) {
                for (size_t w = 0; w < kDim[3]; ++w) {
                    for (size_t x = 0; x < kDim[4]; ++x) {
                        for (size_t y = 0; y < kDim[5]; ++y) {
                            REQUIRE(f_wire_cost_map[from_layer][to_layer][z][w][x][y].delay == (x + 1) * (y + 1) * (z + 1) * (w + 1));
                            REQUIRE(f_wire_cost_map[from_layer][to_layer][z][w][x][y].congestion == 2 * (x + 1) * (y + 1) * (z + 1) * (w + 1));
                        }
                    }
                }
            }
        }
    }
}

#endif

} // namespace
