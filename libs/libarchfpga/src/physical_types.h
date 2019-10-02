/*
 * Data types describing the physical components on the FPGA architecture.
 *
 * We assume an island style FPGA where complex logic blocks are arranged in a grid and each side of the logic block has access to the inter-block interconnect.  To keep the logic blocks general,
 * we allow arbitrary hierarchy, modes, primitives, and interconnect within each complex logic block.  The data structures here describe the properties of the island-style FPGA as well as the details on
 * hierarchy, modes, primitives, and interconnects within each logic block.
 *
 * Data structures that flesh out
 *
 * The data structures that store the
 *
 * Key data types:
 * t_physical_tile_type: represents the type of a tile in the device grid and describes its physical characteristics (pin locations, area, width, height, etc.)
 * t_logical_block_type: represents and describes the type of a clustered block
 * pb_type: describes the types of physical blocks within the t_logical_block_type in a hierarchy where the top block is the complex block and the leaf blocks implement one logical block
 * pb_graph_node: is a flattened version of pb_type so a pb_type with 10 instances will have 10 pb_graph_nodes representing each instance
 *
 * Additional notes:
 *
 * The interconnect specified in the architecture file gets flattened out in the pb_graph_node netlist.  Each pb_graph_node contains pb_graph_pins which allow it to connect to other pb_graph_nodes.
 * These pins are in connected to other pins through pb_graph_edges. The pin connections are based on what is specified in the <interconnect> tags of the architecture file.
 *
 * Date: February 19, 2009
 * Authors: Jason Luu and Kenneth Kent
 */

#ifndef PHYSICAL_TYPES_H
#define PHYSICAL_TYPES_H

#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <map>
#include <unordered_map>
#include <limits>
#include <numeric>

#include "vtr_ndmatrix.h"
#include "vtr_hash.h"

#include "logic_types.h"
#include "clock_types.h"

//Forward declarations
struct t_clock_arch;
struct t_clock_network;
struct t_power_arch;
struct t_interconnect_pins;
struct t_power_usage;
struct t_pb_type_power;
struct t_mode_power;
struct t_interconnect_power;
struct t_port_power;
struct t_physical_tile_port;
struct t_equivalent_site;
struct t_physical_tile_type;
typedef const t_physical_tile_type* t_physical_tile_type_ptr;
struct t_logical_block_type;
typedef const t_logical_block_type* t_logical_block_type_ptr;
struct t_pb_type;
struct t_pb_graph_pin_power;
struct t_mode;
struct t_pb_graph_node_power;
struct t_port;
class t_pb_graph_node;
struct t_pin_to_pin_annotation;
struct t_interconnect;
class t_pb_graph_pin;
class t_pb_graph_edge;
struct t_cluster_placement_primitive;
struct t_arch;
enum class e_sb_type;

/****************************************************************************/
/* FPGA metadata types                                                      */
/****************************************************************************/
/* t_metadata_value, and t_metadata_dict provide a types to store
 * metadata about the FPGA architecture and routing routing graph along side
 * the pb_type, grid, node and edge descriptions.
 *
 * The metadata is stored as a simple key/value map.  key's are string and an
 * optional coordinate. t_metadata_value provides the value storage, which is a
 * string.
 */

// Metadata value storage.
class t_metadata_value {
  public:
    explicit t_metadata_value(std::string v)
        : value_(v) {}
    explicit t_metadata_value(const t_metadata_value& o)
        : value_(o.value_) {}

    // Return string value.
    std::string as_string() const { return value_; }

  private:
    std::string value_;
};

// Metadata storage dictionary.
struct t_metadata_dict : std::unordered_map<
                             std::string,
                             std::vector<t_metadata_value>> {
    // Is this key present in the map?
    inline bool has(std::string key) const {
        return this->count(key) >= 1;
    }

    // Get all metadata values matching key.
    //
    // Returns nullptr if key is not found.
    inline const std::vector<t_metadata_value>* get(std::string key) const {
        auto iter = this->find(key);
        if (iter != this->end()) {
            return &iter->second;
        }
        return nullptr;
    }

    // Get metadata values matching key.
    //
    // Returns nullptr if key is not found or if multiple values are prsent
    // per key.
    inline const t_metadata_value* one(std::string key) const {
        auto values = get(key);
        if (values == nullptr) {
            return nullptr;
        }
        if (values->size() != 1) {
            return nullptr;
        }
        return &((*values)[0]);
    }

    // Adds value to key.
    void add(std::string key, std::string value) {
        // Get the iterator to the key, which may already have elements if
        // add was called with this key in the past.
        auto iter_inserted = this->emplace(key, std::vector<t_metadata_value>());
        iter_inserted.first->second.push_back(t_metadata_value(value));
    }
};

/*************************************************************************************************/
/* FPGA basic definitions                                                                        */
/*************************************************************************************************/

/* Pins describe I/O into clustered logic block.
 * A pin may be unconnected, driving a net or in the fanout, respectively. */
enum e_pin_type {
    OPEN = -1,
    DRIVER = 0,
    RECEIVER = 1
};

/* Type of interconnect within complex block: Complete for everything connected (full crossbar), direct for one-to-one connections, and mux for many-to-one connections */
enum e_interconnect {
    COMPLETE_INTERC = 1,
    DIRECT_INTERC = 2,
    MUX_INTERC = 3
};

/* Orientations. */
enum e_side : unsigned char {
    TOP = 0,
    RIGHT = 1,
    BOTTOM = 2,
    LEFT = 3,
    NUM_SIDES
};
constexpr std::array<e_side, NUM_SIDES> SIDES = {{TOP, RIGHT, BOTTOM, LEFT}};                    //Set of all side orientations
constexpr std::array<const char*, NUM_SIDES> SIDE_STRING = {{"TOP", "RIGHT", "BOTTOM", "LEFT"}}; //String versions of side orientations

/* pin location distributions */
enum e_pin_location_distr {
    E_SPREAD_PIN_DISTR,
    E_PERIMETER_PIN_DISTR,
    E_SPREAD_INPUTS_PERIMETER_OUTPUTS_PIN_DISTR,
    E_CUSTOM_PIN_DISTR
};

/* pb_type class */
enum e_pb_type_class {
    UNKNOWN_CLASS = 0,
    LUT_CLASS = 1,
    LATCH_CLASS = 2,
    MEMORY_CLASS = 3,
    NUM_CLASSES
};

// Set of all pb_type classes
constexpr std::array<e_pb_type_class, NUM_CLASSES> PB_TYPE_CLASSES = {
    {UNKNOWN_CLASS, LUT_CLASS, LATCH_CLASS, MEMORY_CLASS}};

// String versions of pb_type class values
constexpr std::array<const char*, NUM_CLASSES> PB_TYPE_CLASS_STRING = {
    {"unknown", "lut", "flipflop", "memory"}};

/* Annotations for pin-to-pin connections */
enum e_pin_to_pin_annotation_type {
    E_ANNOT_PIN_TO_PIN_DELAY = 0,
    E_ANNOT_PIN_TO_PIN_CAPACITANCE,
    E_ANNOT_PIN_TO_PIN_PACK_PATTERN
};
enum e_pin_to_pin_annotation_format {
    E_ANNOT_PIN_TO_PIN_MATRIX = 0,
    E_ANNOT_PIN_TO_PIN_CONSTANT
};
enum e_pin_to_pin_delay_annotations {
    E_ANNOT_PIN_TO_PIN_DELAY_MIN = 0,        //pb interconnect or primitive combinational max delay
    E_ANNOT_PIN_TO_PIN_DELAY_MAX,            //pb interconnect or primitive combinational max delay
    E_ANNOT_PIN_TO_PIN_DELAY_TSETUP,         //primitive setup time
    E_ANNOT_PIN_TO_PIN_DELAY_THOLD,          //primitive hold time
    E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN, //primitive min clock-to-q delay
    E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX, //primitive max clock-to-q delay
};
enum e_pin_to_pin_capacitance_annotations {
    E_ANNOT_PIN_TO_PIN_CAPACITANCE_C = 0
};
enum e_pin_to_pin_pack_pattern_annotations {
    E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME = 0
};

