/* This is a core file that defines the major data types used by VPR

 This file is divided into generally 4 major sections:

 1.  Global data types and constants
 2.  Packing specific data types
 3.  Placement specific data types
 4.  Routing specific data types

 Key background file:

 An understanding of libarchfpga/physical_types.h is crucial to understanding this file.  physical_types.h contains information about the architecture described in the architecture description language

 Key data structures:
 t_rr_node - The basic building block of the interconnect in the FPGA architecture

 Cluster-specific main data structure:
 t_pb: Stores the mapping between the user netlist and the logic blocks on the FPGA architecture.  For example, if a user design has 10 clusters of 5 LUTs each, you will have 10 t_pb instances of type cluster and within each of those clusters another 5 t_pb instances of type LUT.
 The t_pb hierarchy follows what is described by t_pb_graph_node

 */

#ifndef VPR_TYPES_H
#define VPR_TYPES_H

#include <vector>
#include <unordered_map>
#include "arch_types.h"
#include "atom_netlist_fwd.h"
#include "clustered_netlist_fwd.h"
#include "constant_nets.h"
#include "clock_modeling.h"

#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector_map.h"
#include "vtr_util.h"

/*******************************************************************************
 * Global data types and constants
 ******************************************************************************/
/*#define CREATE_ECHO_FILES*//* prints echo files */
/*#define PRINT_SINK_DELAYS*//*prints the sink delays to files */
/*#define PRINT_SLACKS*//*prints out all slacks in the circuit */
/*#define PRINT_PLACE_CRIT_PATH*//*prints out placement estimated critical path */
/*#define PRINT_NET_DELAYS*//*prints out delays for all connections */
/*#define PRINT_TIMING_GRAPH*//*prints out the timing graph */
/*#define DUMP_BLIF_ECHO*//*dump blif of internal representation of user circuit.  Useful for ensuring functional correctness via logical equivalence with input blif*/

//#define ROUTER_DEBUG //Prints out very detailed routing progress information if defined

#define TOKENS " \t\n"		/* Input file parsing. */

//#define VERBOSE //Prints additional intermediate data

/* For update_screen.  Denotes importance of update.
 * By default MINOR only updates the screen, while MAJOR
 * pauses graphics for the user to interact */
enum class ScreenUpdatePriority {
    MINOR = 0,
    MAJOR = 1
};

#define MAX_SHORT 32767

/* Values large enough to be way out of range for any data, but small enough
 to allow a small number to be added to them without going out of range. */
#define HUGE_POSITIVE_FLOAT 1.e30
#define HUGE_NEGATIVE_FLOAT -1.e30

/* Used to avoid floating-point errors when comparing values close to 0 */
#define EPSILON 1.e-15
#define NEGATIVE_EPSILON -1.e-15

#define HIGH_FANOUT_NET_LIM 64 /* All nets with this number of sinks or more are considered high fanout nets */

#define FIRST_ITER_WIRELENTH_LIMIT 0.85 /* If used wirelength exceeds this value in first iteration of routing, do not route */

/* Defining macros for the placement_ctx t_grid_blocks. Assumes that ClusterBlockId's won't exceed positive 32-bit integers */
constexpr auto EMPTY_BLOCK_ID = ClusterBlockId(-1);
constexpr auto INVALID_BLOCK_ID = ClusterBlockId(-2);

constexpr const char* EMPTY_BLOCK_NAME = "EMPTY";

/*
 * Files
 */
/*******************************************************************************
 * Packing specific data types and constants
 * Packing takes the circuit described in the technology mapped user netlist
 * and maps it to the complex logic blocks found in the arhictecture
 ******************************************************************************/

#define NO_CLUSTER -1
#define NEVER_CLUSTER -2
#define NOT_VALID -10000  /* Marks gains that aren't valid */
/* Ensure no gain can ever be this negative! */
#ifndef UNDEFINED
#define UNDEFINED -1
#endif


enum class e_route_bb_update {
    STATIC, //Router net bounding boxes are not updated
    DYNAMIC //Rotuer net bounding boxes are updated
};

enum class e_const_gen_inference {
    NONE, //No constant generator inference
    COMB, //Only combinational constant generator inference
    COMB_SEQ //Both combinational and sequential constant generator inference
};

enum class e_unrelated_clustering {
    OFF, ON, AUTO	
};

/* Selection algorithm for selecting next seed  */
enum class e_cluster_seed {
	TIMING, MAX_INPUTS, BLEND, MAX_PINS, MAX_INPUT_PINS, BLEND2
};

enum e_block_pack_status {
	BLK_PASSED,
    BLK_FAILED_FEASIBLE,
    BLK_FAILED_ROUTE,
    BLK_STATUS_UNDEFINED
};

struct t_ext_pin_util {
    t_ext_pin_util() = default;
    t_ext_pin_util(float in, float out)
        : input_pin_util(in), output_pin_util(out) {}

    float input_pin_util = 1.;
    float output_pin_util = 1.;
};

//Specifies the utilization of external input/output pins
//during packing
class t_ext_pin_util_targets {
public:
    t_ext_pin_util_targets() = default;
    t_ext_pin_util_targets(float default_in_util, float default_out_util);

    //Returns the input pin util of the specified block (or default if unspecified)
    t_ext_pin_util get_pin_util(std::string block_type_name) const;

public:
    //Sets the pin util for the specified block type
    //Returns true if non-default was previously set
    void set_block_pin_util(std::string block_type_name, t_ext_pin_util target);

    //Sets the default pin util
    //Returns true if a default was previously set
    void set_default_pin_util(t_ext_pin_util target);
private:
    t_ext_pin_util defaults_;
    std::map<std::string,t_ext_pin_util> overrides_;
};

/* these are defined later, but need to declare here because it is used */
class t_rr_node;
struct t_pack_molecule;
struct t_pb_stats;
struct t_pb_route;

/* A t_pb represents an instance of a clustered block, which may be:
 *    1) A top level clustered block which is placeable at a location in FPGA device
 *       grid location (e.g. a Logic block, RAM block, DSP block), or
 *    2) An internal 'block' representing an intermediate level of hierarchy inside a top level
 *       block (e.g. a BLE), or
 *    3) A leaf (i.e. atom or primitive) block representing an element of netlist (e.g. LUT,
 *       flip-lop, memory slice etc.)
 *
 * t_pb (in combination with t_pb_route) implement the mapping from the netlist elements to architectural
 * instances.
 */
