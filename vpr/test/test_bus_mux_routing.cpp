#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"

#include "bus_mux_route_types.h"
#include "bus_mux_routing.h"
#include "vtr_vector.h"

namespace {

using Catch::Matchers::WithinRel;

constexpr float ACC_FAC = 1.f;

/// @brief Create routing state for one mux with two input sets and three output bits.
vtr::vector<BusMuxId, t_bus_mux_route_inf> make_one_mux() {
    vtr::vector<BusMuxId, t_bus_mux_route_inf> bus_mux_route_inf(1);
    bus_mux_route_inf[BusMuxId(0)].init(/*num_sets=*/2, /*num_bits=*/3);
    return bus_mux_route_inf;
}

/// @brief Increment occupancy for output bit `bit` through input set `set`.
void use_edge(t_bus_mux_route_inf& mux_inf, int bit, int set) {
    mux_inf.set_occ[set]++;
    mux_inf.edge_occ[mux_inf.edge_index(bit, set)]++;
}

TEST_CASE("bus_mux_edge_history_grows_on_used_edges_of_a_split_mux", "[vpr]") {
    vtr::vector<BusMuxId, t_bus_mux_route_inf> bus_mux_route_inf = make_one_mux();
    t_bus_mux_route_inf& mux_inf = bus_mux_route_inf[BusMuxId(0)];

    // Bits 0 and 1 use input set 0; bit 2 uses input set 1.
    use_edge(mux_inf, 0, 0);
    use_edge(mux_inf, 1, 0);
    use_edge(mux_inf, 2, 1);
    REQUIRE(mux_inf.is_control_congested());

    pathfinder_update_bus_mux_acc_cost(bus_mux_route_inf, ACC_FAC);
    pathfinder_update_bus_mux_acc_cost(bus_mux_route_inf, ACC_FAC);

    // Only occupied edges accumulate history cost.
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(0, 0)], WithinRel(1.f + 2.f * ACC_FAC));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(1, 0)], WithinRel(1.f + 2.f * ACC_FAC));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(2, 1)], WithinRel(1.f + 2.f * ACC_FAC));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(0, 1)], WithinRel(1.f));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(1, 1)], WithinRel(1.f));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(2, 0)], WithinRel(1.f));
}

TEST_CASE("bus_mux_edge_history_fades_once_the_mux_agrees", "[vpr]") {
    vtr::vector<BusMuxId, t_bus_mux_route_inf> bus_mux_route_inf = make_one_mux();
    t_bus_mux_route_inf& mux_inf = bus_mux_route_inf[BusMuxId(0)];

    use_edge(mux_inf, 0, 0);
    use_edge(mux_inf, 1, 0);
    use_edge(mux_inf, 2, 1);
    pathfinder_update_bus_mux_acc_cost(bus_mux_route_inf, ACC_FAC);
    pathfinder_update_bus_mux_acc_cost(bus_mux_route_inf, ACC_FAC);

    // All three bits use input set 0.
    mux_inf.clear_occ();
    use_edge(mux_inf, 0, 0);
    use_edge(mux_inf, 1, 0);
    use_edge(mux_inf, 2, 0);
    REQUIRE_FALSE(mux_inf.is_control_congested());

    // Each update halves every edge's history cost above 1.
    pathfinder_update_bus_mux_acc_cost(bus_mux_route_inf, ACC_FAC);
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(0, 0)], WithinRel(1.f + ACC_FAC));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(2, 1)], WithinRel(1.f + ACC_FAC));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(2, 0)], WithinRel(1.f));

    pathfinder_update_bus_mux_acc_cost(bus_mux_route_inf, ACC_FAC);
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(0, 0)], WithinRel(1.f + ACC_FAC / 2.f));
}

TEST_CASE("bus_mux_edge_history_does_not_grow_with_a_zero_factor", "[vpr]") {
    vtr::vector<BusMuxId, t_bus_mux_route_inf> bus_mux_route_inf = make_one_mux();
    t_bus_mux_route_inf& mux_inf = bus_mux_route_inf[BusMuxId(0)];

    use_edge(mux_inf, 0, 0);
    use_edge(mux_inf, 2, 1);
    pathfinder_update_bus_mux_acc_cost(bus_mux_route_inf, /*acc_fac=*/0.f);

    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(0, 0)], WithinRel(1.f));
    REQUIRE_THAT(mux_inf.edge_acc_cost[mux_inf.edge_index(2, 1)], WithinRel(1.f));
}

} // namespace