/* Power Estimation type for a PB */
enum e_power_estimation_method_ {
    POWER_METHOD_UNDEFINED = 0,
    POWER_METHOD_IGNORE,          /* Ignore power of this PB, and all children PB */
    POWER_METHOD_SUM_OF_CHILDREN, /* Ignore power of this PB, but consider children */
    POWER_METHOD_AUTO_SIZES,      /* Transistor-level, auto-sized buffers/wires */
    POWER_METHOD_SPECIFY_SIZES,   /* Transistor-level, user-specified buffers/wires */
    POWER_METHOD_TOGGLE_PINS,     /* Dynamic: Energy per pin toggle, Static: Absolute */
    POWER_METHOD_C_INTERNAL,      /* Dynamic: Equiv. Internal capacitance, Static: Absolute */
    POWER_METHOD_ABSOLUTE         /* Dynamic: Aboslute, Static: Absolute */
};
typedef enum e_power_estimation_method_ e_power_estimation_method;
typedef enum e_power_estimation_method_ t_power_estimation_method;

/* Specifies what part of the FPGA a custom switchblock should be built in (i.e. perimeter, core, everywhere) */
enum e_sb_location {
    E_PERIMETER = 0,
    E_CORNER,
    E_FRINGE, /* perimeter minus corners */
    E_CORE,
    E_EVERYWHERE
};

/*************************************************************************************************/
/* FPGA grid layout data types                                                                   */
/*************************************************************************************************/
/* Grid location specification
 *  Each member is a formula evaluated in terms of 'W' (device width),
 *  and 'H' (device height). Formulas can be evaluated using parse_formula()
 *  from expr_eval.h.
 */
struct t_grid_loc_spec {
    t_grid_loc_spec(std::string start, std::string end, std::string repeat, std::string incr)
        : start_expr(start)
        , end_expr(end)
        , repeat_expr(repeat)
        , incr_expr(incr) {}

    std::string start_expr; //Starting position (inclusive)
    std::string end_expr;   //Ending position (inclusive)

    std::string repeat_expr; //Distance between repeated
                             // region instances

    std::string incr_expr; //Distance between block instantiations
                           // with the region
};

/* Definition of how to place physical logic block in the grid.
 *  This defines a region of the grid to be set to a specific type
 *  (provided it's priority is high enough to override other blocks).
 *
 *  The diagram below illustrates the layout specification.
 *
 *                      +----+                +----+           +----+
 *                      |    |                |    |           |    |
 *                      |    |                |    |    ...    |    |
 *                      |    |                |    |           |    |
 *                      +----+                +----+           +----+
 *
 *                        .                     .                 .
 *                        .                     .                 .
 *                        .                     .                 .
 *
 *                      +----+                +----+           +----+
 *                      |    |                |    |           |    |
 *                      |    |                |    |    ...    |    |
 *                      |    |                |    |           |    |
 *                      +----+                +----+           +----+
 *                   ^
 *                   |
 *           repeaty |
 *                   |
 *                   v        (endx,endy)
 *                      +----+                +----+           +----+
 *                      |    |                |    |           |    |
 *                      |    |                |    |    ...    |    |
 *                      |    |                |    |           |    |
 *                      +----+                +----+           +----+
 *       (startx,starty)
 *                            <-------------->
 *                                 repeatx
 *
 *  startx/endx and endx/endy define a rectangular region instances dimensions.
 *  The region instance is then repeated every repeatx/repeaty (if specified).
 *
 *  Within a particular region instance a block of block_type is laid down every
 *  incrx/incry units (if not specified defaults to block width/height):
 *
 *
 *    * = an instance of block_type within the region
 *
 *                    +------------------------------+
 *                    |*         *         *        *|
 *                    |                              |
 *                    |                              |
 *                    |                              |
 *                    |                              |
 *                    |                              |
 *                    |*         *         *        *|
 *                ^   |                              |
 *                |   |                              |
 *          incry |   |                              |
 *                |   |                              |
 *                v   |                              |
 *                    |*         *         *        *|
 *                    +------------------------------+
 *
 *                      <------->
 *                        incrx
 *
 *  In the above diagram incrx = 10, and incry = 6
 */
struct t_grid_loc_def {
    t_grid_loc_def(std::string block_type_val, int priority_val)
        : block_type(block_type_val)
        , priority(priority_val)
        , x("0", "W-1", "max(w+1,W)", "w") //Fill in x direction, no repeat, incr by block width
        , y("0", "H-1", "max(h+1,H)", "h") //Fill in y direction, no repeat, incr by block height
    {}

    std::string block_type; //The block type name

    int priority = 0; //Priority of the specification.
                      // In case of conflicting specifications
                      // the largest priority wins.

    t_grid_loc_spec x; //Horizontal location specification
    t_grid_loc_spec y; //Veritcal location specification

    // When 1 metadata tag is split among multiple t_grid_loc_def, one
    // t_grid_loc_def is arbitrarily chosen to own the metadata, and the other
    // t_grid_loc_def point to the owned version.
    std::unique_ptr<t_metadata_dict> owned_meta;
    t_metadata_dict* meta = nullptr; // Metadata for this location definition. This
                                     // metadata may be shared with multiple grid_locs
                                     // that come from a common definition.
};

enum GridDefType {
    AUTO,
    FIXED
};

struct t_grid_def {
    GridDefType grid_type = GridDefType::AUTO; //The type of this grid specification

    std::string name = ""; //The name of this device

    int width = -1;  //Fixed device width (only valid for grid_type == FIXED)
    int height = -1; //Fixed device height (only valid for grid_type == FIXED)

    float aspect_ratio = 1.; //Aspect ratio for auto-sized devices (only valid for
                             //grid_type == AUTO)

    std::vector<t_grid_loc_def> loc_defs; //The list of grid location definitions for this grid specification
};

/************************* POWER ***********************************/

/* Global clock architecture */
struct t_clock_arch {
    int num_global_clocks;
    t_clock_network* clock_inf; /* Details about each clock */
};

/* Architecture information for a single clock */
struct t_clock_network {
    bool autosize_buffer; /* autosize clock buffers */
    float buffer_size;    /* if not autosized, the clock buffer size */
    float C_wire;         /* Wire capacitance (per meter) */

    float prob;   /* Static probability of net assigned to this clock */
    float dens;   /* Switching density of net assigned to this clock */
    float period; /* Period of clock */
};

/* Power-related architecture information */
struct t_power_arch {
    float C_wire_local; /* Capacitance of local interconnect (per meter) */
    //int seg_buffer_split; /* Split segment for distributed buffer (no split=1) */
    float logical_effort_factor;
    float local_interc_factor;
    float transistors_per_SRAM_bit;
    float mux_transistor_size;
    float FF_size;
    float LUT_transistor_size;
};

/* Power usage for an entity */
struct t_power_usage {
    float dynamic;
    float leakage;
};

/*************************************************************************************************/
/* FPGA Physical Logic Blocks data types                                                         */
/*************************************************************************************************/

enum class PortEquivalence {
    NONE,    //The pins within the port are not equivalent and can not be swapped
    FULL,    //The pins within the port are fully equivalent and can be freely swapped (e.g. logically equivalent or modelling a full-crossbar)
    INSTANCE //The port is equivalent with instance swapping (more restrictive that FULL)
};