struct t_pb {
	char *name = nullptr; /* Name of this physical block */
	t_pb_graph_node *pb_graph_node = nullptr; /* pointer to pb_graph_node this pb corresponds to */

	int mode = 0; /* mode that this pb is set to */

	t_pb **child_pbs = nullptr; /* children pbs attached to this pb [0..num_child_pb_types - 1][0..child_type->num_pb - 1] */
	t_pb *parent_pb = nullptr; /* pointer to parent node */

	t_pb_stats *pb_stats = nullptr; /* statistics for current pb */

	/* Representation of intra-logic block routing, t_pb_route describes all internal hierarchy routing.
	*  t_pb_route is an array of size [t_pb->pb_graph_node->total_pb_pins]
	*  Only valid for the top-level t_pb (parent_pb == nullptr). On any child pb, t_pb_route will be nullptr. */
	t_pb_route *pb_route = nullptr;

	int clock_net = 0; /* Records clock net driving a flip-flop, valid only for lowest-level, flip-flop PBs */


	int get_num_child_types() const {
		if (child_pbs != nullptr && has_modes()) {
			return pb_graph_node->pb_type->modes[mode].num_pb_type_children;
		} else {
			return 0;
		}
	}

	int get_num_children_of_type(int type_index) const {
        t_mode* mode_ptr = get_mode();
        if (mode_ptr) {
            return mode_ptr->pb_type_children[type_index].num_pb;
        }
        return 0; //No mode
	}

	t_mode* get_mode() const {
		if (has_modes()) {
			return &pb_graph_node->pb_type->modes[mode];
		} else {
			return nullptr;
		}
	}

	bool has_modes() const {
		return pb_graph_node->pb_type->num_modes > 0;
	}

    //Returns the t_pb associated with the specified gnode which is contained
    //within the current pb
    const t_pb* find_pb(const t_pb_graph_node* gnode) const {
        //Base case
        if(pb_graph_node == gnode) {
            return this;
        }

        //Search recursively
        for(int ichild_type = 0; ichild_type < get_num_child_types(); ++ichild_type) {

            if(child_pbs[ichild_type] == nullptr) continue;

            for(int ipb = 0; ipb < get_num_children_of_type(ichild_type); ++ipb) {

                const t_pb* child_pb = &child_pbs[ichild_type][ipb];

                const t_pb* found_pb = child_pb->find_pb(gnode);
                if(found_pb != nullptr) {
                    VTR_ASSERT(found_pb->pb_graph_node == gnode);
                    return found_pb; //Found
                }
            }
        }
        return nullptr; //Not found
    }

    const t_pb* find_pb_for_model(const std::string& blif_model) const {
        //Base case
        const t_model* model = pb_graph_node->pb_type->model;
        if (model && model->name == blif_model) {
            return this;
        }

        //Search recursively
        for(int ichild_type = 0; ichild_type < get_num_child_types(); ++ichild_type) {

            if(child_pbs[ichild_type] == nullptr) continue;

            for(int ipb = 0; ipb < get_num_children_of_type(ichild_type); ++ipb) {

                const t_pb* child_pb = &child_pbs[ichild_type][ipb];

                const t_pb* matching_pb = child_pb->find_pb_for_model(blif_model);
                if (matching_pb) {
                    return this;
                }
            }
        }
        return nullptr; //Not found
    }

    //Returns the root pb containing this pb
    const t_pb* root_pb() const {

        const t_pb* curr_pb = this;
        while(!is_root()) {
            curr_pb = curr_pb->parent_pb;
        }

        VTR_ASSERT(curr_pb->parent_pb == nullptr);
        return curr_pb;
    }

    bool is_root() const {
        return parent_pb == nullptr;
    }

    //Returns true if this pb corresponds to a primitive block (i.e. in the AtomNetlist)
    bool is_primitive() const {
        return child_pbs == nullptr;
    }

    std::string hierarchical_type_name() const {
        std::vector<std::string> names;

        int child_mode = OPEN;
        for (const t_pb* curr = this; curr != nullptr; curr = curr->parent_pb) {
            std::string type_name;

            //Type
            if (curr->pb_graph_node) {
                type_name = curr->pb_graph_node->pb_type->name;
                type_name += "[" + std::to_string(curr->pb_graph_node->placement_index) + "]";

                //Mode
                if (child_mode != OPEN) {
                    std::string mode_name = curr->pb_graph_node->pb_type->modes[child_mode].name;
                    type_name += "[" + mode_name + "]";
                }
            }

            child_mode = curr->mode; //Save the mode of curr since it will be child on next iteration

            names.push_back(type_name);
        }

        //We walked up from the leaf to root, so we join in reverse order
        return vtr::join(names.rbegin(), names.rend(), "/");
    }

    //Returns the bit index into the AtomPort for the specified primitive
    //pb_graph_pin, considering any pin rotations which have been applied to logically
    //equivalent pins
    BitIndex atom_pin_bit_index(const t_pb_graph_pin* gpin) const {
        VTR_ASSERT_MSG(is_primitive(), "Atom pin indicies can only be looked up from primitives");

        auto iter = pin_rotations_.find(gpin);

        if(iter != pin_rotations_.end()) {
            //Return the original atom pin index
            return iter->second;
        } else {
            //No re-mapping, return the index directly
            return gpin->pin_number;
        }
    }

    //For a given gpin, sets the mapping to the original atom netlist pin's bit index in
    //it's AtomPort.  This is used to record any pin rotations which have been applied to
    //logically equivalent pins
    void set_atom_pin_bit_index(const t_pb_graph_pin* gpin, BitIndex atom_pin_bit_idx) {
        pin_rotations_[gpin] = atom_pin_bit_idx;
    }

private:
    std::map<const t_pb_graph_pin*,BitIndex> pin_rotations_; //Contains the atom netlist port bit index associated
                                                       //with any primitive pins which have been rotated during clustering

};

