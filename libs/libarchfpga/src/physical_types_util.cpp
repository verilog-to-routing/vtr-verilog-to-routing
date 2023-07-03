#include <set>
#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_util.h"

#include "arch_types.h"
#include "arch_util.h"
#include "arch_error.h"

#include "physical_types_util.h"

/**
 * @brief Data structure that holds information about a physical pin
 *
 * This structure holds the following information on a pin:
 *   - sub_tile_index: index of the sub tile within the physical tile type containing this pin
 *   - logical_block_index: index of the logical block which this pin in located in it.
 *                          To retrieve the logical block -> device_ctx.logical_block_types[logical_block_index]
 *   - capacity_instance: sub tile instance containing this physical pin.
 *                        Each sub tile has a capacity field, which determines how many of its
 *                        instances are present in the belonging physical tile.
 *                        E.g.:
 *                          - The sub tile BUFG has a capacity of 4 within its belonging physical tile CLOCK_TILE.
 *                          - The capacity instance of a pin in the CLOCK_TILE identifies which of the 4 instances
 *                            the pin belongs to.
 *   - pb_type_idx: If pin is located inside a cluster, it would belong to a pb_block.
 *   - port_index: Each sub tile has a set of ports with a variable number of pins. The port_index field identifies
 *                 which port the physical pin belongs to.
 *   - pin_index_in_port: Given that ports can have multiple pins, we need also a field to identify which one of the
 *                        multiple pins of the port corresponds to the physical pin.
 *   - pin_physical_num: pin physical_number
 *
 */
struct t_pin_inst_port {
    int sub_tile_index;      // Sub Tile index
    int logical_block_index; // Logical block index - valid if flat_routing is enabled
    int capacity_instance;   // within capacity
    int pb_type_idx;         // valid if flat_routing is enabled
    int port_index;          // Port index
    int pin_index_in_port;   // Pin's index within the port
    int pin_physical_num;
};

/******************** Subroutine declarations and definition ****************************/

static std::tuple<int, int, int, int, int> get_pin_index_for_inst(t_physical_tile_type_ptr type, int pin_physical_num, bool is_flat);

static t_pin_inst_port block_type_pin_index_to_pin_inst(t_physical_tile_type_ptr type, int pin_physical_num, bool is_flat);

static int get_sub_tile_num_internal_classes(const t_sub_tile* sub_tile);

static int get_sub_tile_physical_pin_num_offset(t_physical_tile_type_ptr physical_tile,
                                                const t_sub_tile* curr_sub_tile);

static int get_sub_tile_inst_physical_pin_num_offset(t_physical_tile_type_ptr physical_tile,
                                                     const t_sub_tile* curr_sub_tile,
                                                     const int curr_relative_cap);

static int get_logical_block_physical_pin_num_offset(t_physical_tile_type_ptr physical_tile,
                                                     const t_sub_tile* curr_sub_tile,
                                                     t_logical_block_type_ptr curr_logical_block,
                                                     const int curr_relative_cap);

static int get_pin_logical_num_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int physical_num);

/**
 *
 * @param physical_type physical tile which pin belongs to
 * @param sub_tile  sub_tile in which physical tile located
 * @param logical_block logical block mapped to the sub_tile
 * @param relative_cap relative capacity of the sub_tile
 * @param pin
 * @return physical number of the pins derived the given pin
 */
static std::vector<int> get_pb_pin_src_pins(t_physical_tile_type_ptr physical_type,
                                            const t_sub_tile* sub_tile,
                                            t_logical_block_type_ptr logical_block,
                                            int relative_cap,
                                            const t_pb_graph_pin* pin);

/**
 *
 * @param physical_type physical tile which pin belongs to
 * @param sub_tile  sub_tile in which physical tile located
 * @param logical_block logical block mapped to the sub_tile
 * @param relative_cap relative capacity of the sub_tile
 * @param pin
 * @return physical number of the pins derived by the given pin
 */
static std::vector<int> get_pb_pin_sink_pins(t_physical_tile_type_ptr physical_type,
                                             const t_sub_tile* sub_tile,
                                             t_logical_block_type_ptr logical_block,
                                             int relative_cap,
                                             const t_pb_graph_pin* pin);

/**
 *
 * @param physical_type
 * @param logical_block
 * @param pin_physical_num physical number of the pin *on* the tile. It shouldn't be for a pin inside the tile
 * @return return the pb_pin corresponding to the given pin_physical num.
 */
static const t_pb_graph_pin* get_tile_pin_pb_pin(t_physical_tile_type_ptr physical_type,
                                                 t_logical_block_type_ptr logical_block,
                                                 int pin_physical_num);

static t_pb_graph_pin* get_mutable_tile_pin_pb_pin(t_physical_tile_type* physical_type,
                                                   t_logical_block_type* logical_block,
                                                   int pin_physical_num);

/**
 *
 * @param physical_tile
 * @param class_physical_num
 * @return A vector containing all of the parent pb_graph_nodes and the pb_graph_node of the class_physical_num itself
 */
static std::vector<const t_pb_graph_node*> get_sink_hierarchical_parents(t_physical_tile_type_ptr physical_tile,
                                                                         int class_physical_num);

/**
 *
 * @param physical_tile
 * @param pin_physcial_num
 * @param ref_sink_num
 * @param sink_grp
 * @return Return zero if the ref_sink_num is not reachable by pin_physical_num, otherwise return the number sinks in sink_grp
 * reachable by pin_physical_num
 */
static int get_num_reachable_sinks(t_physical_tile_type_ptr physical_tile,
                                   const int pin_physcial_num,
                                   const int ref_sink_num,
                                   const std::vector<int>& sink_grp);

static std::tuple<int, int, int, int, int> get_pin_index_for_inst(t_physical_tile_type_ptr type, int pin_physical_num, bool is_flat) {
    int max_ptc = get_tile_pin_max_ptc(type, is_flat);
    VTR_ASSERT(pin_physical_num < max_ptc);

    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    int pin_inst_num;
    int logical_block_idx;
    int pb_type_idx;

    bool on_tile_pin = is_pin_on_tile(type, pin_physical_num);

    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(type, pin_physical_num);

    if (on_tile_pin) {
        int pin_offset = 0;
        for (int sub_tile_idx = 0; sub_tile_idx < sub_tile->index; sub_tile_idx++) {
            pin_offset += type->sub_tiles[sub_tile_idx].num_phy_pins;
        }
        int pins_per_inst = sub_tile->num_phy_pins / sub_tile->capacity.total();
        pin_inst_num = (pin_physical_num - pin_offset) % pins_per_inst;
    } else {
        int pin_offset = get_sub_tile_inst_physical_pin_num_offset(type, sub_tile, sub_tile_cap);
        int pins_per_inst = get_total_num_sub_tile_internal_pins(sub_tile) / sub_tile->capacity.total();
        pin_inst_num = (pin_physical_num - pin_offset) % pins_per_inst;
    }

    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(type, pin_physical_num);
    VTR_ASSERT(sub_tile_cap != -1);

    if (on_tile_pin) {
        logical_block_idx = -1;
        pb_type_idx = 0;
    } else {
        auto logical_block = get_logical_block_from_pin_physical_num(type, pin_physical_num);
        auto pb_type = get_pb_pin_from_pin_physical_num(type, pin_physical_num)->parent_node->pb_type;
        VTR_ASSERT(logical_block != nullptr);
        logical_block_idx = logical_block->index;
        pb_type_idx = pb_type->index_in_logical_block;
    }

    return std::make_tuple(pin_inst_num, sub_tile->index, sub_tile_cap, logical_block_idx, pb_type_idx);
}

