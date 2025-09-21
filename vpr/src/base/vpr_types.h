#pragma once
/**
 * @file
 * @brief This is a core file that defines the major data types used by VPR
 *
 * This file is divided into generally 4 major sections:
 *
 * 1.  Global data types and constants
 * 2.  Packing specific data types
 * 3.  Placement specific data types
 * 4.  Routing specific data types
 *
 * Key background file:
 *
 * An understanding of libarchfpga/physical_types.h is crucial to understanding this file.  physical_types.h contains information about the architecture described in the architecture description language
 *
 * Key data structures:
 * t_rr_node - The basic building block of the interconnect in the FPGA architecture
 *
 * Cluster-specific main data structure:
 * t_pb: Stores the mapping between the user netlist and the logic blocks on the FPGA architecture.  For example, if a user design has 10 clusters of 5 LUTs each, you will have 10 t_pb instances of type cluster and within each of those clusters another 5 t_pb instances of type LUT.
 * The t_pb hierarchy follows what is described by t_pb_graph_node
 */

#include <vector>
#include <set>
#include <string_view>
#include "ap_flow_enums.h"
#include "atom_netlist_fwd.h"
#include "clustered_netlist_fwd.h"
#include "constant_nets.h"
#include "clock_modeling.h"
#include "heap_type.h"

#include "vtr_assert.h"
#include "vtr_vector.h"
#include "vtr_flat_map.h"
#include "rr_node_types.h"
#include "rr_graph_fwd.h"
#include "rr_graph_cost.h"
#include "rr_graph_type.h"

/*******************************************************************************
 * Global data types and constants
 ******************************************************************************/
/*#define CREATE_ECHO_FILES*/     /* prints echo files */
/*#define PRINT_SINK_DELAYS*/     /*prints the sink delays to files */
/*#define PRINT_SLACKS*/          /*prints out all slacks in the circuit */
/*#define PRINT_PLACE_CRIT_PATH*/ /*prints out placement estimated critical path */
/*#define PRINT_NET_DELAYS*/      /*prints out delays for all connections */
/*#define PRINT_TIMING_GRAPH*/    /*prints out the timing graph */
/*#define DUMP_BLIF_ECHO*/        /*dump blif of internal representation of user circuit.  Useful for ensuring functional correctness via logical equivalence with input blif*/

//#define ROUTER_DEBUG //Prints out very detailed routing progress information if defined

#define TOKENS " \t\n" /* Input file parsing. */

//#define VERBOSE //Prints additional intermediate data

/*
 * We need to define the maximum number of layers to address a specific issue.
 * For certain data structures, such as `num_sink_pin_layer` in the placer context, dynamically allocating
 * memory based on the number of layers can lead to a performance hit due to additional pointer chasing and
 * cache locality concerns. Defining a constant variable helps optimize the memory allocation process.
 */
constexpr int MAX_NUM_LAYERS = 2;

/**
 * @brief For update_screen. Denotes importance of update.
 *
 * By default MINOR only updates the screen, while MAJOR
 * pauses graphics for the user to interact
 */
enum class ScreenUpdatePriority {
    MINOR = 0,
    MAJOR = 1
};

#ifdef VTR_ENABLE_DEBUG_LOGGING
constexpr bool VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR = true;
#else
constexpr bool VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR = false;
#endif

/*
 * Files
 */
/*******************************************************************************
 * Packing specific data types and constants
 * Packing takes the circuit described in the technology mapped user netlist
 * and maps it to the complex logic blocks found in the architecture
 ******************************************************************************/

#define NOT_VALID (-10000) /* Marks gains that aren't valid */
/* Ensure no gain can ever be this negative! */

// Used for illegal/undefined values of indices, where legal values should be greater or equal to zero
constexpr int UNDEFINED = -1;

///@brief Router lookahead types.
enum class e_router_lookahead {
    ///@brief VPR's classic lookahead (assumes uniform wire types)
    CLASSIC,
    ///@brief Lookahead considering different wire types (see Oleg Petelin's MASc Thesis)
    MAP,
    ///@brief Similar to MAP, but use a sparse sampling of the chip
    COMPRESSED_MAP,
    ///@brief Lookahead with a more extensive node sampling method
    EXTENDED_MAP,
    ///@brief Simple distance-based lookahead
    SIMPLE,
    ///@brief A no-operation lookahead which always returns zero
    NO_OP
};

enum class e_route_bb_update {
    STATIC, ///<Router net bounding boxes are not updated
    DYNAMIC ///<Router net bounding boxes are updated
};

enum class e_router_initial_timing {
    ALL_CRITICAL,
    LOOKAHEAD
};

enum class e_const_gen_inference {
    NONE,    ///<No constant generator inference
    COMB,    ///<Only combinational constant generator inference
    COMB_SEQ ///<Both combinational and sequential constant generator inference
};

enum class e_unrelated_clustering {
    OFF,
    ON,
    AUTO
};

enum class e_balance_block_type_util {
    OFF,
    ON,
    AUTO
};

enum class e_check_route_option {
    OFF,
    QUICK,
    FULL
};

///@brief Selection algorithm for selecting next seed
enum class e_cluster_seed {
    TIMING,
    MAX_INPUTS,
    BLEND,
    MAX_PINS,
    MAX_INPUT_PINS,
    BLEND2
};

struct t_ext_pin_util {
    t_ext_pin_util() = default;
    t_ext_pin_util(float in, float out)
        : input_pin_util(in)
        , output_pin_util(out) {}

    float input_pin_util = 1.;
    float output_pin_util = 1.;
};

/**
 * @brief Specifies the utilization of external input/output pins during packing
 */
class t_ext_pin_util_targets {
  public:
    t_ext_pin_util_targets() = default;
    t_ext_pin_util_targets(float default_in_util, float default_out_util);
    t_ext_pin_util_targets(const std::vector<std::string>& specs);
    t_ext_pin_util_targets& operator=(t_ext_pin_util_targets&& other) noexcept;

    ///@brief Returns the input pin util of the specified block (or default if unspecified)
    t_ext_pin_util get_pin_util(std::string_view block_type_name) const;

    ///@brief Returns a string describing input/output pin utilization targets
    std::string to_string() const;

  public:
    /**
     * @brief Sets the pin util for the specified block type
     * @return true if non-default was previously set
     */
    void set_block_pin_util(const std::string& block_type_name, t_ext_pin_util target);

    /**
     * @brief Sets the default pin util
     * @return Returns true if a default was previously set
     */
    void set_default_pin_util(t_ext_pin_util target);

  private:
    t_ext_pin_util defaults_;
    std::map<std::string, t_ext_pin_util, std::less<>> overrides_;
};

class t_pack_high_fanout_thresholds {
  public:
    t_pack_high_fanout_thresholds() = default;
    explicit t_pack_high_fanout_thresholds(int threshold);
    explicit t_pack_high_fanout_thresholds(const std::vector<std::string>& specs);
    t_pack_high_fanout_thresholds& operator=(t_pack_high_fanout_thresholds&& other) noexcept;

    ///@brief Returns the high fanout threshold of the specifi  ed block
    int get_threshold(std::string_view block_type_name) const;

    ///@brief Returns a string describing high fanout thresholds for different block types
    std::string to_string() const;