/* Representation of intra-logic block routing */
struct t_pb_route {
    AtomNetId atom_net_id; /* which net in the atom netlist uses this pin */
	int driver_pb_pin_id; /* The pb_pin id of the pb_pin that drives this pin */
    std::vector<int> sink_pb_pin_ids; /* The pb_pin id's of the pb_pins driven by this node */
    const t_pb_graph_pin* pb_graph_pin = nullptr; /* The graph pin associated with this node */

	t_pb_route() {
		atom_net_id = AtomNetId::INVALID();
		driver_pb_pin_id = OPEN;
	}
};

enum e_pack_pattern_molecule_type {
	MOLECULE_SINGLE_ATOM, MOLECULE_FORCED_PACK
};

/* Represents a grouping of atom blocks that match a pack_pattern, these groups are intended to be placed as a single unit during packing
 * Store in linked list
 *
 * A chain is a special type of pack pattern.  A chain can extend across multiple logic blocks.
 * Must segment the chain to fit in a logic block by identifying the actual atom that forms the root of the new chain.
 * Assumes that the root of a chain is the primitive that starts the chain or is driven from outside the logic block
 */
struct t_pack_molecule {
	enum e_pack_pattern_molecule_type type; /* what kind of molecule is this? */
	t_pack_patterns *pack_pattern; /* If this is a forced_pack molecule, pattern this molecule matches */
	t_model_chain_pattern *chain_pattern; /* If this is a chain molecule, chain that this molecule matches */
    std::vector<AtomBlockId> atom_block_ids; /* [0..num_blocks-1] IDs of atom blocks that implements this molecule,
                                                index on pack_pattern_block->index of pack pattern */
	bool valid; /* Whether or not this molecule is still valid */

	int num_blocks; /* number of atom blocks of molecule */
	int root; /* root index of molecule, atom_block_ids[root] is the root atom block */

	float base_gain; /* Intrinsic "goodness" score for molecule independant of rest of netlist */

	t_pack_molecule *next;
};

/* Stats keeper for placement information during packing
 * Contains linked lists to placement locations based on status of primitive
 */
struct t_cluster_placement_stats {
	int num_pb_types; /* num primitive pb_types inside complex block */
	const t_pack_molecule *curr_molecule; /* current molecule being considered for packing */
	t_cluster_placement_primitive **valid_primitives; /* [0..num_pb_types-1] ptrs to linked list of valid primitives, for convenience, each linked list head is empty */
	t_cluster_placement_primitive *in_flight; /* ptrs to primitives currently being considered */
	t_cluster_placement_primitive *tried; /* ptrs to primitives that are open but current logic block unable to pack to */
	t_cluster_placement_primitive *invalid; /* ptrs to primitives that are invalid */
};

/******************************************************************
 * Timing data types
 *******************************************************************/

//Enable the legacy STA engine to run along side the
//new STA engine
//#define ENABLE_CLASSIC_VPR_STA

// #define PATH_COUNTING 'P'
/* Uncomment this to turn on path counting. Its value determines how path criticality
 is calculated from forward and backward weights.  Possible values:
 'S' - sum of forward and backward weights
 'P' - product of forward and backward weights
 'L' - natural log of the product of forward and backward weights
 'R' - product of the natural logs of forward and backward weights
 See path_delay.h for further path-counting options. */

/* Timing graph information */
struct t_tedge {
	/* Edge in the timing graph. */
	int to_node; /* index of node at the sink end of this edge */
	float Tdel; /* delay to go to to_node along this edge */
};

enum e_tnode_type {
	/* Types of tnodes (timing graph nodes). */
	TN_INPAD_SOURCE, /* input to an input I/O pad */
	TN_INPAD_OPIN, /* output from an input I/O pad */
	TN_OUTPAD_IPIN, /* input to an output I/O pad */
	TN_OUTPAD_SINK, /* output from an output I/O pad */
	TN_CB_IPIN, /* input pin to complex block */
	TN_CB_OPIN, /* output pin from complex block */
	TN_INTERMEDIATE_NODE, /* Used in post-packed timing graph only:
	 connection between intra-cluster pins. */
	TN_PRIMITIVE_IPIN, /* input pin to a primitive (e.g. a LUT) */
	TN_PRIMITIVE_OPIN, /* output pin from a primitive (e.g. a LUT) */
	TN_FF_IPIN, /* input pin to a flip-flop - goes to TN_FF_SINK */
	TN_FF_OPIN, /* output pin from a flip-flop - comes from TN_FF_SOURCE */
	TN_FF_SINK, /* sink (D) pin of flip-flop */
	TN_FF_SOURCE, /* source (Q) pin of flip-flop */
	TN_FF_CLOCK, /* clock pin of flip-flop */
    TN_CLOCK_SOURCE, /* An on-chip clock generator such as a pll */
    TN_CLOCK_OPIN, /* Output pin from an on-chip clock source - comes from TN_CLOCK_SOURCE */
	TN_CONSTANT_GEN_SOURCE /* source of a constant logic 1 or 0 */
};

struct t_prepacked_tnode_data {
	/* Data only used by prepacked tnodes. Stored separately so it
	 doesn't need to be allocated in the post-packed netlist. */
	int model_port, model_pin; /* technology mapped model pin */
	t_model_ports *model_port_ptr;
#ifndef PATH_COUNTING
	long num_critical_input_paths, num_critical_output_paths; /* count of critical paths fanning into/out of this tnode */
	float normalized_slack; /* slack (normalized with respect to max slack) */
	float normalized_total_critical_paths; /* critical path count (normalized with respect to max count) */
	float normalized_T_arr; /* arrival time (normalized with respect to max time) */
#endif
};

struct t_tnode {
	/* Node in the timing graph. Note: we combine 2 members into a bit field. */
	e_tnode_type type; /* see the above enum */
	t_tedge *out_edges; /* [0..num_edges - 1] array of edges fanning out from this tnode.
	 Note: there is a correspondence in indexing between out_edges and the
	 net data structure: out_edges[iedge] = net[inet].node_block[iedge + 1]
	 There is an offset of 1 because net[inet].node_block includes the driver
	 node at index 0, while out_edges is part of the driver node and does
	 not bother to refer to itself. */
	int num_edges;
	float T_arr; /* Arrival time of the last input signal to this node. */
	float T_req; /* Required arrival time of the last input signal to this node
	 if the critical path is not to be lengthened. */
	ClusterBlockId block; /* atom block primitive which this tnode is part of */

#ifdef PATH_COUNTING
	float forward_weight, backward_weight; /* Weightings of the importance of paths
	 fanning into and out of this node, respectively. */
#endif