static t_pin_inst_port block_type_pin_index_to_pin_inst(t_physical_tile_type_ptr type, int pin_physical_num, bool is_flat) {
    int pin_index, sub_tile_index, inst_num, logical_num, pb_type_idx;
    std::tie<int, int, int, int>(pin_index, sub_tile_index, inst_num, logical_num, pb_type_idx) = get_pin_index_for_inst(type, pin_physical_num, is_flat);

    t_pin_inst_port pin_inst_port;
    pin_inst_port.sub_tile_index = sub_tile_index;
    pin_inst_port.capacity_instance = inst_num;
    pin_inst_port.logical_block_index = logical_num;
    pin_inst_port.pb_type_idx = pb_type_idx;
    pin_inst_port.pin_physical_num = pin_physical_num;
    pin_inst_port.port_index = OPEN;
    pin_inst_port.pin_index_in_port = OPEN;

    if (is_flat && logical_num != -1) {
        auto pb_pin = get_pb_pin_from_pin_physical_num(type, pin_physical_num);
        auto port = pb_pin->port;
        pin_inst_port.pin_index_in_port = pb_pin->pin_number;
        pin_inst_port.port_index = port->index;
    } else {
        /* pin is located on the tile */
        for (auto const& port : type->sub_tiles[sub_tile_index].ports) {
            if (pin_index >= port.absolute_first_pin_index && pin_index < port.absolute_first_pin_index + port.num_pins) {
                pin_inst_port.port_index = port.index;
                pin_inst_port.pin_index_in_port = pin_index - port.absolute_first_pin_index;
                break;
            }
        }
    }
    VTR_ASSERT(pin_inst_port.port_index != OPEN);
    VTR_ASSERT(pin_inst_port.pin_index_in_port != OPEN);
    return pin_inst_port;
}

static int get_sub_tile_num_internal_classes(const t_sub_tile* sub_tile) {
    int num_classes = 0;
    for (auto eq_site : sub_tile->equivalent_sites) {
        num_classes += (int)eq_site->primitive_logical_class_inf.size();
    }
    num_classes *= sub_tile->capacity.total();

    return num_classes;
}

static int get_sub_tile_physical_pin_num_offset(t_physical_tile_type_ptr physical_tile,
                                                const t_sub_tile* curr_sub_tile) {
    int offset = physical_tile->num_pins;
    for (const auto& tmp_sub_tile : physical_tile->sub_tiles) {
        if (&tmp_sub_tile == curr_sub_tile)
            break;
        else
            offset += get_total_num_sub_tile_internal_pins(&tmp_sub_tile);
    }

    return offset;
}

static int get_sub_tile_inst_physical_pin_num_offset(t_physical_tile_type_ptr physical_tile,
                                                     const t_sub_tile* curr_sub_tile,
                                                     const int curr_relative_cap) {
    int offset = get_sub_tile_physical_pin_num_offset(physical_tile, curr_sub_tile);
    int sub_tile_inst_num_pins = get_total_num_sub_tile_internal_pins(curr_sub_tile) / curr_sub_tile->capacity.total();

    offset += (curr_relative_cap * sub_tile_inst_num_pins);

    return offset;
}

static int get_logical_block_physical_pin_num_offset(t_physical_tile_type_ptr physical_tile,
                                                     const t_sub_tile* curr_sub_tile,
                                                     t_logical_block_type_ptr curr_logical_block,
                                                     const int curr_relative_cap) {
    int offset;
    offset = get_sub_tile_inst_physical_pin_num_offset(physical_tile, curr_sub_tile, curr_relative_cap);

    for (auto eq_site : curr_sub_tile->equivalent_sites) {
        if (eq_site == curr_logical_block)
            break;
        offset += (int)eq_site->pin_logical_num_to_pb_pin_mapping.size();
    }
    return offset;
}

static int get_pin_logical_num_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int physical_num) {
    VTR_ASSERT(!is_pin_on_tile(physical_tile, physical_num));
    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(physical_tile, physical_num);
    VTR_ASSERT(sub_tile_cap != -1);
    auto logical_block = get_logical_block_from_pin_physical_num(physical_tile, physical_num);

    int pin_logical_num;

    int offset = get_logical_block_physical_pin_num_offset(physical_tile,
                                                           sub_tile,
                                                           logical_block,
                                                           sub_tile_cap);
    pin_logical_num = physical_num - offset;

    return pin_logical_num;
}

static std::vector<int> get_pb_pin_src_pins(t_physical_tile_type_ptr physical_type,
                                            const t_sub_tile* sub_tile,
                                            t_logical_block_type_ptr logical_block,
                                            int relative_cap,
                                            const t_pb_graph_pin* pin) {
    std::vector<int> driving_pins;
    const auto& edges = pin->input_edges;
    t_pb_graph_pin** connected_pins_ptr;
    int num_edges = pin->num_input_edges;
    int num_pins = 0;

    for (int edge_idx = 0; edge_idx < num_edges; edge_idx++) {
        const t_pb_graph_edge* pb_graph_edge = edges[edge_idx];
        num_pins += pb_graph_edge->num_input_pins;
    }
    driving_pins.reserve(num_pins);

    for (int edge_idx = 0; edge_idx < num_edges; edge_idx++) {
        const t_pb_graph_edge* pb_graph_edge = edges[edge_idx];
        connected_pins_ptr = pb_graph_edge->input_pins;
        num_pins = pb_graph_edge->num_input_pins;

        for (int pin_idx = 0; pin_idx < num_pins; pin_idx++) {
            auto conn_pin = connected_pins_ptr[pin_idx];
            if (conn_pin->is_root_block_pin()) {
                driving_pins.push_back(get_physical_pin_at_sub_tile_location(physical_type,
                                                                             logical_block,
                                                                             sub_tile->capacity.low + relative_cap,
                                                                             conn_pin->pin_count_in_cluster));
            } else {
                driving_pins.push_back(get_pb_pin_physical_num(physical_type,
                                                               sub_tile,
                                                               logical_block,
                                                               relative_cap,
                                                               conn_pin));
            }
        }
    }

    return driving_pins;
}

static std::vector<int> get_pb_pin_sink_pins(t_physical_tile_type_ptr physical_type,
                                             const t_sub_tile* sub_tile,
                                             t_logical_block_type_ptr logical_block,
                                             int relative_cap,
                                             const t_pb_graph_pin* pin) {
    std::vector<int> sink_pins;
    const auto& edges = pin->output_edges;
    t_pb_graph_pin** connected_pins_ptr;
    int num_edges = pin->num_output_edges;
    int num_pins = 0;

    for (int edge_idx = 0; edge_idx < num_edges; edge_idx++) {
        const t_pb_graph_edge* pb_graph_edge = edges[edge_idx];
        num_pins += pb_graph_edge->num_output_pins;
    }
    sink_pins.reserve(num_pins);

    for (int edge_idx = 0; edge_idx < num_edges; edge_idx++) {
        const t_pb_graph_edge* pb_graph_edge = edges[edge_idx];
        connected_pins_ptr = pb_graph_edge->output_pins;
        num_pins = pb_graph_edge->num_output_pins;

        for (int pin_idx = 0; pin_idx < num_pins; pin_idx++) {
            auto conn_pin = connected_pins_ptr[pin_idx];
            if (conn_pin->is_root_block_pin()) {
                sink_pins.push_back(get_physical_pin_at_sub_tile_location(physical_type,
                                                                          logical_block,
                                                                          sub_tile->capacity.low + relative_cap,
                                                                          conn_pin->pin_count_in_cluster));
            } else {
                sink_pins.push_back(get_pb_pin_physical_num(physical_type,
                                                            sub_tile,
                                                            logical_block,
                                                            relative_cap,
                                                            conn_pin));
            }
        }
    }

    return sink_pins;
}