  public:
    /**
     * @brief Sets the pin util for the specified block type
     * @return true if non-default was previously set
     */
    void set(const std::string& block_type_name, int threshold);

    /**
     * @brief Sets the default pin util
     * @return true if a default was previously set
     */
    void set_default(int threshold);

  private:
    int default_;
    std::map<std::string, int, std::less<>> overrides_;
};

/* these are defined later, but need to declare here because it is used */
class t_rr_node;
struct t_pb_stats;
struct t_pb_route;

typedef vtr::flat_map2<int, t_pb_route> t_pb_routes;

/**
 * @brief A t_pb represents an instance of a clustered block.
 *
 * The instance may be:
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
class t_pb {
  public:
    char* name = nullptr;                     ///<Name of this physical block
    t_pb_graph_node* pb_graph_node = nullptr; ///<pointer to pb_graph_node this pb corresponds to

    int mode = 0; ///<mode that this pb is set to

    t_pb** child_pbs = nullptr; ///<children pbs attached to this pb [0..num_child_pb_types - 1][0..child_type->num_pb - 1]
    t_pb* parent_pb = nullptr;  ///<pointer to parent node

    t_pb_stats* pb_stats = nullptr; ///<statistics for current pb

    /**
     * @brief  Representation of intra-logic block routing, t_pb_route describes all internal hierarchy routing.
     *
     * t_pb_route is an array of size [t_pb->pb_graph_node->total_pb_pins]
     * Only valid for the top-level t_pb (parent_pb == nullptr). On any child pb, t_pb_route will be nullptr. */
    t_pb_routes pb_route;

    int clock_net = 0; ///<Records clock net driving a flip-flop, valid only for lowest-level, flip-flop PBs

    // Member functions

    ///@brief Returns true if this block has not parent pb block
    bool is_root() const { return parent_pb == nullptr; }

    ///@brief Returns true if this pb corresponds to a primitive block (i.e. LUT, FF, etc.)
    bool is_primitive() const { return child_pbs == nullptr; }

    ///@brief Returns true if this pb has modes
    bool has_modes() const { return this->pb_graph_node->pb_type->num_modes > 0; }

    int get_num_child_types() const;

    int get_num_children_of_type(int type_index) const;

    t_mode* get_mode() const;

    /**
     * @brief Returns the read-only t_pb associated with the specified gnode which is contained
     *        within the current pb
     */
    const t_pb* find_pb(const t_pb_graph_node* gnode) const;

    /**
     * @brief Returns the mutable t_pb associated with the specified gnode which is contained
     *        within the current pb
     */
    t_pb* find_mutable_pb(const t_pb_graph_node* gnode);

    const t_pb* find_pb_for_model(LogicalModelId blif_model_id) const;

    ///@brief Returns the root pb containing this pb
    const t_pb* root_pb() const;

    /**
     * @brief  Returns a string containing the hierarchical type name of a physical block
     *
     * Ex: clb[0][default]/lab[0][default]/fle[3][n1_lut6]/ble6[0][default]/lut6[0]
     */
    std::string hierarchical_type_name() const;

    /**
     * @brief Returns the bit index into the AtomPort for the specified primitive
     *        pb_graph_pin, considering any pin rotations which have been applied to logically
     *        equivalent pins
     */
    BitIndex atom_pin_bit_index(const t_pb_graph_pin* gpin) const;

    /**
     * @brief For a given gpin, sets the mapping to the original atom
     *         netlist pin's bit index in it's AtomPort.
     *
     * This is used to record any pin rotations which have been applied to
     * logically equivalent pins
     */
    void set_atom_pin_bit_index(const t_pb_graph_pin* gpin, BitIndex atom_pin_bit_idx);

  private:
    /**
     * @brief Contains the atom netlist port bit index associated
     *        with any primitive pins which have been rotated during clustering
     */
    std::map<const t_pb_graph_pin*, BitIndex> pin_rotations_;
};

///@brief Representation of intra-logic block routing
struct t_pb_route {
    AtomNetId atom_net_id;                        ///<which net in the atom netlist uses this pin
    int driver_pb_pin_id = UNDEFINED;             ///<The pb_pin id of the pb_pin that drives this pin
    std::vector<int> sink_pb_pin_ids;             ///<The pb_pin id's of the pb_pins driven by this node
    const t_pb_graph_pin* pb_graph_pin = nullptr; ///<The graph pin associated with this node
};

/******************************************************************
 * Timing data types
 *******************************************************************/

/* Cluster timing delays:
 * C_ipin_cblock: Capacitance added to a routing track by the isolation     *
 *                buffer between a track and the Cblocks at an (i,j) loc.   *
 * T_ipin_cblock: Delay through an input pin connection box (from a         *
 *                   routing track to a logic block input pin).             */
struct t_timing_inf {
    bool timing_analysis_enabled;
    float C_ipin_cblock;
    std::string SDCFile;
};

enum class e_timing_update_type {
    FULL,
    INCREMENTAL,
    AUTO
};

/***************************************************************************
 * Placement and routing data types
 ****************************************************************************/

/* Values of number of placement available move types */
constexpr int NUM_PL_MOVE_TYPES = 7;
constexpr int NUM_PL_NONTIMING_MOVE_TYPES = 3;

/* Timing data structures end */
enum class e_sched_type {
    AUTO_SCHED,
    USER_SCHED
};
/* Annealing schedule */

enum pic_type {
    NO_PICTURE,
    PLACEMENT,
    ROUTING
};
/* What's on screen? */

enum pfreq {
    PLACE_NEVER,
    PLACE_ONCE,
    PLACE_ALWAYS
};

///@brief  Power data for t_netlist structure

struct t_net_power {
    ///@brief Signal probability - long term probability that signal is logic-high
    float probability;

    /**
     * @brief Transition density - average # of transitions per clock cycle
     *
     * For example, a clock would have density = 2
     */
    float density;
};

/**
 * @brief Stores a 3D bounding box in terms of the minimum and
 *        maximum coordinates: x, y, layer
 * 
 * @var xmin: The minimum x-coordinate of the bounding box
 * @var xmax: The maximum x-coordinate of the bounding box
 * @var ymin: The minimum y-coordinate of the bounding box
 * @var ymax: The maximum y-coordinate of the bounding box
 * @var layer_min: The minimum layer of the bounding box
 * @var layer_max: The maximum layer of the bounding box
 */
struct t_bb {
    t_bb() = default;
    t_bb(int xmin_, int xmax_, int ymin_, int ymax_, int layer_min_, int layer_max_)
        : xmin(xmin_)
        , xmax(xmax_)
        , ymin(ymin_)
        , ymax(ymax_)
        , layer_min(layer_min_)
        , layer_max(layer_max_) {
        VTR_ASSERT(xmax_ >= xmin_);
        VTR_ASSERT(ymax_ >= ymin_);
        VTR_ASSERT(layer_max_ >= layer_min_);
    }
    int xmin = UNDEFINED;
    int xmax = UNDEFINED;
    int ymin = UNDEFINED;
    int ymax = UNDEFINED;
    int layer_min = UNDEFINED;
    int layer_max = UNDEFINED;
};

/**
 * @brief Stores a 2D bounding box in terms of the minimum and maximum x and y
 * @note layer_num indicates the layer that the bounding box is on.
 */
