#ifndef RR_GRAPH_CLOCK_H
#define RR_GRAPH_CLOCK_H

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

struct t_coordinates {
    int x;
    int y;
};

enum e_clock_type {
    spine,
    rib,
    h_tree
};

enum e_switch_type {
    source,
    tap
};

struct t_metal_layer {
    float Rmetal;
    float Cmetal;
};

struct t_clock_connections {
    std::string from;
    std::string to;
    int fc;
};

class t_clock_network_description {
    protected:
        enum e_clock_type type;
        std::string clock_name;
        int num_inst;
        int repeat_x;
        int repeat_y;
    public:
        virtual ~t_clock_network_description() = 0;
        virtual void create_rr_nodes_for_clock_network_wires() = 0;
};

class t_clock_ribs_description : private t_clock_network_description {
    private:
        t_metal_layer metal_layer;
        int y_pos;
        int start_x;
        int end_x;
        int source_x_offset;
        int tap_x_offset;
        int tap_x_incr;
    public:
        void create_rr_nodes_for_clock_network_wires();
};

class t_clock_spines_description : private t_clock_network_description {
    private:
        t_metal_layer metal_layer;
        int x_pos;
        int start_y;
        int end_y;
        int source_y_offset;
        int tap_y_offset;
        int tap_y_incr;
    public:
        void create_rr_nodes_for_clock_network_wires();
};

class ClockRRGraph {
    public:
        /* Reverse lookup for to find the clock source and tap locations for each clock_network
           The map key is the the clock network name and the switch type (source or sink).
           The map value is a map of all of the coordinates to the number of times they repeat */
//        std::unordered_map<std::pair<std::string, enum e_switch_type>,
//            std::unordered_map<t_coordinates, int>>
//                clock_tap_and_source_locations;

    public:
        /* Creates the routing resourse (rr) graph of the clock network and appends it to the
           existing rr graph created in build_rr_graph for inter-block and intra-block routing. */
        static void create_and_append_clock_rr_graph();

    private:
        /* Dummy clock network that connects every I/O input to every clock pin. */
        static void create_star_model_network();

        /* Create the wires of the clock network, supports rib and spines (TODO: suport H-trees).
           TODO: will need to include buffers or have a sperate buffer insertion function */
        static void create_clock_network_wires(
                const t_clock_network_description& clock_network);

        /* loop over all of the clock networks and create their wires */
        static void create_clock_networks_wires(
                std::vector<t_clock_network_description>& clock_networks);

        /* Connects the switches an connections between clock network wires */
        static void create_clock_network_switches(
                std::string clock_network_from,
                std::string clock_network_to,
                const int fc);

        /* loop over all clock routing connections and create the switches and connections */
        static void create_clock_networks_switches(
                std::vector<t_clock_connections>& clock_connections);

        /* Connects the inter-block routing to the clock source at the specified coordinates */
        static void create_switches_from_routing_to_clock_source(
                const t_coordinates& clock_source_coordinates,
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
        static void create_switches_from_routing_to_clock_pins(
                const int fc);

};

#endif