/* A class of CLB pins that share common properties
 * port_name: name of this class of pins
 * type:  DRIVER or RECEIVER (what is this pinclass?)              *
 * num_pins:  The number of logically equivalent pins forming this *
 *           class.                                                *
 * pinlist[]:  List of clb pin numbers which belong to this class. */
struct t_class {
    enum e_pin_type type;
    PortEquivalence equivalence;
    int num_pins;
    int* pinlist; /* [0..num_pins - 1] */
};

enum e_power_wire_type {
    POWER_WIRE_TYPE_UNDEFINED = 0,
    POWER_WIRE_TYPE_IGNORED,
    POWER_WIRE_TYPE_C,
    POWER_WIRE_TYPE_ABSOLUTE_LENGTH,
    POWER_WIRE_TYPE_RELATIVE_LENGTH,
    POWER_WIRE_TYPE_AUTO
};

enum e_power_buffer_type {
    POWER_BUFFER_TYPE_UNDEFINED = 0,
    POWER_BUFFER_TYPE_NONE,
    POWER_BUFFER_TYPE_AUTO,
    POWER_BUFFER_TYPE_ABSOLUTE_SIZE
};

struct t_port_power {
    /* Transistor-Level Power Properties */

    // Wire
    e_power_wire_type wire_type;
    union {
        float C;
        float absolute_length;
        float relative_length;
    } wire;

    // Buffer
    e_power_buffer_type buffer_type;
    float buffer_size;

    /* Pin-Toggle Power Properties */
    bool pin_toggle_initialized;
    float energy_per_toggle;
    t_port* scaled_by_port;
    int scaled_by_port_pin_idx;
    bool reverse_scaled; /* Scale by (1-prob) */
};

//The type of Fc specification
enum class e_fc_type {
    IN, //The fc specification for an input pin
    OUT //The fc specification for an output pin
};

//The value type of the Fc specification
enum class e_fc_value_type {
    FRACTIONAL, //Fractional Fc specification (i.e. fraction of routing channel tracks)
    ABSOLUTE    //Absolute Fc specification (i.e. absolute number of tracks)
};

//Describes the Fc specification for a set of pins and a segment
struct t_fc_specification {
    e_fc_type fc_type;             //What type of Fc
    e_fc_value_type fc_value_type; //How to interpret the Fc value
    float fc_value;                //The Fc value
    int seg_index;                 //The target segment index
    std::vector<int> pins;         //The block pins collectively effected by this Fc
};

//Defines the default Fc specification for an architecture
struct t_default_fc_spec {
    bool specified = false;         //Whether or not a default specification exists
    e_fc_value_type in_value_type;  //Type of the input value (frac or abs)
    float in_value;                 //Input Fc value
    e_fc_value_type out_value_type; //Type of the output value (frac or abs)
    float out_value;                //Output Fc value
};

enum class e_sb_type {
    NONE,       //No SB at this location
    HORIZONTAL, //Horizontal straight-through connections
    VERTICAL,   //Vertical straight-through connections
    TURNS,      //Turning connections only
    STRAIGHT,   //Straight-through connections (i.e. vertical + horizontal)
    FULL        //Full SB at this location (i.e. turns + straight)

};

constexpr int NO_SWITCH = -1;
constexpr int DEFAULT_SWITCH = -2;

/* Describes the type for a physical tile
 * name: unique identifier for type
 * num_pins: Number of pins for the block
 * capacity: Number of blocks of this type that can occupy one grid tile (typically used by IOs).
 * width: Width of large block in grid tiles
 * height: Height of large block in grid tiles
 *
 * pinloc: Is set to true if a given pin exists on a certain position of a
 *         block. Derived from pin_location_distribution/pin_loc_assignments
 *
 * pin_location_distribution: The pin distribution type
 * num_pin_loc_assignments: The number of strings within each pin_loc_assignments
 * pin_loc_assignments: The strings for a custom pin location distribution.
 *                      Valid only for pin_location_distribution == E_CUSTOM_PIN_DISTR
 *
 * num_class: Number of logically-equivalent pin classes
 * class_inf: Information of each logically-equivalent class
 *
 * pin_avg_width_offset: Average width offset to specified pin (exact if only a single physical pin instance)
 * pin_avg_height_offset: Average height offset to specified pin (exact if only a single physical pin instance)
 * pin_class: The class a pin belongs to
 * is_ignored_pin: Whether or not a pin is ignored durring rr_graph generation and routing.
 *                 This is usually the case for clock pins and other global pins unless the
 *                 clock_modeling option is set to route the clock through regular inter-block
 *                 wiring or through a dedicated clock network.
 * is_pin_global: Whether or not this pin is marked as global. Clock pins and other specified
 *                global pins in the architecture file are marked as global.
 *
 * fc_specs: The Fc specifications for all pins
 *
 * switchblock_locations: Switch block configuration for this block.
 *                        Each element describes the type of SB which should be
 *                        constructed at the specified location.
 *                        Note that the SB is located to the top-right of the
 *                        grid tile location. [0..width-1][0..height-1]
 *
 * area: Describes how much area this logic block takes, if undefined, use default
 * type_timing_inf: timing information unique to this type
 * num_drivers: Total number of output drivers supplied
 * num_receivers: Total number of input receivers supplied
 * index: Keep track of type in array for easy access
 * logical_tile_index: index of the corresponding logical block type
 */
struct t_physical_tile_type {
    char* name = nullptr;
    int num_pins = 0;
    int num_input_pins = 0;
    int num_output_pins = 0;
    int num_clock_pins = 0;

    int capacity = 0;

    int width = 0;
    int height = 0;

    bool**** pinloc = nullptr; /* [0..width-1][0..height-1][0..3][0..num_pins-1] */

    enum e_pin_location_distr pin_location_distribution = E_SPREAD_PIN_DISTR;
    int*** num_pin_loc_assignments = nullptr; /* [0..width-1][0..height-1][0..3] */
    char***** pin_loc_assignments = nullptr;  /* [0..width-1][0..height-1][0..3][0..num_tokens-1][0..string_name] */

    int num_class = 0;
    t_class* class_inf = nullptr; /* [0..num_class-1] */

    std::vector<t_physical_tile_port> ports;
    std::vector<int> pin_width_offset;  //[0..num_pins-1]
    std::vector<int> pin_height_offset; //[0..num_pins-1]
    int* pin_class = nullptr;           /* [0..num_pins-1] */
    bool* is_ignored_pin = nullptr;     /* [0..num_pins-1] */
    bool* is_pin_global = nullptr;      /* [0..num_pins -1] */

    std::vector<t_fc_specification> fc_specs;

    vtr::Matrix<e_sb_type> switchblock_locations;
    vtr::Matrix<int> switchblock_switch_overrides;

    float area = 0;

    /* This info can be determined from class_inf and pin_class but stored for faster access */
    int num_drivers = 0;
    int num_receivers = 0;

    int index = -1; /* index of type descriptor in array (allows for index referencing) */

    std::vector<std::string> equivalent_sites_names;
    std::vector<t_logical_block_type_ptr> equivalent_sites;

    /* Map holding the priority for which this logical block needs to be placed.
     * logical_blocks_priority[priority] -> vector holding the logical_block indices */
    std::map<int, std::vector<int>> logical_blocks_priority;

    /* Unordered map indexed by the logical block index.
     * tile_block_pin_directs_map[logical block index][logical block pin] -> physical tile pin */
    std::unordered_map<int, std::unordered_map<int, int>> tile_block_pin_directs_map;

    /* Returns the indices of pins that contain a clock for this physical logic block */
    std::vector<int> get_clock_pins_indices() const;
};