static const t_pb_graph_pin* get_tile_pin_pb_pin(t_physical_tile_type_ptr physical_type,
                                                 t_logical_block_type_ptr logical_block,
                                                 int pin_physical_num) {
    VTR_ASSERT(is_pin_on_tile(physical_type, pin_physical_num));
    return physical_type->on_tile_pin_num_to_pb_pin.at(pin_physical_num).at(logical_block);
}

static t_pb_graph_pin* get_mutable_tile_pin_pb_pin(t_physical_tile_type* physical_type,
                                                   t_logical_block_type* logical_block,
                                                   int pin_physical_num) {
    VTR_ASSERT(is_pin_on_tile(physical_type, pin_physical_num));
    return physical_type->on_tile_pin_num_to_pb_pin.at(pin_physical_num).at(logical_block);
}

static std::vector<const t_pb_graph_node*> get_sink_hierarchical_parents(t_physical_tile_type_ptr physical_tile,
                                                                         int class_physical_num) {
    std::vector<const t_pb_graph_node*> pb_nodes;
    const t_pb_graph_node* curr_pb_node = get_pb_graph_node_from_pin_physical_num(physical_tile,
                                                                                  get_pin_list_from_class_physical_num(physical_tile,
                                                                                                                       class_physical_num)[0]);
    pb_nodes.reserve(curr_pb_node->pb_type->depth + 1);
    VTR_ASSERT(curr_pb_node != nullptr);

    while (curr_pb_node != nullptr) {
        pb_nodes.push_back(curr_pb_node);
        curr_pb_node = curr_pb_node->parent_pb_graph_node;
    }

    return pb_nodes;
}

static int get_num_reachable_sinks(t_physical_tile_type_ptr physical_tile,
                                   const int pin_physcial_num,
                                   const int ref_sink_num,
                                   const std::vector<int>& sink_grp) {
    int num_reachable_sinks = 0;

    auto pb_pin = get_pb_pin_from_pin_physical_num(physical_tile,
                                                   pin_physcial_num);
    const auto& connected_sinks = pb_pin->connected_sinks_ptc;

    // If ref_sink_num is not reachable by pin_physical_num return 0
    if (connected_sinks.find(ref_sink_num) == connected_sinks.end()) {
        return 0;
    }

    for (auto sink_num : sink_grp) {
        if (connected_sinks.find(sink_num) != connected_sinks.end()) {
            num_reachable_sinks++;
        }
    }

    return num_reachable_sinks;
}

/******************** End Subroutine declarations and definition ************************/

int get_sub_tile_physical_pin(int sub_tile_index,
                              t_physical_tile_type_ptr physical_tile,
                              t_logical_block_type_ptr logical_block,
                              int pin) {
    t_logical_pin logical_pin(pin);

    const auto& direct_map = physical_tile->tile_block_pin_directs_map.at(logical_block->index).at(sub_tile_index);
    auto result = direct_map.find(logical_pin);

    if (result == direct_map.end()) {
        archfpga_throw(__FILE__, __LINE__,
                       "Couldn't find the corresponding physical tile pin of the logical block pin %d."
                       "Physical Tile Type: %s, Logical Block Type: %s.\n",
                       pin, physical_tile->name, logical_block->name);
    }

    return result->second.pin;
}

int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block) {
    int sub_tile_index = OPEN;
    for (const auto& sub_tile : physical_tile->sub_tiles) {
        auto eq_sites = sub_tile.equivalent_sites;
        auto it = std::find(eq_sites.begin(), eq_sites.end(), logical_block);
        if (it != eq_sites.end()) {
            sub_tile_index = sub_tile.index;
        }
    }

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Found no instances of logical block type '%s' within physical tile type '%s'. ",
                       logical_block->name, physical_tile->name);
    }

    return sub_tile_index;
}

int get_physical_pin(t_physical_tile_type_ptr physical_tile,
                     t_logical_block_type_ptr logical_block,
                     int pin) {
    int sub_tile_index = get_logical_block_physical_sub_tile_index(physical_tile, logical_block);

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Couldn't find the corresponding physical tile type pin of the logical block type pin %d.",
                       pin);
    }

    int sub_tile_physical_pin = get_sub_tile_physical_pin(sub_tile_index, physical_tile, logical_block, pin);
    return physical_tile->sub_tiles[sub_tile_index].sub_tile_to_tile_pin_indices[sub_tile_physical_pin];
}

int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block,
                                              int sub_tile_capacity) {
    int sub_tile_index = OPEN;
    for (const auto& sub_tile : physical_tile->sub_tiles) {
        auto eq_sites = sub_tile.equivalent_sites;
        auto it = std::find(eq_sites.begin(), eq_sites.end(), logical_block);
        if (it != eq_sites.end()
            && (sub_tile.capacity.is_in_range(sub_tile_capacity))) {
            sub_tile_index = sub_tile.index;
            break;
        }
    }

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Found no instances of logical block type '%s' within physical tile type '%s'. ",
                       logical_block->name, physical_tile->name);
    }

    return sub_tile_index;
}

/**
 * This function returns the most common physical tile type given a logical block
 */
t_physical_tile_type_ptr pick_physical_type(t_logical_block_type_ptr logical_block) {
    return logical_block->equivalent_tiles[0];
}

t_logical_block_type_ptr pick_logical_type(t_physical_tile_type_ptr physical_tile) {
    return physical_tile->sub_tiles[0].equivalent_sites[0];
}

bool is_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block) {
    const auto& equivalent_tiles = logical_block->equivalent_tiles;
    return std::find(equivalent_tiles.begin(), equivalent_tiles.end(), physical_tile) != equivalent_tiles.end();
}

bool is_sub_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block, int sub_tile_loc) {
    bool capacity_compatible = false;
    for (auto& sub_tile : physical_tile->sub_tiles) {
        auto result = std::find(sub_tile.equivalent_sites.begin(), sub_tile.equivalent_sites.end(), logical_block);

        if (sub_tile.capacity.is_in_range(sub_tile_loc) && result != sub_tile.equivalent_sites.end()) {
            capacity_compatible = true;
            break;
        }
    }

    return capacity_compatible && is_tile_compatible(physical_tile, logical_block);
}

int get_physical_pin_at_sub_tile_location(t_physical_tile_type_ptr physical_tile,
                                          t_logical_block_type_ptr logical_block,
                                          int sub_tile_capacity,
                                          int pin) {
    VTR_ASSERT(pin < physical_tile->num_pins);
    int sub_tile_index = get_logical_block_physical_sub_tile_index(physical_tile, logical_block, sub_tile_capacity);

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Couldn't find the corresponding physical tile type pin of the logical block type pin %d.",
                       pin);
    }

    int sub_tile_physical_pin = get_sub_tile_physical_pin(sub_tile_index, physical_tile, logical_block, pin);

    /* Find the relative capacity of the logical_block in this sub tile */
    int relative_capacity = sub_tile_capacity - physical_tile->sub_tiles[sub_tile_index].capacity.low;

    /* Find the number of pins per block in the equivalent site list
     * of the sub tile. Otherwise, the current logical block may have smaller/larger number of pins
     * than other logical blocks that can be placed in the sub-tile. This will lead to an error
     * when computing the pin index!
     */
    int block_num_pins = physical_tile->sub_tiles[sub_tile_index].num_phy_pins / physical_tile->sub_tiles[sub_tile_index].capacity.total();

    return relative_capacity * block_num_pins
           + physical_tile->sub_tiles[sub_tile_index].sub_tile_to_tile_pin_indices[sub_tile_physical_pin];
}

int get_max_num_pins(t_logical_block_type_ptr logical_block) {
    int max_num_pins = 0;

    for (auto physical_tile : logical_block->equivalent_tiles) {
        max_num_pins = std::max(max_num_pins, physical_tile->num_pins);
    }

    return max_num_pins;
}