struct t_2D_bb {
    t_2D_bb() = default;
    t_2D_bb(int xmin_, int xmax_, int ymin_, int ymax_, int layer_num_)
        : xmin(xmin_)
        , xmax(xmax_)
        , ymin(ymin_)
        , ymax(ymax_)
        , layer_num(layer_num_) {
        VTR_ASSERT(xmax_ >= xmin_);
        VTR_ASSERT(ymax_ >= ymin_);
        VTR_ASSERT(layer_num_ >= 0);
    }

    int xmin = UNDEFINED;
    int xmax = UNDEFINED;
    int ymin = UNDEFINED;
    int ymax = UNDEFINED;
    int layer_num = UNDEFINED;
};

/**
 * @brief An offset between placement locations (t_pl_loc)
 * @note In the case of comparing the offset, the layer offset should be equal
 * x: x-offset
 * y: y-offset
 * sub_tile: sub_tile-offset
 * layer: layer-offset
 */
struct t_pl_offset {
    t_pl_offset() = default;
    t_pl_offset(int xoffset, int yoffset, int sub_tile_offset, int layer_offset)
        : x(xoffset)
        , y(yoffset)
        , sub_tile(sub_tile_offset)
        , layer(layer_offset) {}

    int x = 0;
    int y = 0;
    int sub_tile = 0;
    int layer = 0;

    t_pl_offset& operator+=(const t_pl_offset& rhs) {
        x += rhs.x;
        y += rhs.y;
        sub_tile += rhs.sub_tile;
        layer += rhs.layer;
        return *this;
    }

    t_pl_offset& operator-=(const t_pl_offset& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        sub_tile -= rhs.sub_tile;
        layer -= rhs.layer;
        return *this;
    }

    friend t_pl_offset operator+(t_pl_offset lhs, const t_pl_offset& rhs) {
        lhs += rhs;
        return lhs;
    }

    friend t_pl_offset operator-(t_pl_offset lhs, const t_pl_offset& rhs) {
        lhs -= rhs;
        return lhs;
    }

    friend t_pl_offset operator-(const t_pl_offset& other) {
        return t_pl_offset(-other.x, -other.y, -other.sub_tile, -other.layer);
    }
    friend t_pl_offset operator+(const t_pl_offset& other) {
        return t_pl_offset(+other.x, +other.y, +other.sub_tile, +other.layer);
    }

    friend bool operator<(const t_pl_offset& lhs, const t_pl_offset& rhs) {
        VTR_ASSERT(lhs.layer == rhs.layer);
        return std::tie(lhs.x, lhs.y, lhs.sub_tile) < std::tie(rhs.x, rhs.y, rhs.sub_tile);
    }

    friend bool operator==(const t_pl_offset& lhs, const t_pl_offset& rhs) {
        return std::tie(lhs.x, lhs.y, lhs.sub_tile, lhs.layer) == std::tie(rhs.x, rhs.y, rhs.sub_tile, rhs.layer);
    }

    friend bool operator!=(const t_pl_offset& lhs, const t_pl_offset& rhs) {
        return !(lhs == rhs);
    }
};

namespace std {
template<>
struct hash<t_pl_offset> {
    std::size_t operator()(const t_pl_offset& v) const noexcept {
        std::size_t seed = std::hash<int>{}(v.x);
        vtr::hash_combine(seed, v.y);
        vtr::hash_combine(seed, v.sub_tile);
        vtr::hash_combine(seed, v.layer);
        return seed;
    }
};
} // namespace std

/// @brief Sentinel value for indicating that a block does not have a valid x location, used to check whether a block has been placed
static constexpr int INVALID_X = -1;

/**
 * @brief A placement location coordinate
 *
 * x: x-coordinate
 * y: y-coordinate
 * sub_tile: sub-tile number (capacity position)
 * layer: layer (die) number
 *
 * @note t_pl_offset should be used to represent an offset between t_pl_loc.
 */
struct t_pl_loc {
    t_pl_loc() = default;
    t_pl_loc(int xloc, int yloc, int sub_tile_loc, int layer_num)
        : x(xloc)
        , y(yloc)
        , sub_tile(sub_tile_loc)
        , layer(layer_num) {}
    t_pl_loc(const t_physical_tile_loc& phy_loc, int sub_tile_loc)
        : x(phy_loc.x)
        , y(phy_loc.y)
        , sub_tile(sub_tile_loc)
        , layer(phy_loc.layer_num) {}

    int x = UNDEFINED;
    int y = UNDEFINED;
    int sub_tile = UNDEFINED;
    int layer = UNDEFINED;

    t_pl_loc& operator+=(const t_pl_offset& rhs) {
        layer += rhs.layer;
        x += rhs.x;
        y += rhs.y;
        sub_tile += rhs.sub_tile;
        return *this;
    }

    t_pl_loc& operator-=(const t_pl_offset& rhs) {
        layer -= rhs.layer;
        x -= rhs.x;
        y -= rhs.y;
        sub_tile -= rhs.sub_tile;
        return *this;
    }

    friend t_pl_loc operator+(t_pl_loc lhs, const t_pl_offset& rhs) {
        lhs += rhs;
        return lhs;
    }
    friend t_pl_loc operator+(t_pl_offset lhs, const t_pl_loc& rhs) {
        return rhs + lhs;
    }

    friend t_pl_loc operator-(t_pl_loc lhs, const t_pl_offset& rhs) {
        lhs -= rhs;
        return lhs;
    }
    friend t_pl_loc operator-(t_pl_offset lhs, const t_pl_loc& rhs) {
        return rhs - lhs;
    }

    friend t_pl_offset operator-(const t_pl_loc& lhs, const t_pl_loc& rhs) {
        return {lhs.x - rhs.x,
                lhs.y - rhs.y,
                lhs.sub_tile - rhs.sub_tile,
                lhs.layer - rhs.layer};
    }

    friend bool operator<(const t_pl_loc& lhs, const t_pl_loc& rhs) {
        VTR_ASSERT(lhs.layer == rhs.layer);
        return std::tie(lhs.x, lhs.y, lhs.sub_tile) < std::tie(rhs.x, rhs.y, rhs.sub_tile);
    }

    friend bool operator==(const t_pl_loc& lhs, const t_pl_loc& rhs) {
        return std::tie(lhs.layer, lhs.x, lhs.y, lhs.sub_tile) == std::tie(rhs.layer, rhs.x, rhs.y, rhs.sub_tile);
    }

    friend bool operator!=(const t_pl_loc& lhs, const t_pl_loc& rhs) {
        return !(lhs == rhs);
    }
};

namespace std {
template<>
struct hash<t_pl_loc> {
    std::size_t operator()(const t_pl_loc& v) const noexcept {
        std::size_t seed = std::hash<int>{}(v.x);
        vtr::hash_combine(seed, v.y);
        vtr::hash_combine(seed, v.sub_tile);
        vtr::hash_combine(seed, v.layer);
        return seed;
    }
};
} // namespace std

/**
 * @brief  Represents the placement location of a clustered block
 *
 * x: x-coordinate
 * y: y-coordinate
 * z: occupancy coordinate
 * is_fixed: true if this block's position is fixed by the user and shouldn't be moved during annealing
 */
struct t_block_loc {
    t_pl_loc loc;

    bool is_fixed = false;
};

