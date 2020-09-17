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

#ifndef VPR_TYPES_H
#define VPR_TYPES_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include "arch_types.h"
#include "atom_netlist_fwd.h"
#include "clustered_netlist_fwd.h"
#include "constant_nets.h"
#include "clock_modeling.h"
#include "heap_type.h"

#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"
#include "vtr_util.h"
#include "vtr_flat_map.h"
#include "vtr_cache.h"
#include "vtr_string_view.h"
#include "vtr_dynamic_bitset.h"
#include "rr_graph_fwd.h"

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

#define MAX_SHORT 32767

/* Values large enough to be way out of range for any data, but small enough
 * to allow a small number to be added to them without going out of range. */
#define HUGE_POSITIVE_FLOAT 1.e30
#define HUGE_NEGATIVE_FLOAT -1.e30

/* Used to avoid floating-point errors when comparing values close to 0 */
#define EPSILON 1.e-15
#define NEGATIVE_EPSILON -1.e-15

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
 * and maps it to the complex logic blocks found in the architecture
 ******************************************************************************/

#define NO_CLUSTER -1
#define NEVER_CLUSTER -2
#define NOT_VALID -10000 /* Marks gains that aren't valid */
/* Ensure no gain can ever be this negative! */
#ifndef UNDEFINED
#    define UNDEFINED -1
#endif

enum class e_router_lookahead {
    CLASSIC,      ///<VPR's classic lookahead (assumes uniform wire types)
    MAP,          ///<Lookahead considering different wire types (see Oleg Petelin's MASc Thesis)
    EXTENDED_MAP, ///<Lookahead with a more extensive node sampling method
    NO_OP         ///<A no-operation lookahead which always returns zero
};

enum class e_route_bb_update {
    STATIC, ///<Router net bounding boxes are not updated
    DYNAMIC ///<Rotuer net bounding boxes are updated
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

enum e_block_pack_status {
    BLK_PASSED,
    BLK_FAILED_FEASIBLE,
    BLK_FAILED_ROUTE,
    BLK_STATUS_UNDEFINED
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

    ///@brief Returns the input pin util of the specified block (or default if unspecified)
    t_ext_pin_util get_pin_util(std::string block_type_name) const;

  public:
    /**
     * @brief Sets the pin util for the specified block type
     * @return true if non-default was previously set
     */
    void set_block_pin_util(std::string block_type_name, t_ext_pin_util target);

    /**
     * @brief Sets the default pin util
     * @return Returns true if a default was previously set
     */
    void set_default_pin_util(t_ext_pin_util target);

  private:
    t_ext_pin_util defaults_;
    std::map<std::string, t_ext_pin_util> overrides_;
};

class t_pack_high_fanout_thresholds {
  public:
    t_pack_high_fanout_thresholds() = default;
    t_pack_high_fanout_thresholds(int threshold);

    int get_threshold(std::string block_type_name) const;

  public:
    /**
     * @brief Sets the pin util for the specified block type
     * @return true if non-default was previously set
     */
    void set(std::string block_type_name, int threshold);

    /**
     * @brief Sets the default pin util
     * @return true if a default was previously set
     */
    void set_default(int threshold);

  private:
    int default_;
    std::map<std::string, int> overrides_;
};

///@brief Type used to express rr_node edge index.
typedef uint16_t t_edge_size;

/**
 * @brief An iterator that dereferences to an edge index
 *
 * Used inconjunction with vtr::Range to return ranges of edge indices
 */
class edge_idx_iterator : public std::iterator<std::bidirectional_iterator_tag, t_edge_size> {
  public:
    edge_idx_iterator(value_type init)
        : value_(init) {}
    iterator operator++() {
        value_ += 1;
        return *this;
    }
    iterator operator--() {
        value_ -= 1;
        return *this;
    }
    reference operator*() { return value_; }
    pointer operator->() { return &value_; }

    friend bool operator==(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return lhs.value_ == rhs.value_; }
    friend bool operator!=(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return !(lhs == rhs); }

  private:
    value_type value_;
};

typedef vtr::Range<edge_idx_iterator> edge_idx_range;

/* these are defined later, but need to declare here because it is used */
class t_rr_node;
class t_pack_molecule;
struct t_pb_stats;
struct t_pb_route;
struct t_chain_info;

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
     * @brief Returns the t_pb associated with the specified gnode which is contained
     *        within the current pb
     */
    const t_pb* find_pb(const t_pb_graph_node* gnode) const;