//Returns the pin class associated with the specified pin_index_in_port within the port port_name on type
int find_pin_class(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port, e_pin_type pin_type) {
    int iclass = OPEN;

    int ipin = find_pin(type, port_name, pin_index_in_port);

    if (ipin != OPEN) {
        iclass = type->pin_class[ipin];

        if (iclass != OPEN) {
            VTR_ASSERT(type->class_inf[iclass].type == pin_type);
        }
    }
    return iclass;
}

int find_pin(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port) {
    int ipin = OPEN;
    int port_base_ipin = 0;
    int num_pins = OPEN;
    int pin_offset = 0;

    bool port_found = false;
    for (const auto& sub_tile : type->sub_tiles) {
        for (const auto& port : sub_tile.ports) {
            if (0 == strcmp(port.name, port_name.c_str())) {
                port_found = true;
                num_pins = port.num_pins;
                break;
            }

            port_base_ipin += port.num_pins;
        }

        if (port_found) {
            break;
        }

        port_base_ipin = 0;
        pin_offset += sub_tile.num_phy_pins;
    }

    if (num_pins != OPEN) {
        VTR_ASSERT(pin_index_in_port < num_pins);

        ipin = port_base_ipin + pin_index_in_port + pin_offset;
    }

    return ipin;
}

std::pair<int, int> get_capacity_location_from_physical_pin(t_physical_tile_type_ptr physical_tile, int pin) {
    int pins_to_remove = 0;
    for (const auto& sub_tile : physical_tile->sub_tiles) {
        auto capacity = sub_tile.capacity;
        int sub_tile_num_pins = sub_tile.num_phy_pins;
        int sub_tile_pin = pin - pins_to_remove;

        if (sub_tile_pin < sub_tile_num_pins) {
            int rel_capacity = sub_tile_pin / (sub_tile_num_pins / capacity.total());
            int rel_pin = sub_tile_pin % (sub_tile_num_pins / capacity.total());

            return std::pair<int, int>(rel_capacity + capacity.low, rel_pin);
        }

        pins_to_remove += sub_tile_num_pins;
    }

    archfpga_throw(__FILE__, __LINE__,
                   "Couldn't find sub tile that contains the pin %d in physical tile %s.\n",
                   pin, physical_tile->name);
}

int get_physical_pin_from_capacity_location(t_physical_tile_type_ptr physical_tile, int relative_pin, int capacity_location) {
    int pins_to_add = 0;
    for (auto sub_tile : physical_tile->sub_tiles) {
        auto capacity = sub_tile.capacity;
        int rel_capacity = capacity_location - capacity.low;
        int num_inst_pins = sub_tile.num_phy_pins / capacity.total();

        if (capacity.is_in_range(capacity_location)) {
            return pins_to_add + num_inst_pins * rel_capacity + relative_pin;
        }

        pins_to_add += sub_tile.num_phy_pins;
    }

    archfpga_throw(__FILE__, __LINE__,
                   "Couldn't find sub tile that contains the relative pin %d at the capacity location %d in physical tile %s.\n",
                   relative_pin, capacity_location, physical_tile->name);
}
bool is_opin(int ipin, t_physical_tile_type_ptr type) {
    /* Returns true if this clb pin is an output, false otherwise. */

    if (ipin > type->num_pins) {
        //Not a top level pin
        return false;
    }

    int iclass = type->pin_class[ipin];

    if (type->class_inf[iclass].type == DRIVER)
        return true;
    else
        return false;
}

bool is_pin_conencted_to_layer(t_physical_tile_type_ptr type, int ipin, int from_layer, int to_layer, int num_of_avail_layer) {
    if (type->is_empty()) { //if type is empty, there is no pins
        return false;
    }
    //ipin should be a valid pin in physical type
    VTR_ASSERT(ipin < type->num_pins);
    int pin_layer = from_layer + type->pin_layer_offset[ipin];
    //if pin_offset specifies a layer that doesn't exist in arch file, we do a wrap around
    pin_layer = (pin_layer < num_of_avail_layer) ? pin_layer : pin_layer % num_of_avail_layer;
    if (from_layer == to_layer || pin_layer == to_layer) {
        return true;
    } else {
        return false;
    }
    //not reachable
    return false;
}

// TODO: Remove is_input_type / is_output_type / is_io_type as part of
// https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/1193
bool is_input_type(t_physical_tile_type_ptr type) {
    return type->is_input_type;
}

bool is_output_type(t_physical_tile_type_ptr type) {
    return type->is_output_type;
}

bool is_io_type(t_physical_tile_type_ptr type) {
    return is_input_type(type)
           || is_output_type(type);
}

std::string block_type_pin_index_to_name(t_physical_tile_type_ptr type, int pin_physical_num, bool is_flat) {
    int max_ptc = get_tile_pin_max_ptc(type, is_flat);
    VTR_ASSERT(pin_physical_num < max_ptc);
    int pin_index;
    std::string pin_name = type->name;

    int sub_tile_index, inst_num, logical_num, pb_type_idx;
    std::tie<int, int, int, int, int>(pin_index, sub_tile_index, inst_num, logical_num, pb_type_idx) = get_pin_index_for_inst(type, pin_physical_num, is_flat);
    if (type->sub_tiles[sub_tile_index].capacity.total() > 1) {
        pin_name += "[" + std::to_string(inst_num) + "]";
    }

    if (!is_pin_on_tile(type, pin_physical_num)) {
        pin_name += ".";
        auto pb_graph_node = get_pb_graph_node_from_pin_physical_num(type, pin_physical_num);
        pin_name += pb_graph_node->hierarchical_type_name();
    }

    pin_name += ".";

    if (is_pin_on_tile(type, pin_physical_num)) {
        for (auto const& port : type->sub_tiles[sub_tile_index].ports) {
            if (pin_index >= port.absolute_first_pin_index && pin_index < port.absolute_first_pin_index + port.num_pins) {
                //This port contains the desired pin index
                int index_in_port = pin_index - port.absolute_first_pin_index;
                pin_name += port.name;
                pin_name += "[" + std::to_string(index_in_port) + "]";
                return pin_name;
            }
        }
    } else {
        VTR_ASSERT(is_flat == true);
        auto pb_graph_pin = get_pb_pin_from_pin_physical_num(type, pin_physical_num);

        pin_name += pb_graph_pin->port->name;
        pin_name += "[" + std::to_string(pb_graph_pin->pin_number) + "]";
        return pin_name;
    }

    return "<UNKOWN>";
}

