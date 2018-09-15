#ifndef RR_GRAPH_CLOCK_H
#define RR_GRAPH_CLOCK_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "globals.h"
#include "clock_network_types.h"

class ClockNetwork;

struct Coordinates {
    int x;
    int y;
};

/* Data Structure that contatin*/
struct ClockConnection {

    std::string from;
    std::string to;
    int iswitch;
    int fc;

    // used only to connect from local routing to the clock drive point
    Coordinates switch_location;
};


class SwitchPoint {
    public:
        // [grid_width][grid_height][nodes]
        std::vector<std::vector<std::vector<int>>> rr_node_indices;
        std::vector<Coordinates> locations;
    public:
        /*
         * Constructors
         */
        SwitchPoint() {};
        SwitchPoint(int grid_width, int grid_height) :
            rr_node_indices(
                    grid_width,
                    std::vector<std::vector<int>>(grid_height, std::vector<int>())) {};
        void insert_node_idx(int x, int y, int node_idx) {
            rr_node_indices[x][y].push_back(node_idx);
            Coordinates location = {x, y};
            locations.push_back(location);
        }
};

class SwitchPoints {
    public:
        std::unordered_map<std::string, SwitchPoint> switch_name_to_switch_location;
    public:
        /** Getters **/

        /** Setters **/
        void insert_switch(std::string switch_name, SwitchPoint switch_point) {
            // TODO make sure every switch name is unique
            switch_name_to_switch_location.emplace(switch_name, switch_point);
        }
        void insert_switch_node_idx(std::string switch_name, int x, int y, int node_index) {
            switch_name_to_switch_location[switch_name].insert_node_idx(x, y, node_index);
        }
};

class ClockRRGraph {
    public:
        /* Reverse lookup for to find the clock source and tap locations for each clock_network
           The map key is the the clock network name and value are all the switch points*/
        std::unordered_map<std::string, SwitchPoints> clock_name_to_switch_points;

    public:
        /* Creates the routing resourse (rr) graph of the clock network and appends it to the
           existing rr graph created in build_rr_graph for inter-block and intra-block routing. */
        static void create_and_append_clock_rr_graph();

    public:
        void allocate_lookup(const std::vector<std::unique_ptr<ClockNetwork>>& clock_networks);
        void add_switch_location(
                std::string clock_name,
                std::string switch_name,
                int x,
                int y,
                int node_index);
    private:
        /* Dummy clock network that connects every I/O input to every clock pin. */
        static void create_star_model_network();

        /* Create the wires of the clock network, supports rib and spines (TODO: suport H-trees).
            TODO: will need to include buffers or have a sperate buffer insertion function */
        /* loop over all of the clock networks and create their wires */
        void create_clock_networks_wires(std::vector<std::unique_ptr<ClockNetwork>>& clock_networks);

        /* loop over all clock routing connections and create the switches and connections */
        static void create_clock_networks_switches(std::vector<ClockConnection>& clock_connections);

        /* Connects the switches and connections between clock network wires */
        static void create_clock_network_switches(ClockConnection& clock_connection);

        /* Connects the inter-block routing to the clock source at the specified coordinates */
        static void create_switches_from_routing_to_clock_source(
                const Coordinates& clock_source_coordinates,
                std::string& clock_network_name,
                const int fc);

        /* Connects the clock tap to a clock source/root */
        static void create_switches_from_clock_tap_to_clock_source(
                std::string clock_network_tap,
                std::string clock_network_source,
                const int fc);

        /* Connects the clock tap to the block clock pins */
        static void create_switches_from_clock_tap_to_clock_pins(
                std::string clock_network_tap,
                const int fc);

        /* Connects inter-block routing to the clock pins*/
        static void create_switches_from_routing_to_clock_pins(const int fc);

};

#endif

