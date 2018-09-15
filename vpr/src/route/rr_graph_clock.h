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

struct RibDrive {
    int offset;
    int switch_idx;
};

struct RibTaps {
    int offset;
    int increment;
};

struct SpineDrive {
    int offset;
    int switch_idx;
};

struct SpineTaps {
    int offset;
    int increment;
};

struct HtreeDrive {
    Coordinates offset;
    int switch_idx;
};

struct HtreeTaps {
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
        int get_num_inst() const {return num_inst;}
        std::string get_name() const {return this->clock_name;}
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
        virtual void create_rr_nodes_for_one_instance(int inst_num, ClockRRGraph& clock_graph) = 0;
};

class ClockRib : public ClockNetwork {
    private:

        // start and end x and position in the y
        Wire x_chan_wire;
        WireRepeat repeat;

        // offset in the x
        RibDrive drive;

        // offset and incr in the x
        RibTaps tap;

    public:
        /** Constructor**/
        ClockRib() {}; // default
        ClockRib(Wire wire1, WireRepeat repeat1, RibDrive drive1, RibTaps tap1):
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
            drive.offset = offset_x;
        }
        void set_drive_switch(int switch_idx) {
            drive.switch_idx = switch_idx;
        }
        void set_tap_locations(int offset_x, int increment_x) {
            tap.offset = offset_x;
            tap.increment = increment_x;
        }

        /*
         * Member functions
         */
        void create_rr_nodes_for_one_instance(int inst_num, ClockRRGraph& clock_graph);
        int create_chanx_wire(
                int x_start,
                int x_end,
                int y,
                int ptc_num,
                std::vector<t_rr_node>& rr_nodes);
        void record_tap_locations(
                unsigned x_start,
                unsigned x_end,
                unsigned y,
                int left_rr_node_idx,
                int right_rr_node_idx,
                ClockRRGraph& clock_graph);
};

class ClockSpine : public ClockNetwork {
    private:

        // start and end y and position in the x
        Wire y_chan_wire;
        WireRepeat repeat;

        // offset in the y
        SpineDrive drive;

        // offset and incr in the y
        SpineTaps tap;

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
            drive.offset = offset_y;
        }
        void set_drive_switch(int switch_idx) {
            drive.switch_idx = switch_idx;
        }
        void set_tap_locations(int offset_y, int increment_y) {
            tap.offset = offset_y;
            tap.increment = increment_y;
        }

        /*
         * Member functions
         */
        void create_rr_nodes_for_one_instance(int inst_num, ClockRRGraph& clock_graph);
        int create_chany_wire(
            int y_start,
            int y_end,
            int x,
            int ptc_num,
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

        HtreeDrive drive;

        HtreeTaps tap;

    public:
        ClockType get_network_type() const {return ClockType::H_TREE;}
        // TODO: Unimplemented member function
        void create_rr_nodes_for_one_instance(int inst_num, ClockRRGraph& clock_graph);
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

