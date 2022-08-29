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
 *   - capacity_instance: sub tile instance containing this physical pin.
 *                        Each sub tile has a capacity field, which determines how many of its
 *                        instances are present in the belonging physical tile.
 *                        E.g.:
 *                          - The sub tile BUFG has a capacity of 4 within its belonging physical tile CLOCK_TILE.
 *                          - The capacity instance of a pin in the CLOCK_TILE identifies which of the 4 instances
 *                            the pin belongs to.
 *   - port_index: Each sub tile has a set of ports with a variable number of pins. The port_index field identifies
 *                 which port the physical pin belongs to.
 *   - pin_index_in_port: Given that ports can have multiple pins, we need also a field to identify which one of the
 *                        multiple pins of the port corresponds to the physical pin.
 *
 */
struct t_pin_inst_port {
    int sub_tile_index;    // Sub Tile index
    int capacity_instance; // within capacity
    int port_index;        // Port index
    int pin_index_in_port; // Pin's index within the port
};

/******************** Subroutine declarations and definition ****************************/

static std::tuple<int, int, int, int> get_pin_index_for_inst(t_physical_tile_type_ptr type, int pin_index, bool is_flat);

// #TODO: remove the default value for is_flat
static t_pin_inst_port block_type_pin_index_to_pin_inst(t_physical_tile_type_ptr type, int pin_index, bool is_flat = false);

static int get_sub_tile_num_internal_classes(const t_sub_tile* sub_tile);

static int get_sub_tile_physical_class_num_offset(t_physical_tile_type_ptr physical_tile,
                                                  const t_sub_tile* curr_sub_tile);

static int get_sub_tile_inst_physical_class_num_offset(t_physical_tile_type_ptr physical_tile,
                                                       const t_sub_tile* curr_sub_tile,
                                                       const int curr_relative_cap);

static int get_class_logical_num_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int physical_class_num);

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

static int get_class_physical_num_from_class_logical_num(t_physical_tile_type_ptr physical_tile,
                                                         const t_sub_tile* curr_sub_tile,
                                                         t_logical_block_type_ptr curr_logical_block,
                                                         int curr_relative_cap,
                                                         int logical_class_num);

static std::vector<const t_pb_graph_pin*> get_pb_graph_node_pins(const t_pb_graph_node* pb_graph_node);

static std::vector<int> get_pb_pin_driving_pins(t_physical_tile_type_ptr physical_type,
                                                const t_sub_tile* sub_tile,
                                                t_logical_block_type_ptr logical_block,
                                                int relative_cap,
                                                const t_pb_graph_pin* pin);

//static std::vector<const t_pb_graph_node*> get_child_pb_graph_node_mode(const t_pb_graph_node* parent_node, int mode_num);

static std::tuple<int, int, int, int> get_pin_index_for_inst(t_physical_tile_type_ptr type, int pin_index, bool is_flat) {
    int max_ptc = get_tile_ipin_opin_max_ptc(type, is_flat);
    VTR_ASSERT(pin_index < max_ptc);

    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    int pin_inst_num;
    int logical_block_idx;

    bool on_tile_pin = is_pin_on_tile(type, pin_index);

    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(type, pin_index);

    if (on_tile_pin) {
        int pin_offset = 0;
        for (int sub_tile_idx = 0; sub_tile_idx < sub_tile->index; sub_tile_idx++) {
            pin_offset += type->sub_tiles[sub_tile_idx].num_phy_pins;
        }
        int pins_per_inst = sub_tile->num_phy_pins / sub_tile->capacity.total();
        pin_inst_num = (pin_index - pin_offset) % pins_per_inst;
    } else {
        int pin_offset = get_sub_tile_inst_physical_pin_num_offset(type, sub_tile, sub_tile_cap);
        int pins_per_inst = get_total_num_sub_tile_internal_pins(sub_tile) / sub_tile->capacity.total();
        pin_inst_num = (pin_index - pin_offset) % pins_per_inst;
    }

    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(type, pin_index);
    VTR_ASSERT(sub_tile_cap != -1);

    if (on_tile_pin) {
        logical_block_idx = -1;
    } else {
        auto logical_block = get_logical_block_from_pin_physical_num(type, pin_index);
        VTR_ASSERT(logical_block != nullptr);
        logical_block_idx = logical_block->index;
    }

    return std::make_tuple(pin_inst_num, sub_tile_cap, sub_tile->index, logical_block_idx);
}

