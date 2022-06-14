#include "rr_graph_clock.h"

#include "globals.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_area.h"
#include "rr_graph_util.h"
#include "rr_graph_indexed_data.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "vpr_error.h"

void ClockRRGraphBuilder::create_and_append_clock_rr_graph(int num_seg_types_x,
                                                           t_rr_edge_info_set* rr_edges_to_create) {
    vtr::ScopedStartFinishTimer timer("Build clock network routing resource graph");

    const auto& device_ctx = g_vpr_ctx.device();
    auto& clock_networks = device_ctx.clock_networks;
    auto& clock_routing = device_ctx.clock_connections;

    create_clock_networks_wires(clock_networks, num_seg_types_x, rr_edges_to_create);
    create_clock_networks_switches(clock_routing, rr_edges_to_create);
}

// Clock network information comes from the arch file
void ClockRRGraphBuilder::create_clock_networks_wires(const std::vector<std::unique_ptr<ClockNetwork>>& clock_networks,
                                                      int num_segments_x,
                                                      t_rr_edge_info_set* rr_edges_to_create) {
    // Add rr_nodes for each clock network wire
    for (auto& clock_network : clock_networks) {
        clock_network->create_rr_nodes_for_clock_network_wires(*this, rr_nodes_, *rr_graph_builder_, rr_edges_to_create, num_segments_x);
    }

    // Reduce the capacity of rr_nodes for performance
    rr_nodes_->shrink_to_fit();
}

// Clock switch information comes from the arch file
void ClockRRGraphBuilder::create_clock_networks_switches(const std::vector<std::unique_ptr<ClockConnection>>& clock_connections,
                                                         t_rr_edge_info_set* rr_edges_to_create) {
    for (auto& clock_connection : clock_connections) {
        clock_connection->create_switches(*this, rr_edges_to_create);
    }
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
    // ptc_num is determined by the channel width
    // The channel width lets the drawing engine how much to space the LBs appart
    int ptc_num = chan_width_.x_max + (chanx_ptc_idx_++);
    return ptc_num;
}

int ClockRRGraphBuilder::get_and_increment_chany_ptc_num() {
    // ptc_num is determined by the channel width
    // The channel width lets the drawing engine how much to space the LBs appart
    int ptc_num = chan_width_.y_max + (chany_ptc_idx_++);
    return ptc_num;
}

void ClockRRGraphBuilder::update_chan_width(t_chan_width* chan_width) const {
    chan_width->x_max += chanx_ptc_idx_;
    chan_width->y_max += chany_ptc_idx_;
    chan_width->max = std::max(chan_width->max, chan_width->x_max);
    chan_width->max = std::max(chan_width->max, chan_width->y_max);

    for (size_t i = 0; i < grid_.height(); ++i) {
        chan_width->x_list[i] += chanx_ptc_idx_;
    }
    for (size_t i = 0; i < grid_.width(); ++i) {
        chan_width->y_list[i] += chany_ptc_idx_;
    }
}

size_t ClockRRGraphBuilder::estimate_additional_nodes(const DeviceGrid& grid) {
    size_t num_additional_nodes = 0;

    const auto& device_ctx = g_vpr_ctx.device();
    auto& clock_networks = device_ctx.clock_networks;
    auto& clock_routing = device_ctx.clock_connections;

    for (auto& clock_network : clock_networks) {
        num_additional_nodes += clock_network->estimate_additional_nodes(grid);
    }
    for (auto& clock_connection : clock_routing) {
        num_additional_nodes += clock_connection->estimate_additional_nodes();
    }

    return num_additional_nodes;
}

void ClockRRGraphBuilder::map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map) {
    const auto& device_ctx = g_vpr_ctx.device();

    for (auto& clock_network : device_ctx.clock_networks)
        clock_network->map_relative_seg_indices(indices_map);
}

void ClockRRGraphBuilder::add_edge(t_rr_edge_info_set* rr_edges_to_create,
                                   RRNodeId src_node,
                                   RRNodeId sink_node,
                                   int arch_switch_idx) const {
    const auto& device_ctx = g_vpr_ctx.device();
    VTR_ASSERT(arch_switch_idx < device_ctx.num_arch_switches);
    rr_edges_to_create->emplace_back(src_node, sink_node, arch_switch_idx);

    const auto& sw = device_ctx.arch_switch_inf[arch_switch_idx];
    if (!sw.buffered() && !sw.configurable()) {
        // This is short, create a reverse edge.
        rr_edges_to_create->emplace_back(sink_node, src_node, arch_switch_idx);
    }
}
