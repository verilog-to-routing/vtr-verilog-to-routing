#include "rr_graph_clock.h"

#include "globals.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_area.h"
#include "rr_graph_util.h"
#include "rr_graph_indexed_data.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_error.h"

void ClockRRGraphBuilder::create_and_append_clock_rr_graph(std::vector<t_segment_inf>& segment_inf,
                                                           const float R_minW_nmos,
                                                           const float R_minW_pmos,
                                                           int wire_to_rr_ipin_switch,
                                                           const enum e_base_cost_type base_cost_type) {
    vtr::printf_info("Starting clock network routing resource graph generation...\n");
    clock_t begin = clock();

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto* chan_width = &device_ctx.chan_width;
    auto& clock_networks = device_ctx.clock_networks;
    auto& clock_routing = device_ctx.clock_connections;

    size_t clock_nodes_start_idx = device_ctx.rr_nodes.size();

    ClockRRGraphBuilder clock_graph = ClockRRGraphBuilder();
    clock_graph.create_clock_networks_wires(clock_networks, segment_inf.size());
    clock_graph.create_clock_networks_switches(clock_routing);

    // Reset fanin to account for newly added clock rr_nodes
    init_fan_in(device_ctx.rr_nodes, device_ctx.rr_nodes.size());

    clock_graph.add_rr_switches_and_map_to_nodes(clock_nodes_start_idx, R_minW_nmos, R_minW_pmos);

    // "Partition the rr graph edges for efficient access to configurable/non-configurable
    //  edge subsets. Must be done after RR switches have been allocated"
    partition_rr_graph_edges(device_ctx);

    alloc_and_load_rr_indexed_data(segment_inf, device_ctx.rr_node_indices,
                                   chan_width->max, wire_to_rr_ipin_switch, base_cost_type);

    float elapsed_time = (float)(clock() - begin) / CLOCKS_PER_SEC;
    vtr::printf_info("Building clock network resource graph took %g seconds\n", elapsed_time);
}

// Clock network information comes from the arch file
void ClockRRGraphBuilder::create_clock_networks_wires(std::vector<std::unique_ptr<ClockNetwork>>& clock_networks,
                                                      int num_segments) {
    // Add rr_nodes for each clock network wire
    for (auto& clock_network : clock_networks) {
        clock_network->create_rr_nodes_for_clock_network_wires(*this, num_segments);
    }

    // Reduce the capacity of rr_nodes for performance
    auto& rr_nodes = g_vpr_ctx.mutable_device().rr_nodes;
    rr_nodes.shrink_to_fit();
}

// Clock switch information comes from the arch file
void ClockRRGraphBuilder::create_clock_networks_switches(std::vector<std::unique_ptr<ClockConnection>>& clock_connections) {
    for (auto& clock_connection : clock_connections) {
        clock_connection->create_switches(*this);
    }
}

void ClockRRGraphBuilder::add_rr_switches_and_map_to_nodes(size_t node_start_idx,
                                                           const float R_minW_nmos,
                                                           const float R_minW_pmos) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_nodes = device_ctx.rr_nodes;

    // Check to see that clock nodes were sucessfully appended to rr_nodes
    VTR_ASSERT(rr_nodes.size() > node_start_idx);

    std::unordered_map<int, int> arch_switch_to_rr_switch;

    // The following assumes that arch_switch was specified earlier when the edges where added
    for (size_t node_idx = node_start_idx; node_idx < rr_nodes.size(); node_idx++) {
        auto& from_node = rr_nodes[node_idx];
        for (t_edge_size edge_idx = 0; edge_idx < from_node.num_edges(); edge_idx++) {
            int arch_switch_idx = from_node.edge_switch(edge_idx);

            int rr_switch_idx;
            auto itter = arch_switch_to_rr_switch.find(arch_switch_idx);
            if (itter == arch_switch_to_rr_switch.end()) {
                rr_switch_idx = add_rr_switch_from_arch_switch_inf(arch_switch_idx,
                                                                   R_minW_nmos,
                                                                   R_minW_pmos);
                arch_switch_to_rr_switch[arch_switch_idx] = rr_switch_idx;
            } else {
                rr_switch_idx = itter->second;
            }

            from_node.set_edge_switch(edge_idx, rr_switch_idx);
        }
    }

    device_ctx.rr_switch_inf.shrink_to_fit();
}

int ClockRRGraphBuilder::add_rr_switch_from_arch_switch_inf(int arch_switch_idx,
                                                            const float R_minW_nmos,
                                                            const float R_minW_pmos) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& rr_switch_inf = device_ctx.rr_switch_inf;
    auto& arch_switch_inf = device_ctx.arch_switch_inf;

    rr_switch_inf.emplace_back();
    int rr_switch_idx = rr_switch_inf.size() - 1;

    // TODO: Add support for non fixed Tdel based on fanin information
    //       and move assigning Tdel into add_rr_switch
    VTR_ASSERT(arch_switch_inf[arch_switch_idx].fixed_Tdel());
    int fanin = UNDEFINED;

    load_rr_switch_from_arch_switch(arch_switch_idx, rr_switch_idx, fanin, R_minW_nmos, R_minW_pmos);

    return rr_switch_idx;
}