static t_pin_inst_port block_type_pin_index_to_pin_inst(t_physical_tile_type_ptr type, int pin_index, bool is_flat) {
    int sub_tile_index, inst_num, logical_num;
    std::tie<int, int, int, int>(pin_index, inst_num, sub_tile_index, logical_num) = get_pin_index_for_inst(type, pin_index, is_flat);

    t_pin_inst_port pin_inst_port;
    pin_inst_port.sub_tile_index = sub_tile_index;
    pin_inst_port.capacity_instance = inst_num;
    pin_inst_port.port_index = OPEN;
    pin_inst_port.pin_index_in_port = OPEN;

    for (auto const& port : type->sub_tiles[sub_tile_index].ports) {
        if (pin_index >= port.absolute_first_pin_index && pin_index < port.absolute_first_pin_index + port.num_pins) {
            pin_inst_port.port_index = port.index;
            pin_inst_port.pin_index_in_port = pin_index - port.absolute_first_pin_index;
            break;
        }
    }
    return pin_inst_port;
}

static int get_sub_tile_num_internal_classes(const t_sub_tile* sub_tile) {
    int num_classes = 0;
    for (auto eq_site : sub_tile->equivalent_sites) {
        num_classes += (int)eq_site->logical_class_inf.size();
    }
    num_classes *= sub_tile->capacity.total();

    return num_classes;
}

static int get_sub_tile_physical_class_num_offset(t_physical_tile_type_ptr physical_tile,
                                                  const t_sub_tile* curr_sub_tile) {
    int offset = (int)physical_tile->class_inf.size();
    for (const auto& tmp_sub_tile : physical_tile->sub_tiles) {
        if (&tmp_sub_tile == curr_sub_tile)
            break;
        else
            offset += get_total_num_sub_tile_internal_classes(&tmp_sub_tile);
    }

    return offset;
}

static int get_sub_tile_inst_physical_class_num_offset(t_physical_tile_type_ptr physical_tile,
                                                       const t_sub_tile* curr_sub_tile,
                                                       const int curr_relative_cap) {
    int offset = get_sub_tile_physical_class_num_offset(physical_tile, curr_sub_tile);
    int sub_tile_inst_num_classes = get_total_num_sub_tile_internal_classes(curr_sub_tile) / curr_sub_tile->capacity.total();

    for (int sub_tile_cap = 0; sub_tile_cap < curr_relative_cap; sub_tile_cap++) {
        offset += sub_tile_inst_num_classes;
    }
    return offset;
}

static int get_class_logical_num_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int physical_class_num) {
    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_class_physical_num(physical_tile, physical_class_num);
    VTR_ASSERT(sub_tile_cap != -1);
    auto logical_block = get_logical_block_from_class_physical_num(physical_tile, physical_class_num);
    int start_physical_class_num = get_class_physical_num_from_class_logical_num(physical_tile,
                                                                                 sub_tile,
                                                                                 logical_block,
                                                                                 sub_tile_cap,
                                                                                 0);
    VTR_ASSERT(start_physical_class_num != -1);
    return physical_class_num - start_physical_class_num;
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
    VTR_ASSERT(physical_num >= physical_tile->num_pins);
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

static int get_class_physical_num_from_class_logical_num(t_physical_tile_type_ptr physical_tile,
                                                         const t_sub_tile* curr_sub_tile,
                                                         t_logical_block_type_ptr curr_logical_block,
                                                         int curr_relative_cap,
                                                         int logical_class_num) {
    int num_seen_class = (int)physical_tile->class_inf.size();

    // Add the number of classes in the previous sub_tiles
    for (int sub_tile_idx = 0; sub_tile_idx < curr_sub_tile->index; sub_tile_idx++) {
        num_seen_class += get_sub_tile_num_internal_classes(&physical_tile->sub_tiles[sub_tile_idx]);
    }

    // Add the number of classes in the previous instances (capacity)
    if (curr_relative_cap != 0) {
        for (auto logical_block : curr_sub_tile->equivalent_sites) {
            num_seen_class += ((int)(logical_block->logical_class_inf.size()) * curr_relative_cap);
        }
    }

    // Add the number of classes in the previous logical block which can be mapped to the current sub tile
    for (auto logical_block : curr_sub_tile->equivalent_sites) {
        if (logical_block == curr_logical_block)
            break;

        num_seen_class += ((int)logical_block->logical_class_inf.size());
    }

    // Add the offset of the class in the current logical block
    num_seen_class += logical_class_num;

    return num_seen_class;
}

static std::vector<const t_pb_graph_pin*> get_pb_graph_node_pins(const t_pb_graph_node* pb_graph_node) {
    std::vector<const t_pb_graph_pin*> pins(pb_graph_node->num_pins());
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
                const t_pb_graph_pin* pin = &(pb_pins[port_idx][pin_idx]);
                pins[num_added_pins] = pin;
                num_added_pins++;
            }
        }
    }
    VTR_ASSERT(num_added_pins == pb_graph_node->num_pins());
    return pins;
}