/** Describes I/O and clock ports of a physical tile type
 *
 *  It corresponds to <port/> tags in the FPGA architecture description
 *
 *  Data members:
 *      name: name of the port
 *      is_clock: whether or not this port is a clock
 *      is_non_clock_global: Applies to top level pb_type, this pin is not a clock but
 *                           is a global signal (useful for stuff like global reset signals,
 *                           perhaps useful for VCC and GND)
 *      num_pins: the number of pins this port has
 *      tile_type: pointer to the associated tile type
 *      port_class: port belongs to recognized set of ports in class library
 *      index: port index by index in array of parent pb_type
 *      absolute_first_pin_index: absolute index of the first pin in the physical tile.
 *                                All the other pin indices can be calculated with num_pins
 *      port_index_by_type index of port by type (index by input, output, or clock)
 *      equivalence: Applies to logic block I/Os and to primitive inputs only
 */
struct t_physical_tile_port {
    char* name;
    enum PORTS type;
    bool is_clock;
    bool is_non_clock_global;
    int num_pins;
    PortEquivalence equivalent = PortEquivalence::NONE;

    int index;
    int absolute_first_pin_index;
    int port_index_by_type;
    int tile_type_index;
};

/* Describes the type for a logical block
 * name: unique identifier for type
 * pb_type: Internal subblocks and routing information for this physical block
 * pb_graph_head: Head of DAG of pb_types_nodes and their edges
 *
 * index: Keep track of type in array for easy access
 * physical_tile_index: index of the corresponding physical tile type
 */
struct t_logical_block_type {
    char* name = nullptr;

    /* Clustering info */
    t_pb_type* pb_type = nullptr;
    t_pb_graph_node* pb_graph_head = nullptr;

    int index = -1; /* index of type descriptor in array (allows for index referencing) */

    std::vector<t_physical_tile_type_ptr> equivalent_tiles;

    /* Map holding the priority for which this logical block needs to be placed.
     * physical_tiles_priority[priority] -> vector holding the physical tile indices */
    std::map<int, std::vector<int>> physical_tiles_priority;
};

/*************************************************************************************************
 * PB Type Hierarchy                                                                             *
 *************************************************************************************************
 *
 * VPR represents the 'type' of block types corresponding to FPGA grid locations using a hierarchy
 * of t_pb_type objects.
 *
 * The root t_pb_type corresponds to a single top level block type and maps to a particular type
 * of location in the FPGA device grid (e.g. Logic, DSP, RAM etc.).
 *
 * A non-root t_pb_type represents an intermediate level of hierarchy within the root block type.
 *
 * The PB Type hierarchy corresponds to the tags specified in the FPGA architecture description:
 *
 *      struct              XML Tag
 *      ------              ------------
 *      t_pb_type           <pb_type/>
 *      t_mode              <mode/>
 *      t_interconnect      <interconnect/>
 *      t_port              <port/>
 *
 * The various structures hold pointers to each other which encode the hierarchy.
 */

/** Describes the type of clustered block if a root (parent_mode == nullptr), an
 *  intermediate level of hierarchy (parent_mode != nullptr), or a leaf/primitive
 *  (num_modes == 0, model != nullptr).
 *
 *  This (along with t_mode) corresponds to the hierarchical specification of
 *  block modes that users provide in the architecture (i.e. <pb_type/> tags).
 *
 *  It is also useful to note that a single t_pb_type may represent multiple instances of that
 *  type in the architecture (see the num_pb field).
 *
 *  In VPR there is a single instance of a t_pb_type for each type, which is referenced as a
 *  flyweight by other objects (e.g. t_pb_graph_node).
 *
 *  Data members:
 *      name: name of the physical block type
 *      num_pb: maximum number of instances of this physical block type sharing one parent
 *      blif_model: the string in the blif circuit that corresponds with this pb type
 *      class_type: Special library name
 *      modes: Different modes accepted
 *      ports: I/O and clock ports
 *      num_clock_pins: A count of the total number of clock pins
 *      num_input_pins: A count of the total number of input pins
 *      num_output_pins: A count of the total number of output pins
 *      num_pins: A count of the total number of pins
 *      timing: Timing matrix of block [0..num_inputs-1][0..num_outputs-1]
 *      parent_mode: mode of the parent block
 *      t_mode_power: ???
 *      meta: Table storing extra arbitrary metadata attributes.
 */
struct t_pb_type {
    char* name = nullptr;
    int num_pb = 0;
    char* blif_model = nullptr;
    t_model* model = nullptr;
    enum e_pb_type_class class_type = UNKNOWN_CLASS;

    t_mode* modes = nullptr; /* [0..num_modes-1] */
    int num_modes = 0;
    t_port* ports = nullptr; /* [0..num_ports] */
    int num_ports = 0;

    int num_clock_pins = 0;
    int num_input_pins = 0; /* inputs not including clock pins */
    int num_output_pins = 0;

    int num_pins = 0;

    t_mode* parent_mode = nullptr;
    int depth = 0; /* depth of pb_type */

    t_pin_to_pin_annotation* annotations = nullptr; /* [0..num_annotations-1] */
    int num_annotations = 0;

    /* Power related members */
    t_pb_type_power* pb_type_power = nullptr;

    t_metadata_dict meta;
};

/** Describes an operational mode of a clustered logic block
 *
 *  This forms part of the t_pb_type hierarchical description of a clustered logic block.
 *  It corresponds to <mode/> tags in the FPGA architecture description
 *
 *  Data members:
 *      name: name of the mode
 *      pb_type_children: pb_types it contains
 *      interconnect: interconnect of parent pb_type to children pb_types or children to children pb_types
 *      num_interconnect: Total number of interconnect tags specified by user
 *      parent_pb_type: Which parent contains this mode
 *      index: Index of mode in array with other modes
 *      t_mode_power: ???
 *      meta: Table storing extra arbitrary metadata attributes.
 */
struct t_mode {
    char* name = nullptr;
    t_pb_type* pb_type_children = nullptr; /* [0..num_child_pb_types] */
    int num_pb_type_children = 0;
    t_interconnect* interconnect = nullptr;
    int num_interconnect = 0;
    t_pb_type* parent_pb_type = nullptr;
    int index = 0;

    /* Power related members */
    t_mode_power* mode_power = nullptr;

    t_metadata_dict meta;
};

/** Describes an interconnect edge inside a cluster
 *
 *  This forms part of the t_pb_type hierarchical description of a clustered logic block.
 *  It corresponds to <interconnect/> tags in the FPGA architecture description
 *
 *  Data members:
 *      type: type of the interconnect
 *      name: identifier for interconnect
 *      input_string: input string verbatim to parse later
 *      output_string: input string output to parse later
 *      annotations: Annotations for delay, power, etc
 *      num_annotations: Total number of annotations
 *      infer_annotations: This interconnect is autogenerated, if true, infer pack_patterns
 *                         such as carry-chains and forced packs based on interconnect linked to it
 *      parent_mode_index: Mode of parent as int
 */
struct t_interconnect {
    enum e_interconnect type;
    char* name = nullptr;

    char* input_string = nullptr;
    char* output_string = nullptr;

    t_pin_to_pin_annotation* annotations = nullptr; /* [0..num_annotations-1] */
    int num_annotations = 0;
    bool infer_annotations = false;

    int line_num = 0; /* Interconnect is processed later, need to know what line number it messed up on to give proper error message */

    int parent_mode_index = 0;

    /* Power related members */
    t_mode* parent_mode = nullptr;

    t_interconnect_power* interconnect_power = nullptr;
    t_metadata_dict meta;
};