    const t_pb* find_pb_for_model(const std::string& blif_model) const;

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
    int driver_pb_pin_id = OPEN;                  ///<The pb_pin id of the pb_pin that drives this pin
    std::vector<int> sink_pb_pin_ids;             ///<The pb_pin id's of the pb_pins driven by this node
    const t_pb_graph_pin* pb_graph_pin = nullptr; ///<The graph pin associated with this node
};

///@brief Describes the molecule type
enum e_pack_pattern_molecule_type {
    MOLECULE_SINGLE_ATOM, ///<single atom forming a molecule (no pack pattern associated)
    MOLECULE_FORCED_PACK  ///<more than one atom representing a packing pattern forming a large molecule
};

/**
 * @brief Represents a grouping of atom blocks that match a pack_pattern,
 *        these groups are intended to be placed as a single unit during packing
 *
 * Store in linked list
 *
 * A chain is a special type of pack pattern.  A chain can extend across multiple logic blocks.
 * Must segment the chain to fit in a logic block by identifying the actual atom that forms the root of the new chain.
 * Assumes that the root of a chain is the primitive that starts the chain or is driven from outside the logic block
 *
 * Data members:
 *
 *      type           : either a single atom or more atoms representing a packing pattern
 *      pack_pattern   : if not a single atom, this is the pack pattern representing this molecule
 *      atom_block_ids : [0..num_blocks-1] IDs of atom blocks that implements this molecule, indexed by
 *                       t_pack_pattern_block->block_id
 *      chain_info     : if this is a molecule representing a chained pack pattern, this data structure will
 *                       hold the data shared between all molecules forming a chain together.
 *      valid          : whether the molecule is still valid for packing or not.
 *      num_blocks     : maximum number of atom blocks that can fit in this molecule
 *      root           : index of the pack_pattern->root_block in the atom_blocks_ids. root_block_id = atom_block_ids[root]
 *      base_gain      : intrinsic "goodness" score for molecule independent of rest of netlist
 *      next           : next molecule in the linked list
 */
class t_pack_molecule {
  public:
    /* general molecule info */
    bool valid;
    float base_gain;
    enum e_pack_pattern_molecule_type type;

    /* large molecules info */
    t_pack_patterns* pack_pattern;
    int root;
    int num_blocks;
    std::vector<AtomBlockId> atom_block_ids;
    std::shared_ptr<t_chain_info> chain_info;

    t_pack_molecule* next;
    // a molecule is chain is it is a forced pack and its pack pattern is chain
    bool is_chain() const { return type == MOLECULE_FORCED_PACK && pack_pattern->is_chain; }
};

/**
 * @brief Holds information to be shared between molecules that represent the same chained pack pattern.
 *
 * For example, molecules that are representing a long carry chain that spans multiple logic blocks.
 *
 * Data members:
 *      is_long_chain         : is this a long that is divided on multiple clusters (divided on multiple molecules).
 *      chain_id              : is used to access the chain_root_pins vector in the t_pack_patterns of the molecule. To get
 *                              the starting point of this chain in the cluster. This id is useful when we have multiple
 *                              (architectural) carry chains in a logic block, for example. It lets us see which of the chains
 *                              is being used for this long (netlist) chain, so we continue to use that chain in the packing
 *                              of other molecules of this long chain.
 *      first_packed_molecule : first molecule to be packed out of the molecules forming this chain. This is the molecule
 *                              setting the value of the chain_id.
 */
struct t_chain_info {
    bool is_long_chain = false;
    int chain_id = -1;
    t_pack_molecule* first_packed_molecule = nullptr;
};

/**
 * @brief Stats keeper for placement information during packing
 *
 * Contains linked lists to placement locations based on status of primitive
 */