///@brief Names of various files
struct t_file_name_opts {
    std::string ArchFile;
    std::string CircuitName;
    std::string CircuitFile;
    std::string NetFile;
    std::string FlatPlaceFile;
    std::string PlaceFile;
    std::string RouteFile;
    std::string FPGAInterchangePhysicalFile;
    std::string ActFile;
    std::string PowerFile;
    std::string CmosTechFile;
    std::string out_file_prefix;
    std::string read_vpr_constraints_file;
    std::string write_vpr_constraints_file;
    std::string write_constraints_file;
    std::string read_flat_place_file;
    std::string write_flat_place_file;
    std::string write_legalized_flat_place_file;
    std::string write_block_usage;
    bool verify_file_digests;
};

///@brief Options for netlist loading
struct t_netlist_opts {
    e_const_gen_inference const_gen_inference = e_const_gen_inference::COMB;
    bool absorb_buffer_luts = true;
    bool sweep_dangling_primary_ios = true;
    bool sweep_dangling_blocks = true;
    bool sweep_dangling_nets = true;
    bool sweep_constant_primary_outputs = false;

    int netlist_verbosity = 1; ///<Verbose output during netlist cleaning
};

/**
 * @brief Specifies the action to take for a CAD flow stage.
 * 
 * @details
 * SKIP - Do not perform this algorithm at all (End flow early).
 * LOAD - Load previous result from file.
 * DO - Run the specified algorithm.
 * SKIP_IF_PRIOR_FAIL - Run the specified algorithm if possible. 
 * Currently used to avoid analysis if we don't succeed at routing.
 */
enum class e_stage_action {
    SKIP = 0,
    LOAD,
    DO,
    SKIP_IF_PRIOR_FAIL,
    NUM_STAGE_ACTIONS
};

///@brief String representations of e_stage_action
constexpr vtr::array<e_stage_action, const char*, (size_t)e_stage_action::NUM_STAGE_ACTIONS> stage_action_strings{
    "DISABLED", "LOAD", "ENABLED", "SKIP IF PRIOR FAIL"};

/**
 * @brief Options for packing
 *
 *   @param circuit_file_name
 *          Path to technology mapped user circuit in BLIF format.
 *   @param output_file
 *          Path to packed user circuit in net format.
 *   @param timing_driven
 *          Whether or not to do timing driven clustering. (Default: on)
 *   @param timing_gain_weight
 *          Controls the optimization of timing vs area in timing driven
 *          clustering. 
 *          A value of 0 focuses only on area; 1 focuses only on timing.
 *          (Default: 0.75)
 *   @param connection_gain_weight
 *          Controls the optimization of smaller net absorption vs. signal 
 *          sharing in connection driven clustering. 
 *          A value of 0 focuses solely on signal sharing; a value of 1 
 *          focuses solely on absorbing smaller nets into a cluster. 
 *          (Default: 0.9)
 *   @param cluster_seed_type
 *          Selection algorithm for selecting next seed. (Default: blend2 if 
 *          timing_driven is on; max_inputs otherwise)
 *   @param target_device_utilization
 *          Sets the target device utilization. (Default: 1.0)
 *   @param allow_unrelated_clustering     
 *          Allows primitives which have no attraction to the given cluster
 *          to be packed into it. (Default: auto)
 *   @param connection_driven
 *          Controls whether or not packing prioritizes the absorption of nets 
 *          with fewer connections into a complex logic block over nets with 
 *          more connections. (Default: on)
 *   @param pack_verbosity
 *          Controls how verbose clustering's output is. (Default: 2)
 *   @param enable_pin_feasibility_filter
 *          Counts the number of available pins in groups/classes of mutually 
 *          connected pins within a cluster, then filters out candidate 
 *          primitives/atoms/molecules for which the cluster has insufficient 
 *          pins to route (without performing a full routing). (Default: on)
 *   @param balance_block_type_utilization
 *          If enabled, when a primitive can potentially be mapped to multiple 
 *          block types the packer will pick the block type which (currently) 
 *          has the lowest utilization. (Default: auto)
 *   @param target_external_pin_util
 *          Sets the external pin utilization target. (Default: auto)
 *   @param prioritize_transitive_connectivity
 *          Whether transitive connectivity is prioritized over high-fanout 
 *          connectivity. (Default: on)
 *   @param feasible_block_array_size
 *          Max size of the priority queue for candidates that pass the early 
 *          filter legality test, but not the more detailed routing test.
 *          (Default: 30)
 *   @param doPacking
 *          Run packing stage.
 *   @param device_layout
 *          Controls which device layout/floorplan is used from the 
 *          architecture file. (Default: smallest device which satisfies the 
 *          circuit's resource requirements)
 *   @param timing_update_type
 *          Controls how timing analysis updates are performed. (Default: auto)
 *   @param load_flat_placement
 *          Whether to reconstruct a packing solution from a flat placement
 *          file. (Default: off; on if <stage option: --legalize> is on)
 */
struct t_packer_opts {
    std::string circuit_file_name;
    std::string sdc_file_name;
    std::string output_file;
    bool timing_driven;
    enum e_cluster_seed cluster_seed_type;
    float timing_gain_weight;
    float connection_gain_weight;
    float target_device_utilization;
    e_unrelated_clustering allow_unrelated_clustering;
    bool connection_driven;
    int pack_verbosity;
    bool enable_pin_feasibility_filter;
    e_balance_block_type_util balance_block_type_utilization;
    std::vector<std::string> target_external_pin_util;
    bool prioritize_transitive_connectivity;
    std::vector<std::string> high_fanout_threshold;
    int transitive_fanout_threshold;
    int feasible_block_array_size;
    e_stage_action doPacking;
    std::string device_layout;
    e_timing_update_type timing_update_type;
    bool load_flat_placement = false;
};

/**
 * @brief Annealing schedule information for the placer.
 *
 * The schedule type is either USER_SCHED or AUTO_SCHED. Inner_num is
 * multiplied by num_blocks^4/3 to find the number of moves per temperature.
 * The remaining information is used only for USER_SCHED, and have
 * the obvious meanings.
 */
struct t_annealing_sched {
    e_sched_type type;
    float inner_num;
    float init_t;
    float alpha_t;
    float exit_t;
};

/******************************************************************
 * Placer data types
 *******************************************************************/

/**
 * @brief Types of placement algorithms used in the placer.
 *
 *   @param BOUNDING_BOX_PLACE
 *              Focuses purely on minimizing the bounding
 *              box wirelength of the circuit.
 *   @param CRITICALITY_TIMING_PLACE
 *              Focuses on minimizing both the wirelength and the
 *              connection timing costs (criticality * delay).
 *   @param SLACK_TIMING_PLACE
 *              Focuses on improving the circuit slack values
 *              to reduce critical path delay.
 *
 * The default is to use CRITICALITY_TIMING_PLACE. BOUNDING_BOX_PLACE
 * is used when there is no timing information available (wiring only).
 * SLACK_TIMING_PLACE is mainly feasible during placement quench.
 */
enum class e_place_algorithm {
    BOUNDING_BOX_PLACE,
    CRITICALITY_TIMING_PLACE,
    SLACK_TIMING_PLACE
};

enum class e_place_bounding_box_mode {
    AUTO_BB,
    CUBE_BB,
    PER_LAYER_BB
};