std::vector<std::string> block_type_class_index_to_pin_names(t_physical_tile_type_ptr type,
                                                             int class_physical_num,
                                                             bool is_flat) {
    t_class class_inf;
    bool class_on_tile = is_class_on_tile(type, class_physical_num);
    VTR_ASSERT(class_on_tile || is_flat);
    if (class_on_tile) {
        class_inf = type->class_inf[class_physical_num];
    } else {
        class_inf = type->primitive_class_inf.at(class_physical_num);
    }

    std::vector<t_pin_inst_port> pin_info;
    for (int ipin = 0; ipin < class_inf.num_pins; ++ipin) {
        int pin_physical_num = class_inf.pinlist[ipin];
        pin_info.push_back(block_type_pin_index_to_pin_inst(type, pin_physical_num, is_flat));
    }

    auto cmp = [](const t_pin_inst_port& lhs, const t_pin_inst_port& rhs) {
        return lhs.pin_physical_num < rhs.pin_physical_num;
    };

    //Ensure all the pins are in order
    std::sort(pin_info.begin(), pin_info.end(), cmp);

    //Determine ranges for each capacity instance and port pair
    std::map<std::tuple<int, int, int, int, int>, std::array<int, 4>> pin_ranges;
    for (const auto& pin_inf : pin_info) {
        auto key = std::make_tuple(pin_inf.sub_tile_index, pin_inf.capacity_instance, pin_inf.logical_block_index, pin_inf.pb_type_idx, pin_inf.port_index);
        if (!pin_ranges.count(key)) {
            pin_ranges[key][0] = pin_inf.pin_index_in_port;
            pin_ranges[key][1] = pin_inf.pin_index_in_port;
            pin_ranges[key][2] = pin_inf.pin_physical_num;
            pin_ranges[key][3] = pin_inf.pin_physical_num;
        } else {
            VTR_ASSERT(pin_ranges[key][1] == pin_inf.pin_index_in_port - 1);
            VTR_ASSERT(pin_ranges[key][3] == pin_inf.pin_physical_num - 1);
            pin_ranges[key][1] = pin_inf.pin_index_in_port;
            pin_ranges[key][3] = pin_inf.pin_physical_num;
        }
    }

    //Format pin ranges
    std::vector<std::string> pin_names;
    for (auto kv : pin_ranges) {
        auto type_port = kv.first;
        auto pins = kv.second;

        int isub_tile, logical_num, icapacity, iport, pb_idx;
        std::tie<int, int, int, int, int>(isub_tile, icapacity, logical_num, pb_idx, iport) = type_port;

        int ipin_start = pins[0];
        int ipin_end = pins[1];

        int pin_physical_start = pins[2];
        int pin_physical_end = pins[3];

        auto& sub_tile = type->sub_tiles[isub_tile];

        std::string port_name;
        std::string block_name;
        if (is_pin_on_tile(type, pin_physical_start)) {
            VTR_ASSERT(is_pin_on_tile(type, pin_physical_end) == true);
            port_name = sub_tile.ports[iport].name;
            block_name = vtr::string_fmt("%s[%d]", type->name, icapacity);
        } else {
            VTR_ASSERT(is_pin_on_tile(type, pin_physical_end) == false);
            auto pb_pin = get_pb_pin_from_pin_physical_num(type, pin_physical_start);
            port_name = pb_pin->port->name;
            auto pb_graph_node = get_pb_graph_node_from_pin_physical_num(type, pin_physical_start);
            block_name = vtr::string_fmt("%s[%d].%s",
                                         type->name,
                                         icapacity,
                                         pb_graph_node->hierarchical_type_name().c_str());
        }

        std::string pin_name;
        if (ipin_start == ipin_end) {
            pin_name = vtr::string_fmt("%s.%s[%d]",
                                       block_name.c_str(),
                                       port_name.c_str(),
                                       ipin_start);
        } else {
            pin_name = vtr::string_fmt("%s.%s[%d:%d]",
                                       block_name.c_str(),
                                       port_name.c_str(),
                                       ipin_start,
                                       ipin_end);
        }

        pin_names.push_back(pin_name);
    }

    return pin_names;
}

const t_physical_tile_port* get_port_by_name(t_sub_tile* sub_tile, const char* port_name) {
    for (auto port : sub_tile->ports) {
        if (0 == strcmp(port.name, port_name)) {
            return &sub_tile->ports[port.index];
        }
    }

    return nullptr;
}

const t_port* get_port_by_name(t_logical_block_type_ptr type, const char* port_name) {
    auto pb_type = type->pb_type;

    for (int i = 0; i < pb_type->num_ports; i++) {
        auto port = pb_type->ports[i];
        if (0 == strcmp(port.name, port_name)) {
            return &pb_type->ports[port.index];
        }
    }

    return nullptr;
}

const t_physical_tile_port* get_port_by_pin(const t_sub_tile* sub_tile, int pin) {
    for (auto port : sub_tile->ports) {
        if (pin >= port.absolute_first_pin_index && pin < port.absolute_first_pin_index + port.num_pins) {
            return &sub_tile->ports[port.index];
        }
    }

    return nullptr;
}

const t_port* get_port_by_pin(t_logical_block_type_ptr type, int pin) {
    auto pb_type = type->pb_type;

    for (int i = 0; i < pb_type->num_ports; i++) {
        auto port = pb_type->ports[i];
        if (pin >= port.absolute_first_pin_index && pin < port.absolute_first_pin_index + port.num_pins) {
            return &pb_type->ports[port.index];
        }
    }

    return nullptr;
}

/* Access information related to pin classes */

/** get information given class physical num **/

std::tuple<const t_sub_tile*, int> get_sub_tile_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int physical_class_num) {
    bool is_on_tile = is_class_on_tile(physical_tile, physical_class_num);
    int num_seen_class = (is_on_tile) ? 0 : (int)physical_tile->class_inf.size();
    int class_num_offset = num_seen_class;

    for (auto& sub_tile : physical_tile->sub_tiles) {
        int sub_tile_num_class = is_on_tile ? sub_tile.class_range.total_num() : get_sub_tile_num_internal_classes(&sub_tile);
        num_seen_class += sub_tile_num_class;

        if (physical_class_num < num_seen_class) {
            int num_class_per_inst = sub_tile_num_class / sub_tile.capacity.total();
            int sub_tile_cap = (physical_class_num - class_num_offset) / num_class_per_inst;
            return std::make_tuple(&sub_tile, sub_tile_cap);
        }

        class_num_offset = num_seen_class;
    }

    return std::make_tuple(nullptr, -1);
}

t_logical_block_type_ptr get_logical_block_from_class_physical_num(t_physical_tile_type_ptr physical_tile,
                                                                   int class_physical_num) {
    auto pin_list = get_pin_list_from_class_physical_num(physical_tile, class_physical_num);
    VTR_ASSERT((int)pin_list.size() != 0);
    return get_logical_block_from_pin_physical_num(physical_tile, pin_list[0]);
}

const std::vector<int>& get_pin_list_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[class_physical_num];
        return pin_class.pinlist;
    } else {
        const t_class& pin_class = physical_tile->primitive_class_inf.at(class_physical_num);
        return pin_class.pinlist;
    }
}

PortEquivalence get_port_equivalency_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[class_physical_num];
        return pin_class.equivalence;
    } else {
        const t_class& pin_class = physical_tile->primitive_class_inf.at(class_physical_num);
        return pin_class.equivalence;
    }
}

e_pin_type get_class_type_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        return physical_tile->class_inf[class_physical_num].type;

    } else {
        const t_class& pin_class = physical_tile->primitive_class_inf.at(class_physical_num);
        return pin_class.type;
    }
}

int get_class_num_pins_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        return physical_tile->class_inf[class_physical_num].num_pins;
    } else {
        const t_class& pin_class = physical_tile->primitive_class_inf.at(class_physical_num);
        return pin_class.num_pins;
    }
}

t_class_range get_pb_graph_node_class_physical_range(t_physical_tile_type_ptr /*physical_tile*/,
                                                     const t_sub_tile* sub_tile,
                                                     t_logical_block_type_ptr logical_block,
                                                     int sub_tile_relative_cap,
                                                     const t_pb_graph_node* pb_graph_node) {
    VTR_ASSERT(pb_graph_node->is_primitive());

    t_class_range class_range = logical_block->pb_graph_node_class_range.at(pb_graph_node);
    int logical_block_class_offset = sub_tile->primitive_class_range[sub_tile_relative_cap].at(logical_block).low;

    class_range.low += logical_block_class_offset;
    class_range.high += logical_block_class_offset;

    return class_range;
}

std::vector<int> get_tile_root_classes(t_physical_tile_type_ptr physical_type) {
    // The class numbers are distributed in a way that the classes on the tile are from 0 to the
    // maximum number of classes on the tile. For internal classes, the number starts from class_inf.size()
    std::vector<int> classes(physical_type->class_inf.size());
    std::iota(classes.begin(), classes.end(), 0);

    return classes;
}

