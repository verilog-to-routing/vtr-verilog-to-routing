#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <utility>

#include "rr_graph_builder.h"

#include "clock_network_builders.h"
#include "clock_connection_builders.h"
#include "rr_graph_type.h"

class ClockNetwork;
class ClockConnection;
class t_rr_graph_storage;

class SwitchPoint {
    /* A switch point object: keeps information on the location and rr_node indices
     * for a certain clock switch. clock connections are grouped with their own unique
     * name; this object holds information for only one such grouping.
     * Examples of SwitchPoint(s) are rib-to-spine, driver-to-spine. */
  public:
    // [grid_width][grid_height][0..nodes_at_this_location-1]
    std::vector<std::vector<std::vector<int>>> rr_node_indices;
    // Set of all the locations for this switch point. Used to quickly find
    // if the switch point exists at a certain location.
    std::set<std::pair<int, int>> locations; // x,y
  public:
    /** Accessors **/
    std::vector<int> get_rr_node_indices_at_location(int x, int y) const;

    std::set<std::pair<int, int>> get_switch_locations() const;

    /** Mutators **/
    void insert_node_idx(int x, int y, int node_idx);
};

class SwitchPoints {
    /* This Class holds a map from a uniquely named switch to all of its locations on
     * the device and its rr_node_indices. The location and rr_node_indices are stored
     * in the SwitchPoint object*/
  public:
    std::unordered_map<std::string, SwitchPoint> switch_point_name_to_switch_location;

  public:
    /** Accessors **/

    /* Example: x,y = middle of the chip, switch_point_name == name of main drive
     * of global clock spine, returns the rr_nodes of all the clock spines that
     * start the network there*/
    std::vector<int> get_rr_node_indices_at_location(std::string switch_point_name,
                                                     int x,
                                                     int y) const;

    std::set<std::pair<int, int>> get_switch_locations(std::string switch_point_name) const;

    /** Mutators **/
    void insert_switch_node_idx(std::string switch_point_name, int x, int y, int node_idx);
};

class ClockRRGraphBuilder {
  public:
    /* Returns the current ptc num where the wire should be drawn and updates the
     * channel width. Note: The ptc_num is determined by the channel width. The channel
     * width global state gets incremented everytime there is a request for a new ptc_num*/
    int get_and_increment_chanx_ptc_num();
    int get_and_increment_chany_ptc_num();

    /* Reverse lookup for to find the clock source and tap locations for each clock_network
     * The map key is the clock network name and value are all the switch points*/
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
        , chanx_ptc_idx_(0)
        , chany_ptc_idx_(0) {
    }

    const DeviceGrid& grid() const {
        return grid_;
    }

    /* Saves a map from switch rr_node idx -> {x, y} location */
    void add_switch_location(std::string clock_name,
                             std::string switch_point_name,
                             int x,
                             int y,
                             int node_index);

    /* Returns the rr_node idx of the switch at location {x, y} */
    std::vector<int> get_rr_node_indices_at_switch_location(std::string clock_name,
                                                            std::string switch_point_name,
                                                            int x,
                                                            int y) const;

    /* Returns all the switch locations for the a certain clock network switch */
    std::set<std::pair<int, int>> get_switch_locations(std::string clock_name,
                                                       std::string switch_point_name) const;

    void update_chan_width(t_chan_width* chan_width) const;

    static size_t estimate_additional_nodes(const DeviceGrid& grid);

    /* AA: map the segment indices in all networks to corresponding indices in axis based segment vectors as defined in build_rr_graph
     * Refer to clock_network_builders.h: map_relative_seg_indices*/

    static void map_relative_seg_indices(const t_unified_to_parallel_seg_index& indices_map);

    /***
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

  public:
    /* Creates the routing resource (rr) graph of the clock network and appends it to the
     * existing rr graph created in build_rr_graph for inter-block and intra-block routing. */
    void create_and_append_clock_rr_graph(int num_segments_x,
                                          t_rr_edge_info_set* rr_edges_to_create);

  private:
    /* loop over all of the clock networks and create their wires */
    void create_clock_networks_wires(const std::vector<std::unique_ptr<ClockNetwork>>& clock_networks,
                                     int num_segments_x,
                                     t_rr_edge_info_set* rr_edges_to_create);

    /* loop over all clock routing connections and create the switches and connections */
    void create_clock_networks_switches(const std::vector<std::unique_ptr<ClockConnection>>& clock_connections,
                                        t_rr_edge_info_set* rr_edges_to_create);

    const t_chan_width& chan_width_;
    const DeviceGrid& grid_;
    t_rr_graph_storage* rr_nodes_;
    RRGraphBuilder* rr_graph_builder_;

    int chanx_ptc_idx_;
    int chany_ptc_idx_;
};