/** Describes I/O and clock ports
 *
 *  This forms part of the t_pb_type hierarchical description of a clustered logic block.
 *  It corresponds to <port/> tags in the FPGA architecture description
 *
 *  Data members:
 *      name: name of the port
 *      model_port: associated model port
 *      is_clock: whether or not this port is a clock
 *      is_non_clock_global: Applies to top level pb_type, this pin is not a clock but
 *                           is a global signal (useful for stuff like global reset signals,
 *                           perhaps useful for VCC and GND)
 *      num_pins: the number of pins this port has
 *      parent_pb_type: pointer to the parent pb_type
 *      port_class: port belongs to recognized set of ports in class library
 *      index: port index by index in array of parent pb_type
 *      port_index_by_type index of port by type (index by input, output, or clock)
 *      equivalence: Applies to logic block I/Os and to primitive inputs only
 */
struct t_port {
    char* name;
    t_model_ports* model_port;
    enum PORTS type;
    bool is_clock;
    bool is_non_clock_global;
    int num_pins;
    PortEquivalence equivalent;
    t_pb_type* parent_pb_type;
    char* port_class;

    int index;
    int port_index_by_type;
    int absolute_first_pin_index;

    t_port_power* port_power;
};

struct t_pb_type_power {
    /* Type of power estimation for this pb */
    e_power_estimation_method estimation_method;

    t_power_usage absolute_power_per_instance; /* User-provided absolute power per block */

    float C_internal;         /*Internal capacitance of the pb */
    int leakage_default_mode; /* Default mode for leakage analysis, if block has no set mode */

    t_power_usage power_usage;            /* Total power usage of this pb type */
    t_power_usage power_usage_bufs_wires; /* Power dissipated in local buffers and wire switching (Subset of total power) */
};

struct t_interconnect_power {
    t_power_usage power_usage;

    /* These are not necessarily power-related; however, at the moment
     * only power estimation uses them
     */
    bool port_info_initialized;
    int num_input_ports;
    int num_output_ports;
    int num_pins_per_port;
    float transistor_cnt;
};

struct t_interconnect_pins {
    t_interconnect* interconnect;

    t_pb_graph_pin*** input_pins;  // [0..num_input_ports-1][0..num_pins_per_port-1]
    t_pb_graph_pin*** output_pins; // [0..num_output_ports-1][0..num_pins_per_port-1]
};

struct t_mode_power {
    t_power_usage power_usage; /* Power usage of this mode */
};

/** Info placed between pins in the architecture file (e.g. delay annotations),
 *
 * This is later for additional information.
 *
 * Data Members:
 *      value: value/property pair
 *      prop: value/property pair
 *      type: type of annotation
 *      format: formatting of data
 *      input_pins: input pins as string affected by annotation
 *      output_pins: output pins as string affected by annotation
 *      clock_pin: clock as string affected by annotation
 */
struct t_pin_to_pin_annotation {
    char** value; /* [0..num_value_prop_pairs - 1] */
    int* prop;    /* [0..num_value_prop_pairs - 1] */
    int num_value_prop_pairs;

    enum e_pin_to_pin_annotation_type type;
    enum e_pin_to_pin_annotation_format format;

    char* input_pins;
    char* output_pins;
    char* clock;

    int line_num; /* used to report what line number this annotation is found in architecture file */
};

/*************************************************************************************************
 * PB Graph                                                                                      *
 *************************************************************************************************
 *
 * The PB graph represents the flattened and elaborated connectivity within a t_pb_type (i.e.
 * the routing resource graph), derived from the t_pb_type hierarchy.
 *
 * The PB graph is built of t_pb_graph_node and t_pb_graph_pin objects.
 *
 * There is a single PB graph associated with each root t_pb_type, and it is referenced in other objects (e.g.
 * t_pb) as a flyweight.
 *
 */

/** Describes the internal connectivity corresponding to a t_pb_type and t_mode of a cluster.
 *
 *  There is a t_pb_graph_node for each instance of the pb_type (i.e. t_pb_type may describe
 *  num_pb instances of the type, with each instance represented as a t_pb_graph_node).
 *  The distinction between the pb_type and the pb_graph_node is necessary since the 'position'
 *  of a particular instance in the cluster is important when routing the cluster (since the routing
 *  accessible from each position may be different).
 *
 *  Data members:
 *      pb_type               : Pointer to the type of pb graph node this belongs to
 *      placement_index       : there are a certain number of pbs available, this gives the index of the node
 *      child_pb_graph_nodes  : array of children pb graph nodes organized into modes
 *      parent_pb_graph_node  : parent pb graph node
 *      total_primitive_count : Total number of this primitive type in the cluster. If there are 10 ALMs per cluster
 *                              and 2 FFs per ALM (given the mode of the parent of this primitive) then the total is 20.
 *      illegal_modes         : vector containing illegal modes that result in conflicts during routing
 */
class t_pb_graph_node {
  public:
    t_pb_type* pb_type;

    int placement_index;

    /* Contains a collection of mode indices that cannot be used as they produce conflicts during VPR packing stage
     *
     * Illegal modes do arise when children of a graph_node do have inconsistent `edge_modes` with respect to
     * the parent_pb.
     * Example: Edges that connect LUTs A, B and C to the parent pb_graph_node refer to the correct parent's mode which is set to "LUTs",
     *          but edges of LUT D have the mode of edge corresponding to a wrong parent's pb_graph_node mode, namely "LUTRAM".
     *          This situation is unfeasible as the edge modes are inconsistent between siblings of the same parent pb_graph_node.
     *          In this case, the "LUTs" mode of the parent pb_graph_node cannot be used as the LUT D is not able to have a feasible
     *          edge mode that does relate with the other sibling's edge modes.
     *
     *          The "LUTs" index mode is added to the illegal_modes vector. The conflicting mode marked as illegal is the most restrictive one.
     *          This means that LUT D is unable to be routed if using the parent's "LUTs" mode (otherwise "LUTs" mode would be selected for LUT D
     *          as well), but LUTs A, B and C could still be routed using the parent pb_graph_node's mode "LUTRAM".
     *          Therefore, "LUTs" is marked as illegal and all the LUTs (A, B, C and D) will have a consistent parent pb_graph_node mode, namely "LUTRAM".
     *
     * Usage: cluster_router uses this information to exclude the expansion of a node which has a not cosistent mode.
     *        Everytime the mode consistency check fails, the index of the mode that causes the conflict is added to this vector.
     * */
    std::vector<int> illegal_modes;

    t_pb_graph_pin** input_pins;  /* [0..num_input_ports-1] [0..num_port_pins-1]*/
    t_pb_graph_pin** output_pins; /* [0..num_output_ports-1] [0..num_port_pins-1]*/
    t_pb_graph_pin** clock_pins;  /* [0..num_clock_ports-1] [0..num_port_pins-1]*/

    int num_input_ports;
    int num_output_ports;
    int num_clock_ports;

    int* num_input_pins;  /* [0..num_input_ports - 1] */
    int* num_output_pins; /* [0..num_output_ports - 1] */
    int* num_clock_pins;  /* [0..num_clock_ports - 1] */

    t_pb_graph_node*** child_pb_graph_nodes; /* [0..num_modes-1][0..num_pb_type_in_mode-1][0..num_pb-1] */
    t_pb_graph_node* parent_pb_graph_node;

    int total_pb_pins; /* only valid for top-level */

    void* temp_scratch_pad;                                     /* temporary data, useful for keeping track of things when traversing data structure */
    t_cluster_placement_primitive* cluster_placement_primitive; /* pointer to indexing structure useful during packing stage */

    int* input_pin_class_size;  /* Stores the number of pins that belong to a particular input pin class */
    int num_input_pin_class;    /* number of input pin classes that this pb_graph_node has */
    int* output_pin_class_size; /* Stores the number of pins that belong to a particular output pin class */
    int num_output_pin_class;   /* number of output pin classes that this pb_graph_node has */