t_class_range get_flat_tile_primitive_classes(t_physical_tile_type_ptr physical_type) {
    int low_class_num = physical_type->primitive_class_starting_idx;
    return {low_class_num, low_class_num + (int)physical_type->primitive_class_inf.size() - 1};
}

/** **/
int get_tile_class_max_ptc(t_physical_tile_type_ptr tile, bool is_flat) {
    if (is_flat) {
        return (int)tile->class_inf.size() + (int)tile->primitive_class_inf.size();
    } else {
        VTR_ASSERT(is_flat == false);
        return (int)tile->class_inf.size();
    }
}

/** get information given pin physical number **/
std::tuple<const t_sub_tile*, int> get_sub_tile_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int physical_num) {
    const t_sub_tile* target_sub_tile = nullptr;
    int target_sub_tile_cap = OPEN;

    bool pin_on_tile = is_pin_on_tile(physical_tile, physical_num);

    int total_pin_counts = pin_on_tile ? 0 : physical_tile->num_pins;
    int pin_offset = total_pin_counts;

    for (auto& sub_tile : physical_tile->sub_tiles) {
        int sub_tile_num_pins = pin_on_tile ? sub_tile.num_phy_pins : get_total_num_sub_tile_internal_pins(&sub_tile);
        total_pin_counts += sub_tile_num_pins;

        if (physical_num < total_pin_counts) {
            int pins_per_inst = sub_tile_num_pins / sub_tile.capacity.total();
            target_sub_tile_cap = (physical_num - pin_offset) / pins_per_inst;
            target_sub_tile = &sub_tile;
            break;
        }

        pin_offset = total_pin_counts;
    }

    return std::make_tuple(target_sub_tile, target_sub_tile_cap);
}

t_logical_block_type_ptr get_logical_block_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int physical_num) {
    VTR_ASSERT(physical_num >= physical_tile->num_pins);
    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    t_logical_block_type_ptr logical_block = nullptr;

    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(physical_tile, physical_num);
    VTR_ASSERT(sub_tile_cap != OPEN);

    for (auto logical_block_pin_range_pair : sub_tile->intra_pin_range[sub_tile_cap]) {
        if (physical_num >= logical_block_pin_range_pair.second.low) {
            logical_block = logical_block_pin_range_pair.first;
        }
    }

    return logical_block;
}

const t_pb_graph_pin* get_pb_pin_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int physical_num) {
    // Without providing logical_block to specifies which equivalent site is mapped to the sub_tile, there is no way to
    // get the pb_graph_pin corresponding to the pin on the tile.
    VTR_ASSERT(physical_num >= physical_tile->num_pins);
    return physical_tile->pin_num_to_pb_pin.at(physical_num);
}

const t_pb_graph_pin* get_pb_pin_from_pin_physical_num(t_physical_tile_type_ptr physical_type,
                                                       t_logical_block_type_ptr logical_block,
                                                       int pin_physical_num) {
    const t_pb_graph_pin* pb_pin;
    if (is_pin_on_tile(physical_type, pin_physical_num)) {
        pb_pin = get_tile_pin_pb_pin(physical_type,
                                     logical_block,
                                     pin_physical_num);
    } else {
        pb_pin = get_pb_pin_from_pin_physical_num(physical_type,
                                                  pin_physical_num);
    }

    return pb_pin;
}

t_pb_graph_pin* get_mutable_pb_pin_from_pin_physical_num(t_physical_tile_type* physical_tile, t_logical_block_type* logical_block, int physical_num) {
    if (is_pin_on_tile(physical_tile, physical_num)) {
        return get_mutable_tile_pin_pb_pin(physical_tile, logical_block, physical_num);
    } else {
        return physical_tile->pin_num_to_pb_pin.at(physical_num);
    }
}

PortEquivalence get_port_equivalency_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int pin_physical_num) {
    if (is_pin_on_tile(physical_tile, pin_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[physical_tile->pin_class[pin_physical_num]];
        return pin_class.equivalence;
    } else {
        VTR_ASSERT(is_primitive_pin(physical_tile, pin_physical_num));
        const t_class& pin_class = physical_tile->primitive_class_inf.at(physical_tile->primitive_pin_class.at(pin_physical_num));
        return pin_class.equivalence;
    }
}

e_pin_type get_pin_type_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int pin_physical_num) {
    if (is_pin_on_tile(physical_tile, pin_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[physical_tile->pin_class[pin_physical_num]];
        return pin_class.type;
    } else {
        auto pb_pin = get_pb_pin_from_pin_physical_num(physical_tile, pin_physical_num);
        auto port_type = pb_pin->port->type;
        if (port_type == PORTS::IN_PORT) {
            return e_pin_type::RECEIVER;
        } else {
            VTR_ASSERT(port_type == PORTS::OUT_PORT);
            return e_pin_type::DRIVER;
        }
    }
}

std::tuple<std::vector<int>, std::vector<int>, std::vector<e_side>> get_pin_coordinates(t_physical_tile_type_ptr physical_type,
                                                                                        int pin_physical_num,
                                                                                        const std::vector<e_side>& sides) {
    // For the pins which are on a tile, they may be located in multiple sides/locations to model a pin which is connected to multiple
    // channels. However, in the case of internal pins, currently, they are only located on e_side::TOP and the root location of the
    // tile.
    std::vector<int> x_vec;
    std::vector<int> y_vec;
    std::vector<e_side> side_vec;
    if (is_pin_on_tile(physical_type, pin_physical_num)) {
        for (e_side side : sides) {
            for (int width = 0; width < physical_type->width; ++width) {
                for (int height = 0; height < physical_type->height; ++height) {
                    if (physical_type->pinloc[width][height][side][pin_physical_num]) {
                        x_vec.push_back(width);
                        y_vec.push_back(height);
                        side_vec.push_back(side);
                    }
                }
            }
        }
    } else {
        x_vec.push_back(0);
        y_vec.push_back(0);
        side_vec.push_back(e_side::TOP);
    }

    return std::make_tuple(x_vec, y_vec, side_vec);
}

int get_class_num_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int pin_physical_num) {
    if (is_pin_on_tile(physical_tile, pin_physical_num)) {
        return physical_tile->pin_class[pin_physical_num];
    } else {
        VTR_ASSERT(is_primitive_pin(physical_tile, pin_physical_num));
        return physical_tile->primitive_pin_class.at(pin_physical_num);
    }
}

bool is_primitive_pin(t_physical_tile_type_ptr physical_tile, int pin_physical_num) {
    // We assume that pins on the tile are not primitive
    if (is_pin_on_tile(physical_tile, pin_physical_num)) {
        return false;
    } else {
        auto pb_pin = get_pb_pin_from_pin_physical_num(physical_tile, pin_physical_num);
        return pb_pin->is_primitive_pin();
    }
}

int get_pb_graph_node_num_pins(const t_pb_graph_node* pb_graph_node) {
    int num_pb_graph_pins = 0;
    for (int port_type_idx = 0; port_type_idx < 3; port_type_idx++) {
        int num_ports;
        int* num_pins;

        if (port_type_idx == 0) {
            num_ports = pb_graph_node->num_input_ports;
            num_pins = pb_graph_node->num_input_pins;

        } else if (port_type_idx == 1) {
            num_ports = pb_graph_node->num_output_ports;
            num_pins = pb_graph_node->num_output_pins;
        } else {
            VTR_ASSERT(port_type_idx == 2);
            num_ports = pb_graph_node->num_clock_ports;
            num_pins = pb_graph_node->num_clock_pins;
        }

        for (int port_idx = 0; port_idx < num_ports; port_idx++) {
            num_pb_graph_pins += num_pins[port_idx];
        }
    }

    return num_pb_graph_pins;
}