	/* Valid values for TN_FF_SINK, TN_FF_SOURCE, TN_FF_CLOCK, TN_INPAD_SOURCE, and TN_OUTPAD_SINK only: */
	int clock_domain; /* Index of the clock in timing_ctx.sdc->constrained_clocks which this flip-flop or I/O is constrained on. */
	float clock_delay; /* The time taken for a clock signal to get to the flip-flop or I/O (assumed 0 for I/Os). */

	/* Used in post-packing timing graph only: */
	t_pb_graph_pin *pb_graph_pin; /* pb_graph_pin that this block is connected to */

	/* Used in pre-packing timing graph only: */
	t_prepacked_tnode_data * prepacked_data;

	unsigned int is_comb_loop_breakpoint : 1; /* Indicates that this tnode had input edges purposely
											     disconnected to break a combinational loop */
};

/* Other structures storing timing information */
struct t_clock {
	/* Stores information on clocks given timing constraints.
	 Used in SDC parsing and timing analysis. */
	char * name;
	bool is_netlist_clock; /* Is this a netlist or virtual (external) clock? */
	int fanout;
};

struct t_io {
	/* Stores information on I/Os given timing constraints.
	 Used in SDC parsing and timing analysis. */
	char * name; /* I/O port name with an SDC constraint */
	char * clock_name; /* Clock it was constrained on */
	float delay; /* Delay through the I/O in this constraint */
	int file_line_number; /* line in the SDC file I/O was constrained on - used for error reporting */
};

struct t_timing_stats {
	/* Timing statistics for final reporting for each constraint
	 (pair of constrained source and sink clock domains).

	 cpd holds the critical path delay, the longest path between the
	 pair of domains, or equivalently the path with the least slack.

	 least_slack holds the slack of the connection with the least slack
	 over all paths in this constraint, even if this connection is part
	 of another constraint and has a lower slack from that constraint.

	 The "critical path" of the entire design is the path with the least
	 slack in the constraint with the least slack
	 (see get_critical_path_delay()). */

	float ** cpd;
	float ** least_slack;
};

struct t_slack {
	/* Matrices storing slacks and criticalities of each sink pin on each net
	 [0..net.size()-1][1..num_pins-1] for post-packed netlists. */
	float ** slack;
	float ** timing_criticality;
};

struct t_override_constraint {
	/* A special-case constraint to override the default, calculated, timing constraint.  Holds data from
	 set_clock_groups, set_false_path, set_max_delay, and set_multicycle_path commands. Can hold data for
	 clock-to-clock, clock-to-flip-flop, flip-flop-to-clock or flip-flop-to-flip-flop constraints, each of
	 which has its own array (timing_ctx.sdc->cc_constraints, timing_ctx.sdc->cf_constraints, timing_ctx.sdc->fc_constraints, and timing_ctx.sdc->ff_constraints). */
	char ** source_list; /* Array of net names of flip-flops or clocks */
	char ** sink_list;
	int num_source;
	int num_sink;
	float constraint;
	int num_multicycles;
	int file_line_number; /* line in the SDC file clock was constrained on - used for error reporting */
};

struct t_timing_constraints { /* Container structure for all SDC timing constraints.
 See top-level comment to read_sdc.c for details on members. */
	int num_constrained_clocks; /* number of clocks with timing constraints */
	t_clock * constrained_clocks; /* [0..timing_ctx.sdc->num_constrained_clocks - 1] array of clocks with timing constraints */

    vtr::Matrix<float> domain_constraint; /* [0..num_constrained_clocks - 1 (source)][0..num_constrained_clocks - 1 (destination)] */

	int num_constrained_inputs; /* number of inputs with timing constraints */
	t_io * constrained_inputs; /* [0..num_constrained_inputs - 1] array of inputs with timing constraints */

	int num_constrained_outputs; /* number of outputs with timing constraints */
	t_io * constrained_outputs; /* [0..num_constrained_outputs - 1] array of outputs with timing constraints */

	int num_cc_constraints; /* number of special-case clock-to-clock constraints overriding default, calculated, timing constraints */
	t_override_constraint * cc_constraints; /*  [0..num_cc_constraints - 1] array of such constraints */

	int num_cf_constraints; /* number of special-case clock-to-flipflop constraints */
	t_override_constraint * cf_constraints; /*  [0..num_cf_constraints - 1] array of such constraints */

	int num_fc_constraints; /* number of special-case flipflop-to-clock constraints */
	t_override_constraint * fc_constraints; /*  [0..num_fc_constraints - 1] */

	int num_ff_constraints; /* number of special-case flipflop-to-flipflop constraints */
	t_override_constraint * ff_constraints; /*  [0..num_ff_constraints - 1] array of such constraints */
};

/* Cluster timing delays:
 * C_ipin_cblock: Capacitance added to a routing track by the isolation     *
 *                buffer between a track and the Cblocks at an (i,j) loc.   *
 * T_ipin_cblock: Delay through an input pin connection box (from a         *
 *                   routing track to a logic block input pin).             */
struct t_timing_inf {
	bool timing_analysis_enabled;
	float C_ipin_cblock;
	float T_ipin_cblock;
    std::string SDCFile;
    std::string slack_definition;
};

/***************************************************************************
 * Placement and routing data types
 ****************************************************************************/

/* Timing data structures end */
enum sched_type {
	AUTO_SCHED, USER_SCHED
};
/* Annealing schedule */

enum pic_type {
	NO_PICTURE, PLACEMENT, ROUTING
};
/* What's on screen? */

enum pfreq {
	PLACE_NEVER, PLACE_ONCE, PLACE_ALWAYS
};

/* Are the pads free to be moved, locked in a random configuration, or
 * locked in user-specified positions?                                 */
enum e_pad_loc_type {
	FREE, RANDOM, USER
};

/* Power data for t_netlist structure */
struct t_net_power {
	/* Signal probability - long term probability that signal is logic-high*/
	float probability;