static std::vector<int> get_pb_pin_driving_pins(t_physical_tile_type_ptr physical_type,
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
                driving_pins.push_back(get_pb_pin_physical_num(sub_tile,
                                                               logical_block,
                                                               relative_cap,
                                                               conn_pin));
            }
        }
    }

    return driving_pins;
}

//static std::vector<const t_pb_graph_node*> get_child_pb_graph_node_mode(const t_pb_graph_node* parent_node, int mode_num) {
//    int num_child_pb_nodes = 0;
//    std::vector<const t_pb_graph_node*> child_pb_nodes;
//
//    if(parent_node->is_primitive())
//        return child_pb_nodes;
//
//    const t_mode& mode = parent_node->pb_type->modes[mode_num];
//    for(int pb_type_idx = 0; pb_type_idx < mode.num_pb_type_children; pb_type_idx++) {
//        for(int pb_idx = 0; pb_idx < mode.pb_type_children[pb_type_idx].num_pb; pb_idx++) {
//            num_child_pb_nodes++;
//        }
//    }
//    child_pb_nodes.resize(num_child_pb_nodes);
//    int num_added_pb_nodes = 0;
//    for(int pb_type_idx = 0; pb_type_idx < mode.num_pb_type_children; pb_type_idx++) {
//        for(int pb_idx = 0; pb_idx < mode.pb_type_children[pb_type_idx].num_pb; pb_idx++) {
//            child_pb_nodes[num_added_pb_nodes] = &parent_node->child_pb_graph_nodes[mode_num][pb_type_idx][pb_idx];
//            num_added_pb_nodes++;
//        }
//    }
//    VTR_ASSERT(num_added_pb_nodes == num_child_pb_nodes);
//    return child_pb_nodes;
//}

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
    int max_ptc = get_tile_ipin_opin_max_ptc(type, is_flat);
    VTR_ASSERT(pin_physical_num < max_ptc);
    int pin_index;
    std::string pin_name = type->name;

    int sub_tile_index, inst_num, logical_num;
    std::tie<int, int, int, int>(pin_index, inst_num, sub_tile_index, logical_num) = get_pin_index_for_inst(type, pin_physical_num, is_flat);

    if (type->sub_tiles[sub_tile_index].capacity.total() > 1) {
        pin_name += "[" + std::to_string(inst_num) + "]";
    }

    pin_name += ".";

    if (!is_pin_on_tile(type, pin_physical_num)) {
        pin_name += "[" + std::to_string(logical_num) + "]";
        pin_name += ".";

        auto pb_pin = get_pb_pin_from_pin_physical_num(type, pin_physical_num);
        auto pb_pin_port = pb_pin->port;
        pin_name += pb_pin_port->name;

        pin_name += "[" + std::to_string(pb_pin->pin_number) + "]";

        return pin_name;
    } else {
        VTR_ASSERT(is_pin_on_tile(type, pin_physical_num));
        for (auto const& port : type->sub_tiles[sub_tile_index].ports) {
            if (pin_index >= port.absolute_first_pin_index && pin_index < port.absolute_first_pin_index + port.num_pins) {
                //This port contains the desired pin index
                int index_in_port = pin_index - port.absolute_first_pin_index;
                pin_name += port.name;
                pin_name += "[" + std::to_string(index_in_port) + "]";
                return pin_name;
            }
        }
    }

    return "<UNKOWN>";
}

