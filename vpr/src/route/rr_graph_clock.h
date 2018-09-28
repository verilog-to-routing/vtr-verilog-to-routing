#ifndef RR_GRAPH_CLOCK_H
#define RR_GRAPH_CLOCK_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include <utility>

#include "globals.h"

#include "clock_fwd.h"

#include "clock_network_types.h"
#include "clock_connection_types.h"

class ClockNetwork;
class ClockConnection;

class SwitchPoint {
    public:
        // [grid_width][grid_height][nodes]
        std::vector<std::vector<std::vector<int>>> rr_node_indices;
        std::set<std::pair<int, int>> locations; // x,y
    public:
        /** Getters **/
        std::vector<int> get_rr_node_indices_at_location(int x, int y) const;

        std::set<std::pair<int, int>> get_switch_locations() const;

        /** Setters **/
        void insert_node_idx(int x, int y, int node_idx);
};

class SwitchPoints {
    public:
        std::unordered_map<std::string, SwitchPoint> switch_name_to_switch_location;
    public:
        /** Getters **/
        std::vector<int> get_rr_node_indices_at_location(
            std::string switch_name,
            int x,
            int y) const;

        std::set<std::pair<int, int>> get_switch_locations(std::string switch_name) const;

        /** Setters **/
        void insert_switch_node_idx(std::string switch_name, int x, int y, int node_idx);
};

class ClockRRGraph {

    public: //constructor
        /* Constructor calls set_ptc_nums() member function which is used to keep track of the ptc
           number that should be used next. */
        ClockRRGraph(int channel_width);

    /* Drawing Related Variables and functions */
    private:
        /* The next ptc number to be used for x-channel and/or y-channel wire drawing */
        int chanx_next_ptc;
        int chany_next_ptc;
    private:
        /* Sets the initial chanx and the chany ptc numbers based on the channel width
           of the inter-block routing. Function should only be called durring construction
           of this class */
        void set_ptc_nums(int channel_width);
    public:
        /* Return the current ptc num where the wire should be drawn and updates the ptc count by
           one. */
        int get_and_increment_chanx_ptc_num();
        int get_and_increment_chany_ptc_num();


    public:
        /* Reverse lookup for to find the clock source and tap locations for each clock_network
           The map key is the the clock network name and value are all the switch points*/
        std::unordered_map<std::string, SwitchPoints> clock_name_to_switch_points;

        void add_switch_location(
            std::string clock_name,
            std::string switch_name,
            int x,
            int y,
            int node_index);

        std::vector<int> get_rr_node_indices_at_switch_location(
            std::string clock_name,
            std::string switch_name,
            int x,
            int y) const;

        std::set<std::pair<int, int>> get_switch_locations(
            std::string clock_name,
            std::string switch_name) const;

    public:
        /* Creates the routing resourse (rr) graph of the clock network and appends it to the
           existing rr graph created in build_rr_graph for inter-block and intra-block routing. */
        static void create_and_append_clock_rr_graph();

    private:
        /* Dummy clock network that connects every I/O input to every clock pin. */
        static void create_star_model_network();

        /* loop over all of the clock networks and create their wires */
        void create_clock_networks_wires(std::vector<std::unique_ptr<ClockNetwork>>& clock_networks);

        /* loop over all clock routing connections and create the switches and connections */
        void create_clock_networks_switches(
            std::vector<std::unique_ptr<ClockConnection>>& clock_connections);
};

#endif