	/* Transistion density - average # of transitions per clock cycle
	 * For example, a clock would have density = 2
	 */
	float density;
};

/* s_grid_tile is the minimum tile of the fpga
 * type:  Pointer to type descriptor, NULL for illegal
 * width_offset: Number of grid tiles reserved based on width (right) of a block
 * height_offset: Number of grid tiles reserved based on height (top) of a block */
struct t_grid_tile {
	t_type_ptr type = nullptr;
	int width_offset = 0;
	int height_offset = 0;
};

/* Stores the bounding box of a net in terms of the minimum and   *
 * maximum coordinates of the blocks forming the net, clipped to  *
 * the region:                                                    *
 *  (1..device_ctx.grid.width()-2, 1..device_ctx.grid.height()-1) */
struct t_bb {
	int xmin;
	int xmax;
	int ymin;
	int ymax;
};

/* capacity:   Capacity of this region, in tracks.               *
 * occupancy:  Expected number of tracks that will be occupied.  *
 * cost:       Current cost of this usage.                       */
struct t_place_region {
	float capacity;
	float inv_capacity;
	float occupancy;
	float cost;
};

/* Stores the information of the move for a block that is       *
 * moved during placement                                       *
 * block_num: the index of the moved block                      *
 * xold: the x_coord that the block is moved from               *
 * xnew: the x_coord that the block is moved to                 *
 * yold: the y_coord that the block is moved from               *
 * xnew: the x_coord that the block is moved to                 */
struct t_pl_moved_block {
	ClusterBlockId block_num;
	int xold;
	int xnew;
	int yold;
	int ynew;
	int zold;
	int znew;
	int swapped_to_was_empty;
	int swapped_from_is_empty;
};

/* Stores the list of blocks to be moved in a swap during       *
 * placement.                                                   *
 * num_moved_blocks: total number of blocks moved when          *
 *                   swapping two blocks.                       *
 * moved blocks: a list of moved blocks data structure with     *
 *               information on the move.                       *
 *               [0...num_moved_blocks-1]                       */
struct t_pl_blocks_to_be_moved {
	int num_moved_blocks;
	t_pl_moved_block * moved_blocks;
};

/* legal positions for type */
struct t_legal_pos {
	int x;
	int y;
	int z;
};

/* Represents the placement location of a clustered block
 * x: x-coordinate
 * y: y-coordinate
 * z: occupancy coordinate
 * is_fixed: true if this block's position is fixed by the user and shouldn't be moved during annealing
 * nets_and_pins_synced_to_z_coordinate: true if the associated clb's pins have been synced to the z location (i.e. after placement) */
struct t_block_loc {
    int x = OPEN;
    int y = OPEN;
    int z = OPEN;

	bool is_fixed = false;
    bool nets_and_pins_synced_to_z_coordinate = false;
};

/* Stores the clustered blocks placed at a particular grid location */
struct t_grid_blocks {
    //How many valid blocks are in use at this location
    int usage;

    //The clustered blocks associated with this grid location
    std::vector<ClusterBlockId> blocks;
};

/* Names of various files */
struct t_file_name_opts {
    std::string ArchFile;
	std::string CircuitName;
	std::string BlifFile;
	std::string NetFile;
	std::string PlaceFile;
	std::string RouteFile;
	std::string ActFile;
	std::string PowerFile;
	std::string CmosTechFile;
	std::string out_file_prefix;
    bool verify_file_digests;
};

/* Options for netlist loading */
struct t_netlist_opts {
    e_const_gen_inference const_gen_inference = e_const_gen_inference::COMB;
    bool absorb_buffer_luts = true;
    bool sweep_dangling_primary_ios = true;
    bool sweep_dangling_blocks = true;
    bool sweep_dangling_nets = true;
    bool sweep_constant_primary_outputs = false;

    int netlist_verbosity = 1; //Verbose output during netlist cleaning
};

//Should a stage in the CAD flow be skipped, loaded from a file, or performed
enum e_stage_action {
    STAGE_SKIP = 0,
    STAGE_LOAD,
    STAGE_DO,
    STAGE_AUTO
};

/* Options for packing
 * TODO: document each packing parameter         */
enum e_packer_algorithm {
	PACK_GREEDY, PACK_BRUTE_FORCE
};

struct t_packer_opts {
	std::string blif_file_name;
	std::string sdc_file_name;
	std::string output_file;
	bool global_clocks;
	bool hill_climbing_flag;
	bool timing_driven;
	enum e_cluster_seed cluster_seed_type;
	float alpha;
	float beta;
	float inter_cluster_net_delay;
    float target_device_utilization;
	bool auto_compute_inter_cluster_net_delay;
	e_unrelated_clustering allow_unrelated_clustering;
	bool connection_driven;
	int pack_verbosity;
    bool enable_pin_feasibility_filter;
    std::vector<std::string> target_external_pin_util;
	e_stage_action doPacking;
	enum e_packer_algorithm packer_algorithm;
    std::string device_layout;
	std::string hmetis_input_file;
};

/* Annealing schedule information for the placer.  The schedule type      *
 * is either USER_SCHED or AUTO_SCHED.  Inner_num is multiplied by        *
 * num_blocks^4/3 to find the number of moves per temperature.  The       *
 * remaining information is used only for USER_SCHED, and have the        *
 * obvious meanings.                                                      */
struct t_annealing_sched {
	enum sched_type type;
	float inner_num;
	float init_t;
	float alpha_t;
	float exit_t;
};

