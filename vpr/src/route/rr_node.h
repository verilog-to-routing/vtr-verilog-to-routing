#ifndef RR_NODE_H
#define RR_NODE_H
#include <memory>
#include "vpr_types.h"

/* Main structure describing one routing resource node.  Everything in       *
 * this structure should describe the graph -- information needed only       *
 * to store algorithm-specific data should be stored in one of the           *
 * parallel rr_node_* structures.                                            *
 *                                                                           *
 * xlow, xhigh, ylow, yhigh:  Integer coordinates (see route.c for           *
 *       coordinate system) of the ends of this routing resource.            *
 *       xlow = xhigh and ylow = yhigh for pins or for segments of           *
 *       length 1.  These values are used to decide whether or not this      *
 *       node should be added to the expansion heap, based on things         *
 *       like whether it's outside the net bounding box or is moving         *
 *       further away from the target, etc.                                  *
 * type:  What is this routing resource?                                     *
 * ptc_num:  Pin, track or class number, depending on rr_node type.          *
 *           Needed to properly draw.                                        *
 * cost_index: An integer index into the table of routing resource indexed   *
 *             data t_rr_index_data (this indirection allows quick dynamic   *
 *             changes of rr base costs, and some memory storage savings for *
 *             fields that have only a few distinct values).                 *
 * capacity:   Capacity of this node (number of routes that can use it).     *
 * num_edges:  Number of edges exiting this node.  That is, the number       *
 *             of nodes to which it connects.                                *
 * edges[0..num_edges-1]:  Array of indices of the neighbours of this        *
 *                         node.                                             *
 * switches[0..num_edges-1]:  Array of switch indexes for each of the        *
 *                            edges leaving this node.                       *
 *                                                                           *
 * The following parameters are only needed for timing analysis.             *
 * R:  Resistance to go through this node.  This is only metal               *
 *     resistance (end to end, so conservative) -- it doesn't include the    *
 *     switch that leads to another rr_node.                                 *
 * C:  Total capacitance of this node.  Includes metal capacitance, the      *
 *     input capacitance of all switches hanging off the node, the           *
 *     output capacitance of all switches to the node, and the connection    *
 *     box buffer capacitances hanging off it.                               *
 * direction: if the node represents a track, this field                     *
 *            indicates the direction of the track. Otherwise                *
 *            the value contained in the field should be                     *
 *            ignored.                                                       *
 * side: The side of a grid location where an IPIN or OPIN is located.       *
 *       This field is valid only for IPINs and OPINs and should be ignored  *
 *       otherwise.                                                          */

class t_rr_node {
    public: //Accessors
        t_rr_type type() const { return type_; }
        const char *type_string() const; /* Retrieve type as a string */

        short num_edges() const { return num_edges_; }
        int edge_sink_node(int iedge) const { VTR_ASSERT_SAFE(iedge < num_edges()); return edge_sink_nodes_[iedge]; }
        short edge_switch(int iedge) const { VTR_ASSERT_SAFE(iedge < num_edges()); return edge_switches_[iedge]; }
        short fan_in() const;

        short xlow() const;
        short ylow() const;
        short xhigh() const;
        short yhigh() const;
        signed short length() const;

        short capacity() const;

        short ptc_num() const;
        short pin_num() const; //Same as ptc_num() but checks that type() is consistent
        short track_num() const; //Same as ptc_num() but checks that type() is consistent
        short class_num() const; //Same as ptc_num() but checks that type() is consistent

        short cost_index() const;
        bool has_direction() const;
        e_direction direction() const;
        const char *direction_string() const;

        e_side side() const;
        const char *side_string() const;

        float R() const { return R_; }
        float C() const { return C_; }

    public: //Mutators
        void set_type(t_rr_type new_type);

        void set_num_edges(short);
        void set_edge_sink_node(short iedge, int sink_node);
        void set_edge_switch(short iedge, short switch_index);
        void set_fan_in(short);

        void set_coordinates(short x1, short y1, short x2, short y2);

        void set_capacity(short);

        void set_ptc_num(short);
        void set_pin_num(short); //Same as set_ptc_num() by checks type() is consistent
        void set_track_num(short); //Same as set_ptc_num() by checks type() is consistent
        void set_class_num(short); //Same as set_ptc_num() by checks type() is consistent


        void set_cost_index(short);

        void set_direction(e_direction);
        void set_side(e_side);

        void set_R(float new_R);
        void set_C(float new_C);

    private: //Data
        short xlow_ = -1;
        short ylow_ = -1;
        short xhigh_ = -1;
        short yhigh_ = -1;

        union {
            short pin_num;
            short track_num;
            short class_num;
        } ptc_;
        short cost_index_ = -1;
        short fan_in_ = 0;
        short capacity_ = -1;

        float R_ = 0.;
        float C_ = 0.;

        //Note: we use plain arrays and a single size counter to save space vs std::vector
        //      (using two std::vector's nearly doubles the size of the class)
        std::unique_ptr<int[]> edge_sink_nodes_ = nullptr;
        std::unique_ptr<short[]> edge_switches_ = nullptr;
        short num_edges_ = 0;

        union {
            e_direction direction; //Valid only for CHANX/CHANY
            e_side side; //Valid only for IPINs/OPINs
        } dir_side_;
        t_rr_type type_ = NUM_RR_TYPES;
};


/* Data that is pointed to by the .cost_index member of t_rr_node.  It's     *
 * purpose is to store the base_cost so that it can be quickly changed       *
 * and to store fields that have only a few different values (like           *
 * seg_index) or whose values should be an average over all rr_nodes of a    *
 * certain type (like T_linear etc., which are used to predict remaining     *
 * delay in the timing_driven router).                                       *
 *                                                                           *
 * base_cost:  The basic cost of using an rr_node.                           *
 * ortho_cost_index:  The index of the type of rr_node that generally        *
 *                    connects to this type of rr_node, but runs in the      *
 *                    orthogonal direction (e.g. vertical if the direction   *
 *                    of this member is horizontal).                         *
 * seg_index:  Index into segment_inf of this segment type if this type of   *
 *             rr_node is an CHANX or CHANY; OPEN (-1) otherwise.            *
 * inv_length:  1/length of this type of segment.                            *
 * T_linear:  Delay through N segments of this type is N * T_linear + N^2 *  *
 *            T_quadratic.  For buffered segments all delay is T_linear.     *
 * T_quadratic:  Dominant delay for unbuffered segments, 0 for buffered      *
 *               segments.                                                   *
 * C_load:  Load capacitance seen by the driver for each segment added to    *
 *          the chain driven by the driver.  0 for buffered segments.        */

struct t_rr_indexed_data {
	float base_cost;
	float saved_base_cost;
	int ortho_cost_index;
	int seg_index;
	float inv_length;
	float T_linear;
	float T_quadratic;
	float C_load;

	/* Power Estimation: Wire capacitance in (Farads * tiles / meter)
	 * This is used to calculate capacitance of this segment, by
	 * multiplying it by the length per tile (meters/tile).
	 * This is only the wire capacitance, not including any switches */
	float C_tile_per_m;
};

#endif