    int total_primitive_count; /* total number of this primitive type in the cluster */

    /* Interconnect instances for this pb
     * Only used for power
     */
    t_pb_graph_node_power* pb_node_power;
    t_interconnect_pins** interconnect_pins; /* [0..num_modes-1][0..num_interconnect_in_mode] */

    // Returns true if this pb_graph_node represents a primitive type (primitives have 0 modes)
    bool is_primitive() const { return this->pb_type->num_modes == 0; }

    // Returns true if this pb_graph_node represents a root graph node (ex. clb)
    bool is_root() const { return this->parent_pb_graph_node == nullptr; }

    //Returns the number of pins on this graph node
    //  Note this is the total for all ports on this node excluding any children (i.e. sum of all num_input_pins, num_output_pins, num_clock_pins)
    int num_pins() const;
    // Returns a string containing the hierarchical type name of the pb_graph_node
    // Ex: clb[0][default]/lab[0][default]/fle[3][n1_lut6]/ble6[0][default]/lut6[0]
    std::string hierarchical_type_name() const;
};

/* Identify pb pin type for timing purposes */
enum e_pb_graph_pin_type {
    PB_PIN_NORMAL = 0,
    PB_PIN_SEQUENTIAL,
    PB_PIN_INPAD,
    PB_PIN_OUTPAD,
    PB_PIN_TERMINAL,
    PB_PIN_CLOCK
};

/** Describes a pb graph pin
 *
 *  Data Members:
 *      port: pointer to the port that this pin is associated with
 *      pin_number: pin number of the port that this pin is associated with
 *      input edges: [0..num_input_edges - 1]edges incoming
 *      num_input_edges: number edges incoming
 *      output edges: [0..num_output_edges - 1]edges out_going
 *      num_output_edges: number edges out_going
 *      parent_node: parent pb_graph_node
 *      pin_count_in_cluster: Unique number for pin inside cluster
 */
class t_pb_graph_pin {
  public:
    t_port* port = nullptr;
    int pin_number = 0;
    t_pb_graph_edge** input_edges = nullptr; /* [0..num_input_edges] */
    int num_input_edges = 0;
    t_pb_graph_edge** output_edges = nullptr; /* [0..num_output_edges] */
    int num_output_edges = 0;

    t_pb_graph_node* parent_node = nullptr;
    int pin_count_in_cluster = 0;

    int scratch_pad = 0; /* temporary data structure useful to store traversal info */

    enum e_pb_graph_pin_type type = PB_PIN_NORMAL; /* The type of this pin (sequential, i/o etc.) */

    /* sequential timing information */
    float tsu = std::numeric_limits<float>::quiet_NaN();     /* For sequential logic elements the setup time */
    float thld = std::numeric_limits<float>::quiet_NaN();    /* For sequential logic elements the hold time */
    float tco_min = std::numeric_limits<float>::quiet_NaN(); /* For sequential logic elements the minimum clock to output time */
    float tco_max = std::numeric_limits<float>::quiet_NaN(); /* For sequential logic elements the maximum clock to output time */
    t_pb_graph_pin* associated_clock_pin = nullptr;          /* For sequentail elements, the associated clock */

    /* combinational timing information */
    int num_pin_timing = 0;                   /* Number of ipin to opin timing edges*/
    t_pb_graph_pin** pin_timing = nullptr;    /* timing edge sink pins  [0..num_pin_timing-1]*/
    float* pin_timing_del_max = nullptr;      /* primitive ipin to opin max-delay [0..num_pin_timing-1]*/
    float* pin_timing_del_min = nullptr;      /* primitive ipin to opin min-delay [0..num_pin_timing-1]*/
    int num_pin_timing_del_max_annotated = 0; //The list of valid pin_timing_del_max entries runs from [0..num_pin_timing_del_max_annotated-1]
    int num_pin_timing_del_min_annotated = 0; //The list of valid pin_timing_del_max entries runs from [0..num_pin_timing_del_min_annotated-1]

    /* Applies to clusters only */
    int pin_class = 0;

    /* Applies to pins of primitive only */
    int* parent_pin_class = nullptr; /* [0..depth-1] the grouping of pins that this particular pin belongs to */
    /* Applies to output pins of primitives only */
    t_pb_graph_pin*** list_of_connectable_input_pin_ptrs = nullptr; /* [0..depth-1][0..num_connectable_primitive_input_pins-1] what input pins this output can connect to without exiting cluster at given depth */
    int* num_connectable_primitive_input_pins = nullptr;            /* [0..depth-1] number of input pins that this output pin can reach without exiting cluster at given depth */

    bool is_forced_connection = false; /* This output pin connects to one and only one input pin */

    t_pb_graph_pin_power* pin_power = nullptr;

    // class member functions
  public:
    // Returns true if this pin belongs to a primitive block like
    // a LUT or FF, instead of a cluster-level block like a CLB.
    bool is_primitive_pin() const {
        return this->parent_node->is_primitive();
    }
    // Returns true if this pin belongs to a root pb_block which is a pb_block
    // that has no parent block. For example, pins of a CLB, IO, DSP, etc.
    bool is_root_block_pin() const {
        return this->parent_node->is_root();
    }
    // This function returns a string that contains the name of the pin
    // and the entire sequence of pb_types in the hierarchy from the block
    // of this pin back to the cluster-level (top-level) pb_type in the
    // following format: clb[0]/lab[0]/fle[3]/ble6[0]/lut6[0].in[0]
    // if full_description is set to false it will only return lut6[0].in[0]
    std::string to_string(const bool full_description = true) const;
};

/** Describes a pb graph edge
 *
 *  Note that this is a "fat" edge which supports bused based connections
 *
 *  Data members:
 *      input_pins: array of pb_type graph input pins ptrs entering this edge
 *      num_input_pins: Number of input pins entering this edge
 *      output_pins: array of pb_type graph output pins ptrs exiting this edge
 *      num_output_pins: Number of output pins exiting this edge
 *
 *      num_pack_patterns: number of pack patterns this edge belongs to
 *      pack_pattern_names: [0..num_pack_patterns-1] name of each pack pattern
 *      pack_pattern_indices: [0..num_pack_patterns-1] id of each pack pattern
 *      infer_pattern: if true, pattern of this edge could be inferred by checking
 *                     input/output edges. This is true when the edge is a single
 *                     fanout edge and is driven or driving another edge which is
 *                     annotated with a pack pattern.
 */
class t_pb_graph_edge {
  public:
    /* edge connectivity */
    t_pb_graph_pin** input_pins;
    int num_input_pins;
    t_pb_graph_pin** output_pins;
    int num_output_pins;

    /* timing information */
    float delay_max;
    float delay_min;
    float capacitance;

    /* who drives this edge */
    t_interconnect* interconnect;
    int driver_set;
    int driver_pin;

    /* pack pattern info */
    int num_pack_patterns;
    const char** pack_pattern_names;
    int* pack_pattern_indices;
    bool infer_pattern;

    // class member functions
  public:
    // Returns true is this edge is annotated with the given pattern_index
    //  pattern_index : index of the packing pattern
    bool annotated_with_pattern(int pattern_index) const;

    // Returns true is this edge is annotated with pattern_index or its pattern
    // is inferred and a connected output edge is annotated with pattern_index
    //   pattern_index : index of the packing pattern
    bool belongs_to_pattern(int pattern_index) const;
};

struct t_pb_graph_node_power {
    float transistor_cnt_pb_children; /* Total transistor size of this pb */
    float transistor_cnt_interc;      /* Total transistor size of the interconnect in this pb */
    float transistor_cnt_buffers;
};

struct t_pb_graph_pin_power {
    /* Transistor-level Power Properties */
    float C_wire;
    float buffer_size;