void ClockRRGraphBuilder::add_switch_location(std::string clock_name,
                                              std::string switch_point_name,
                                              int x,
                                              int y,
                                              int node_index) {
    // Note use of operator[] will automatically insert clock name if it doesn't exist
    clock_name_to_switch_points[clock_name].insert_switch_node_idx(switch_point_name, x, y, node_index);
}

void SwitchPoints::insert_switch_node_idx(std::string switch_point_name, int x, int y, int node_idx) {
    // Note use of operator[] will automatically insert switch name if it doesn't exit
    switch_point_name_to_switch_location[switch_point_name].insert_node_idx(x, y, node_idx);
}

void SwitchPoint::insert_node_idx(int x, int y, int node_idx) {
    // allocate 2d vector of grid size
    if (rr_node_indices.empty()) {
        auto& grid = g_vpr_ctx.device().grid;
        rr_node_indices.resize(grid.width());
        for (size_t i = 0; i < grid.width(); i++) {
            rr_node_indices[i].resize(grid.height());
        }
    }

    // insert node_idx at location
    rr_node_indices[x][y].push_back(node_idx);
    locations.insert({x, y});
}

std::vector<int> ClockRRGraphBuilder::get_rr_node_indices_at_switch_location(std::string clock_name,
                                                                             std::string switch_point_name,
                                                                             int x,
                                                                             int y) const {
    auto itter = clock_name_to_switch_points.find(clock_name);

    // assert that clock name exists in map
    VTR_ASSERT(itter != clock_name_to_switch_points.end());

    auto& switch_points = itter->second;
    return switch_points.get_rr_node_indices_at_location(switch_point_name, x, y);
}

std::vector<int> SwitchPoints::get_rr_node_indices_at_location(std::string switch_point_name,
                                                               int x,
                                                               int y) const {
    auto itter = switch_point_name_to_switch_location.find(switch_point_name);

    // assert that switch name exists in map
    VTR_ASSERT(itter != switch_point_name_to_switch_location.end());

    auto& switch_point = itter->second;
    std::vector<int> rr_node_indices = switch_point.get_rr_node_indices_at_location(x, y);
    return rr_node_indices;
}

std::vector<int> SwitchPoint::get_rr_node_indices_at_location(int x, int y) const {
    // assert that switch is connected to nodes at the location
    VTR_ASSERT(!rr_node_indices[x][y].empty());

    return rr_node_indices[x][y];
}

std::set<std::pair<int, int>> ClockRRGraphBuilder::get_switch_locations(std::string clock_name,
                                                                        std::string switch_point_name) const {
    auto itter = clock_name_to_switch_points.find(clock_name);

    // assert that clock name exists in map
    VTR_ASSERT(itter != clock_name_to_switch_points.end());

    auto& switch_points = itter->second;
    return switch_points.get_switch_locations(switch_point_name);
}

std::set<std::pair<int, int>> SwitchPoints::get_switch_locations(std::string switch_point_name) const {
    auto itter = switch_point_name_to_switch_location.find(switch_point_name);

    // assert that switch name exists in map
    VTR_ASSERT(itter != switch_point_name_to_switch_location.end());

    auto& switch_point = itter->second;
    return switch_point.get_switch_locations();
}

std::set<std::pair<int, int>> SwitchPoint::get_switch_locations() const {
    // assert that switch is connected to nodes at the location
    VTR_ASSERT(!locations.empty());

    return locations;
}

int ClockRRGraphBuilder::get_and_increment_chanx_ptc_num() {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& grid = device_ctx.grid;
    auto* channel_width = &device_ctx.chan_width;

    // ptc_num is determined by the channel width
    // The channel width lets the drawing engine how much to space the LBs appart
    int ptc_num = channel_width->x_max++;
    if (channel_width->x_max > channel_width->max) {
        channel_width->max = channel_width->x_max;
    }

    for (size_t i = 0; i < grid.height(); ++i) {
        device_ctx.chan_width.x_list[i]++;
    }

    return ptc_num;
}

int ClockRRGraphBuilder::get_and_increment_chany_ptc_num() {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& grid = device_ctx.grid;
    auto* channel_width = &device_ctx.chan_width;

    // ptc_num is determined by the channel width
    // The channel width lets the drawing engine how much to space the LBs appart
    int ptc_num = channel_width->y_max++;
    if (channel_width->y_max > channel_width->max) {
        channel_width->max = channel_width->y_max;
    }

    for (size_t i = 0; i < grid.width(); ++i) {
        device_ctx.chan_width.y_list[i]++;
    }

    return ptc_num;
}