std::vector<std::string> block_type_class_index_to_pin_names(t_physical_tile_type_ptr type, int class_index) {
    VTR_ASSERT(class_index < (int)type->class_inf.size());

    auto class_inf = type->class_inf[class_index];

    std::vector<t_pin_inst_port> pin_info;
    for (int ipin = 0; ipin < class_inf.num_pins; ++ipin) {
        int pin_index = class_inf.pinlist[ipin];
        pin_info.push_back(block_type_pin_index_to_pin_inst(type, pin_index));
    }

    auto cmp = [](const t_pin_inst_port& lhs, const t_pin_inst_port& rhs) {
        return std::tie(lhs.capacity_instance, lhs.port_index, lhs.pin_index_in_port)
               < std::tie(rhs.capacity_instance, rhs.port_index, rhs.pin_index_in_port);
    };

    //Ensure all the pins are in order
    std::sort(pin_info.begin(), pin_info.end(), cmp);

    //Determine ranges for each capacity instance and port pair
    std::map<std::tuple<int, int, int>, std::pair<int, int>> pin_ranges;
    for (const auto& pin_inf : pin_info) {
        auto key = std::make_tuple(pin_inf.sub_tile_index, pin_inf.capacity_instance, pin_inf.port_index);
        if (!pin_ranges.count(key)) {
            pin_ranges[key].first = pin_inf.pin_index_in_port;
            pin_ranges[key].second = pin_inf.pin_index_in_port;
        } else {
            VTR_ASSERT(pin_ranges[key].second == pin_inf.pin_index_in_port - 1);
            pin_ranges[key].second = pin_inf.pin_index_in_port;
        }
    }

    //Format pin ranges
    std::vector<std::string> pin_names;
    for (auto kv : pin_ranges) {
        auto type_port = kv.first;
        auto pins = kv.second;

        int isub_tile, icapacity, iport;
        std::tie<int, int, int>(isub_tile, icapacity, iport) = type_port;

        int ipin_start = pins.first;
        int ipin_end = pins.second;

        auto& sub_tile = type->sub_tiles[isub_tile];

        std::string pin_name;
        if (ipin_start == ipin_end) {
            pin_name = vtr::string_fmt("%s[%d].%s[%d]",
                                       type->name,
                                       icapacity,
                                       sub_tile.ports[iport].name,
                                       ipin_start);
        } else {
            pin_name = vtr::string_fmt("%s[%d].%s[%d:%d]",
                                       type->name,
                                       icapacity,
                                       sub_tile.ports[iport].name,
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

t_logical_block_type_ptr get_logical_block_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int physical_class_num) {
    VTR_ASSERT(physical_class_num >= (int)physical_tile->class_inf.size());

    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_class_physical_num(physical_tile, physical_class_num);
    VTR_ASSERT(sub_tile_cap != -1);
    int class_num_offset = get_sub_tile_inst_physical_class_num_offset(physical_tile, sub_tile, sub_tile_cap);
    VTR_ASSERT(physical_class_num >= class_num_offset);

    for (auto tmp_logical_block : sub_tile->equivalent_sites) {
        if (physical_class_num < (class_num_offset + (int)tmp_logical_block->logical_class_inf.size())) {
            return tmp_logical_block;
        } else {
            class_num_offset += (int)tmp_logical_block->logical_class_inf.size();
        }
    }

    return nullptr;
}

std::vector<int> get_pin_list_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[class_physical_num];
        return pin_class.pinlist;
    } else {
        const t_class& pin_class = physical_tile->internal_class_inf.at(class_physical_num);
        return pin_class.pinlist;
    }
}

PortEquivalence get_port_equivalency_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[class_physical_num];
        return pin_class.equivalence;
    } else {
        const t_class& pin_class = physical_tile->internal_class_inf.at(class_physical_num);
        return pin_class.equivalence;
    }
}

e_pin_type get_class_type_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        return physical_tile->class_inf[class_physical_num].type;

    } else {
        const t_class& pin_class = physical_tile->internal_class_inf.at(class_physical_num);
        return pin_class.type;
    }
}

int get_class_num_pins_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    if (is_class_on_tile(physical_tile, class_physical_num)) {
        return physical_tile->class_inf[class_physical_num].num_pins;

    } else {
        const t_class& pin_class = physical_tile->internal_class_inf.at(class_physical_num);
        return pin_class.num_pins;
    }
}

