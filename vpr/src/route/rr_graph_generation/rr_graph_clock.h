#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <utility>

#include "vtr_ndmatrix.h"

#include "rr_graph_builder.h"

#include "clock_network_builders.h"
#include "clock_connection_builders.h"
#include "rr_graph_type.h"

class ClockNetwork;
class ClockConnection;
class t_rr_graph_storage;

/// @brief All the RR node indices at every device grid location for one uniquely-named
/// clock switch point (e.g. "rib-to-spine", "driver-to-spine"). One SwitchPoint groups
/// every RR node created for that switch point across every instance of its clock network.
class SwitchPoint {
  public:
    std::vector<std::vector<std::vector<int>>> rr_node_indices; ///< [grid_width][grid_height][0..nodes_at_this_location-1]
    std::set<std::pair<int, int>> locations;                    ///< Every (x,y) this switch point exists at, for fast membership checks.

  public:
    std::vector<int> get_rr_node_indices_at_location(int x, int y) const;
    std::set<std::pair<int, int>> get_switch_locations() const;

    void insert_node_idx(int x, int y, int node_idx);
};

/// @brief Maps every uniquely-named switch point of one clock network to its SwitchPoint
/// (locations + rr_node_indices).
class SwitchPoints {
  public:
    std::unordered_map<std::string, SwitchPoint> switch_point_name_to_switch_location;

  public:
    /// @brief Returns the rr_node indices of switch_point_name at (x,y); e.g. x,y =
    /// middle of the chip, switch_point_name == a global clock spine's main drive point,
    /// returns the rr_nodes of every clock spine that starts the network there.
    ///
    /// clock_name is only used to produce a helpful message if switch_point_name was
    /// never registered for this clock network (a malformed arch file, e.g. a
    /// <clock_routing> tap referencing a switch point whose offset never landed on any
    /// instance of the network; see the "does not correspond to any switch box
    /// location" warning emitted when that happens).
    std::vector<int> get_rr_node_indices_at_location(const std::string& clock_name,
                                                     std::string switch_point_name,
                                                     int x,
                                                     int y) const;

    std::set<std::pair<int, int>> get_switch_locations(const std::string& clock_name,
                                                       std::string switch_point_name) const;

    void insert_switch_node_idx(std::string switch_point_name, int x, int y, int node_idx);
};

/// @brief Builds the RR graph for every dedicated clock network declared in the
/// architecture and appends it to the RR graph built by build_rr_graph for inter-block
/// and intra-block routing. Also owns the reverse lookup (clock_name_to_switch_points)
/// from a clock network's switch points to the RR nodes at each of their locations,
/// which ClockConnection/ClockNetwork subclasses use to wire up taps/drives.
class ClockRRGraphBuilder {
  public:
    /// @brief Returns a ptc num reserved across the whole grid, i.e. guaranteed to collide
    /// with no other clock-network node anywhere on the device.
    int get_and_increment_chanx_ptc_num();
    int get_and_increment_chany_ptc_num();

    /// @brief Reserves and returns a ptc num that is guaranteed unique only among nodes
    /// touching some (x,y) with x in [x_lo,x_hi] and y in [y_lo,y_hi] (inclusive), i.e. the
    /// smallest value not already reserved anywhere in that box.
    int reserve_chanx_ptc(int x_lo, int x_hi, int y_lo, int y_hi);
    int reserve_chany_ptc(int x_lo, int x_hi, int y_lo, int y_hi);

    /// @brief Reverse lookup to find the clock source and tap locations for each clock
    /// network. The map key is the clock network name; the value holds all its switch points.
    std::unordered_map<std::string, SwitchPoints> clock_name_to_switch_points;