/* Various options for the placer.                                           *
 * place_algorithm:  BOUNDING_BOX_PLACE or PATH_TIMING_DRIVEN_PLACE          *
 * timing_tradeoff:  When TIMING_DRIVEN_PLACE mode, what is the tradeoff     *
 *                   timing driven and BOUNDING_BOX_PLACE.                   *
 * place_cost_exp:  Power to which denominator is raised for linear_cong.    *
 * place_chan_width:  The channel width assumed if only one placement is     *
 *                    performed.                                             *
 * pad_loc_type:  Are pins FREE, fixed randomly, or fixed from a file.       *
 * pad_loc_file:  File to read pin locations form if pad_loc_type            *
 *                     is USER.                                              *
 * place_freq:  Should the placement be skipped, done once, or done for each *
 *              channel width in the binary search.                          *
 * recompute_crit_iter: how many temperature stages pass before we recompute *
 *               criticalities based on average point to point delay         *
 * enable_timing_computations: in bounding_box mode, normally, timing        *
 *               information is not produced, this causes the information    *
 *               to be computed. in *_TIMING_DRIVEN modes, this has no effect*
 * inner_loop_crit_divider: (move_lim/inner_loop_crit_divider) determines how*
 *               many inner_loop iterations pass before a recompute of       *
 *               criticalities is done.                                      *
 * td_place_exp_first: exponent that is used on the timing_driven criticlity *
 *               it is the value that the exponent starts at.                *
 * td_place_exp_last: value that the criticality exponent will be at the end *
 * doPlacement: true if placement is supposed to be done in the CAD flow, false otherwise */
enum e_place_algorithm {
	BOUNDING_BOX_PLACE, PATH_TIMING_DRIVEN_PLACE
};

struct t_placer_opts {
	enum e_place_algorithm place_algorithm;
	float timing_tradeoff;
	float place_cost_exp;
	int place_chan_width;
	enum e_pad_loc_type pad_loc_type;
    std::string pad_loc_file;
	enum pfreq place_freq;
	int recompute_crit_iter;
	bool enable_timing_computations;
	int inner_loop_recompute_divider;
	float td_place_exp_first;
	int seed;
	float td_place_exp_last;
	e_stage_action doPlacement;
};

/* All the parameters controlling the router's operation are in this        *
 * structure.                                                               *
 * first_iter_pres_fac:  Present sharing penalty factor used for the        *
 *                 very first (congestion mapping) Pathfinder iteration.    *
 * initial_pres_fac:  Initial present sharing penalty factor for            *
 *                    Pathfinder; used to set pres_fac on 2nd iteration.    *
 * pres_fac_mult:  Amount by which pres_fac is multiplied each              *
 *                 routing iteration.                                       *
 * acc_fac:  Historical congestion cost multiplier.  Used unchanged         *
 *           for all iterations.                                            *
 * bend_cost:  Cost of a bend (usually non-zero only for global routing).   *
 * max_router_iterations:  Maximum number of iterations before giving       *
 *                up.                                                       *
 * min_incremental_reroute_fanout: Minimum fanout a net needs to have 		*
 *				for incremental reroute to be applied to it through route 	*
 *				tree pruning. Larger circuits should get larger thresholds	*
 * bb_factor:  Linear distance a route can go outside the net bounding      *
 *             box.                                                         *
 * route_type:  GLOBAL or DETAILED.                                         *
 * fixed_channel_width:  Only attempt to route the design once, with the    *
 *                       channel width given.  If this variable is          *
 *                       == NO_FIXED_CHANNEL_WIDTH, do a binary search      *
 *                       on channel width.                                  *
 * router_algorithm:  BREADTH_FIRST or TIMING_DRIVEN.  Selects the desired  *
 *                    routing algorithm.                                    *
 * base_cost_type: Specifies how to compute the base cost of each type of   *
 *                 rr_node.  DELAY_NORMALIZED -> base_cost = "demand"       *
 *                 x average delay to route past 1 CLB.  DEMAND_ONLY ->     *
 *                 expected demand of this node (old breadth-first costs).  *
 *                                                                          *
 * The following parameters are used only by the timing-driven router.      *
 *                                                                          *
 * astar_fac:  Factor (alpha) used to weight expected future costs to       *
 *             target in the timing_driven router.  astar_fac = 0 leads to  *
 *             an essentially breadth-first search, astar_fac = 1 is near   *
 *             the usual astar algorithm and astar_fac > 1 are more         *
 *             aggressive.                                                  *
 * max_criticality: The maximum criticality factor (from 0 to 1) any sink   *
 *                  will ever have (i.e. clip criticality to this number).  *
 * criticality_exp: Set criticality to (path_length(sink) / longest_path) ^ *
 *                  criticality_exp (then clip to max_criticality).         *
 * doRouting: true if routing is supposed to be done, false otherwise	    *
 * routing_failure_predictor: sets the configuration to be used by the	    *
 * routing failure predictor, how aggressive the threshold used to judge    *
 * and abort routings deemed unroutable							            *
 * write_rr_graph_name: stores the file name of the output rr graph         *
 * read_rr_graph_name:  stores the file name of the rr graph to be read by vpr */
enum e_route_type {
	GLOBAL, DETAILED
};
enum e_router_algorithm {
	BREADTH_FIRST, TIMING_DRIVEN, NO_TIMING
};
enum e_base_cost_type {
	DELAY_NORMALIZED, DEMAND_ONLY
};
enum e_routing_failure_predictor {
	OFF, SAFE, AGGRESSIVE
};
enum e_routing_budgets_algorithm {
    MINIMAX, SCALE_DELAY, DISABLE
};

enum class e_timing_report_detail {
    NETLIST,            //Only show netlist elements
    AGGREGATED,         //Show aggregated intra-block and inter-block delays
    //DETAILED_ROUTING, //Show inter-block routing resources used
};

enum class e_incr_reroute_delay_ripup {
    ON,
    OFF,
    AUTO
};

constexpr int NO_FIXED_CHANNEL_WIDTH = -1;

struct t_router_opts {
	float first_iter_pres_fac;
	float initial_pres_fac;
	float pres_fac_mult;
	float acc_fac;
	float bend_cost;
	int max_router_iterations;
	int min_incremental_reroute_fanout;
    e_incr_reroute_delay_ripup incr_reroute_delay_ripup;
	int bb_factor;
	enum e_route_type route_type;
	int fixed_channel_width;
    int min_channel_width_hint; //Hint to binary search of what the minimum channel width is
	bool trim_empty_channels;
	bool trim_obs_channels;
	enum e_router_algorithm router_algorithm;
	enum e_base_cost_type base_cost_type;
	float astar_fac;
	float max_criticality;
	float criticality_exp;
	bool verify_binary_search;
	bool full_stats;
	bool congestion_analysis;
	bool fanout_analysis;
    bool switch_usage_analysis;
	e_stage_action doRouting;
	enum e_routing_failure_predictor routing_failure_predictor;
	enum e_routing_budgets_algorithm routing_budgets_algorithm;
    bool save_routing_per_iteration;
    float congested_routing_iteration_threshold_frac;
    e_route_bb_update route_bb_update;
    int router_debug_net;
};