/**
 * @brief Provides a wrapper around enum e_place_algorithm.
 *
 * Supports the method is_timing_driven(), which allows flexible updates
 * to the placer algorithms if more timing driven placement strategies
 * are added in tht future. This method is used across various placement
 * setup files, and it can be useful for major placer routines as well.
 *
 * More methods can be added to this class if the placement strategies
 * will be further divided into more categories the future.
 *
 * Also supports assignments and comparisons between t_place_algorithm
 * and e_place_algorithm so as not to break down previous codes.
 */
class t_place_algorithm {
  public:
    //Constructors
    t_place_algorithm() = default;
    t_place_algorithm(const t_place_algorithm&) = default;
    t_place_algorithm(e_place_algorithm _algo)
        : algo(_algo) {}
    ~t_place_algorithm() = default;

    //Assignment operators
    t_place_algorithm& operator=(const t_place_algorithm& rhs) {
        algo = rhs.algo;
        return *this;
    }
    t_place_algorithm& operator=(e_place_algorithm rhs) {
        algo = rhs;
        return *this;
    }

    //Equality operators
    bool operator==(const t_place_algorithm& rhs) const { return algo == rhs.algo; }
    bool operator==(e_place_algorithm rhs) const { return algo == rhs; }
    bool operator!=(const t_place_algorithm& rhs) const { return algo != rhs.algo; }
    bool operator!=(e_place_algorithm rhs) const { return algo != rhs; }

    ///@brief Check if the algorithm belongs to the timing driven category.
    inline bool is_timing_driven() const {
        return algo == e_place_algorithm::CRITICALITY_TIMING_PLACE || algo == e_place_algorithm::SLACK_TIMING_PLACE;
    }

    ///@brief Accessor: returns the underlying e_place_algorithm enum value.
    e_place_algorithm get() const { return algo; }

  private:
    ///@brief The underlying algorithm. Default set to CRITICALITY_TIMING_PLACE.
    e_place_algorithm algo = e_place_algorithm::CRITICALITY_TIMING_PLACE;
};

enum class e_pad_loc_type {
    FREE,
    RANDOM
};

/**
 * @brief Used to determine the RL agent's algorithm
 *
 * This algorithm controls the exploration-exploitation and how we select the new action
 * Currently, the supported algorithms are: epsilon greedy and softmax
 * For more details, check simpleRL_move_generator.cpp
 */
enum class e_agent_algorithm {
    E_GREEDY,
    SOFTMAX
};

/**
 * @brief Used to determines the dimensionality of the RL agent exploration space
 *
 * Agent exploration space can be either based on only move types or
 * can be based on (block_type, move_type) pair.
 *
 */
enum class e_agent_space {
    MOVE_TYPE,
    MOVE_BLOCK_TYPE
};

///@brief Used to calculate the inner placer loop's block swapping limit move_lim.
enum e_place_effort_scaling {
    CIRCUIT,       ///<Effort scales based on circuit size only
    DEVICE_CIRCUIT ///<Effort scales based on both circuit and device size
};

enum class PlaceDelayModelType {
    SIMPLE,
    DELTA,          ///<Delta x/y based delay model
    DELTA_OVERRIDE, ///<Delta x/y based delay model with special case delay overrides
};

enum class e_reducer {
    MIN,
    MAX,
    MEDIAN,
    ARITHMEAN,
    GEOMEAN
};

enum class e_file_type {
    PDF,
    PNG,
    SVG,
    NONE
};

enum class e_place_delta_delay_algorithm {
    ASTAR_ROUTE,
    DIJKSTRA_EXPANSION,
};

/**
 * @brief Enumeration of the different initial temperature estimators available
 *        for the placer.
 */
enum class e_anneal_init_t_estimator {
    COST_VARIANCE,      ///<Estimate the initial temperature using the variance in cost of a set of trial swaps.
    EQUILIBRIUM,        ///<Estimate the initial temperature by predicting the equilibrium temperature for the initial placement.
};

enum class e_move_type;

/**
 * @brief Various options for the placer.
 *
 *   @param place_algorithm
 *              Controls which placement algorithm is used.
 *   @param place_quench_algorithm
 *              Controls which placement algorithm is used
 *              during placement quench.
 *   @param timing_tradeoff
 *              When in CRITICALITY_TIMING_PLACE mode, what is the
 *              tradeoff between timing and wiring costs.
 *   @param place_chan_width
 *              The channel width assumed if only one placement is performed.
 *   @param pad_loc_type
 *              Are pins FREE or fixed randomly.
 *   @param constraints_file
 *              File that specifies locations of locked down (constrained)
 *              blocks for placement. Empty string means no constraints file.
 *   @param write_initial_place_file
 *              Write the initial placement into this file. Empty string means
 *              the initial placement is not written.
 *   @param pad_loc_file
 *              File to read pad locations from if pad_loc_type is USER.
 *   @param place_freq
 *              Should the placement be skipped, done once, or done
 *              for each channel width in the binary search. (Default: ONCE)
 *   @param recompute_crit_iter
 *              How many temperature stages pass before we recompute
 *              criticalities based on the current placement and its
 *              estimated point-to-point delays.
 *   @param inner_loop_crit_divider
 *              (move_lim/inner_loop_crit_divider) determines how
 *              many inner_loop iterations pass before a recompute
 *              of criticalities is done.
 *   @param td_place_exp_first
 *              Exponent that is used in the CRITICALITY_TIMING_PLACE
 *              mode to specify the initial value of `crit_exponent`.
 *              After we map the slacks to criticalities, this value
 *              is used to `sharpen` the criticalities, making connections
 *              with worse slacks more critical.
 *   @param td_place_exp_last
 *              Value that the crit_exponent will be at the end.
 *   @param doPlacement
 *              True if placement is supposed to be done in the CAD flow.
 *              False if otherwise.
 *   @param place_constraint_expand
 *              Integer value that specifies how far to expand the floorplan
 *              region when printing out floorplan constraints based on
 *              current placement.
 *   @param place_constraint_subtile
 *              True if subtiles should be specified when printing floorplan
 *              constraints. False if not.
 *   @param place_auto_init_t_scale
 *              When the annealer is using the automatic schedule, this option
 *              scales the initial temperature selected.
 *   @param anneal_init_t_estimator
 *              When the annealer is using the automatic schedule, this option
 *              selects which estimator is used to select an initial temperature.
 */
struct t_placer_opts {
    t_place_algorithm place_algorithm;
    t_place_algorithm place_quench_algorithm;
    t_annealing_sched anneal_sched; ///<Placement option annealing schedule
    float timing_tradeoff;
    int place_chan_width;
    enum e_pad_loc_type pad_loc_type;
    std::string constraints_file;
    std::string write_initial_place_file;
    std::string read_initial_place_file;
    enum pfreq place_freq;
    int recompute_crit_iter;
    int inner_loop_recompute_divider;
    int quench_recompute_divider;
    float td_place_exp_first;
    int seed;
    float td_place_exp_last;
    e_stage_action doPlacement;
    float rlim_escape_fraction;
    std::string move_stats_file;
    int placement_saves_per_temperature;
    e_place_effort_scaling effort_scaling;
    e_timing_update_type timing_update_type;

    PlaceDelayModelType delay_model_type;
    e_reducer delay_model_reducer;