std::vector<t_pb_graph_pin*> get_mutable_pb_graph_node_pb_pins(t_pb_graph_node* pb_graph_node) {
    std::vector<t_pb_graph_pin*> pins(pb_graph_node->num_pins());
    int num_added_pins = 0;
    for (int port_type_idx = 0; port_type_idx < 3; port_type_idx++) {
        t_pb_graph_pin** pb_pins;
        int num_ports;
        int* num_pins;

        if (port_type_idx == 0) {
            pb_pins = pb_graph_node->input_pins;
            num_ports = pb_graph_node->num_input_ports;
            num_pins = pb_graph_node->num_input_pins;

        } else if (port_type_idx == 1) {
            pb_pins = pb_graph_node->output_pins;
            num_ports = pb_graph_node->num_output_ports;
            num_pins = pb_graph_node->num_output_pins;
        } else {
            VTR_ASSERT(port_type_idx == 2);
            pb_pins = pb_graph_node->clock_pins;
            num_ports = pb_graph_node->num_clock_ports;
            num_pins = pb_graph_node->num_clock_pins;
        }

        for (int port_idx = 0; port_idx < num_ports; port_idx++) {
            for (int pin_idx = 0; pin_idx < num_pins[port_idx]; pin_idx++) {
                t_pb_graph_pin* pin = &(pb_pins[port_idx][pin_idx]);
                pins[num_added_pins] = pin;
                num_added_pins++;
            }
        }
    }
    VTR_ASSERT(num_added_pins == pb_graph_node->num_pins());
    return pins;
}

t_pin_range get_pb_graph_node_pins(t_physical_tile_type_ptr /*physical_tile*/,
                                   const t_sub_tile* sub_tile,
                                   t_logical_block_type_ptr logical_block,
                                   int relative_cap,
                                   const t_pb_graph_node* pb_graph_node) {
    VTR_ASSERT(!pb_graph_node->is_root());

    int physical_pin_offset = sub_tile->intra_pin_range[relative_cap].at(logical_block).low;

    return {physical_pin_offset + pb_graph_node->pin_num_range.low,
            physical_pin_offset + pb_graph_node->pin_num_range.high};
}

std::vector<int> get_tile_root_pins(t_physical_tile_type_ptr physical_tile) {
    std::vector<int> tile_pins(physical_tile->num_pins);
    std::iota(tile_pins.begin(), tile_pins.end(), 0);

    return tile_pins;
}

std::vector<int> get_flat_tile_pins(t_physical_tile_type_ptr physical_type) {
    std::vector<int> tile_pins;
    tile_pins.reserve(physical_type->num_pins + physical_type->pin_num_to_pb_pin.size());

    for (int pin_num = 0; pin_num < physical_type->num_pins; pin_num++) {
        tile_pins.push_back(pin_num);
    }

    for (const auto& pin_num_pb_pin_pair : physical_type->pin_num_to_pb_pin) {
        auto pb_pin = pin_num_pb_pin_pair.second;
        if (pb_pin->is_root_block_pin()) {
            continue;
        } else {
            tile_pins.push_back(pin_num_pb_pin_pair.first);
        }
    }

    return tile_pins;
}

std::vector<int> get_physical_pin_src_pins(t_physical_tile_type_ptr physical_type,
                                           t_logical_block_type_ptr logical_block,
                                           int pin_physical_num) {
    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(physical_type,
                                                                          pin_physical_num);
    auto pb_pin = get_pb_pin_from_pin_physical_num(physical_type,
                                                   logical_block,
                                                   pin_physical_num);

    return get_pb_pin_src_pins(physical_type,
                               sub_tile,
                               logical_block,
                               sub_tile_cap,
                               pb_pin);
}

std::vector<int> get_physical_pin_sink_pins(t_physical_tile_type_ptr physical_type,
                                            t_logical_block_type_ptr logical_block,
                                            int pin_physical_num) {
    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(physical_type,
                                                                          pin_physical_num);

    auto pb_pin = get_pb_pin_from_pin_physical_num(physical_type,
                                                   logical_block,
                                                   pin_physical_num);

    return get_pb_pin_sink_pins(physical_type,
                                sub_tile,
                                logical_block,
                                sub_tile_cap,
                                pb_pin);
}

int get_pb_pin_physical_num(t_physical_tile_type_ptr physical_tile,
                            const t_sub_tile* sub_tile,
                            t_logical_block_type_ptr logical_block,
                            int relative_cap,
                            const t_pb_graph_pin* pin) {
    int pin_physical_num = OPEN;
    if (pin->is_root_block_pin()) {
        pin_physical_num = get_physical_pin_at_sub_tile_location(physical_tile,
                                                                 logical_block,
                                                                 sub_tile->capacity.low + relative_cap,
                                                                 pin->pin_count_in_cluster);
    } else {
        int offset = sub_tile->intra_pin_range[relative_cap].at(logical_block).low;
        pin_physical_num = pin->pin_count_in_cluster + offset;
    }

    return pin_physical_num;
}

int get_edge_sw_arch_idx(t_physical_tile_type_ptr physical_tile,
                         t_logical_block_type_ptr logical_block,
                         int from_pin_physical_num,
                         int to_pin_physical_num) {
    const t_pb_graph_pin* from_pb_pin;
    const t_pb_graph_pin* to_pb_pin;
    if (is_pin_on_tile(physical_tile, from_pin_physical_num)) {
        from_pb_pin = get_tile_pin_pb_pin(physical_tile, logical_block, from_pin_physical_num);
    } else {
        from_pb_pin = get_pb_pin_from_pin_physical_num(physical_tile, from_pin_physical_num);
    }

    if (is_pin_on_tile(physical_tile, to_pin_physical_num)) {
        to_pb_pin = get_tile_pin_pb_pin(physical_tile, logical_block, to_pin_physical_num);
    } else {
        to_pb_pin = get_pb_pin_from_pin_physical_num(physical_tile, to_pin_physical_num);
    }

    int edge_idx = from_pb_pin->sink_pin_edge_idx_map.at(to_pb_pin);
    int sw_idx = from_pb_pin->output_edges[edge_idx]->switch_type_idx;

    return sw_idx;
}

const t_pb_graph_node* get_pb_graph_node_from_pin_physical_num(t_physical_tile_type_ptr physical_type,
                                                               int pin_physical_num) {
    VTR_ASSERT(!is_pin_on_tile(physical_type, pin_physical_num));
    t_logical_block_type_ptr logical_block = nullptr;

    logical_block = get_logical_block_from_pin_physical_num(physical_type, pin_physical_num);
    VTR_ASSERT(logical_block != nullptr);

    int pin_logical_num = get_pin_logical_num_from_pin_physical_num(physical_type, pin_physical_num);
    auto pb_graph_pin = logical_block->pin_logical_num_to_pb_pin_mapping.at(pin_logical_num);

    return pb_graph_pin->parent_node;
}

int get_total_num_sub_tile_internal_pins(const t_sub_tile* sub_tile) {
    int num_pins = 0;
    for (auto eq_site : sub_tile->equivalent_sites) {
        num_pins += (int)eq_site->pin_logical_num_to_pb_pin_mapping.size();
    }
    num_pins *= sub_tile->capacity.total();
    return num_pins;
}

int get_tile_pin_max_ptc(t_physical_tile_type_ptr tile, bool is_flat) {
    if (is_flat) {
        return tile->num_pins + (int)tile->pin_num_to_pb_pin.size();
    } else {
        return tile->num_pins;
    }
}

int get_tile_num_internal_pin(t_physical_tile_type_ptr tile) {
    return (int)tile->pin_num_to_pb_pin.size();
}