struct t_analysis_opts {
    e_stage_action doAnalysis;

    bool gen_post_synthesis_netlist;

    int timing_report_npaths;
    e_timing_report_detail timing_report_detail;
    bool timing_report_skew;
};

/* Defines the detailed routing architecture of the FPGA.  Only important   *
 * if the route_type is DETAILED.                                           *
 * (UDSD by AY) directionality: Should the tracks be uni-directional or     *
 *                            bi-directional?                               *
 * switch_block_type:  Pattern of switches at each switch block.  I         *
 *           assume Fs is always 3.  If the type is SUBSET, I use a         *
 *           Xilinx-like switch block where track i in one channel always   *
 *           connects to track i in other channels.  If type is WILTON,     *
 *           I use a switch block where track i does not always connect     *
 *           to track i in other channels.  See Steve Wilton, Phd Thesis,   *
 *           University of Toronto, 1996.  The UNIVERSAL switch block is    *
 *           from Y. W. Chang et al, TODAES, Jan. 1996, pp. 80 - 101.       *
 *           A CUSTOM switch block has also been added which allows a user  *
 *           to describe custom permutation functions and connection        *
 *           patterns. See comment at top of SRC/route/build_switchblocks.c *
 * switchblocks: A vector of custom switch block descriptions that is       *
 *           used with the CUSTOM switch block type. See comment at top of  *
 *           SRC/route/build_switchblocks.c                                 *
 * num_segment:  Number of distinct segment types in the FPGA.              *
 * delayless_switch:  Index of a zero delay switch (used to connect things  *
 *                    that should have no delay).                           *
 * wire_to_arch_ipin_switch: keeps track of the type of architecture switch *
 *                           that connects wires to ipins                   *
 * wire_to_rr_ipin_switch: keeps track of the type of RR graph switch that  *
 *                         connects wires to ipins in the RR graph          *
 * R_minW_nmos:  Resistance (in Ohms) of a minimum width nmos transistor.   *
 *               Used only in the FPGA area model.                          *
 * R_minW_pmos:  Resistance (in Ohms) of a minimum width pmos transistor.   *
 *                                                                          *
 * read_rr_graph_filename: File to read the RR graph from (overrides        *
 *                         architecture)                                    *
 * write_rr_graph_filename: File to write the RR graph to after generation  *
 *                                                                          */

struct t_det_routing_arch {
	enum e_directionality directionality; /* UDSD by AY */
	int Fs;
	enum e_switch_block_type switch_block_type;
	std::vector<t_switchblock_inf> switchblocks;
	int num_segment;

	short global_route_switch;
	short delayless_switch;
	int wire_to_arch_ipin_switch;
	int wire_to_rr_ipin_switch;
	float R_minW_nmos;
	float R_minW_pmos;

    std::string read_rr_graph_filename;
    std::string write_rr_graph_filename;
};

enum e_direction : unsigned char {
	INC_DIRECTION = 0,
    DEC_DIRECTION = 1,
    BI_DIRECTION = 2,
    NO_DIRECTION = 3,
    NUM_DIRECTIONS
};

constexpr std::array<const char*, NUM_DIRECTIONS> DIRECTIONS_STRING = { {"INC_DIRECTION", "DEC_DIRECTION", "BI_DIRECTION", "NO_DIRECTION"} };

/* Lists detailed information about segmentation.  [0 .. W-1].              *
 * length:  length of segment.                                              *
 * start:  index at which a segment starts in channel 0.                    *
 * longline:  true if this segment spans the entire channel.                *
 * sb:  [0..length]:  true for every channel intersection, relative to the  *
 *      segment start, at which there is a switch box.                      *
 * cb:  [0..length-1]:  true for every logic block along the segment at     *
 *      which there is a connection box.                                    *
 * arch_wire_switch: Index of the switch type that connects other wires     *
 *                   *to* this segment. Note that this index is in relation *
 *                   to the switches from the architecture file, not the    *
 *                   expanded list of switches that is built at the end of  *
 *                   build_rr_graph.                                        *
 * arch_opin_switch: Index of the switch type that connects output pins     *
 *                   (OPINs) *to* this segment. Note that this index is in  *
 *                   relation to the switches from the architecture file,   *
 *                   not the expanded list of switches that is is built     *
 *                   at the end of build_rr_graph                           *
 * Cmetal: Capacitance of a routing track, per unit logic block length.     *
 * Rmetal: Resistance of a routing track, per unit logic block length.      *
 * direction: The direction of a routing track.                             *
 * index: index of the segment type used for this track.                    *
 * type_name_ptr: pointer to name of the segment type this track belongs    *
 *                to. points to the appropriate name in s_segment_inf       */
struct t_seg_details {
	int length;
	int start;
	bool longline;
	bool *sb;
	bool *cb;
	short arch_wire_switch;
	short arch_opin_switch;
	float Rmetal;
	float Cmetal;
	bool twisted;
	enum e_direction direction;
	int group_start;
	int group_size;
	int seg_start;
	int seg_end;
	int index;
	float Cmetal_per_m; /* Used for power */
	const char *type_name_ptr;
};

/* Defines a 2-D array of t_seg_details data structures (one per channel)   */
typedef vtr::Matrix<t_seg_details*> t_chan_details;

/* A linked list of float pointers.  Used for keeping track of   *
 * which pathcosts in the router have been changed.              */

struct t_linked_f_pointer {
	t_linked_f_pointer *next;
	float *fptr;
};

typedef std::vector<std::vector<std::vector<std::vector<std::vector<int>>>>> t_rr_node_indices; //[0..num_rr_types-1][0..grid_width-1][0..grid_height-1][0..NUM_SIDES-1][0..max_ptc-1]

/* Uncomment lines below to save some memory, at the cost of debugging ease. */
/*enum e_rr_type {SOURCE, SINK, IPIN, OPIN, CHANX, CHANY}; */
/* typedef short t_rr_type */