struct t_cluster_placement_stats {
    int num_pb_types;                                 ///<num primitive pb_types inside complex block
    bool has_long_chain;                              ///<specifies if this cluster has a molecule placed in it that belongs to a long chain (a chain that spans more than one cluster)
    const t_pack_molecule* curr_molecule;             ///<current molecule being considered for packing
    t_cluster_placement_primitive** valid_primitives; ///<[0..num_pb_types-1] ptrs to linked list of valid primitives, for convenience, each linked list head is empty
    t_cluster_placement_primitive* in_flight;         ///<ptrs to primitives currently being considered
    t_cluster_placement_primitive* tried;             ///<ptrs to primitives that are open but current logic block unable to pack to
    t_cluster_placement_primitive* invalid;           ///<ptrs to primitives that are invalid
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

/* Timing data structures end */
enum sched_type {
    AUTO_SCHED,
    DUSTY_SCHED,
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
     * @brief Transistion density - average # of transitions per clock cycle
     *
     * For example, a clock would have density = 2
     */
    float density;
};

/**
 * @brief Stores the bounding box of a net in terms of the minimum and
 *        maximum coordinates of the blocks forming the net, clipped to
 *        the region: (1..device_ctx.grid.width()-2, 1..device_ctx.grid.height()-1)
 */
struct t_bb {
    int xmin = 0;
    int xmax = 0;
    int ymin = 0;
    int ymax = 0;
};

/**
 * @brief An offset between placement locations (t_pl_loc)
 *
 * x: x-offset
 * y: y-offset
 * z: z-offset
 */
struct t_pl_offset {
    t_pl_offset() = default;
    t_pl_offset(int xoffset, int yoffset, int sub_tile_offset)
        : x(xoffset)
        , y(yoffset)
        , sub_tile(sub_tile_offset) {}

    int x = 0;
    int y = 0;
    int sub_tile = 0;

    t_pl_offset& operator+=(const t_pl_offset& rhs) {
        x += rhs.x;
        y += rhs.y;
        sub_tile += rhs.sub_tile;
        return *this;
    }

    t_pl_offset& operator-=(const t_pl_offset& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        sub_tile -= rhs.sub_tile;
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
        return t_pl_offset(-other.x, -other.y, -other.sub_tile);
    }
    friend t_pl_offset operator+(const t_pl_offset& other) {
        return t_pl_offset(+other.x, +other.y, +other.sub_tile);
    }

    friend bool operator<(const t_pl_offset& lhs, const t_pl_offset& rhs) {
        return std::tie(lhs.x, lhs.y, lhs.sub_tile) < std::tie(rhs.x, rhs.y, rhs.sub_tile);
    }

    friend bool operator==(const t_pl_offset& lhs, const t_pl_offset& rhs) {
        return std::tie(lhs.x, lhs.y, lhs.sub_tile) == std::tie(rhs.x, rhs.y, rhs.sub_tile);
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
        return seed;
    }
};
} // namespace std

/**
 * @brief A placement location coordinate
 *
 * x: x-coordinate
 * y: y-coordinate
 * z: z-coordinate (capacity postion)
 *
 * @note t_pl_offset should be used to represent an offset between t_pl_loc.
 */
struct t_pl_loc {
    t_pl_loc() = default;
    t_pl_loc(int xloc, int yloc, int sub_tile_loc)
        : x(xloc)
        , y(yloc)
        , sub_tile(sub_tile_loc) {}

    int x = OPEN;
    int y = OPEN;
    int sub_tile = OPEN;

    t_pl_loc& operator+=(const t_pl_offset& rhs) {
        x += rhs.x;
        y += rhs.y;
        sub_tile += rhs.sub_tile;
        return *this;
    }

    t_pl_loc& operator-=(const t_pl_offset& rhs) {
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
        return t_pl_offset(lhs.x - rhs.x, lhs.y - rhs.y, lhs.sub_tile - rhs.sub_tile);
    }

    friend bool operator<(const t_pl_loc& lhs, const t_pl_loc& rhs) {
        return std::tie(lhs.x, lhs.y, lhs.sub_tile) < std::tie(rhs.x, rhs.y, rhs.sub_tile);
    }

    friend bool operator==(const t_pl_loc& lhs, const t_pl_loc& rhs) {
        return std::tie(lhs.x, lhs.y, lhs.sub_tile) == std::tie(rhs.x, rhs.y, rhs.sub_tile);
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
        return seed;
    }
};
} // namespace std

struct t_place_region {
    float capacity; ///<Capacity of this region, in tracks.
    float inv_capacity;
    float occupancy; ///<Expected number of tracks that will be occupied.
    float cost;      ///<Current cost of this usage.
};

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

///@brief Stores the clustered blocks placed at a particular grid location
struct t_grid_blocks {
    int usage; ///<How many valid blocks are in use at this location