    float delay_offset;
    int delay_ramp_delta_threshold;
    float delay_ramp_slope;
    float tsu_rel_margin;
    float tsu_abs_margin;

    std::string post_place_timing_report_file;

    bool strict_checks;

    std::string write_placement_delay_lookup;
    std::string read_placement_delay_lookup;
    vtr::vector<e_move_type, float> place_static_move_prob;
    bool RL_agent_placement;
    bool place_agent_multistate;
    bool place_checkpointing;
    int place_high_fanout_net;
    e_place_bounding_box_mode place_bounding_box_mode;
    e_agent_algorithm place_agent_algorithm;
    float place_agent_epsilon;
    float place_agent_gamma;
    float place_dm_rlim;
    e_agent_space place_agent_space;
    std::string place_reward_fun;
    float place_crit_limit;
    int place_constraint_expand;
    bool place_constraint_subtile;
    int floorplan_num_horizontal_partitions;
    int floorplan_num_vertical_partitions;
    bool place_quench_only;

    int placer_debug_block;
    int placer_debug_net;

    /**
     * @brief Tile types that should be used during delay sampling.
     *
     * Useful for excluding tiles that have abnormal delay behavior, e.g.
     * clock tree elements like PLL's, global/local clock buffers, etc.
     */
    std::string allowed_tiles_for_delay_model;

    e_place_delta_delay_algorithm place_delta_delay_matrix_calculation_method;

    float place_auto_init_t_scale;

    e_anneal_init_t_estimator anneal_init_t_estimator;
};

/******************************************************************
 * Analytical Placer data types
 *******************************************************************/

/**
 * @brief Various options for the Analytical Placer.
 *
 *   @param doAnalyticalPlacement
 *              True if analytical placement is supposed to be done in the CAD
 *              flow. False if otherwise.
 *   @param analytical_solver_type
 *              The type of analytical solver the Global Placer in the AP flow
 *              will use.
 *   @param partial_legalizer_type
 *              The type of partial legalizer the Global Placer in the AP flow
 *              will use.
 *   @param full_legalizer_type
 *              The type of full legalizer the AP flow will use.
 *   @param detailed_placer_type
 *              The type of detailed placter the AP flow will use.
 *   @param ap_timing_tradeoff
 *              A trade-off parameter used to decide how focused the AP flow
 *              should be on optimizing timing over wirelength.
 *   @param ap_high_fanout_threshold;
 *              The threshold to ignore nets with higher fanout than that
 *              value while constructing the solver.
 *   @param ap_partial_legalizer_target_density
 *              Vector of strings passed by the user to configure the target
 *              density of different physical tiles on the device.
 *   @param appack_max_dist_th
 *              Array of string passed by the user to configure the max candidate
 *              distance thresholds.
 *   @param appack_unrelated_clustering_args
 *              Array of strings passed by the user to configure the unrelated
 *              clustering parameters used by APPack.
 *   @param num_threads
 *              The number of threads the AP flow can use.
 *   @param log_verbosity
 *              The verbosity level of log messages in the AP flow, with higher
 *              values leading to more verbose messages.
 *   @param generate_mass_report
 *              Whether to generate a mass report during global placement or not.
 */
struct t_ap_opts {
    e_stage_action doAP;

    e_ap_analytical_solver analytical_solver_type;

    e_ap_partial_legalizer partial_legalizer_type;

    e_ap_full_legalizer full_legalizer_type;

    e_ap_detailed_placer detailed_placer_type;

    float ap_timing_tradeoff;

    int ap_high_fanout_threshold;

    std::vector<std::string> ap_partial_legalizer_target_density;

    std::vector<std::string> appack_max_dist_th;

    std::vector<std::string> appack_unrelated_clustering_args;

    unsigned num_threads;

    int log_verbosity;

    bool generate_mass_report;
};

/******************************************************************
 * Router data types
 *******************************************************************/

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
 * min_incremental_reroute_fanout: Minimum fanout a net needs to have       *
 *              for incremental reroute to be applied to it through route   *
 *              tree pruning. Larger circuits should get larger thresholds  *
 * bb_factor:  Linear distance a route can go outside the net bounding      *
 *             box.                                                         *
 * route_type:  GLOBAL or DETAILED.                                         *
 * fixed_channel_width:  Only attempt to route the design once, with the    *
 *                       channel width given.  If this variable is          *
 *                       == NO_FIXED_CHANNEL_WIDTH, do a binary search      *
 *                       on channel width.                                  *
 * router_algorithm:  TIMING_DRIVEN or PARALLEL.  Selects the desired       *
 * routing algorithm.                                                       *
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
 * astar_offset: Offset that is subtracted from the lookahead (expected     *
 *               future costs) in the timing-driven router.                 *
 * max_criticality: The maximum criticality factor (from 0 to 1) any sink   *
 *                  will ever have (i.e. clip criticality to this number).  *
 * criticality_exp: Set criticality to (path_length(sink) / longest_path) ^ *
 *                  criticality_exp (then clip to max_criticality).         *
 * doRouting: true if routing is supposed to be done, false otherwise       *
 * routing_failure_predictor: sets the configuration to be used by the      *
 * routing failure predictor, how aggressive the threshold used to judge    *
 * and abort routings deemed unroutable                                     *
 * write_rr_graph_name: stores the file name of the output rr graph         *
 * read_rr_graph_name:  stores the file name of the rr graph to be read by vpr */

enum e_router_algorithm {
    NESTED,
    PARALLEL,
    PARALLEL_DECOMP,
    TIMING_DRIVEN,
};

enum e_routing_failure_predictor {
    OFF,
    SAFE,
    AGGRESSIVE
};

// How to allocate budgets, and if RCV should be enabled
enum e_routing_budgets_algorithm {
    MINIMAX, // Use MINIMAX-PERT algorithm to allocate budgets
    YOYO,    // Use MINIMAX as above, and enable RCV algorithm to resolve negative hold slack
    SCALE_DELAY,
    DISABLE // Do not allocate budgets and run default router
};

enum class e_timing_report_detail {
    NETLIST,          //Only show netlist elements
    AGGREGATED,       //Show aggregated intra-block and inter-block delays
    DETAILED_ROUTING, //Show inter-block routing resources used
    DEBUG,            //Show additional internal debugging information
};

enum class e_post_synth_netlist_unconn_handling {
    UNCONNECTED, // Leave unrouted ports unconnected
    NETS,        // Leave unrouted ports unconnected but add new named nets to each of them
    GND,         // Tie unrouted ports to ground (only for input ports)
    VCC          // Tie unrouted ports to VCC (only for input ports)
};

struct t_timing_analysis_profile_info {
    double timing_analysis_wallclock_time() const {
        return sta_wallclock_time + slack_wallclock_time;
    }

    size_t num_full_updates() const {
        return num_full_setup_updates + num_full_hold_updates + num_full_setup_hold_updates;
    }

    double sta_wallclock_time = 0.;
    double slack_wallclock_time = 0.;
    size_t num_full_setup_updates = 0;
    size_t num_full_hold_updates = 0;
    size_t num_full_setup_hold_updates = 0;
};

enum class e_incr_reroute_delay_ripup {
    ON,
    OFF,
    AUTO
};

constexpr int NO_FIXED_CHANNEL_WIDTH = -1;

