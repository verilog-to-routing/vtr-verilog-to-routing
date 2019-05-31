#ifndef RR_GRAPH_CLOCK_H
#define RR_GRAPH_CLOCK_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <utility>

#include "clock_fwd.h"

#include "clock_network_builders.h"
#include "clock_connection_builders.h"

class ClockNetwork;
class ClockConnection;

class SwitchPoint {
    /* A switch point object: keeps information on the location and and rr_node indices
     * for a certain clock switch. clock connections are grouped with their own unique
     * name; this object holds information for only one such grouping.
     * Examples of SwitchPoint(s) are rib-to-spine, driver-to-spine. */
  public:
    // [grid_width][grid_height][0..nodes_at_this_location-1]
    std::vector<std::vector<std::vector<int>>> rr_node_indices;
    // Set of all the locations for this switch point. Used to quickly find
    // if the switch point exists at a certian location.
    std::set<std::pair<int, int>> locations; // x,y
  public:
    /** Getters **/
    std::vector<int> get_rr_node_indices_at_location(int x, int y) const;

    std::set<std::pair<int, int>> get_switch_locations() const;

    /** Setters **/
    void insert_node_idx(int x, int y, int node_idx);
};

class SwitchPoints {
    /* This Class holds a map from a uniquely named switch to all of its locations on
     * the device and its rr_node_indices. The location and rr_node_indices are stored
     * in the SwitchPoint object*/
  public:
    std::unordered_map<std::string, SwitchPoint> switch_point_name_to_switch_location;

  public:
    /** Getters **/

    /* Example: x,y = middle of the chip, switch_point_name == name of main drive
     * of global clock spine, returns the rr_nodes of all the clock spines that
     * start the newtork there*/
    std::vector<int> get_rr_node_indices_at_location(std::string switch_point_name,
                                                     int x,
                                                     int y) const;

    std::set<std::pair<int, int>> get_switch_locations(std::string switch_point_name) const;

    /** Setters **/
    void insert_switch_node_idx(std::string switch_point_name, int x, int y, int node_idx);
};

class ClockRRGraphBuilder {
  public:
    /* Returns the current ptc num where the wire should be drawn and updates the
     * channel width. Note: The ptc_num is determined by the channel width. The channel
     * width global state gets incremented everytime there is a request for a new ptc_num*/
    int get_and_increment_chanx_ptc_num();
    int get_and_increment_chany_ptc_num();

  public:
    /* Reverse lookup for to find the clock source and tap locations for each clock_network
     * The map key is the the clock network name and value are all the switch points*/
    std::unordered_map<std::string, SwitchPoints> clock_name_to_switch_points;

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

  public:
    /* Creates the routing resourse (rr) graph of the clock network and appends it to the
     * existing rr graph created in build_rr_graph for inter-block and intra-block routing. */
    static void create_and_append_clock_rr_graph(std::vector<t_segment_inf>& segment_inf,
                                                 const float R_minW_nmos,
                                                 const float R_minW_pmos,
                                                 int wire_to_rr_ipin_switch,
                                                 const enum e_base_cost_type base_cost_type);

  private:
    /* loop over all of the clock networks and create their wires */
    void create_clock_networks_wires(std::vector<std::unique_ptr<ClockNetwork>>& clock_networks,
                                     int num_segments);

    /* loop over all clock routing connections and create the switches and connections */
    void create_clock_networks_switches(std::vector<std::unique_ptr<ClockConnection>>& clock_connections);

    /* Adds the architecture switches that the clock rr_nodes use to the rr switches and
     * maps the newly added rr_switches to the nodes.
     * The input nodes_start_idx ~ corresponds to the rr_node index of the first node
     * used to create the clock network. Every node from node_start_idx..rr_nodes.size-1
     * is a node in the clock network.*/
    // TODO: Change to account for swtich fanin. Note: this function is simular to
    //       remap_rr_node_switch_indices but does not take into account node fanin.
    void add_rr_switches_and_map_to_nodes(size_t nodes_start_idx,
                                          const float R_minW_nmos,
                                          const float R_minW_pmos);

    /* Returns the index of the newly added rr_switch. The rr_switch information is coppied
     * in from the arch_switch information */
    // TODO: Does not account for fanin information when copping Tdel. Note: this function
    //       is simular to load_rr_switch_inf but does not take into account node fanin.
    int add_rr_switch_from_arch_switch_inf(int arch_switch_idx,
                                           const float R_minW_nmos,
                                           const float R_minW_pmos);
};

#endif