    /**
     * @brief The clustered blocks associated with this grid location.
     *
     * Index range: [0..device_ctx.grid[x_loc][y_loc].type->capacity]
     */
    std::vector<ClusterBlockId> blocks;

    /**
     * @brief Test if a subtile at a grid location is occupied by a block.
     *
     * Returns true if the subtile corresponds to the passed-in id is not
     * occupied by a block at this grid location. The subtile id serves
     * as the z-dimensional offset in the grid indexing.
     */
    inline bool subtile_empty(size_t isubtile) const {
        return blocks[isubtile] == EMPTY_BLOCK_ID;
    }
};

///@brief Names of various files
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

///@brief Should a stage in the CAD flow be skipped, loaded from a file, or performed
enum e_stage_action {
    STAGE_SKIP = 0,
    STAGE_LOAD,
    STAGE_DO,
    STAGE_AUTO
};

/**
 * @brief Options for packing
 *
 * TODO: document each packing parameter
 */
enum e_packer_algorithm {
    PACK_GREEDY,
    PACK_BRUTE_FORCE
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
    e_balance_block_type_util balance_block_type_utilization;
    std::vector<std::string> target_external_pin_util;
    bool prioritize_transitive_connectivity;
    std::vector<std::string> high_fanout_threshold;
    int transitive_fanout_threshold;
    int feasible_block_array_size;
    e_stage_action doPacking;
    enum e_packer_algorithm packer_algorithm;
    std::string device_layout;
    e_timing_update_type timing_update_type;
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
    enum sched_type type;
    float inner_num;
    float init_t;
    float alpha_t;
    float exit_t;

    /* Parameters for DUSTY_SCHED                                         *
     * The alpha ranges from alpha_min to alpha_max, decaying each        *
     * iteration by `alpha_decay`.                                        *
     * `restart_filter` is the low-pass coefficient (EWMA) for updating   *
     * the new starting temperature for each alpha.                       *
     * Give up after `wait` alphas.                                       */
    float alpha_min;
    float alpha_max;
    float alpha_decay;
    float success_min;
    float success_target;
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
enum e_place_algorithm {
    BOUNDING_BOX_PLACE,
    CRITICALITY_TIMING_PLACE,
    SLACK_TIMING_PLACE
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
        return algo == CRITICALITY_TIMING_PLACE || algo == SLACK_TIMING_PLACE;
    }

    ///@brief Accessor: returns the underlying e_place_algorithm enum value.
    e_place_algorithm get() const { return algo; }

  private:
    ///@brief The underlying algorithm. Default set to CRITICALITY_TIMING_PLACE.
    e_place_algorithm algo = e_place_algorithm::CRITICALITY_TIMING_PLACE;
};

enum e_pad_loc_type {
    FREE,
    RANDOM
};

///@brief Used to calculate the inner placer loop's block swapping limit move_lim.
enum e_place_effort_scaling {
    CIRCUIT,       ///<Effort scales based on circuit size only
    DEVICE_CIRCUIT ///<Effort scales based on both circuit and device size
};

enum class PlaceDelayModelType {
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
 *   @param place_cost_exp
 *              Power to which denominator is raised for linear_cong.
 *   @param place_chan_width
 *              The channel width assumed if only one placement is performed.
 *   @param pad_loc_type
 *              Are pins FREE or fixed randomly.
 *   @param constraints_file
 *              File that specifies locations of locked down (constrained)
 *              blocks for placement. Empty string means no constraints file.
 *   @param pad_loc_file
 *              File to read pad locations from if pad_loc_type is USER.
 *   @param place_freq
 *              Should the placement be skipped, done once, or done
 *              for each channel width in the binary search.
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
 */
struct t_placer_opts {
    t_place_algorithm place_algorithm;
    t_place_algorithm place_quench_algorithm;
    float timing_tradeoff;
    float place_cost_exp;
    int place_chan_width;
    enum e_pad_loc_type pad_loc_type;
    std::string constraints_file;
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

    /**
     * @brief Tile types that should be used during delay sampling.
     *
     * Useful for excluding tiles that have abnormal delay behavior, e.g.
     * clock tree elements like PLL's, global/local clock buffers, etc.
     */
    std::string allowed_tiles_for_delay_model;

    e_place_delta_delay_algorithm place_delta_delay_matrix_calculation_method;

