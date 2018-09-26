#ifndef CLOCK_NETWORK_TYPES_H
#define CLOCK_NETWORK_TYPES_H

#include <string>
#include <vector>

#include "clock_fwd.h"

#include "globals.h"
#include "rr_graph_clock.h"

class ClockRRGraph;

enum class ClockType {
    SPINE,
    RIB,
    H_TREE
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
    std::string name;
    int offset;
    int switch_idx;
};

struct RibTaps {
    std::string name;
    int offset;
    int increment;
};

struct SpineDrive {
    std::string name;
    int offset;
    int switch_idx;
};

struct SpineTaps {
    std::string name;
    int offset;
    int increment;
};

struct HtreeDrive {
    std::string name;
    Coordinates offset;
    int switch_idx;
};

struct HtreeTaps {
    std::string name;
    Coordinates offset;
    Coordinates increment;
};

class ClockNetwork {
    protected:
        std::string clock_name_;
        int num_inst_;
    public:
        /*
         * Destructor
         */
        virtual ~ClockNetwork() {};

        /*
         * Getters
         */
        int get_num_inst() const;
        std::string get_name() const;
        virtual ClockType get_network_type() const = 0;

        /*
         * Setters
         */
        void set_clock_name(std::string clock_name); 
        void set_num_instance(int num_inst);

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
        ClockType get_network_type() const;

        /*
         * Setters
         */
        void set_metal_layer(float r_metal, float c_metal);
        void set_initial_wire_location(int start_x, int end_x, int y);
        void set_wire_repeat(int repeat_x, int repeat_y);
        void set_drive_location(int offset_x);
        void set_drive_switch(int switch_idx);
        void set_drive_name(std::string name);
        void set_tap_locations(int offset_x, int increment_x);
        void set_tap_name(std::string name);

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
        /*
         * Getters
         */
        ClockType get_network_type() const;

        /*
         * Setters
         */
        void set_metal_layer(float r_metal, float c_metal);
        void set_initial_wire_location(int start_y, int end_y, int x);
        void set_wire_repeat(int repeat_x, int repeat_y);
        void set_drive_location(int offset_y);
        void set_drive_switch(int switch_idx);
        void set_drive_name(std::string name);
        void set_tap_locations(int offset_y, int increment_y);
        void set_tap_name(std::string name);

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
        void record_tap_locations(
                unsigned y_start,
                unsigned y_end,
                unsigned x,
                int left_node_idx,
                int right_node_idx,
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

#endif