    /* Pin-Toggle Power Properties */
    t_pb_graph_pin* scaled_by_pin;
};

/*************************************************************************************************/
/* FPGA Routing architecture                                                                     */
/*************************************************************************************************/

/* Description of routing channel distribution across the FPGA, only available for global routing
 * Width is standard dev. for Gaussian.  xpeak is where peak     *
 * occurs. dc is the dc offset for Gaussian and pulse waveforms. */
enum e_stat {
    UNIFORM,
    GAUSSIAN,
    PULSE,
    DELTA
};
struct t_chan {
    enum e_stat type;
    float peak;
    float width;
    float xpeak;
    float dc;
};

/* chan_x_dist: Describes the x-directed channel width distribution.         *
 * chan_y_dist: Describes the y-directed channel width distribution.         */
struct t_chan_width_dist {
    t_chan chan_x_dist;
    t_chan chan_y_dist;
};

enum e_directionality {
    UNI_DIRECTIONAL,
    BI_DIRECTIONAL
};
enum e_switch_block_type {
    SUBSET,
    WILTON,
    UNIVERSAL,
    FULL,
    CUSTOM
};
typedef enum e_switch_block_type t_switch_block_type;
enum e_Fc_type {
    ABSOLUTE,
    FRACTIONAL
};

/* Lists all the important information about a certain segment type.  Only   *
 * used if the route_type is DETAILED.  [0 .. det_routing_arch.num_segment]  *
 * name: the name of this segment                                            *
 * frequency:  ratio of tracks which are of this segment type.               *
 * length:     Length (in clbs) of the segment.                              *
 * arch_wire_switch: Index of the switch type that connects other wires      *
 *                   *to* this segment. Note that this index is in relation  *
 *                   to the switches from the architecture file, not the     *
 *                   expanded list of switches that is built at the end of   *
 *                   build_rr_graph.                                         *
 * arch_opin_switch: Index of the switch type that connects output pins      *
 *                   (OPINs) *to* this segment. Note that this index is in   *
 *                   relation to the switches from the architecture file,    *
 *                   not the expanded list of switches that is built         *
 *                   at the end of build_rr_graph                            *
 * frac_cb:  The fraction of logic blocks along its length to which this     *
 *           segment can connect.  (i.e. internal population).               *
 * frac_sb:  The fraction of the length + 1 switch blocks along the segment  *
 *           to which the segment can connect.  Segments that aren't long    *
 *           lines must connect to at least two switch boxes.                *
 * Cmetal: Capacitance of a routing track, per unit logic block length.      *
 * Rmetal: Resistance of a routing track, per unit logic block length.       *
 * (UDSD by AY) drivers: How do signals driving a routing track connect to   *
 *                       the track?                                          *
 * meta: Table storing extra arbitrary metadata attributes.                  */
struct t_segment_inf {
    std::string name;
    int frequency;
    int length;
    short arch_wire_switch;
    short arch_opin_switch;
    float frac_cb;
    float frac_sb;
    bool longline;
    float Rmetal;
    float Cmetal;
    enum e_directionality directionality;
    std::vector<bool> cb;
    std::vector<bool> sb;
    //float Cmetal_per_m; /* Wire capacitance (per meter) */
};

inline bool operator==(const t_segment_inf& a, const t_segment_inf& b) {
    return a.name == b.name && a.frequency == b.frequency && a.length == b.length && a.arch_wire_switch == b.arch_wire_switch && a.arch_opin_switch == b.arch_opin_switch && a.frac_cb == b.frac_cb && a.frac_sb == b.frac_sb && a.longline == b.longline && a.Rmetal == b.Rmetal && a.Cmetal == b.Cmetal && a.directionality == b.directionality && a.cb == b.cb && a.sb == b.sb;
}

enum class SwitchType {
    MUX = 0,   //A configurable (buffered) mux (single-driver)
    TRISTATE,  //A configurable tristate-able buffer (multi-driver)
    PASS_GATE, //A configurable pass transitor switch (multi-driver)
    SHORT,     //A non-configurable electrically shorted connection (multi-driver)
    BUFFER,    //A non-configurable non-tristate-able buffer (uni-driver)
    INVALID,   //Unspecified, usually an error
    NUM_SWITCH_TYPES
};
constexpr std::array<const char*, size_t(SwitchType::NUM_SWITCH_TYPES)> SWITCH_TYPE_STRINGS = {{"MUX", "TRISTATE", "PASS_GATE", "SHORT", "BUFFER", "INVALID"}};

enum class BufferSize {
    AUTO,
    ABSOLUTE
};

/* Lists all the important information about a switch type read from the     *
 * architecture file.                                                        *
 * [0 .. Arch.num_switch]                                                    *
 * buffered:  Does this switch include a buffer?                             *
 * R:  Equivalent resistance of the buffer/switch.                           *
 * Cin:  Input capacitance.                                                  *
 * Cout:  Output capacitance.                                                *
 * Cinternal: Since multiplexers and tristate buffers are modeled as a       *
 *            parallel stream of pass transistors feeding into a buffer,     *
 *            we would expect an additional "internal capacitance"           *
 *            to arise when the pass transistor is enabled and the signal    *
 *            must propogate to the buffer. See diagram of one stream below: *
 *                                                                           *   
 *                  Pass Transistor                                          *
 *                       |                                                   *
 *                     -----                                                 *
 *                     -----      Buffer                                     *   
 *                    |     |       |\                                       *
 *              ------       -------| \--------                              *
 *                |             |   | /    |                                 *
 *              =====         ===== |/   =====                               *
 *              =====         =====      =====                               *
 *                |             |          |                                 *
 *             Input C    Internal C    Output C                             *
 *                                                                           *
 * Tdel_map: A map where the key is the number of inputs and the entry       *
 *           is the corresponding delay. If there is only one entry at key   *
 *           UNDEFINED, then delay is a constant (doesn't vary with fan-in). *
 *	         A map saves us the trouble of sorting, and has lower access     *
 *           time for interpolation/extrapolation purposes                   *
 * mux_trans_size:  The area of each transistor in the segment's driving mux *
 *                  measured in minimum width transistor units               *
 * buf_size:  The area of the buffer. If set to zero, area should be         *
 *            calculated from R                                              */
struct t_arch_switch_inf {
  public:
    static constexpr int UNDEFINED_FANIN = -1;

    char* name = nullptr;
    float R = 0.;
    float Cin = 0.;
    float Cout = 0.;
    float Cinternal = 0.;
    float mux_trans_size = 1.;
    BufferSize buf_size_type = BufferSize::AUTO;
    float buf_size = 0.;
    e_power_buffer_type power_buffer_type = POWER_BUFFER_TYPE_AUTO;
    float power_buffer_size = 0.;

  public:
    //Returns the type of switch
    SwitchType type() const;

    //Returns true if this switch type isolates its input and output into
    //separate DC-connected subcircuits
    bool buffered() const;

    //Returns true if this switch type is configurable
    bool configurable() const;

    //Returns whether the switch's directionality (e.g. BI_DIR, UNI_DIR)
    e_directionality directionality() const;

    //Returns the intrinsic delay of this switch
    float Tdel(int fanin = UNDEFINED_FANIN) const;

    //Returns true if the Tdel value is independent of fanout
    bool fixed_Tdel() const;

  public:
    void set_Tdel(int fanin, float delay);
    void set_type(SwitchType type_val);

  private:
    SwitchType type_ = SwitchType::INVALID;
    std::map<int, double> Tdel_map_;

    friend void PrintArchInfo(FILE*, const t_arch*);
};