    /*
     * @brief enables the analytic placer.
     *
     * Once analytic placement is done, the result is passed through the quench phase
     * of the annealing placer for local improvement
     */
    bool enable_analytic_placer;
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
 * doRouting: true if routing is supposed to be done, false otherwise       *
 * routing_failure_predictor: sets the configuration to be used by the      *
 * routing failure predictor, how aggressive the threshold used to judge    *
 * and abort routings deemed unroutable                                     *
 * write_rr_graph_name: stores the file name of the output rr graph         *
 * read_rr_graph_name:  stores the file name of the rr graph to be read by vpr */
enum e_route_type {
    GLOBAL,
    DETAILED
};

enum e_router_algorithm {
    BREADTH_FIRST,
    TIMING_DRIVEN,
};

// Node reordering algorithms for rr_nodes
enum e_rr_node_reorder_algorithm {
    DONT_REORDER,
    DEGREE_BFS,
    RANDOM_SHUFFLE,
};

enum e_base_cost_type {
    DELAY_NORMALIZED,
    DELAY_NORMALIZED_LENGTH,
    DELAY_NORMALIZED_FREQUENCY,
    DELAY_NORMALIZED_LENGTH_FREQUENCY,
    DELAY_NORMALIZED_LENGTH_BOUNDED,
    DEMAND_ONLY,
    DEMAND_ONLY_NORMALIZED_LENGTH
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
    float acc_fac;
    float bend_cost;
    int max_router_iterations;
    int min_incremental_reroute_fanout;
    e_incr_reroute_delay_ripup incr_reroute_delay_ripup;
    int bb_factor;
    enum e_route_type route_type;
    int fixed_channel_width;
    int min_channel_width_hint; ///<Hint to binary search of what the minimum channel width is
    bool trim_empty_channels;
    bool trim_obs_channels;
    enum e_router_algorithm router_algorithm;
    enum e_base_cost_type base_cost_type;
    float astar_fac;
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
    int max_convergence_count;
    float reconvergence_cpd_threshold;
    e_router_initial_timing initial_timing;
    bool update_lower_bound_delays;

    std::string first_iteration_timing_report_file;
    bool strict_checks;

    std::string write_router_lookahead;
    std::string read_router_lookahead;

    e_heap_type router_heap;
    bool exit_after_first_routing_iteration;

    e_check_route_option check_route;
    e_timing_update_type timing_update_type;

    size_t max_logged_overused_rr_nodes;
    bool generate_rr_node_overuse_report;

    // Options related to rr_node reordering, for testing and possible cache optimization
    e_rr_node_reorder_algorithm reorder_rr_graph_nodes_algorithm = DONT_REORDER;
    int reorder_rr_graph_nodes_threshold = 0;
    int reorder_rr_graph_nodes_seed = 1;
};

struct t_analysis_opts {
    e_stage_action doAnalysis;

    bool gen_post_synthesis_netlist;

    int timing_report_npaths;
    e_timing_report_detail timing_report_detail;
    bool timing_report_skew;
    std::string echo_dot_timing_graph_node;