int get_pin_physical_num_from_class_physical_num(t_physical_tile_type_ptr physical_tile, int physical_class_num, int pin_logical_num) {
    bool is_on_tile = is_class_on_tile(physical_tile, physical_class_num);
    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    int pin_physical_num = -1;

    if (is_on_tile) {
        pin_physical_num = pin_logical_num;

    } else {
        std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_class_physical_num(physical_tile, physical_class_num);
        VTR_ASSERT(sub_tile_cap != -1);
        auto logical_block = get_logical_block_from_class_physical_num(physical_tile, physical_class_num);
        auto pb_pin = logical_block->pin_logical_num_to_pb_pin_mapping.at(pin_logical_num);
        pin_physical_num = get_pb_pin_physical_num(sub_tile,
                                                   logical_block,
                                                   sub_tile_cap,
                                                   pb_pin);
    }

    return pin_physical_num;
}

bool is_class_on_tile(t_physical_tile_type_ptr physical_tile, int class_physical_num) {
    return (class_physical_num < (int)physical_tile->class_inf.size());
}

std::unordered_map<int, const t_class*> get_pb_graph_node_num_class_pairs(t_physical_tile_type_ptr physical_tile,
                                                                          const t_sub_tile* sub_tile,
                                                                          t_logical_block_type_ptr logical_block,
                                                                          int sub_tile_relative_cap,
                                                                          const t_pb_graph_node* pb_graph_node) {
    std::unordered_set<int> seen_logical_class_num;
    std::unordered_map<int, const t_class*> classes_map;
    const std::unordered_map<const t_pb_graph_pin*, int>& pb_pin_class_map = logical_block->pb_pin_to_class_logical_num_mapping;

    std::vector<const t_pb_graph_pin*> pb_pins = get_pb_graph_node_pins(pb_graph_node);

    for (auto pin : pb_pins) {
        int class_logical_num = pb_pin_class_map.at(pin);
        bool is_added;
        std::tie(std::ignore, is_added) = seen_logical_class_num.emplace(class_logical_num);
        if (is_added) {
            int pin_physical_num = get_pb_pin_physical_num(sub_tile,
                                                           logical_block,
                                                           sub_tile_relative_cap,
                                                           pin);
            int class_physical_num = physical_tile->internal_pin_class.at(pin_physical_num);
            auto& pin_class = physical_tile->internal_class_inf.at(class_physical_num);
            classes_map.insert(std::make_pair(class_physical_num, &pin_class));
        }
    }

    return classes_map;
}

int get_total_num_sub_tile_internal_classes(const t_sub_tile* sub_tile) {
    int num_classes = 0;
    for (auto eq_site : sub_tile->equivalent_sites) {
        num_classes += (int)eq_site->logical_class_inf.size();
    }
    num_classes *= sub_tile->capacity.total();
    return num_classes;
}

int get_total_num_tile_internal_classes(t_physical_tile_type_ptr physical_tile) {
    int num_classes = 0;
    for (const t_sub_tile& sub_tile : physical_tile->sub_tiles) {
        num_classes += get_total_num_sub_tile_internal_classes(&sub_tile);
    }

    return num_classes;
}

int get_tile_class_max_ptc(t_physical_tile_type_ptr tile, bool is_flat) {
    if (is_flat) {
        return (int)tile->class_inf.size() + get_total_num_tile_internal_classes(tile);
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

    for (auto starting_idx : sub_tile->starting_internal_pin_idx[sub_tile_cap]) {
        if (physical_num >= starting_idx.second) {
            logical_block = starting_idx.first;
        }
    }

    return logical_block;
}

const t_pb_graph_pin* get_pb_pin_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int physical_num) {
    VTR_ASSERT(physical_num >= physical_tile->num_pins);

    auto logical_block = get_logical_block_from_pin_physical_num(physical_tile, physical_num);
    VTR_ASSERT(logical_block != nullptr);
    int logical_num = get_pin_logical_num_from_pin_physical_num(physical_tile, physical_num);
    return logical_block->pin_logical_num_to_pb_pin_mapping.at(logical_num);
}

PortEquivalence get_port_equivalency_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int pin_physical_num) {
    if (is_pin_on_tile(physical_tile, pin_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[physical_tile->pin_class[pin_physical_num]];
        return pin_class.equivalence;
    } else {
        const t_class& pin_class = physical_tile->internal_class_inf.at(physical_tile->internal_pin_class.at(pin_physical_num));
        return pin_class.equivalence;
    }
}
e_pin_type get_pin_type_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int pin_physical_num) {
    if (is_pin_on_tile(physical_tile, pin_physical_num)) {
        const t_class& pin_class = physical_tile->class_inf[physical_tile->pin_class[pin_physical_num]];
        return pin_class.type;
    } else {
        const t_class& pin_class = physical_tile->internal_class_inf.at(physical_tile->internal_pin_class.at(pin_physical_num));
        return pin_class.type;
    }
}

