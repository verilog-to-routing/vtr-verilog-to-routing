#ifndef RR_GRAPH_CLOCK_H
#define RR_GRAPH_CLOCK_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "globals.h"

class ClockRRGraph;

enum class ClockType {
    SPINE,
    RIB,
    H_TREE
};

enum class ClockSwitchType {
    DRIVE,
    TAP
};

struct Coordinates {
    int x;
    int y;
};

struct MetalLayer {
    float r_metal;
    float c_metal;
};

struct Wire {
    MetalLayer layer;
    int start;
    int end;
    int position;
};

struct WireRepeat {
    int x;
    int y;
};

struct DriveLocation {
    Coordinates offset;
};

struct TapLocations {
    Coordinates offset;
    Coordinates increment;
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

class ClockNetwork {
    protected:
        std::string clock_name;
        int num_inst;
    public:
        /*
         * Destructor
         */
        virtual ~ClockNetwork() {};

        /*
         * Getters
         */
        int get_num_inst() {return num_inst;}
        std::string get_clock_name() const {return this->clock_name;}
        virtual ClockType get_network_type() const = 0;

        /*
         * Setters
         */
        void set_clock_name(std::string clock_name1) {clock_name = clock_name1;} 
        void set_num_instance(int num_inst1) {num_inst = num_inst1;}

        /*
         * Member funtions
         */
        /* Creates the RR nodes for the clock network wires and adds them to the reverse lookup
           in ClockRRGraph. The reverse lookup maps the nodes to their switch point locations */
        void create_rr_nodes_for_clock_network_wires(ClockRRGraph& clock_graph);
        virtual void create_rr_nodes_for_one_instance(ClockRRGraph& clock_graph) = 0;
};

class ClockRib : public ClockNetwork {
    private:

        // start and end x and position in the y
        Wire x_chan_wire;
        WireRepeat repeat;

        // offset in the x
        DriveLocation drive;

        // offset and incr in the x
        TapLocations tap;

    public:
        /** Constructor**/
        ClockRib() {}; // default
        ClockRib(Wire wire1, WireRepeat repeat1, DriveLocation drive1, TapLocations tap1):
                 x_chan_wire(wire1), repeat(repeat1), drive(drive1), tap(tap1) {};
        /*
         * Getters
         */
        ClockType get_network_type() const {return ClockType::RIB;}

        /*
         * Setters
         */
        void set_metal_layer(int r_metal, int c_metal) {
            x_chan_wire.layer.r_metal = r_metal;
            x_chan_wire.layer.c_metal = c_metal;
        }
        void set_initial_wire_location(int start_x, int end_x, int y) {
            x_chan_wire.start = start_x;
            x_chan_wire.end = end_x;
            x_chan_wire.position = y;
        }
        void set_wire_repeat(int repeat_x, int repeat_y) {
            repeat.x = repeat_x;
            repeat.y = repeat_y;
        }
        void set_drive_location(int offset_x) {
            drive.offset.x = offset_x;
            drive.offset.y = -1;
        }
        void set_tap_location(int offset_x, int increment_x) {
            tap.offset.x = offset_x;
            tap.offset.y = -1;
            tap.increment.x = increment_x;
            tap.increment.y = -1;
        }

        /*
         * Member functions
         */
        void create_rr_nodes_for_one_instance(ClockRRGraph& clock_graph);
        int create_rr_node_for_one_rib(
                int x_start,
                int x_end,
                int y,
                std::vector<t_rr_node>& rr_nodes);
        void record_switch_point_locations_for_rr_node(
                int x_start,
                int x_end,
                int y,
                int rr_node_index,
                ClockRRGraph& clock_graph);
};

class ClockSpine : public ClockNetwork {
    private:

        // start and end y and position in the x
        Wire y_chan_wire;
        WireRepeat repeat;

        // offset in the y
        DriveLocation drive;

        // offset and incr in the y
        TapLocations tap;

    public:
        ClockType get_network_type() const {return ClockType::SPINE;}

        /*
         * Setters
         */
        void set_metal_layer(int r_metal, int c_metal) {
            y_chan_wire.layer.r_metal = r_metal;
            y_chan_wire.layer.c_metal = c_metal;
        }
        void set_initial_wire_location(int start_y, int end_y, int x) {
            y_chan_wire.start = start_y;
            y_chan_wire.end = end_y;
            y_chan_wire.position = x;
        }
        void set_wire_repeat(int repeat_x, int repeat_y) {
            repeat.x = repeat_x;
            repeat.y = repeat_y;
        }
        void set_drive_location(int offset_y) {
            drive.offset.x = -1;
            drive.offset.y = offset_y;
        }
        void set_tap_location(int offset_y, int increment_y) {
            tap.offset.x = -1;
            tap.offset.y = offset_y;
            tap.increment.x = -1;
            tap.increment.y = increment_y;
        }

        /*
         * Member functions
         */
        void create_rr_nodes_for_one_instance(ClockRRGraph& clock_graph);
        int create_rr_node_for_one_spine(
                int y_start,
                int y_end,
                int x,
                std::vector<t_rr_node>& rr_nodes);
        void record_switch_point_locations_for_rr_node(
                int y_start,
                int y_end,
                int x,
                int rr_node_index,
                ClockRRGraph& clock_graph);

};

class ClockHTree : private ClockNetwork {
    private:

        // position not needed since it changes with every root of the tree
        Wire x_chan_wire;
        Wire y_chan_wire;
        WireRepeat repeat;

        DriveLocation drive;

        TapLocations tap;

    public:
        ClockType get_network_type() const {return ClockType::H_TREE;}
        // TODO: Unimplemented member function
        void create_rr_nodes_for_one_instance(ClockRRGraph& clock_graph);
};

class SwitchPoint {
    private:
        // [grid_width][grid_height][nodes]
        std::vector<std::vector<std::vector<int>>> rr_node_indices;
        std::vector<Coordinates> locations;
};

class SwitchPoints {
    private:
        std::unordered_map<std::string, SwitchPoint> switch_name_to_switch_location;
    public:
        /** Getters **/

        /** Setters **/
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