    e_timing_update_type timing_update_type;
};

/**
 * @brief Defines the detailed routing architecture of the FPGA.
 *
 * Only important if the route_type is DETAILED.
 *
 *   @param directionality  Should the tracks be uni-directional or
 *             bi-directional? (UDSD by AY)
 *   @param switch_block_type  Pattern of switches at each switch block.
 *             I assume Fs is always 3.  If the type is SUBSET, I use a
 *             Xilinx-like switch block where track i in one channel always
 *             connects to track i in other channels.  If type is WILTON,
 *             I use a switch block where track i does not always connect
 *             to track i in other channels.  See Steve Wilton, Phd Thesis,
 *             University of Toronto, 1996.  The UNIVERSAL switch block is
 *             from Y. W. Chang et al, TODAES, Jan. 1996, pp. 80 - 101.
 *             A CUSTOM switch block has also been added which allows a user
 *             to describe custom permutation functions and connection
 *             patterns. See comment at top of SRC/route/build_switchblocks.c
 *   @param switchblocks  A vector of custom switch block descriptions that is
 *             used with the CUSTOM switch block type. See comment at top of
 *             SRC/route/build_switchblocks.c
 *   @param delayless_switch  Index of a zero delay switch (used to connect
 *             things that should have no delay).
 *   @param wire_to_arch_ipin_switch  keeps track of the type of architecture
 *             switch that connects wires to ipins
 *   @param wire_to_rr_ipin_switch  keeps track of the type of RR graph switch
 *             that connects wires to ipins in the RR graph
 *   @param R_minW_nmos  Resistance (in Ohms) of a minimum width nmos transistor.
 *             Used only in the FPGA area model.
 *   @param R_minW_pmos  Resistance (in Ohms) of a minimum width pmos transistor.
 *   @param read_rr_graph_filename  File to read the RR graph from (overrides
 *             architecture)
 *   @param write_rr_graph_filename  File to write the RR graph to after generation
 */
struct t_det_routing_arch {
    enum e_directionality directionality; /* UDSD by AY */
    int Fs;
    enum e_switch_block_type switch_block_type;
    std::vector<t_switchblock_inf> switchblocks;

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

constexpr std::array<const char*, NUM_DIRECTIONS> DIRECTION_STRING = {{"INC_DIRECTION", "DEC_DIRECTION", "BI_DIRECTION", "NO_DIRECTION"}};

/**
 * @brief Lists detailed information about segmentation.  [0 .. W-1].
 *
 *   @param length     length of segment.
 *   @param start      index at which a segment starts in channel 0.
 *   @param longline   true if this segment spans the entire channel.
 *   @param sb  [0..length]: true for every channel intersection, relative to the
 *                     segment start, at which there is a switch box.
 *   @param cb  [0..length-1]:  true for every logic block along the segment at
 *                     which there is a connection box.
 *   @param arch_wire_switch  Index of the switch type that connects other wires
 *                     *to* this segment. Note that this index is in relation
 *                     to the switches from the architecture file, not the
 *                     expanded list of switches that is built at the end of
 *                     build_rr_graph.
 *   @param arch_opin_switch  Index of the switch type that connects output pins
 *                     (OPINs) *to* this segment. Note that this index is in
 *                     relation to the switches from the architecture file,
 *                     not the expanded list of switches that is is built
 *                     at the end of build_rr_graph
 *   @param Cmetal     Capacitance of a routing track, per unit logic block length.
 *   @param Rmetal     Resistance of a routing track, per unit logic block length.
 *   @param direction  The direction of a routing track.
 *   @param index      index of the segment type used for this track.
 *   @param type_name_ptr  pointer to name of the segment type this track belongs
 *                     to. points to the appropriate name in s_segment_inf
 */
struct t_seg_details {
    int length = 0;
    int start = 0;
    bool longline = 0;
    std::unique_ptr<bool[]> sb;
    std::unique_ptr<bool[]> cb;
    short arch_wire_switch = 0;
    short arch_opin_switch = 0;
    float Rmetal = 0;
    float Cmetal = 0;
    bool twisted = 0;
    enum e_direction direction = NO_DIRECTION;
    int group_start = 0;
    int group_size = 0;
    int seg_start = 0;
    int seg_end = 0;
    int index = 0;
    float Cmetal_per_m = 0; ///<Used for power
    std::string type_name;
};

class t_chan_seg_details {
  public:
    t_chan_seg_details() = default;
    t_chan_seg_details(const t_seg_details* init_seg_details)
        : length_(init_seg_details->length)
        , seg_detail_(init_seg_details) {}

  public:
    int length() const { return length_; }
    int seg_start() const { return seg_start_; }
    int seg_end() const { return seg_end_; }

    int start() const { return seg_detail_->start; }
    bool longline() const { return seg_detail_->longline; }

    int group_start() const { return seg_detail_->group_start; }
    int group_size() const { return seg_detail_->group_size; }

    bool cb(int pos) const { return seg_detail_->cb[pos]; }
    bool sb(int pos) const { return seg_detail_->sb[pos]; }

    float Rmetal() const { return seg_detail_->Rmetal; }
    float Cmetal() const { return seg_detail_->Cmetal; }
    float Cmetal_per_m() const { return seg_detail_->Cmetal_per_m; }

    short arch_wire_switch() const { return seg_detail_->arch_wire_switch; }
    short arch_opin_switch() const { return seg_detail_->arch_opin_switch; }

    e_direction direction() const { return seg_detail_->direction; }

    int index() const { return seg_detail_->index; }

    const vtr::string_view type_name() const {
        return vtr::string_view(
            seg_detail_->type_name.data(),
            seg_detail_->type_name.size());
    }