/* Lists all the important information about an rr switch type.              *
 * The s_rr_switch_inf describes a switch derived from a switch described    *
 * by s_arch_switch_inf. This indirection allows us to vary properties of a  *
 * given switch, such as varying delay with switch fan-in.                   *
 * buffered:  Does this switch isolate it's input/output into separate       *
 *            DC-connected sub-circuits?                                     *
 * configurable: Is this switch is configurable (i.e. can the switch can be  *
 *               turned on or off)?. This allows modelling of non-optional   *
 *               switches (e.g. fixed buffers, or shorted connections) which *
 *               must be used (e.g. expanded by the router) if a connected   *
 *               segment is used.                                            *
 * R:  Equivalent resistance of the buffer/switch.                           *
 * Cin:  Input capacitance.                                                  *
 * Cout:  Output capacitance.                                                *
 * Cinternal: Internal capacitance, see the definition above.                *
 * Tdel:  Intrinsic delay.  The delay through an unloaded switch is          *
 *        Tdel + R * Cout.                                                   *
 * mux_trans_size:  The area of each transistor in the segment's driving mux *
 *                  measured in minimum width transistor units               *
 * buf_size:  The area of the buffer. If set to zero, area should be         *
 *            calculated from R                                              */
struct t_rr_switch_inf {
    float R = 0.;
    float Cin = 0.;
    float Cout = 0.;
    float Cinternal = 0.;
    float Tdel = 0.;
    float mux_trans_size = 0.;
    float buf_size = 0.;
    const char* name = nullptr;
    e_power_buffer_type power_buffer_type = POWER_BUFFER_TYPE_UNDEFINED;
    float power_buffer_size = 0.;

  public:
    //Returns the type of switch
    SwitchType type() const;

    //Returns true if this switch type isolates its input and output into
    //seperate DC-connected subcircuits
    bool buffered() const;

    //Returns true if this switch type is configurable
    bool configurable() const;

  public:
    void set_type(SwitchType type_val);

  private:
    SwitchType type_ = SwitchType::INVALID;
};

/* Lists all the important information about a direct chain connection.     *
 * [0 .. det_routing_arch.num_direct]                                       *
 * name:  Name of this direct chain connection                              *
 * from_pin:  The type of the pin that drives this chain connection         *
 * In the format of <block_name>.<pin_name>                      *
 * to_pin:  The type of pin that is driven by this chain connection         *
 * In the format of <block_name>.<pin_name>                        *
 * x_offset:  The x offset from the source to the sink of this connection   *
 * y_offset:  The y offset from the source to the sink of this connection   *
 * z_offset:  The z offset from the source to the sink of this connection   *
 * switch_type: The index into the switch list for the switch used by this  *
 *              direct                                                      *
 * line: The line number in the .arch file that specifies this              *
 *       particular placement macro.                                        *
 */
struct t_direct_inf {
    char* name;
    char* from_pin;
    char* to_pin;
    int x_offset;
    int y_offset;
    int z_offset;
    int switch_type;
    e_side from_side;
    e_side to_side;
    int line;
};

enum class SwitchPointOrder {
    FIXED,   //Switchpoints are ordered as specified in architecture
    SHUFFLED //Switchpoints are shuffled (more diversity)
};

//A collection of switchpoints associated with a segment
struct t_wire_switchpoints {
    std::string segment_name;      //The type of segment
    std::vector<int> switchpoints; //The indices of wire points along the segment
};

/* Used to list information about a set of track segments that should connect through a switchblock */
struct t_wireconn_inf {
    std::vector<t_wire_switchpoints> from_switchpoint_set;             //The set of segment/wirepoints representing the 'from' set (union of all t_wire_switchpoints in vector)
    std::vector<t_wire_switchpoints> to_switchpoint_set;               //The set of segment/wirepoints representing the 'to' set (union of all t_wire_switchpoints in vector)
    SwitchPointOrder from_switchpoint_order = SwitchPointOrder::FIXED; //The desired from_switchpoint_set ordering
    SwitchPointOrder to_switchpoint_order = SwitchPointOrder::FIXED;   //The desired to_switchpoint_set ordering

    std::string num_conns_formula; /* Specifies how many connections should be made for this wireconn.
                                    *
                                    * '<int>': A specific number of connections
                                    * 'from':  The number of generated connections between the 'from' and 'to' sets equals the
                                    *          size of the 'from' set. This ensures every element in the from set is connected
                                    *          to an element of the 'to' set.
                                    *          Note: this it may result in 'to' elements being driven by multiple 'from'
                                    *          elements (if 'from' is larger than 'to'), or in some elements of 'to' having
                                    *          no driving connections (if 'to' is larger than 'from').
                                    * 'to':    The number of generated connections is set equal to the size of the 'to' set.
                                    *          This ensures that each element of the 'to' set has precisely one incomming connection.
                                    *          Note: this may result in 'from' elements driving multiple 'to' elements (if 'to' is
                                    *          larger than 'from'), or some 'from' elements driving to 'to' elements (if 'from' is
                                    *          larger than 'to')
                                    */
};

/* represents a connection between two sides of a switchblock */
class SB_Side_Connection {
  public:
    /* specify the two SB sides that form a connection */
    enum e_side from_side = TOP;
    enum e_side to_side = TOP;

    void set_sides(enum e_side from, enum e_side to) {
        from_side = from;
        to_side = to;
    }

    SB_Side_Connection() = default;

    SB_Side_Connection(enum e_side from, enum e_side to)
        : from_side(from)
        , to_side(to) {
    }

    /* overload < operator which will be used by std::map */
    bool operator<(const SB_Side_Connection& obj) const {
        bool result;

        if (from_side < obj.from_side) {
            result = true;
        } else {
            if (from_side == obj.from_side) {
                result = (to_side < obj.to_side) ? true : false;
            } else {
                result = false;
            }
        }

        return result;
    }
};

/* Use a map to index into the string permutation functions used to connect from one side to another */
typedef std::map<SB_Side_Connection, std::vector<std::string>> t_permutation_map;

/* Lists all information about a particular switch block specified in the architecture file */
struct t_switchblock_inf {
    std::string name;                /* the name of this switchblock */
    e_sb_location location;          /* where on the FPGA this switchblock should be built (i.e. perimeter, core, everywhere) */
    e_directionality directionality; /* the directionality of this switchblock (unidir/bidir) */

    t_permutation_map permutation_map; /* map holding the permutation functions attributed to this switchblock */

    std::vector<t_wireconn_inf> wireconns; /* list of wire types/groups this SB will connect */
};

/* Clock related data types used for building a dedicated clock network */
struct t_clock_arch_spec {
    std::vector<t_clock_network_arch> clock_networks_arch;
    std::unordered_map<std::string, t_metal_layer> clock_metal_layers;
    std::vector<t_clock_connection_arch> clock_connections_arch;
};

/*   Detailed routing architecture */
struct t_arch {
    char* architecture_id; //Secure hash digest of the architecture file to uniquely identify this architecture

    t_chan_width_dist Chans;
    enum e_switch_block_type SBType;
    std::vector<t_switchblock_inf> switchblocks;
    float R_minW_nmos;
    float R_minW_pmos;
    int Fs;
    float grid_logic_tile_area;
    std::vector<t_segment_inf> Segments;
    t_arch_switch_inf* Switches = nullptr;
    int num_switches;
    t_direct_inf* Directs = nullptr;
    int num_directs = 0;
    t_model* models = nullptr;
    t_model* model_library = nullptr;
    t_power_arch* power = nullptr;
    t_clock_arch* clocks = nullptr;

    //The name of the switch used for the input connection block (i.e. to
    //connect routing tracks to block pins).
    //This should correspond to a switch in Switches
    std::string ipin_cblock_switch_name;

    std::vector<t_grid_def> grid_layouts; //Set of potential device layouts

    t_clock_arch_spec clock_arch; // Clock related data types
};

#endif