int get_tile_total_num_pin(t_physical_tile_type_ptr tile) {
    return (int)(tile->num_pins + tile->pin_num_to_pb_pin.size());
}

bool intra_tile_nodes_connected(t_physical_tile_type_ptr physical_type,
                                int pin_physical_num,
                                int sink_physical_num) {
    if (is_pin_on_tile(physical_type, pin_physical_num)) {
        const t_sub_tile* from_sub_tile;
        int from_sub_tile_rel_cap;
        std::tie(from_sub_tile, from_sub_tile_rel_cap) = get_sub_tile_from_pin_physical_num(physical_type, pin_physical_num);
        VTR_ASSERT(from_sub_tile != nullptr && from_sub_tile_rel_cap != OPEN);

        const t_sub_tile* to_sub_tile;
        int to_sub_tile_rel_cap;
        std::tie(to_sub_tile, to_sub_tile_rel_cap) = get_sub_tile_from_class_physical_num(physical_type, sink_physical_num);
        VTR_ASSERT(to_sub_tile != nullptr && to_sub_tile_rel_cap != OPEN);

        return (from_sub_tile_rel_cap == to_sub_tile_rel_cap) && (from_sub_tile == to_sub_tile);

    } else {
        const t_pb_graph_pin* from_pb_graph_pin = get_pb_pin_from_pin_physical_num(physical_type, pin_physical_num);

        auto res = from_pb_graph_pin->connected_sinks_ptc.find(sink_physical_num);

        if (res == from_pb_graph_pin->connected_sinks_ptc.end()) {
            return false;
        } else {
            return true;
        }
    }
}

float get_edge_delay(t_physical_tile_type_ptr physical_type,
                     t_logical_block_type_ptr logical_block,
                     int src_pin_physical_num,
                     int sink_pin_physical_num) {
    const t_pb_graph_pin* src_pb_pin = get_pb_pin_from_pin_physical_num(physical_type,
                                                                        logical_block,
                                                                        src_pin_physical_num);
    const t_pb_graph_pin* sink_pb_pin = get_pb_pin_from_pin_physical_num(physical_type,
                                                                         logical_block,
                                                                         sink_pin_physical_num);

    int edge_idx = src_pb_pin->sink_pin_edge_idx_map.at(sink_pb_pin);
    float delay = src_pb_pin->output_edges[edge_idx]->delay_max;

    return delay;
}

float get_pin_primitive_comb_delay(t_physical_tile_type_ptr physical_type,
                                   t_logical_block_type_ptr logical_block,
                                   int pin_physical_num) {
    const t_pb_graph_pin* pb_pin = get_pb_pin_from_pin_physical_num(physical_type,
                                                                    logical_block,
                                                                    pin_physical_num);
    VTR_ASSERT(pb_pin->is_primitive_pin());

    auto it = std::max_element(pb_pin->pin_timing_del_max.begin(), pb_pin->pin_timing_del_max.end());

    if (it == pb_pin->pin_timing_del_max.end()) {
        return 0.;
    } else {
        return *it;
    }
}

bool classes_in_same_block(t_physical_tile_type_ptr physical_tile,
                           int first_class_ptc_num,
                           int second_class_ptc_num,
                           bool is_flat) {
    // We don't do reachability analysis if flat-routing is not enabled
    if (!is_flat) {
        return true;
    }

    // Two functions are considered to be in the same group if share at least two level of blocks
    const int NUM_SIMILAR_PB_NODE_THRESHOLD = 2;
    auto first_class_pin_list = get_pin_list_from_class_physical_num(physical_tile, first_class_ptc_num);
    auto second_class_pin_list = get_pin_list_from_class_physical_num(physical_tile, second_class_ptc_num);

    auto first_pb_graph_pin = get_pb_pin_from_pin_physical_num(physical_tile, first_class_pin_list[0]);
    auto second_pb_graph_pin = get_pb_pin_from_pin_physical_num(physical_tile, second_class_pin_list[0]);

    std::vector<const t_pb_graph_node*> first_pb_graph_node_chain;
    auto curr_pb_graph_node = first_pb_graph_pin->parent_node;
    while (curr_pb_graph_node != nullptr) {
        first_pb_graph_node_chain.push_back(curr_pb_graph_node);
        curr_pb_graph_node = curr_pb_graph_node->parent_pb_graph_node;
    }

    int num_shared_pb_graph_node = 0;
    curr_pb_graph_node = second_pb_graph_pin->parent_node;
    while (curr_pb_graph_node != nullptr) {
        auto find_res = std::find(first_pb_graph_node_chain.begin(), first_pb_graph_node_chain.end(), curr_pb_graph_node);
        if (find_res != first_pb_graph_node_chain.end()) {
            num_shared_pb_graph_node++;
            if (num_shared_pb_graph_node >= NUM_SIMILAR_PB_NODE_THRESHOLD)
                break;
        }
        curr_pb_graph_node = curr_pb_graph_node->parent_pb_graph_node;
    }

    if (num_shared_pb_graph_node < NUM_SIMILAR_PB_NODE_THRESHOLD) {
        return false;
    } else {
        return true;
    }
}

std::map<int, int> get_sink_choking_points(t_physical_tile_type_ptr physical_tile,
                                           int sink_ptc_num,
                                           const std::vector<int>& grp) {
    // There may be a choking point if there are at least two sinks in the group!
    VTR_ASSERT(grp.size() > 1);
    std::map<int, int> choking_point;
    const t_sub_tile* sub_tile;
    int sub_tile_rel_cap;
    std::tie(sub_tile, sub_tile_rel_cap) = get_sub_tile_from_class_physical_num(physical_tile, sink_ptc_num);
    auto logical_block = get_logical_block_from_class_physical_num(physical_tile, sink_ptc_num);
    auto sink_hierarchical_parents_node = get_sink_hierarchical_parents(physical_tile, sink_ptc_num);
    for (auto pb_graph_node : sink_hierarchical_parents_node) {
        // We assume that choke points do not occur on the root-level pb_graph_node due to having a relatively populated crossbar
        if (pb_graph_node->is_root()) {
            continue;
        }

        auto pb_pins = get_pb_graph_node_pins(physical_tile,
                                              sub_tile,
                                              logical_block,
                                              sub_tile_rel_cap,
                                              pb_graph_node);
        std::unordered_map<int, int> reachability_inf;
        bool has_unique_val = false;
        int prev_val = -1;
        // Iterate over all of the block's IPINs, and check how many sinks in the group are reachable by each pin.
        // If there are pins that the number of reachable sinks are different, we consider those pins as choke points
        for (int pin_num = pb_pins.low; pin_num <= pb_pins.high; pin_num++) {
            // Only choke points on IPINs are analyzed
            if (get_pin_type_from_pin_physical_num(physical_tile, pin_num) == e_pin_type::RECEIVER) {
                int num_reachable_sinks = get_num_reachable_sinks(physical_tile,
                                                                  pin_num,
                                                                  sink_ptc_num,
                                                                  grp);

                if (num_reachable_sinks != 0) {
                    if (prev_val == -1) {
                        prev_val = num_reachable_sinks;
                    } else {
                        if (prev_val != num_reachable_sinks) {
                            has_unique_val = true;
                        }
                    }
                    if (num_reachable_sinks > 1) {
                        reachability_inf.insert(std::make_pair(pin_num, num_reachable_sinks));
                    }
                }
            }
        }

        // Choke point happens if there is a discrepancy between the number of sinks reachable by the pins
        if (has_unique_val) {
            for (const auto& reachability_it : reachability_inf) {
                choking_point.insert(std::make_pair(reachability_it.first, reachability_it.second));
            }
        }
    }

    return choking_point;
}
/* */