  public: //Modifiers
    void set_length(int new_len) { length_ = new_len; }
    void set_seg_start(int new_start) { seg_start_ = new_start; }
    void set_seg_end(int new_end) { seg_end_ = new_end; }

  private:
    //The only unique information about a channel segment is it's start/end
    //and length.  All other information is shared accross segment types,
    //so we use a flyweight to the t_seg_details which defines that info.
    //
    //To preserve the illusion of uniqueness we wrap all t_seg_details members
    //so it appears transparent -- client code of this class doesn't need to
    //know about t_seg_details.
    int length_ = -1;
    int seg_start_ = -1;
    int seg_end_ = -1;
    const t_seg_details* seg_detail_ = nullptr;
};

/* Defines a 2-D array of t_seg_details data structures (one per channel)   */
typedef vtr::NdMatrix<t_chan_seg_details, 3> t_chan_details;

/**
 * @brief A linked list of float pointers.
 *
 * Used for keeping track of which pathcosts in the router have been changed.
 */
struct t_linked_f_pointer {
    t_linked_f_pointer* next;
    float* fptr;
};

/**
 * @brief Type of a routing resource node.
 *
 * x-directed channel segment, y-directed channel segment,
 * input pin to a clb to pad, output from a clb or pad
 * (i.e. output pin of a net) and:
 * - SOURCE
 * - SINK
 */
typedef enum e_rr_type : unsigned char {
    SOURCE = 0, ///<A dummy node that is a logical output within a block -- i.e., the gate that generates a signal.
    SINK,       ///<A dummy node that is a logical input within a block -- i.e. the gate that needs a signal.
    IPIN,
    OPIN,
    CHANX,
    CHANY,
    NUM_RR_TYPES
} t_rr_type;

constexpr std::array<t_rr_type, NUM_RR_TYPES> RR_TYPES = {{SOURCE, SINK, IPIN, OPIN, CHANX, CHANY}};
constexpr std::array<const char*, NUM_RR_TYPES> rr_node_typename{{"SOURCE", "SINK", "IPIN", "OPIN", "CHANX", "CHANY"}};

constexpr bool is_pin(e_rr_type type) { return (type == IPIN || type == OPIN); }
constexpr bool is_chan(e_rr_type type) { return (type == CHANX || type == CHANY); }
constexpr bool is_src_sink(e_rr_type type) { return (type == SOURCE || type == SINK); }

//[0..num_rr_types-1][0..grid_width-1][0..grid_height-1][0..NUM_SIDES-1][0..max_ptc-1]
typedef std::array<vtr::NdMatrix<std::vector<int>, 3>, NUM_RR_TYPES> t_rr_node_indices;

/**
 * @brief Basic element used to store the traceback (routing) of each net.
 *
 *   @param index    Array index (ID) of this routing resource node.
 *   @param net_pin_index:    Net pin index associated with the node. This value       
 *                            ranges from 1 to fanout [1..num_pins-1]. For cases when  
 *                            different speed paths are taken to the same SINK for     
 *                            different pins, node index cannot uniquely identify      
 *                            each SINK, so the net pin index guarantees an unique     
 *                            identification for each SINK node. For non-SINK nodes    
 *                            and for SINK nodes with no associated net pin index      
 *                            (i.e. special SINKs like the source of a clock tree      
 *                            which do not correspond to an actual netlist connection),
 *                            the value for this member should be set to OPEN (-1).
 *   @param iswitch  Index of the switch type used to go from this rr_node to
 *                   the next one in the routing.  OPEN if there is no next node
 *                   (i.e. this node is the last one (a SINK) in a branch of the
 *                   net's routing).
 *   @param next     Pointer to the next traceback element in this route.
 */
struct t_trace {
    t_trace* next;
    int index;
    int net_pin_index = OPEN;
    short iswitch;
};

/**
 * @brief Extra information about each rr_node needed only during routing
 *        (i.e. during the maze expansion).
 *
 *   @param prev_node  Index of the previous node (on the lowest cost path known
 *                     to reach this node); used to generate the traceback.
 *                     If there is no predecessor, prev_node = NO_PREVIOUS.
 *   @param prev_edge  Index of the edge (from 0 to num_edges-1 of prev_node)
 *                     that was used to reach this node from the previous node.
 *                     If there is no predecessor, prev_edge = NO_PREVIOUS.
 *   @param acc_cost   Accumulated cost term from previous Pathfinder iterations.
 *   @param path_cost  Total cost of the path up to and including this node +
 *                     the expected cost to the target if the timing_driven router
 *                     is being used.
 *   @param backward_path_cost  Total cost of the path up to and including this
 *                     node. Not used by breadth-first router.
 *   @param target_flag  Is this node a target (sink) for the current routing?
 *                     Number of times this node must be reached to fully route.
 *   @param occ        The current occupancy of the associated rr node
 */
struct t_rr_node_route_inf {
    int prev_node;
    RREdgeId prev_edge;

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

/**
 * @brief Information about the current status of a particular
 *        net as pertains to routing
 */
class t_net_routing_status {
  public:
    void clear() {
        is_routed_.clear();
        is_fixed_.clear();
    }