int get_class_num_from_pin_physical_num(t_physical_tile_type_ptr physical_tile, int pin_physical_num) {
    if (is_pin_on_tile(physical_tile, pin_physical_num)) {
        return physical_tile->pin_class[pin_physical_num];
    } else {
        return physical_tile->internal_pin_class.at(pin_physical_num);
    }
}

bool is_pin_on_tile(t_physical_tile_type_ptr physical_tile, int physical_num) {
    return (physical_num < physical_tile->num_pins);
}

std::vector<int> get_pb_graph_node_pins(const t_sub_tile* sub_tile,
                                        t_logical_block_type_ptr logical_block,
                                        int relative_cap,
                                        const t_pb_graph_node* pb_graph_node) {
    std::vector<int> pins_num;
    auto pins = get_pb_graph_node_pins(pb_graph_node);
    for (auto pin : pins) {
        int pin_num = get_pb_pin_physical_num(sub_tile,
                                              logical_block,
                                              relative_cap,
                                              pin);
        pins_num.push_back(pin_num);
    }
    return pins_num;
}

std::vector<int> get_physical_pin_driving_pins(t_physical_tile_type_ptr physical_type,
                                               t_logical_block_type_ptr logical_block,
                                               int pin_physical_num) {
    const t_sub_tile* sub_tile;
    int sub_tile_cap;
    std::tie(sub_tile, sub_tile_cap) = get_sub_tile_from_pin_physical_num(physical_type,
                                                                          pin_physical_num);

    if (is_pin_on_tile(physical_type, pin_physical_num)) {
        auto direct_map = (physical_type->tile_block_pin_directs_map).at(logical_block->index).at(sub_tile->index);
        int sub_tile_inst_num_pins = sub_tile->num_phy_pins / sub_tile->capacity.total();
        pin_physical_num -= (sub_tile_inst_num_pins * sub_tile_cap);
        auto result = direct_map.find(t_physical_pin(pin_physical_num));
        if (result == direct_map.inverse_end()) {
            archfpga_throw(__FILE__, __LINE__,
                           "Couldn't find the corresponding logical sub tile pin of the physical block pin %d."
                           "Physical Tile Type: %s, Logical Block Type: %s.\n",
                           pin_physical_num, physical_type->name, logical_block->name);
        }
        int pin_logical_num = result->second.pin;
        auto pb_pin = logical_block->pin_logical_num_to_pb_pin_mapping.at(pin_logical_num);
        return get_pb_pin_driving_pins(physical_type,
                                       sub_tile,
                                       logical_block,
                                       sub_tile_cap,
                                       pb_pin);

    } else {
        auto pb_pin = get_pb_pin_from_pin_physical_num(physical_type, pin_physical_num);
        return get_pb_pin_driving_pins(physical_type,
                                       sub_tile,
                                       logical_block,
                                       sub_tile_cap,
                                       pb_pin);
    }
}

int get_pb_pin_physical_num(const t_sub_tile* sub_tile,
                            t_logical_block_type_ptr logical_block,
                            int relative_cap,
                            const t_pb_graph_pin* pin) {
    int pin_ptc = OPEN;
    int offset = sub_tile->starting_internal_pin_idx[relative_cap].at(logical_block);
    pin_ptc = pin->pin_count_in_cluster + offset;

    return pin_ptc;
}

int get_total_num_sub_tile_internal_pins(const t_sub_tile* sub_tile) {
    int num_pins = 0;
    for (auto eq_site : sub_tile->equivalent_sites) {
        num_pins += (int)eq_site->pin_logical_num_to_pb_pin_mapping.size();
    }
    num_pins *= sub_tile->capacity.total();
    return num_pins;
}

int get_total_num_tile_internal_pins(t_physical_tile_type_ptr tile) {
    int num_pins = 0;
    for (const t_sub_tile& sub_tile : tile->sub_tiles) {
        num_pins += get_total_num_sub_tile_internal_pins(&sub_tile);
    }

    return num_pins;
}

int get_tile_ipin_opin_max_ptc(t_physical_tile_type_ptr tile, bool is_flat) {
    if (is_flat) {
        return tile->num_pins + get_total_num_tile_internal_pins(tile);
    } else {
        return tile->num_pins;
    }
}
/* */