struct t_router_opts {
    bool read_rr_edge_metadata = false;
    bool do_check_rr_graph = true;
    float first_iter_pres_fac;
    float initial_pres_fac;
    float pres_fac_mult;
    float max_pres_fac;
    float acc_fac;
    float bend_cost;
    int max_router_iterations;
    int min_incremental_reroute_fanout;
    e_incr_reroute_delay_ripup incr_reroute_delay_ripup;
    int bb_factor;
    enum e_route_type route_type;
    int fixed_channel_width;
    int min_channel_width_hint; ///<Hint to binary search of what the minimum channel width is
    enum e_router_algorithm router_algorithm;
    enum e_base_cost_type base_cost_type;
    float astar_fac;
    float astar_offset;
    float router_profiler_astar_fac;
    bool enable_parallel_connection_router;
    float post_target_prune_fac;
    float post_target_prune_offset;
    int multi_queue_num_threads;
    int multi_queue_num_queues;
    bool multi_queue_direct_draining;
    float max_criticality;
    float criticality_exp;
    float init_wirelength_abort_threshold;
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
    enum e_clock_modeling clock_modeling; ///<How clock pins and nets should be handled
    bool two_stage_clock_routing;         ///<How clock nets on dedicated networks should be routed
    int high_fanout_threshold;
    float high_fanout_max_slope;
    int router_debug_net;
    int router_debug_sink_rr;
    int router_debug_iteration;
    e_router_lookahead lookahead_type;
    double initial_acc_cost_chan_congestion_threshold;
    double initial_acc_cost_chan_congestion_weight;
    int max_convergence_count;
    int route_verbosity;
    float reconvergence_cpd_threshold;
    e_router_initial_timing initial_timing;
    bool update_lower_bound_delays;

    std::string first_iteration_timing_report_file;
    bool strict_checks;

    std::string write_router_lookahead;
    std::string read_router_lookahead;

    std::string write_intra_cluster_router_lookahead;
    std::string read_intra_cluster_router_lookahead;

    e_heap_type router_heap;
    bool exit_after_first_routing_iteration;

    e_check_route_option check_route;
    e_timing_update_type timing_update_type;

    size_t max_logged_overused_rr_nodes;
    bool generate_rr_node_overuse_report;

    bool flat_routing;
    bool has_choke_point;

    int custom_3d_sb_fanin_fanout = 1;

    bool with_timing_analysis;

    /// Whether to verify the switch IDs in the route file with the RR Graph.
    bool verify_route_file_switch_id;

    /// Options related to rr_node reordering, for testing and possible cache optimization
    e_rr_node_reorder_algorithm reorder_rr_graph_nodes_algorithm = DONT_REORDER;
    int reorder_rr_graph_nodes_threshold = 0;
    int reorder_rr_graph_nodes_seed = 1;

    bool generate_router_lookahead_report;
};

struct t_analysis_opts {
    e_stage_action doAnalysis;

    bool gen_post_synthesis_netlist;
    bool gen_post_implementation_merged_netlist;
    bool gen_post_implementation_sdc;
    e_post_synth_netlist_unconn_handling post_synth_netlist_unconn_input_handling;
    e_post_synth_netlist_unconn_handling post_synth_netlist_unconn_output_handling;
    bool post_synth_netlist_module_parameters;

    int timing_report_npaths;
    e_timing_report_detail timing_report_detail;
    bool timing_report_skew;
    std::string echo_dot_timing_graph_node;
    std::string write_timing_summary;
    bool generate_net_timing_report;

    e_timing_update_type timing_update_type;
    bool skip_sync_clustering_and_routing_results;
};

/// Stores NoC specific options, when supplied as an input by the user
struct t_noc_opts {
    bool noc;                                      ///<options to turn on hard NoC modeling & optimization
    std::string noc_flows_file;                    ///<name of the file that contains all the traffic flow information to be sent over the NoC in this design
    std::string noc_routing_algorithm;             ///<controls the routing algorithm used to route packets within the NoC
    double noc_placement_weighting;                ///<controls the significance of the NoC placement cost relative to the total placement cost range:[0-inf)
    double noc_aggregate_bandwidth_weighting;      ///<controls the significance of aggregate used bandwidth relative to other NoC placement costs:[0:-inf)
    double noc_latency_constraints_weighting;      ///<controls the significance of meeting the traffic flow constraints range:[0-inf)
    double noc_latency_weighting;                  ///<controls the significance of the traffic flow latencies relative to the other NoC placement costs range:[0-inf)
    double noc_congestion_weighting;               ///<controls the significance of the link congestions relative to the other NoC placement costs range:[0-inf)
    double noc_centroid_weight;                    ///<controls how much the centroid location is adjusted towards NoC routers in NoC-biased centroid move:[0, 1]
    int noc_swap_percentage;                       ///<controls the number of NoC router block swap attempts relative to the total number of swaps attempted by the placer range:[0-100]
    int noc_sat_routing_bandwidth_resolution;      ///<if this number is N, the SAT formulation models link utilization in increments of 1/N
    int noc_sat_routing_latency_overrun_weighting; ///<controls the importance of reducing traffic flow latency overrun in SAT routing [0-inf)
    int noc_sat_routing_congestion_weighting;      ///<controls the importance of reducing the number of congested NoC links in SAT routing [0-inf)
    int noc_sat_routing_num_workers;               ///<the number of parallel worker threads that the SAT solver can use to explore the solution space
    bool noc_sat_routing_log_search_progress;      ///<indicates whether the detailed log of the SAT solver's search progress in printed
    std::string noc_placement_file_name;           ///<is the name of the output file that contains the NoC placement information
};

/**
 * @brief Defines the detailed routing architecture of the FPGA.
 *
 * Only important if the route_type is DETAILED.
 */
struct t_det_routing_arch {
    /// Should the tracks be uni-directional or bi-directional?
    enum e_directionality directionality;
    int Fs;

    /// Pattern of switches at each switch block. I assume Fs is always 3.
    enum e_switch_block_type switch_block_type;

    /// A vector of custom switch block descriptions that is used with
    /// the CUSTOM switch block type. See comment at top of SRC/route/build_switchblocks.c
    std::vector<t_switchblock_inf> switchblocks;

    // Following options are used only for tileable routing architecture

    /// Whether the routing architecture is tileable
    bool tileable;

    /// Sub type and Fs are applied to pass tracks
    int sub_fs;

    /// Subtype of switch blocks.
    enum e_switch_block_type switch_block_subtype;

    /// Allow connection blocks to appear around the perimeter programmable block (mainly I/Os)
    bool perimeter_cb;

    /// Remove all the routing wires in empty regions
    bool shrink_boundary;

    /// Allow routing channels to pass through multi-width and multi-height programmable blocks.
    bool through_channel;

    /// Allow each output pin of a programmable block to drive the routing tracks on all the
    /// sides of its adjacent switch block
    bool opin2all_sides;

    ///In each switch block, allow each routing track which ends to drive another
    /// routing track on the opposite side
    bool concat_wire;

    /// In each switch block, allow each routing track which passes to drive
    /// another routing track on the opposite side
    bool concat_pass_wire;

    // End of tileable routing architecture-specific options

    short global_route_switch;

    /// Index of a zero delay switch (used to connect things that should have no delay).
    short delayless_switch;

    /// Keeps track of the type of architecture switch that connects wires to ipins
    int wire_to_arch_ipin_switch;