    void resize(size_t number_nets) {
        is_routed_.resize(number_nets);
        is_routed_.fill(false);
        is_fixed_.resize(number_nets);
        is_fixed_.fill(false);
    }
    void set_is_routed(ClusterNetId net, bool is_routed) {
        is_routed_.set(index(net), is_routed);
    }
    bool is_routed(ClusterNetId net) const {
        return is_routed_.get(index(net));
    }
    void set_is_fixed(ClusterNetId net, bool is_fixed) {
        is_fixed_.set(index(net), is_fixed);
    }
    bool is_fixed(ClusterNetId net) const {
        return is_fixed_.get(index(net));
    }

  private:
    ClusterNetId index(ClusterNetId net) const {
        VTR_ASSERT_SAFE(net != ClusterNetId::INVALID());
        return net;
    }
    vtr::dynamic_bitset<ClusterNetId> is_routed_; ///<Whether the net has been legally routed
    vtr::dynamic_bitset<ClusterNetId> is_fixed_;  ///<Whether the net is fixed (i.e. not to be re-routed)
};

struct t_node_edge {
    t_node_edge(int fnode, int tnode) {
        from_node = fnode;
        to_node = tnode;
    }

    int from_node;
    int to_node;

    //For std::set
    friend bool operator<(const t_node_edge& lhs, const t_node_edge& rhs) {
        return std::tie(lhs.from_node, lhs.to_node) < std::tie(rhs.from_node, rhs.to_node);
    }
};

///@brief Non-configurably connected nodes and edges in the RR graph
struct t_non_configurable_rr_sets {
    std::set<std::set<int>> node_sets;
    std::set<std::set<t_node_edge>> edge_sets;
};

#define NO_PREVIOUS -1

///@brief Index of the SOURCE, SINK, OPIN, IPIN, etc. member of device_ctx.rr_indexed_data.
enum e_cost_indices {
    SOURCE_COST_INDEX = 0,
    SINK_COST_INDEX,
    OPIN_COST_INDEX,
    IPIN_COST_INDEX,
    CHANX_COST_INDEX_START
};

///@brief Power estimation options
struct t_power_opts {
    bool do_power; ///<Perform power estimation?
};

///@brief Channel width data
struct t_chan_width {
    int max = 0;
    int x_max = 0;
    int y_max = 0;
    int x_min = 0;
    int y_min = 0;
    std::vector<int> x_list;
    std::vector<int> y_list;
};

///@brief Type to store our list of token to enum pairings
struct t_TokenPair {
    const char* Str;
    int Enum;
};

struct t_lb_type_rr_node; /* Defined in pack_types.h */

///@brief Store settings for VPR
struct t_vpr_setup {
    bool TimingEnabled;             ///<Is VPR timing enabled
    t_file_name_opts FileNameOpts;  ///<File names
    t_model* user_models;           ///<blif models defined by the user
    t_model* library_models;        ///<blif models in VPR
    t_netlist_opts NetlistOpts;     ///<Options for packer
    t_packer_opts PackerOpts;       ///<Options for packer
    t_placer_opts PlacerOpts;       ///<Options for placer
    t_annealing_sched AnnealSched;  ///<Placement option annealing schedule
    t_router_opts RouterOpts;       ///<router options
    t_analysis_opts AnalysisOpts;   ///<Analysis options
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

typedef vtr::vector<ClusterBlockId, std::vector<std::vector<int>>> t_clb_opins_used; //[0..num_blocks-1][0..class-1][0..used_pins-1]

typedef std::vector<std::map<int, int>> t_arch_switch_fanin;

#endif