/* Type of a routing resource node.  x-directed channel segment,   *
 * y-directed channel segment, input pin to a clb to pad, output   *
 * from a clb or pad (i.e. output pin of a net) and:               *
 * SOURCE:  A dummy node that is a logical output within a block   *
 *          -- i.e., the gate that generates a signal.             *
 * SINK:    A dummy node that is a logical input within a block    *
 *          -- i.e. the gate that needs a signal.                  */
typedef enum e_rr_type : unsigned char {
	SOURCE = 0, SINK, IPIN, OPIN, CHANX, CHANY, INTRA_CLUSTER_EDGE, NUM_RR_TYPES
} t_rr_type;

constexpr std::array<t_rr_type, NUM_RR_TYPES> RR_TYPES = { {
	SOURCE, SINK, IPIN, OPIN, CHANX, CHANY, INTRA_CLUSTER_EDGE
} };
constexpr std::array<const char*, NUM_RR_TYPES> rr_node_typename { {
	"SOURCE", "SINK", "IPIN", "OPIN", "CHANX", "CHANY", "INTRA_CLUSTER_EDGE"
} };

/* Basic element used to store the traceback (routing) of each net.        *
 * index:   Array index (ID) of this routing resource node.                *
 * iswitch: Index of the switch type used to go from this rr_node to       *
 *          the next one in the routing.  OPEN if there is no next node    *
 *          (i.e. this node is the last one (a SINK) in a branch of the    *
 *          net's routing).                                                *
 * next:    Pointer to the next traceback element in this route.           */
struct t_trace {
	t_trace *next;
	int index;
	short iswitch;
};

/* Extra information about each rr_node needed only during routing (i.e.    *
 * during the maze expansion).                                              *
 *                                                                          *
 * prev_node:  Index of the previous node (on the lowest cost path known to *
 *             reach this node); used to generate the traceback.  If there  *
 *             is no predecessor, prev_node = NO_PREVIOUS.                  *
 * prev_edge:  Index of the edge (from 0 to num_edges-1 of prev_node) that  *
 *             was used to reach this node from the previous node.  If      *
 *             there is no predecessor, prev_edge = NO_PREVIOUS.            *
 * pres_cost:  Present congestion cost term for this node.                  *
 * acc_cost:   Accumulated cost term from previous Pathfinder iterations.   *
 * path_cost:  Total cost of the path up to and including this node +       *
 *             the expected cost to the target if the timing_driven router  *
 *             is being used.                                               *
 * backward_path_cost:  Total cost of the path up to and including this     *
 *                      node.  Not used by breadth-first router.            *
 * target_flag:  Is this node a target (sink) for the current routing?      *
 *               Number of times this node must be reached to fully route.  *
 * occ:        The current occupancy of the associated rr node              */
struct t_rr_node_route_inf {
	int prev_node;
	short prev_edge;

	float pres_cost;
	float acc_cost;
	float path_cost;
	float backward_path_cost;

	short target_flag;

    public: //Accessors
        short occ() const { return occ_; }

    public: //Mutators
        void set_occ(int new_occ) { occ_ = new_occ; }

    private: //Data
        short occ_ = 0;
};

//Information about the current status of a particular net as pertains to routing
struct t_net_routing_status {
    bool is_routed = false; //Whether the net has been legally routed
    bool is_fixed = false; //Whether the net is fixed (i.e. not to be re-routed)
};



#define NO_PREVIOUS -1

/* Index of the SOURCE, SINK, OPIN, IPIN, etc. member of device_ctx.rr_indexed_data.    */
enum e_cost_indices {
	SOURCE_COST_INDEX = 0,
	SINK_COST_INDEX,
	OPIN_COST_INDEX,
	IPIN_COST_INDEX,
	CHANX_COST_INDEX_START
};

/* Power estimation options */
struct t_power_opts {
	bool do_power; /* Perform power estimation? */
};

/* Channel width data */
struct t_chan_width {
	int max;
	int x_max;
	int y_max;
	int x_min;
	int y_min;
	int *x_list;
	int *y_list;
};

/* Type to store our list of token to enum pairings */
struct t_TokenPair {
	const char *Str;
	int Enum;
};

struct t_lb_type_rr_node; /* Defined in pack_types.h */

/* Store settings for VPR */
struct t_vpr_setup {
	bool TimingEnabled; /* Is VPR timing enabled */
	t_file_name_opts FileNameOpts; /* File names */
	t_model * user_models; /* blif models defined by the user */
	t_model * library_models; /* blif models in VPR */
	t_netlist_opts NetlistOpts; /* Options for packer */
	t_packer_opts PackerOpts; /* Options for packer */
	t_placer_opts PlacerOpts; /* Options for placer */
	t_annealing_sched AnnealSched; /* Placement option annealing schedule */
	t_router_opts RouterOpts; /* router options */
    t_analysis_opts AnalysisOpts; /* Analysis options */
	t_det_routing_arch RoutingArch; /* routing architecture */
	std::vector <t_lb_type_rr_node> *PackerRRGraph;
	t_segment_inf * Segments; /* wires in routing architecture */
	t_timing_inf Timing; /* timing information */
	float constant_net_delay; /* timing information when place and route not run */
	bool ShowGraphics; /* option to show graphics */
	bool gen_netlist_as_blif; /* option to print out post-pack/pre-place netlist as blif */
	int GraphPause; /* user interactiveness graphics option */
	t_power_opts PowerOpts;
    std::string device_layout;
    e_constant_net_method constant_net_method; //How constant nets should be handled
    e_clock_modeling clock_modeling; //How clocks should be handled
    bool exit_before_pack; //Exits early before starting packing (useful for collecting statistics without running/loading any stages)
};

class RouteStatus {
    public:
        RouteStatus() = default;
        RouteStatus(bool status_val, int chan_width_val)
            : success_(status_val)
            , chan_width_(chan_width_val) {}

        //Was routing successful?
        operator bool() { return success(); }
        bool success() { return success_; }

        //What was the channel width?
        int chan_width() { return chan_width_; }

    private:
        bool success_ = false;
        int chan_width_ = -1;
};

typedef vtr::vector_map<ClusterBlockId, std::vector<std::vector<int>>> t_clb_opins_used; //[0..num_blocks-1][0..class-1][0..used_pins-1]

#endif