    /// Keeps track of the type of architecture switch that connects
    /// wires from another die to ipins in different die
    int wire_to_arch_ipin_switch_between_dice = -1;

    /// keeps track of the type of RR graph switch
    /// that connects wires to ipins in the RR graph
    int wire_to_rr_ipin_switch;

    /// keeps track of the type of RR graph switch that connects wires
    /// from another die to ipins in different die in the RR graph
    int wire_to_rr_ipin_switch_between_dice = -1;

    /// Resistance (in Ohms) of a minimum width nmos transistor.
    /// Used only in the FPGA area model.
    float R_minW_nmos;

    /// Resistance (in Ohms) of a minimum width pmos transistor.
    float R_minW_pmos;

    /// File to read the RR graph from (overrides architecture)
    std::string read_rr_graph_filename;
    /// File to write the RR graph to after generation
    std::string write_rr_graph_filename;
    /// File to read the RR graph edge attribute overrides.
    std::string read_rr_edge_override_filename;
};

/**
 * @brief Information about the current status of a particular
 *        net as pertains to routing
 */
template<typename NetIdType>
class t_routing_status {
  public:
    void clear() {
        is_routed_.assign(is_routed_.size(), 0);
        is_fixed_.assign(is_routed_.size(), 0);
    }
    void resize(size_t number_nets) {
        is_routed_.resize(number_nets);
        is_routed_.assign(is_routed_.size(), 0);
        is_fixed_.resize(number_nets);
        is_fixed_.assign(is_routed_.size(), 0);
    }
    void set_is_routed(NetIdType net, bool is_routed) {
        is_routed_[index(net)] = is_routed;
    }
    bool is_routed(NetIdType net) const {
        return is_routed_[index(net)];
    }
    void set_is_fixed(NetIdType net, bool is_fixed) {
        is_fixed_[index(net)] = is_fixed;
    }
    bool is_fixed(NetIdType net) const {
        return is_fixed_[index(net)];
    }

  private:
    NetIdType index(NetIdType net) const {
        VTR_ASSERT_SAFE(net != NetIdType::INVALID());
        return net;
    }
    /* vector<int> instead of bitset for thread safety */
    vtr::vector<NetIdType, int> is_routed_; ///<Whether the net has been legally routed
    vtr::vector<NetIdType, int> is_fixed_;  ///<Whether the net is fixed (i.e. not to be re-routed)
};

typedef t_routing_status<ParentNetId> t_net_routing_status;
typedef t_routing_status<AtomNetId> t_atom_net_routing_status;

/** Edge between two RRNodes */
struct t_node_edge {
    t_node_edge(RRNodeId fnode, RRNodeId tnode) noexcept
        : from_node(fnode)
        , to_node(tnode) {}

    RRNodeId from_node;
    RRNodeId to_node;

    //For std::set
    friend bool operator<(const t_node_edge& lhs, const t_node_edge& rhs) {
        return std::tie(lhs.from_node, lhs.to_node) < std::tie(rhs.from_node, rhs.to_node);
    }
};

/**
 * @brief Groups of non-configurably connected nodes and edges in the RR graph.
 * @note Each group is represented by a node set and an edge set, stored at the same index.
 *
 * For example, in an architecture with L-shaped wires formed by an x- and y-directed segment
 * connected by an electrical short, each L-shaped wire corresponds to a new group. The group's
 * index provides access to its node set (containing two RRNodeIds) and edge set (containing two
 * directed edge in opposite directions).
 */
struct t_non_configurable_rr_sets {
    std::vector<std::set<RRNodeId>> node_sets;
    std::vector<std::set<t_node_edge>> edge_sets;
};

///@brief Power estimation options
struct t_power_opts {
    bool do_power; ///<Perform power estimation?
};

/** @brief Channel width data
 * @param max= Maximum channel width between x_max and y_max.
 * @param x_min= Minimum channel width of horizontal channels. Initialized when init_chan() is invoked in rr_graph2.cpp
 * @param y_min= Same as above but for vertical channels.
 * @param x_max= Maximum channel width of horizontal channels. Initialized when init_chan() is invoked in rr_graph2.cpp
 * @param y_max= Same as above but for vertical channels.
 * @param x_list= Stores the channel width of all horizontal channels and thus goes from [0..grid.height()]
 * (imagine a 2D Cartesian grid with horizontal lines starting at every grid point on a line parallel to the y-axis)
 * @param y_list= Stores the channel width of all vertical channels and thus goes from [0..grid.width()]
 * (imagine a 2D Cartesian grid with vertical lines starting at every grid point on a line parallel to the x-axis)
 */

struct t_lb_type_rr_node; /* Defined in pack_types.h */

/// @brief Stores settings for VPR server mode
struct t_server_opts {
    bool is_server_mode_enabled = false;
    int port_num = -1;
};

///@brief Store settings for VPR
struct t_vpr_setup {
    bool TimingEnabled;             ///<Is VPR timing enabled
    t_file_name_opts FileNameOpts;  ///<File names
    t_netlist_opts NetlistOpts;     ///<Options for packer
    t_packer_opts PackerOpts;       ///<Options for packer
    t_placer_opts PlacerOpts;       ///<Options for placer
    t_ap_opts APOpts;               ///<Options for analytical placer
    t_router_opts RouterOpts;       ///<router options
    t_analysis_opts AnalysisOpts;   ///<Analysis options
    t_noc_opts NocOpts;             ///<Options for the NoC
    t_server_opts ServerOpts;       ///<Server options
    t_det_routing_arch RoutingArch; ///<routing architecture
    std::vector<t_lb_type_rr_node>* PackerRRGraph;
    std::vector<t_segment_inf> Segments; ///<wires in routing architecture
    t_timing_inf Timing;                 ///<timing information
    float constant_net_delay;            ///<timing information when place and route not run
    bool ShowGraphics;                   ///<option to show graphics
    int GraphPause;                      ///<user interactiveness graphics option
    bool SaveGraphics;                   ///<option to save graphical contents to pdf, png, or svg
    std::string GraphicsCommands;        ///<commands to control graphics settings
    t_power_opts PowerOpts;
    std::string device_layout;
    e_constant_net_method constant_net_method; ///<How constant nets should be handled
    e_clock_modeling clock_modeling;           ///<How clocks should be handled
    bool two_stage_clock_routing;              ///<How clocks should be routed in the presence of a dedicated clock network
    bool exit_before_pack;                     ///<Exits early before starting packing (useful for collecting statistics without running/loading any stages)
    unsigned int num_workers;                  ///Maximum number of worker threads (determined from an env var or cmdline option)
};

class RouteStatus {
  public:
    RouteStatus() = default;
    RouteStatus(bool status_val, int chan_width_val)
        : success_(status_val)
        , chan_width_(chan_width_val) {}

    //Was routing successful?
    operator bool() const { return success(); }
    bool success() const { return success_; }

    //What was the channel width?
    int chan_width() const { return chan_width_; }

  private:
    bool success_ = false;
    int chan_width_ = -1;
};

typedef vtr::vector<ClusterBlockId, std::vector<std::vector<RRNodeId>>> t_clb_opins_used; //[0..num_blocks-1][0..class-1][0..used_pins-1]

typedef std::vector<std::map<int, int>> t_arch_switch_fanin;