  public:
    ClockRRGraphBuilder(
        const t_chan_width& chan_width,
        const DeviceGrid& grid,
        t_rr_graph_storage* rr_nodes,
        RRGraphBuilder* rr_graph_builder)
        : chan_width_(chan_width)
        , grid_(grid)
        , rr_nodes_(rr_nodes)
        , rr_graph_builder_(rr_graph_builder)
        , chanx_next_free_ptc_({grid.width(), grid.height()}, 0)
        , chany_next_free_ptc_({grid.width(), grid.height()}, 0) {
    }

    const DeviceGrid& grid() const {
        return grid_;
    }

    /// @brief Saves a map from switch rr_node idx -> {x, y} location.
    void add_switch_location(std::string clock_name,
                             std::string switch_point_name,
                             int x,
                             int y,
                             int node_index);

    /// @brief Returns the rr_node idx of the switch at location {x, y}.
    ///
    /// If `required` is true (the default), a clock network/switch point that doesn't
    /// reach (x,y) is treated as an arch mistake and raises a fatal error; appropriate
    /// for callers connecting to one specific, arch-declared coordinate (e.g. a <tap>'s
    /// locationx/locationy). Pass `required=false` for callers that probe many candidate
    /// locations and expect most of them to legitimately not be reached (e.g.
    /// ClockToPinsConnection scanning every tile in the device, where a quadrant-scoped
    /// clock network is only expected to reach tiles within its own quadrant), in that
    /// case a miss just means "not reachable from here" and an empty vector is returned.
    std::vector<int> get_rr_node_indices_at_switch_location(std::string clock_name,
                                                            std::string switch_point_name,
                                                            int x,
                                                            int y,
                                                            bool required = true) const;

    /// @brief Returns all the switch locations for a certain clock network switch.
    std::set<std::pair<int, int>> get_switch_locations(std::string clock_name,
                                                       std::string switch_point_name) const;

    void update_chan_width(t_chan_width* chan_width) const;

    static size_t estimate_additional_nodes(const DeviceGrid& grid);

    /// @brief Maps every clock network's segment indices to their equivalent indices in
    /// the axis-specific segment vectors defined in build_rr_graph. See
    /// ClockNetwork::map_relative_seg_indices for the full rationale.
    static void map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map);

    /**
     * @brief Add an edge to the rr graph
     * @param rr_edges_to_create The interface to rr-graph builder
     * @param src_node End point of the edge
     * @param sink_node Start point of the edge
     * @param arch_switch_idx
     * @param edge_remapped Indicate whether the edge idx refer to arch edge idx or rr graph edge idx. Currently, we only support arch edge idx
     */
    void add_edge(t_rr_edge_info_set* rr_edges_to_create,
                  RRNodeId src_node,
                  RRNodeId sink_node,
                  int arch_switch_idx,
                  bool edge_remapped) const;

    /// @brief Creates the routing resource (rr) graph of the clock network and appends it
    /// to the existing rr graph created in build_rr_graph for inter-block and intra-block
    /// routing.
    void create_and_append_clock_rr_graph(int num_segments_x,
                                          t_rr_edge_info_set* rr_edges_to_create);

  private:
    /// @brief Loops over all of the clock networks and creates their wires.
    void create_clock_networks_wires(const std::vector<std::unique_ptr<ClockNetwork>>& clock_networks,
                                     int num_segments_x,
                                     t_rr_edge_info_set* rr_edges_to_create);

    /// @brief Loops over all clock routing connections and creates their switches/edges.
    void create_clock_networks_switches(const std::vector<std::unique_ptr<ClockConnection>>& clock_connections,
                                        t_rr_edge_info_set* rr_edges_to_create);

    const t_chan_width& chan_width_;
    const DeviceGrid& grid_;
    t_rr_graph_storage* rr_nodes_;
    RRGraphBuilder* rr_graph_builder_;

    // Per-(x,y) next unreserved local ptc value, offset by chan_width_.x_max/y_max to form
    // an actual ptc num; see reserve_chanx_ptc/reserve_chany_ptc.
    vtr::NdMatrix<int, 2> chanx_next_free_ptc_;
    vtr::NdMatrix<int, 2> chany_next_free_ptc_;
};
