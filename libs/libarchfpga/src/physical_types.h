#pragma once
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

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>
#include <unordered_map>
#include <string>
#include <map>
#include <unordered_map>
#include <limits>
#include <unordered_set>

#include "vtr_ndmatrix.h"
#include "vtr_bimap.h"
#include "vtr_string_interning.h"

#include "logic_types.h"
#include "clock_types.h"
#include "switchblock_types.h"
#include "arch_types.h"

#include "vib_inf.h"

#include "scatter_gather_types.h"
#include "interposer_types.h"

//Forward declarations
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
struct t_sub_tile;
struct t_logical_block_type;
typedef const t_logical_block_type* t_logical_block_type_ptr;
struct t_logical_pin;
struct t_physical_pin;
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
struct t_interposer_cut_inf;

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
    explicit t_metadata_value(vtr::interned_string v) noexcept
        : value_(v) {}
    explicit t_metadata_value(const t_metadata_value& o) noexcept
        : value_(o.value_) {}

    // Return string value.
    vtr::interned_string as_string() const { return value_; }

    t_metadata_value& operator=(const t_metadata_value& o) noexcept {
        if (this != &o) {
            value_ = o.value_;
        }
        return *this;
    }

  private:
    vtr::interned_string value_;
};

// Metadata storage dictionary.
struct t_metadata_dict : vtr::flat_map<
                             vtr::interned_string,
                             std::vector<t_metadata_value>,
                             vtr::interned_string_less> {
    // Is this key present in the map?
    inline bool has(vtr::interned_string key) const {
        return this->count(key) >= 1;
    }

    // Get all metadata values matching key.
    //
    // Returns nullptr if key is not found.
    inline const std::vector<t_metadata_value>* get(vtr::interned_string key) const {
        auto iter = this->find(key);
        if (iter != this->end()) {
            return &iter->second;
        }
        return nullptr;
    }

    // Get metadata values matching key.
    //
    // Returns nullptr if key is not found or if multiple values are present
    // per key.
    inline const t_metadata_value* one(vtr::interned_string key) const {
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
    void add(vtr::interned_string key, vtr::interned_string value) {
        // Get the iterator to the key, which may already have elements if
        // add was called with this key in the past.
        (*this)[key].emplace_back(value);
    }
};

/*************************************************************************************************/
/* FPGA basic definitions                                                                        */
/*************************************************************************************************/

/* Pins describe I/O into clustered logic block.
 * A pin may be unconnected, driving a net or in the fanout, respectively. */
enum class e_pin_type : int8_t {
    OPEN = -1,
    DRIVER = 0,
    RECEIVER = 1
};

/* Type of interconnect within complex block: Complete for everything connected (full crossbar), direct for one-to-one connections, and mux for many-to-one connections */
enum e_interconnect {
    COMPLETE_INTERC = 1,
    DIRECT_INTERC = 2,
    MUX_INTERC = 3,
    NUM_INTERC_TYPES = 4
};
/* String version of interconnect types. Use for debugging messages */
constexpr std::array<const char*, NUM_INTERC_TYPES> INTERCONNECT_TYPE_STRING = {{"unknown", "complete", "direct", "mux"}};

/* pin location distributions */
enum class e_pin_location_distr {
    SPREAD,
    PERIMETER,
    SPREAD_INPUTS_PERIMETER_OUTPUTS,
    CUSTOM
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

/*************************************************************************************************/
/* FPGA grid layout data types                                                                   */
/*************************************************************************************************/

struct t_layer_def {
    std::vector<t_grid_loc_def> loc_defs;              ///< List of block location definitions for this layer specification
    std::vector<t_interposer_cut_inf> interposer_cuts; ///< List of interposer cuts in this layer
};

struct t_grid_def {
    e_grid_def_type grid_type = e_grid_def_type::AUTO; //The type of this grid specification

    std::string name = ""; //The name of this device

    int width = -1;  //Fixed device width (only valid for grid_type == FIXED)
    int height = -1; //Fixed device height (only valid for grid_type == FIXED)

    float aspect_ratio = 1.; //Aspect ratio for auto-sized devices (only valid for
                             //grid_type == AUTO)
    std::vector<t_layer_def> layers;
};

/************************* POWER ***********************************/

/* Architecture information for a single clock */
struct t_clock_network {
    bool autosize_buffer; /* autosize clock buffers */
    float buffer_size;    /* if not autosized, the clock buffer size */
    float C_wire;         /* Wire capacitance (per meter) */

    float prob;   /* Static probability of net assigned to this clock */
    float dens;   /* Switching density of net assigned to this clock */
    float period; /* Period of clock */

    t_clock_network() {
        autosize_buffer = false;
        buffer_size = 0.0f;
        C_wire = 0.0f;
        prob = 0.0f;
        dens = 0.0f;
        period = 0.0f;
    }
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

    t_power_arch() {
        C_wire_local = 0.0f;
        logical_effort_factor = 0.0f;
        local_interc_factor = 0.0f;
        transistors_per_SRAM_bit = 0.0f;
        mux_transistor_size = 0.0f;
        FF_size = 0.0f;
        LUT_transistor_size = 0.0f;
    }
};

/* Power usage for an entity */
struct t_power_usage {
    float dynamic;
    float leakage;
    t_power_usage() {
        dynamic = 0.0f;
        leakage = 0.0f;
    }
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
    std::vector<int> pinlist; /* [0..num_pins - 1] */
};

/* Struct to hold the class ranges for specific sub tiles */
struct t_class_range {
    int low = 0;
    int high = 0;
    // Returns the total number of classes
    int total_num() const {
        return high - low + 1;
    }

    t_class_range() = default;

    t_class_range(int low_class_num, int high_class_num)
        : low(low_class_num)
        , high(high_class_num) {}
};

// Struct to hold the pin ranges for a specific sub block
struct t_pin_range {
    int low = 0;
    int high = 0;
    // Returns the total number of pins
    int total_num() const {
        return high - low + 1;
    }

    t_pin_range() = default;

    t_pin_range(int low_class_num, int high_class_num)
        : low(low_class_num)
        , high(high_class_num) {}
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

    t_port_power() {
        wire_type = (e_power_wire_type)0;
        wire = {0.0f}; // Default to C = 0.0f
        buffer_type = (e_power_buffer_type)0;
        buffer_size = 0.0f;
        pin_toggle_initialized = false;
        energy_per_toggle = 0.0f;
        scaled_by_port = nullptr;
        scaled_by_port_pin_idx = 0;
        reverse_scaled = false;
    }
};

/**
 * @enum e_fc_type
 * @brief The type of Fc specification
 */
enum class e_fc_type {
    IN, /**< Fc specification for an input pin. */
    OUT /**< Fc specification for an output pin. */
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
 * pin_layer_offset/pin_width_offset/pin_height_offset: offset from the anchor point
 * of the block type in the x,y, and layer (dice number) direction.
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
 *
 * In general, the physical tile is a placeable physical resource on the FPGA device,
 * and it is allowed to contain a heterogeneous set of logical blocks (pb_types).
 *
 * Each physical tile must specify at least one sub tile, that is a physical location
 * on the sub tiles stacks. This means that a physical tile occupies an (x, y) location on the grid,
 * and it has at least one sub tile slot that allows for a placement within the (x, y) location.
 *
 * Therefore, to identify the location of a logical block within the device grid, we need to
 * specify four different coordinates:
 *      - x         : horizontal coordinate
 *      - y         : vertical coordinate
 *      - sub tile  : location within the sub tile stack at an (x, y) physical location
 *      -layer_num  : the layer that block is located at. In case of a single die, layer_num is 0.
 *
 * A physical tile is heterogeneous as it allows the placement of different kinds of logical blocks within,
 * that can share the same (x, y) placement location.
 *
 */
struct t_physical_tile_type {
    std::string name;
    int num_pins = 0;
    int num_inst_pins = 0;
    int num_input_pins = 0;
    int num_output_pins = 0;
    int num_clock_pins = 0;

    std::vector<int> clock_pin_indices;

    int capacity = 0;

    int width = 0;
    int height = 0;

    vtr::NdMatrix<std::vector<bool>, 3> pinloc; /* [0..width-1][0..height-1][0..3][0..num_pins-1] */

    std::vector<t_class> class_inf; /* [0..num_class-1] */

    // Primitive class is referred to a classes that are in the primitive blocks. These classes are
    // used during flat-routing to route the nets.
    // The starting number of primitive classes
    int primitive_class_starting_idx = -1;
    std::unordered_map<int, t_class> primitive_class_inf; // [primitive_class_num] -> primitive_class_inf

    std::vector<int> pin_layer_offset;                // [0..num_pins-1]
    std::vector<int> pin_width_offset;                // [0..num_pins-1]
    std::vector<int> pin_height_offset;               // [0..num_pins-1]
    std::vector<int> pin_class;                       // [0..num_pins-1]
    std::unordered_map<int, int> primitive_pin_class; // [primitive_pin_num] -> primitive_class_num
    std::vector<bool> is_ignored_pin;                 // [0..num_pins-1]
    std::vector<bool> is_pin_global;                  // [0..num_pins-1]

    std::unordered_map<int, std::unordered_map<t_logical_block_type_ptr, t_pb_graph_pin*>> on_tile_pin_num_to_pb_pin; // [root_pin_physical_num][logical_block] -> t_pb_graph_pin*
    std::unordered_map<int, t_pb_graph_pin*> pin_num_to_pb_pin;                                                       // [intra_tile_pin_physical_num] -> t_pb_graph_pin
    std::unordered_map<const t_pb_graph_pin*, int> pb_pin_to_pin_num;                                                 // [t_pb_graph_pin*] -> intra_tile_pin_physical_num

    std::vector<t_fc_specification> fc_specs;

    vtr::Matrix<e_sb_type> switchblock_locations;
    vtr::Matrix<int> switchblock_switch_overrides;

    float area = 0;

    /* This info can be determined from class_inf and pin_class but stored for faster access */
    int num_drivers = 0;
    int num_receivers = 0;

    int index = -1; /* index of type descriptor in array (allows for index referencing) */

    // vector of the different types of sub tiles allowed for the physical tile.
    std::vector<t_sub_tile> sub_tiles;

    /* Unordered map indexed by the logical block index.
     * tile_block_pin_directs_map[logical block index][logical block pin] -> physical tile pin */
    std::unordered_map<int, std::unordered_map<int, vtr::bimap<t_logical_pin, t_physical_pin>>> tile_block_pin_directs_map;

    // TODO: Remove is_input_type / is_output_type as part of
    // https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/1193

    // Does this t_physical_tile_type contain an inpad?
    bool is_input_type = false;

    // Does this t_physical_tile_type contain an outpad?
    bool is_output_type = false;

  public: // Function members
    ///@brief Returns the indices of pins that contain a clock for this physical logic block
    std::vector<int> get_clock_pins_indices() const;

    ///@brief Returns the sub tile location of the physical tile given an input pin
    int get_sub_tile_loc_from_pin(int pin_num) const;

    ///@brief Is this t_physical_tile_type an empty type?
    bool is_empty() const;

    ///@brief Returns true if the physical tile type can implement either a .input or .output block type
    inline bool is_io() const {
        return is_input_type || is_output_type;
    }

    ///@brief Returns the relative pin index within a sub tile that corresponds to the pin within the given port and its index in the port
    int find_pin(std::string_view port_name, int pin_index_in_port) const;

    ///@brief Returns the pin class associated with the specified pin_index_in_port within the port port_name on type
    int find_pin_class(std::string_view port_name, int pin_index_in_port, e_pin_type pin_type) const;
};

/* Holds the capacity range of a certain sub_tile block within the parent physical tile type.
 * E.g. TILE_X has the following sub tiles:
 *          - SUB_TILE_A: capacity_range --> 0 to 4
 *          - SUB_TILE_B: capacity_range --> 5 to 11
 *          - SUB_TILE_C: capacity_range --> 12 to 16
 *
 * Total TILE_X capacity is 17
 */
struct t_capacity_range {
    int low = 0;
    int high = 0;

    void set(int low_cap, int high_cap) {
        low = low_cap;
        high = high_cap;
    }

    bool is_in_range(int cap) const {
        return cap >= low and cap <= high;
    }

    int total() const {
        return high - low + 1;
    }
};

/**
 * @brief Describes the possible placeable blocks within a physical tile type.
 *
 * Heterogeneous blocks:
 *
 * The sub tile allows to have heterogeneous blocks placed at the same grid location.
 * Heterogeneous blocks are blocks which do not share either the same functionality or the
 * IO interface, but do share the same (x, y) grid location.
 * For each heterogeneous block type than, there should be a corresponding sub tile to enable
 * its placement within the physical tile.
 *
 * For further information there is a tutorial on the VTR documentation page.
 *
 *
 * Equivalent sites:
 *
 * Moreover, the same sub tile enables to allow the placement of different implementations
 * of a logical block.
 * This means that two blocks that have different internal functionalities, but the IO interface of one block
 * is a subset of the other, they can be placed at the same sub tile location within the physical tile.
 * These two blocks can be identified as equivalent, hence they can belong to the same sub tile.
 */
struct t_sub_tile {
    std::string name;

    // Mapping between the subtile's pins and the physical pins corresponding
    // to the physical tile type.
    std::vector<int> sub_tile_to_tile_pin_indices;

    std::vector<t_physical_tile_port> ports;

    std::vector<t_logical_block_type_ptr> equivalent_sites; ///>List of netlist blocks (t_logical_block) that one could
                                                            ///>place within this sub tile.

    t_capacity_range capacity; ///>Indicates the total number of sub tile instances of this type placeable at a
                               ///>physical location.
                               ///>E.g.: capacity can range from 4 to 7, meaning that there are four placeable sub tiles
                               ///>      at a physical location, and compatible netlist blocks can be placed at sub_tile
                               ///>      indices ranging from 4 to 7.
    t_class_range class_range; // Range of the root-level classes

    std::vector<std::unordered_map<t_logical_block_type_ptr, t_class_range>> primitive_class_range; // [rel_cap][logical_block_ptr] -> class_range
    std::vector<std::unordered_map<t_logical_block_type_ptr, t_pin_range>> intra_pin_range;         // [rel_cap][logical_block_ptr] -> intra_pin_range

    int num_phy_pins = 0;

    int index = -1;

  public:
    int total_num_internal_pins() const;

    /**
     * @brief Returns the physical tile port given the port name and the corresponding sub tile
     */
    const t_physical_tile_port* get_port(std::string_view port_name);

    /**
     * @brief Returns the physical tile port given the pin name and the corresponding sub tile
     */
    const t_physical_tile_port* get_port_by_pin(int pin) const;
};

/** A logical pin defines the pin index of a logical block type (i.e. a top level PB type)
 *  This structure wraps the int value of the logical pin to allow its storage in the
 *  vtr::bimap container.
 */
struct t_logical_pin {
    int pin = -1;

    t_logical_pin(int value) {
        pin = value;
    }

    bool operator==(const t_logical_pin o) const {
        return pin == o.pin;
    }

    bool operator<(const t_logical_pin o) const {
        return pin < o.pin;
    }
};

/** A physical pin defines the pin index of a physical tile type (i.e. a grid tile type)
 *  This structure wraps the int value of the physical pin to allow its storage in the
 *  vtr::bimap container.
 */
struct t_physical_pin {
    int pin = -1;

    t_physical_pin(int value) {
        pin = value;
    }

    bool operator==(const t_physical_pin o) const {
        return pin == o.pin;
    }

    bool operator<(const t_physical_pin o) const {
        return pin < o.pin;
    }
};

/**
 * @brief Describes The location of a physical tile
 * @param x The x location of the physical tile on the given die
 * @param y The y location of the physical tile on the given die
 * @param layer_num The die number of the physical tile. If the FPGA only has one die, or the physical tile is located
 *                  on the base die, layer_num is equal to zero. If the physical tile is location on the die immediately
 *                  above the base die, the layer_num is 1 and so on.
 */
struct t_physical_tile_loc {
    int x = ARCH_FPGA_UNDEFINED_VAL;
    int y = ARCH_FPGA_UNDEFINED_VAL;
    int layer_num = ARCH_FPGA_UNDEFINED_VAL;

    t_physical_tile_loc() = default;

    t_physical_tile_loc(int x_val, int y_val, int layer_num_val)
        : x(x_val)
        , y(y_val)
        , layer_num(layer_num_val) {}

    // Returns true if this type location layer_num/x/y is not equal to ARCH_FPGA_UNDEFINED_VAL
    operator bool() const {
        return !(x == ARCH_FPGA_UNDEFINED_VAL || y == ARCH_FPGA_UNDEFINED_VAL || layer_num == ARCH_FPGA_UNDEFINED_VAL);
    }

    /**
     * @brief Comparison operator for t_physical_tile_loc
     *
     * Tiles are ordered first by layer number, then by x, and finally by y.
     */
    friend bool operator<(const t_physical_tile_loc& lhs, const t_physical_tile_loc& rhs) {
        if (lhs.layer_num != rhs.layer_num)
            return lhs.layer_num < rhs.layer_num;
        if (lhs.x != rhs.x)
            return lhs.x < rhs.x;
        return lhs.y < rhs.y;
    }

    friend bool operator==(const t_physical_tile_loc& a, const t_physical_tile_loc& b) {
        return a.x == b.x && a.y == b.y && a.layer_num == b.layer_num;
    }

    friend bool operator!=(const t_physical_tile_loc& a, const t_physical_tile_loc& b) {
        return !(a == b);
    }
};

namespace std {
template<>
struct hash<t_physical_tile_loc> {
    std::size_t operator()(const t_physical_tile_loc& v) const noexcept {
        std::size_t seed = std::hash<int>{}(v.x);
        vtr::hash_combine(seed, v.y);
        vtr::hash_combine(seed, v.layer_num);
        return seed;
    }
};
} // namespace std

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
    PortEquivalence equivalent;

    int index;
    int absolute_first_pin_index;
    int port_index_by_type;

    t_physical_tile_port() {
        is_clock = false;
        is_non_clock_global = false;

        num_pins = 1;
        equivalent = PortEquivalence::NONE;
    }
};

/* Describes the type for a logical block
 * name: unique identifier for type
 * pb_type: Internal subblocks and routing information for this physical block
 * pb_graph_head: Head of DAG of pb_types_nodes and their edges
 *
 * index: Keep track of type in array for easy access
 * physical_tile_index: index of the corresponding physical tile type
 *
 * pin_logical_num_to_pb_pin_mapping: Contains all the pins, including pins on the root-level block and internal pins, in
 * the logical block. The key of this map is the logical number of the pin, and the value is a pointer to the
 * corresponding pb_graph_pin
 *
 * primitive_pb_pin_to_logical_class_num_mapping: Maps each pin to its corresponding class's logical number. To retrieve the actual class, use this number as an
 * index to logical_class_inf.
 *
 * logical_class_inf: Contains all the classes inside the logical block. The index of each class is the logical number associate with the class.
 *
 * A logical block is the implementation of a component's functionality of the FPGA device
 * and it identifies its logical behaviour and internal connections.
 *
 * The logical block type is mainly used during the packing stage of VPR and is used to generate
 * the packed netlist and all the corresponding blocks and their internal structure.
 *
 * The logical blocks than get assigned to a possible physical tile for the placement step.
 *
 * A logical block must correspond to at least one physical tile.
 */
struct t_logical_block_type {
    std::string name;

    /* Clustering info */
    t_pb_type* pb_type = nullptr;
    t_pb_graph_node* pb_graph_head = nullptr;

    int index = -1; /* index of type descriptor in array (allows for index referencing) */

    std::vector<t_physical_tile_type_ptr> equivalent_tiles; ///>List of physical tiles at which one could
                                                            ///>place this type of netlist block.

    std::unordered_map<int, t_pb_graph_pin*> pin_logical_num_to_pb_pin_mapping;                    /* pin_logical_num_to_pb_pin_mapping[pin logical number] -> pb_graph_pin ptr} */
    std::unordered_map<const t_pb_graph_pin*, int> primitive_pb_pin_to_logical_class_num_mapping;  /* primitive_pb_pin_to_logical_class_num_mapping[pb_graph_pin ptr] -> class logical number */
    std::vector<t_class> primitive_logical_class_inf;                                              /* primitive_logical_class_inf[class_logical_number] -> class */
    std::unordered_map<const t_pb_graph_node*, t_class_range> primitive_pb_graph_node_class_range; /* primitive_pb_graph_node_class_range[primitive_pb_graph_node ptr] -> class range for that primitive*/

    // Is this t_logical_block_type empty?
    bool is_empty() const;

    // Returns true if this logical block type is an IO block
    bool is_io() const;

  public:
    /**
     * @brief Returns the logical block port given the port name and the corresponding logical block type
     */
    const t_port* get_port(std::string_view port_name) const;

    /**
     * @brief Returns the logical block port given the pin name and the corresponding logical block type
     */
    const t_port* get_port_by_pin(int pin) const;
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
    LogicalModelId model_id;
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

    std::vector<t_pin_to_pin_annotation> annotations;

    int index_in_logical_block = 0; /* assign a unique id to each pb_type in a logical block */

    /* Power related members */
    t_pb_type_power* pb_type_power = nullptr;

    t_metadata_dict meta;

    /**
     * @brief Check if t_pb_type is the root of the pb graph. Root pb_types correspond to a single top level block type and map to a particular type
     * of location in the FPGA device grid (e.g. Logic, DSP, RAM etc.)
     *
     * @return if t_pb_type is root ot not
     */
    inline bool is_root() const {
        return parent_mode == nullptr;
    }

    /**
     * @brief Check if t_pb_type is a primitive block or equivalently a leaf of the pb graph.
     *
     * @return if t_pb_type is primitive/leaf ot not
     */
    inline bool is_primitive() const {
        return num_modes == 0;
    }

    int get_max_primitives() const;
    int get_max_depth() const;
    int get_max_nets() const;
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
 *      disable_packing: Specify if the mode is disabled/enabled for VPR packer.
 *                       By default, every mode is enabled for VPR packer.
 *                       Users can disable it for VPR packer through arch XML
 *                       When flag is set true, the mode is invisible to VPR packer.
 *                       No logic will be mapped to the pb_type under the mode
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

    /* Packer-related switches */
    bool disable_packing = false;

    /* Power related members */
    t_mode_power* mode_power = nullptr;

    t_metadata_dict meta;
};

/** Info placed between pins in the architecture file (e.g. delay annotations),
 *
 * This is later for additional information.
 *
 * Data Members:
 *      annotation_entries: pairs of annotation subtypes and the annotation values
 *      type: type of annotation
 *      format: formatting of data
 *      input_pins: input pins as string affected by annotation
 *      output_pins: output pins as string affected by annotation
 *      clock_pin: clock as string affected by annotation
 */
struct t_pin_to_pin_annotation {

    std::vector<std::pair<int, std::string>> annotation_entries;

    enum e_pin_to_pin_annotation_type type;
    enum e_pin_to_pin_annotation_format format;

    char* input_pins;
    char* output_pins;
    char* clock;

    int line_num; /* used to report what line number this annotation is found in architecture file */

    t_pin_to_pin_annotation() noexcept {
        annotation_entries = std::vector<std::pair<int, std::string>>();
        input_pins = nullptr;
        output_pins = nullptr;
        clock = nullptr;

        line_num = 0;
        type = (e_pin_to_pin_annotation_type)0;
        format = (e_pin_to_pin_annotation_format)0;
    }
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
 *      infer_annotations: This interconnect is autogenerated, if true, infer pack_patterns
 *                         such as carry-chains and forced packs based on interconnect linked to it
 *      parent_mode_index: Mode of parent as int
 */
struct t_interconnect {
    enum e_interconnect type;
    char* name;

    char* input_string;
    char* output_string;

    std::vector<t_pin_to_pin_annotation> annotations;
    bool infer_annotations;

    int line_num; /* Interconnect is processed later, need to know what line number it messed up on to give proper error message */

    int parent_mode_index;

    /* Power related members */
    t_mode* parent_mode;

    t_interconnect_power* interconnect_power;
    t_metadata_dict meta;

    t_interconnect() {
        type = (e_interconnect)0;
        name = nullptr;
        input_string = nullptr;
        output_string = nullptr;
        infer_annotations = false;
        line_num = 0;
        parent_mode_index = 0;
        parent_mode = nullptr;
        interconnect_power = nullptr;
        meta = t_metadata_dict();
    }
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

    t_port() {
        name = nullptr;
        model_port = nullptr;
        type = (PORTS)0;
        is_clock = false;
        is_non_clock_global = false;
        num_pins = 0;
        equivalent = (PortEquivalence)0;
        parent_pb_type = nullptr;
        port_class = nullptr;
        index = 0;
        port_index_by_type = 0;
        absolute_first_pin_index = 0;
        port_power = nullptr;
    }
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

    t_interconnect_power() {
        power_usage = t_power_usage();
        port_info_initialized = false;
        num_input_ports = 0;
        num_output_ports = 0;
        num_pins_per_port = 0;
        transistor_cnt = 0.0f;
    }
};

struct t_interconnect_pins {
    t_interconnect* interconnect;

    t_pb_graph_pin*** input_pins;  // [0..num_input_ports-1][0..num_pins_per_port-1]
    t_pb_graph_pin*** output_pins; // [0..num_output_ports-1][0..num_pins_per_port-1]
};

struct t_mode_power {
    t_power_usage power_usage; /* Power usage of this mode */
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
 *                              This member is only used by nodes corresponding to primitive sites.
 *      flat_site_index       : Index of this primitive site within its primitive type within this cluster type.
 *                              Values are in [0...total_primitive_count-1], e.g. if there are 10 ALMs per cluster, 2 FFS
 *                              and 2 LUTs per ALM, then flat site indices for FFs would run from 0 to 19, and flat site
 *                              indices for LUTs would run from 0 to 19. This member is only used by nodes corresponding
 *                              to primitive sites. It is used when reconstructing clusters from a flat placement file.
 *      illegal_modes         : vector containing illegal modes that result in conflicts during routing
 */
class t_pb_graph_node {
  public:
    t_pb_type* pb_type;

    int placement_index;

    /*
     * There is a root-level pb_graph_node assigned to each logical type. Each logical type can contain multiple primitives.
     * If this pb_graph_node is associated with a primitive, a unique number is assigned to it within the logical block level.
     */
    int primitive_num = ARCH_FPGA_UNDEFINED_VAL;

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

    t_pin_range pin_num_range;

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

    void* temp_scratch_pad; /* temporary data, useful for keeping track of things when traversing data structure */

    int* input_pin_class_size;  /* Stores the number of pins that belong to a particular input pin class */
    int num_input_pin_class;    /* number of input pin classes that this pb_graph_node has */
    int* output_pin_class_size; /* Stores the number of pins that belong to a particular output pin class */
    int num_output_pin_class;   /* number of output pin classes that this pb_graph_node has */

    int total_primitive_count; /* total number of this primitive type in the cluster */
    int flat_site_index;       /* index of this primitive within sites of its type in this cluster  */

    /* Interconnect instances for this pb
     * Only used for power
     */
    t_pb_graph_node_power* pb_node_power;
    t_interconnect_pins** interconnect_pins; /* [0..num_modes-1][0..num_interconnect_in_mode] */

    // Returns true if this pb_graph_node represents a primitive type (primitives have 0 modes)
    bool is_primitive() const { return this->pb_type->is_primitive(); }

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
    std::vector<t_pb_graph_edge*> input_edges; /* [0..num_input_edges] */
    int num_input_edges = 0;
    // This map is initialized only if flat_routing is enabled
    std::unordered_map<const t_pb_graph_pin*, int> sink_pin_edge_idx_map; /* [t_pb_graph_pin*] -> edge_idx - This is the index of the corresponding edge stored in output_edges vector */

    std::vector<t_pb_graph_edge*> output_edges; /* [0..num_output_edges] */
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
    t_pb_graph_pin* associated_clock_pin = nullptr;          /* For sequential elements, the associated clock */

    /* This member is used when flat-routing and router_opt_choke_points are enabled.
     * It is used to identify choke points.
     * This is only valid for IPINs, and it only contains the pins that are reachable to the pin by a forwarding path.
     * It doesn't take into account feed-back connection.
     * */
    std::unordered_set<int> connected_sinks_ptc; /* ptc numbers of sinks which are directly or indirectly connected to this pin */

    /* combinational timing information */
    int num_pin_timing = 0;                   /* Number of ipin to opin timing edges*/
    std::vector<t_pb_graph_pin*> pin_timing;  /* timing edge sink pins  [0..num_pin_timing-1]*/
    std::vector<float> pin_timing_del_max;    /* primitive ipin to opin max-delay [0..num_pin_timing-1]*/
    std::vector<float> pin_timing_del_min;    /* primitive ipin to opin min-delay [0..num_pin_timing-1]*/
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
    std::vector<const char*> pack_pattern_names;
    int* pack_pattern_indices;
    bool infer_pattern;

    int switch_type_idx = ARCH_FPGA_UNDEFINED_VAL; /* architecture switch id of the edge - used when flat_routing is enabled */

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

/**
 * @brief An attribute of a segment that defines the general category of the wire segment type.
 *
 * @details
 * - `GCLK`: A segment type that is part of the global routing network for clocks.
 * - `GENERAL`: Describes a segment type that is part of the regular routing network.
 */
enum class SegResType {
    GCLK = 0,
    GENERAL = 1,
    NUM_RES_TYPES
};

/// String versions of segment resource types
constexpr std::array<const char*, static_cast<size_t>(SegResType::NUM_RES_TYPES)> RES_TYPE_STRING{"GCLK", "GENERAL"};

/// Defines the type of switch block used in FPGA routing.
enum e_switch_block_type {
    /// If the type is SUBSET, I use a Xilinx-like switch block where track i in one channel always
    /// connects to track i in other channels.
    SUBSET,

    /// If type is WILTON, I use a switch block where track i
    /// does not always connect to track i in other channels.
    /// See Steve Wilton, PhD Thesis, University of Toronto, 1996.
    WILTON,

    /// The UNIVERSAL switch block is from Y. W. Chang et al, TODAES, Jan. 1996, pp. 80 - 101.
    UNIVERSAL,

    /// The FULL switch block type allows for complete connectivity between tracks.
    FULL,

    /// A CUSTOM switch block has also been added which allows a user to describe custom permutation functions and connection patterns.
    /// See comment at top of SRC/route/build_switchblocks.c
    CUSTOM
};

enum e_Fc_type {
    ABSOLUTE,
    FRACTIONAL
};

/**
 * @brief Lists all the important information about a certain segment type.  Only
 * used if the route_type is DETAILED.  [0 .. det_routing_arch.num_segment]
 */
struct t_segment_inf {
    /// The name of the segment type
    std::string name;

    /// brief ratio of tracks which are of this segment type.
    int frequency;

    /// Length (in clbs) of the segment.
    int length;

    /**
     * @brief Index of the switch type that connects other wires to this segment.
     * Note that this index is in relation to the switches from the architecture file,
     * not the expanded list of switches that is built at the end of build_rr_graph.
     */
    short arch_wire_switch;

    /**
     * @brief Index of the switch type that connects output pins to this segment.
     * Note that this index is in relation to the switches from the architecture file,
     * not the expanded list of switches that is built at the end of build_rr_graph.
     */
    short arch_opin_switch;

    /**
     * @brief Same as arch_wire_switch but used only for decremental tracks if it is
     * specified in the architecture file. If -1, this value was not set in the
     * architecture file and arch_wire_switch should be used for "DEC_DIR" wire segments.
     */
    short arch_wire_switch_dec = -1;

    /**
     * @brief Same as arch_opin_switch but used only for decremental tracks if
     * it is specified in the architecture file. If -1, this value was not set in
     * the architecture file and arch_opin_switch should be used for "DEC_DIR" wire segments.
     */
    short arch_opin_switch_dec = -1;

    /**
     * @brief Index of the switch type that connects output pins (OPINs) to this
     * segment from another die (layer). Note that this index is in relation to
     * the switches from the architecture file, not the expanded list of switches
     * that is built at the end of build_rr_graph.
     */
    short arch_inter_die_switch = -1;

    /**
     * @brief The fraction of logic blocks along its length to which this segment can connect.
     * (i.e. internal population).
     */
    float frac_cb;

    /**
     * @brief The fraction of the length + 1 switch blocks along the segment to which the segment can connect.
     * Segments that aren't long lines must connect to at least two switch boxes.
     */
    float frac_sb;

    bool longline;

    /// The resistance of a routing track, per unit logic block length.
    float Rmetal;

    ///  The capacitance of a routing track, per unit logic block length.
    float Cmetal;

    enum e_directionality directionality;

    /**
     * @brief Defines what axis the segment is parallel to. See e_parallel_axis
     * comments for more details on the values.
     */
    e_parallel_axis parallel_axis;

    /// A vector of booleans indicating whether the segment can connect to a logic block.
    std::vector<bool> cb;

    /// A vector of booleans indicating whether the segment can connect to a switch block.
    std::vector<bool> sb;

    /**
     *  @brief This segment is bend or not
     */
    bool is_bend;

    /**
     *  @brief The bend type of the segment, "-"-0, "U"-1, "D"-2
     *  For example: bend pattern <- - U ->; corresponding bend: [0,0,1,0]
     */
    std::vector<int> bend;

    /**
     *  @brief Divide the segment into several parts based on bend position.
     *  For example: length-5 bend segment: <- - U ->;
     *  Corresponding part_len: [3,2]
     */
    std::vector<int> part_len;

    /**
     *  @brief The index of the segment as stored in the appropriate Segs list.
     * Upon loading the architecture, we use this field to keep track of the
     * segment's index in the unified segment_inf vector. This is useful when
     * building the rr_graph for different Y & X channels in terms of track
     * distribution and segment type.
     */
    int seg_index;

    /**
     *  @brief Determines the routing network to which the segment belongs.
     *  Possible values are:
     *   - GENERAL: The segment is part of the general routing resources.
     *   - GCLK: The segment is part of the global routing network.
     * For backward compatibility, this attribute is optional. If not specified,
     * the resource type for the segment is considered to be GENERAL.
     */
    enum SegResType res_type = SegResType::GENERAL;
};

inline bool operator==(const t_segment_inf& a, const t_segment_inf& b) {
    return a.name == b.name
           && a.frequency == b.frequency
           && a.length == b.length
           && a.arch_wire_switch == b.arch_wire_switch
           && a.arch_opin_switch == b.arch_opin_switch
           && a.arch_inter_die_switch == b.arch_inter_die_switch
           && a.frac_cb == b.frac_cb
           && a.frac_sb == b.frac_sb
           && a.longline == b.longline
           && a.Rmetal == b.Rmetal
           && a.Cmetal == b.Cmetal
           && a.directionality == b.directionality
           && a.parallel_axis == b.parallel_axis
           && a.cb == b.cb
           && a.sb == b.sb;
}

/*provide hashing for t_segment_inf to enable the use of many std containers.
 * Only the most important/varying fields are used (not worth the extra overhead to include all fields)*/

struct t_hash_segment_inf {
    size_t operator()(const t_segment_inf& seg_inf) const noexcept {
        size_t result;
        result = ((((std::hash<std::string>()(seg_inf.name)
                     ^ std::hash<int>()(seg_inf.frequency) << 10)
                    ^ std::hash<int>()(seg_inf.length) << 20)
                   ^ std::hash<int>()((int)seg_inf.arch_opin_switch) << 30));
        return result;
    }
};
enum class SwitchType {
    MUX = 0,   //A configurable (buffered) mux (single-driver)
    TRISTATE,  //A configurable tristate-able buffer (multi-driver)
    PASS_GATE, //A configurable pass transistor switch (multi-driver)
    SHORT,     //A non-configurable electrically shorted connection (multi-driver)
    BUFFER,    //A non-configurable non-tristate-able buffer (uni-driver)
    INVALID,   //Unspecified, usually an error
    NUM_SWITCH_TYPES
};
constexpr std::array<const char*, size_t(SwitchType::NUM_SWITCH_TYPES)> SWITCH_TYPE_STRINGS = {{"MUX", "TRISTATE", "PASS_GATE", "SHORT", "BUFFER", "INVALID"}};

/* Constant/Reserved names for switches in architecture XML
 * Delayless switch:
 *   The zero-delay switch created by VPR internally
 *   This is a special switch just to ease CAD algorithms
 *   It is mainly used in
 *     - the edges between SOURCE and SINK nodes in routing resource graphs
 *     - the edges in CLB-to-CLB connections (defined by <directlist> in arch XML)
 *
 */
constexpr const char* VPR_DELAYLESS_SWITCH_NAME = "__vpr_delayless_switch__";

/* An intracluster switch automatically added to the RRG by the flat router. */
constexpr const char* VPR_INTERNAL_SWITCH_NAME = "__vpr_intra_cluster_switch__";

enum class BufferSize {
    AUTO,
    ABSOLUTE
};

/**
 * @struct t_arch_switch_inf
 * @brief Lists all the important information about a switch type read from the architecture file.
 */
struct t_arch_switch_inf {
  public:
    static constexpr int UNDEFINED_FANIN = -1;

    std::string name;
    /// Equivalent resistance of the buffer/switch
    float R = 0.;
    /// Input capacitance
    float Cin = 0.;
    /// Output capacitance
    float Cout = 0.;
    /**
     * @brief   The internal capacitance.
     * @details Since multiplexers and tristate buffers are modeled as a
     *          parallel stream of pass transistors feeding into a buffer,
     *          we would expect an additional "internal capacitance
     *          to arise when the pass transistor is enabled and the signal
     *          must propagate to the buffer. See diagram of one stream below:
     *
     *              Pass Transistor
     *                   |
     *                 -----
     *                 -----      Buffer
     *                |     |       |\
     *          ------       -------| \ -----
     *            |             |   | /    |
     *          =====         ===== |/   =====
     *          =====         =====      =====
     *            |             |          |
     *          Input C    Internal C    Output C
     */
    float Cinternal = 0.;
    /// The area of each transistor in the segment's driving mux measured in minimum width transistor units
    float mux_trans_size = 1.;
    BufferSize buf_size_type = BufferSize::AUTO;
    /// The area of the buffer. If set to zero, area should be calculated from R
    float buf_size = 0.;
    e_power_buffer_type power_buffer_type = POWER_BUFFER_TYPE_AUTO;
    float power_buffer_size = 0.;

    bool intra_tile = false;

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

    /**
     * @brief   Maps the number of inputs to a delay.
     * @details A map where the key is the number of inputs and the entry
     *          is the corresponding delay. If there is only one entry at key
     *          UNDEFINED, then delay is a constant (doesn't vary with fan-in).
     *          A map saves us the trouble of sorting, and has lower access
     *          time for interpolation/extrapolation purposes
     */
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
 *            calculated from R
 * intra_tile: Indicate whether this rr_switch is a switch type used inside  *
 *             clusters. These switch types are not specified in the         *
 *             architecture description file and are added when flat router  *
 *             is enabled                                                    */
struct t_rr_switch_inf {
    float R = 0.;
    float Cin = 0.;
    float Cout = 0.;
    float Cinternal = 0.;
    float Tdel = 0.;
    float mux_trans_size = 0.;
    float buf_size = 0.;
    std::string name;
    e_power_buffer_type power_buffer_type = POWER_BUFFER_TYPE_UNDEFINED;
    float power_buffer_size = 0.;

    bool intra_tile = false;

  public:
    /// Returns the type of switch
    SwitchType type() const;

    /// Returns true if this switch type isolates its input and output into
    /// separate DC-connected subcircuits
    bool buffered() const;

    /// Returns true if this switch type is configurable
    bool configurable() const;

    bool operator==(const t_rr_switch_inf& other) const;

    /**
     * @brief Functor for computing a hash value for t_rr_switch_inf.
     *
     * This custom hasher enables the use of t_rr_switch_inf objects as keys
     * in unordered containers such as std::unordered_map or std::unordered_set.
     */
    struct Hasher {
        std::size_t operator()(const t_rr_switch_inf& s) const;
    };

  public:
    void set_type(SwitchType type_val);

  private:
    SwitchType type_ = SwitchType::INVALID;
};

/**
 * @struct t_direct_inf
 * @brief Lists all the important information about a direct chain connection.
 */
struct t_direct_inf {
    /// Name of this direct chain connection
    std::string name;
    /// The type of the pin that drives this chain connection in the format of <block_name>.<pin_name>
    std::string from_pin;
    /// The type of pin that is driven by this chain connection in the format of <block_name>.<pin_name>
    std::string to_pin;
    /// The x offset from the source to the sink of this connection
    int x_offset;
    /// The y offset from the source to the sink of this connection
    int y_offset;
    /// The subtile offset from the source to the sink of this connection
    int sub_tile_offset;
    /// The index into the switch list for the switch used by this direct
    int switch_type;
    /// The associated from_pin’s block side
    e_side from_side;
    /// The associated to_pin’s block side
    e_side to_side;
    /// The line number in the architecture file that specifies this particular placement macro.
    int line;
};

/* Clock related data types used for building a dedicated clock network */
struct t_clock_arch_spec {
    std::vector<t_clock_network_arch> clock_networks_arch;
    std::unordered_map<std::string, t_metal_layer> clock_metal_layers;
    std::vector<t_clock_connection_arch> clock_connections_arch;
};

struct t_lut_cell {
    std::string name;
    std::string init_param;
    std::vector<std::string> inputs;
};

struct t_lut_bel {
    std::string name;

    std::vector<std::string> input_pins;
    std::string output_pin;

    bool operator==(const t_lut_bel& other) const {
        return name == other.name && input_pins == other.input_pins && output_pin == other.output_pin;
    }
};

struct t_lut_element {
    std::string site_type;
    int width;
    std::vector<t_lut_bel> lut_bels;

    bool operator==(const t_lut_element& other) const {
        return site_type == other.site_type && width == other.width && lut_bels == other.lut_bels;
    }
};

/**
 * Represents a Network-on-chip(NoC) Router data type. It is used
 * to store individual router information when parsing the arch file.
 * */
struct t_router {
    /** A unique id provided by the user to identify a router. Must be a positive value*/
    int id = -1;

    /** A value representing the approximate horizontal position on the FPGA device where the router
     * tile is located*/
    float device_x_position = -1;
    /** A value representing the approximate vertical position on the FPGA device where the router
     * tile is located*/
    float device_y_position = -1;
    /** A value representing the exact layer in the FPGA device where the router tile is located.*/
    int device_layer_position = -1;

    /** A list of router ids that are connected to the current router*/
    std::vector<int> connection_list;
};

/**
 * Network-on-chip(NoC) data type used to store the network properties
 * when parsing the arh file. This is used when building the dedicated on-chip
 * network during the device creation.
 * */
struct t_noc_inf {
    double link_bandwidth; /*!< The maximum bandwidth supported in the NoC. This value is the same for all links. units in bps*/
    double link_latency;   /*!< The worst case latency seen when traversing a link. This value is the same for all links. units in seconds*/
    double router_latency; /*!< The worst case latency seen when traversing a router. This value is the same for all routers, units in seconds*/

    /** A list of all routers in the NoC*/
    std::vector<t_router> router_list;

    /** Stores NoC routers that have a different latency than the NoC-wide router latency.
     * (router_user_id, overridden router latency)*/
    std::map<int, double> router_latency_overrides;
    /** Stores NoC links that have a different latency than the NoC-wide link latency.
     * ((source router id, destination router id), overridden link latency)*/
    std::map<std::pair<int, int>, double> link_latency_overrides;
    /** Stores NoC links that have a different bandwidth than the NoC-wide link bandwidth.
     * ((source router id, destination router id), overridden link bandwidth)*/
    std::map<std::pair<int, int>, double> link_bandwidth_overrides;

    /** Represents the name of a router tile on the FPGA device. This should match the name used in the arch file when
     * describing a NoC router tile within the FPGA device*/
    std::string noc_router_tile_name;
};

// Detailed routing architecture
struct t_arch {
    /// Stores unique strings used as key and values in <metadata> tags,
    /// i.e. implements a flyweight pattern to save memory.
    mutable vtr::string_internment strings;
    std::vector<vtr::interned_string> interned_strings;

    /// Secure hash digest of the architecture file to uniquely identify this architecture
    char* architecture_id;

    // Options for tileable routing architectures
    // These are used for an alternative, tilable, rr-graph generator that can produce
    // OpenFPGA-compatible FPGAs that can be implemented to silicon via the OpenFPGA flow

    /// Whether the routing architecture is tileable
    bool tileable;

    /// Allow connection blocks to appear around the perimeter programmable block
    bool perimeter_cb;

    /// Remove all the routing wires in empty regions
    bool shrink_boundary;

    /// Allow routing channels to pass through multi-width and
    /// multi-height programable blocks
    bool through_channel;

    /// Allow each output pin of a programmable block to drive the
    /// routing tracks on all the sides of its adjacent switch block
    bool opin2all_sides;

    /// Whether the routing architecture has concat wire
    /// For further detail, please refer to documentation
    bool concat_wire;

    /// Whether the routing architecture has concat pass wire
    /// For further detail, please refer to documentation
    bool concat_pass_wire;

    /// Connectivity parameter for pass tracks in each switch block
    int sub_fs;

    /// Connecting type for pass tracks in each switch block
    enum e_switch_block_type sb_sub_type;

    // End of tileable architecture options

    t_chan_width_dist Chans;
    enum e_switch_block_type sb_type;
    std::vector<t_switchblock_inf> switchblocks;
    float R_minW_nmos;
    float R_minW_pmos;
    int Fs;
    float grid_logic_tile_area;
    std::vector<t_segment_inf> Segments;

    /// Contains information from all switch types defined in the architecture file.
    std::vector<t_arch_switch_inf> switches;

    /// Contains information about all direct chain connections in the architecture
    std::vector<t_direct_inf> directs;

    LogicalModels models;

    t_power_arch* power = nullptr;

    std::shared_ptr<std::vector<t_clock_network>> clocks;

    //determine which layers in multi-die FPGAs require to build global routing resources
    std::vector<bool> layer_global_routing;

    // Constants
    // VCC and GND cells are special virtual cells that are
    // used to handle the constant network of the device.
    //
    // Similarly, the constant nets are defined to identify
    // the generic name for the constant network.
    //
    // Given that usually, the constants have a dedicated network in
    // real FPGAs, this information becomes relevant to identify which
    // nets from the circuit netlist are belonging to the constant network,
    // and assigned to it accordingly.
    //
    // NOTE: At the moment, the constant cells and nets are primarily used
    // for the interchange netlist format, to determine which are the constants
    // net names and which virtual cell is responsible to generate them.
    // The information is present in the device database.
    std::pair<std::string, std::string> gnd_cell;
    std::pair<std::string, std::string> vcc_cell;

    std::string gnd_net = "$__gnd_net";
    std::string vcc_net = "$__vcc_net";
    std::string default_clock_network_name = "clock_network";

    // Luts
    std::vector<t_lut_cell> lut_cells;
    std::unordered_map<std::string, std::vector<t_lut_element>> lut_elements;

    // The name of the switch used for the input connection block (i.e. to
    // connect routing tracks to block pins). tracks can be connected to
    // ipins through the same die or from other dice, each of these
    // types of connections requires a different switch, all names should correspond to a switch in Switches.
    std::vector<std::string> ipin_cblock_switch_name;

    std::vector<t_grid_def> grid_layouts; //Set of potential device layouts

    // the layout that is chosen to be used with command line options
    // It is used to generate custom SB for a specific locations within the device
    // If the layout is not specified in the command line options, this variable will be set to "auto"
    std::string device_layout;

    t_clock_arch_spec clock_arch; // Clock related data types

    /// Stores NoC-related architectural information when there is an embedded NoC
    t_noc_inf* noc = nullptr;

    /// VIB grid layouts
    std::vector<t_vib_grid_def> vib_grid_layouts;

    // added for vib
    std::vector<VibInf> vib_infs;

    /// Stores information for scatter-gather patterns that can be used to define some of the rr-graph connectivity
    std::vector<t_scatter_gather_pattern> scatter_gather_patterns;
};
