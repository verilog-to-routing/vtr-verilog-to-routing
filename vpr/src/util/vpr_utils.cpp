#include <cstring>
#include <unordered_set>
#include <regex>
#include <algorithm>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_random.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "physical_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "cluster_placement.h"
#include "place_macro.h"
#include "string.h"
#include "pack_types.h"
#include "device_grid.h"

/* This module contains subroutines that are used in several unrelated parts *
 * of VPR.  They are VPR-specific utility routines.                          */

/* This defines the maximum string length that could be parsed by functions  *
 * in vpr_utils.                                                             */
#define MAX_STRING_LEN 128

/******************** File-scope variables declarations **********************/

/* These three mappings are needed since there are two different netlist *
 * conventions - in the cluster level, ports and port pins are used      *
 * while in the post-pack level, block pins are used. The reason block   *
 * type is used instead of blocks is to save memories.                   */

/* f_port_pin_to_block_pin array allows us to quickly find what block                   *
 * pin a port pin corresponds to.                                                       *
 * [0...device_ctx.physical_tile_types.size()-1][0...num_ports-1][0...num_port_pins-1]  */
static int*** f_blk_pin_from_port_pin = nullptr;

//Regular expressions used to determine register and logic primitives
//TODO: Make this set-able from command-line?
const std::regex REGISTER_MODEL_REGEX("(.subckt\\s+)?.*(latch|dff).*", std::regex::icase);
const std::regex LOGIC_MODEL_REGEX("(.subckt\\s+)?.*(lut|names|lcell).*", std::regex::icase);

/******************** Subroutine declarations ********************************/

/* Allocates and loads blk_pin_from_port_pin array.                      *
 * The arrays are freed in free_placement_structs()                      */
static void alloc_and_load_blk_pin_from_port_pin();

/* Go through all the ports in all the blocks to find the port that has the same   *
 * name as port_name and belongs to the block type that has the name pb_type_name. *
 * Then, check that whether start_pin_index and end_pin_index are specified. If    *
 * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
 * Otherwise, mark down all the pins in that port.                                 */
static void mark_direct_of_ports(int idirect, int direct_type, char* pb_type_name, char* port_name, int end_pin_index, int start_pin_index, char* src_string, int line, int** idirect_from_blk_pin, int** direct_type_from_blk_pin);

/* Mark the pin entry in idirect_from_blk_pin with idirect and the pin entry in    *
 * direct_type_from_blk_pin with direct_type from start_pin_index to               *
 * end_pin_index.                                                                  */
static void mark_direct_of_pins(int start_pin_index, int end_pin_index, int itype, int iport, int** idirect_from_blk_pin, int idirect, int** direct_type_from_blk_pin, int direct_type, int line, char* src_string);

static void load_pb_graph_pin_lookup_from_index_rec(t_pb_graph_pin** pb_graph_pin_lookup_from_index, t_pb_graph_node* pb_graph_node);

static void load_pin_id_to_pb_mapping_rec(t_pb* cur_pb, t_pb** pin_id_to_pb_mapping);

static std::vector<int> find_connected_internal_clb_sink_pins(ClusterBlockId clb, int pb_pin);
static void find_connected_internal_clb_sink_pins_recurr(ClusterBlockId clb, int pb_pin, std::vector<int>& connected_sink_pb_pins);
static AtomPinId find_atom_pin_for_pb_route_id(ClusterBlockId clb, int pb_route_id, const IntraLbPbPinLookup& pb_gpin_lookup);

static bool block_type_contains_blif_model(t_logical_block_type_ptr type, const std::regex& blif_model_regex);
static bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::regex& blif_model_regex);

/******************** Subroutine definitions *********************************/

const t_model* find_model(const t_model* models, const std::string& name, bool required) {
    for (const t_model* model = models; model != nullptr; model = model->next) {
        if (name == model->name) {
            return model;
        }
    }

    if (required) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find architecture modedl '%s'\n", name.c_str());
    }

    return nullptr;
}

const t_model_ports* find_model_port(const t_model* model, const std::string& name, bool required) {
    VTR_ASSERT(model);

    for (const t_model_ports* model_ports : {model->inputs, model->outputs}) {
        for (const t_model_ports* port = model_ports; port != nullptr; port = port->next) {
            if (port->name == name) {
                return port;
            }
        }
    }

    if (required) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find port '%s; on architecture modedl '%s'\n", name.c_str(), model->name);
    }

    return nullptr;
}

/**
 * print tabs given number of tabs to file
 */
void print_tabs(FILE* fpout, int num_tab) {
    int i;
    for (i = 0; i < num_tab; i++) {
        fprintf(fpout, "\t");
    }
}

/* Points the place_ctx.grid_blocks structure back to the blocks list */
void sync_grid_to_blocks() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& device_grid = device_ctx.grid;

    /* Reset usage and allocate blocks list if needed */
    auto& grid_blocks = place_ctx.grid_blocks;
    grid_blocks.resize({device_grid.width(), device_grid.height()});
    for (size_t x = 0; x < device_grid.width(); ++x) {
        for (size_t y = 0; y < device_grid.height(); ++y) {
            auto& grid_block = grid_blocks[x][y];
            grid_block.blocks.resize(device_ctx.grid[x][y].type->capacity);

            for (int z = 0; z < device_ctx.grid[x][y].type->capacity; ++z) {
                grid_block.blocks[z] = EMPTY_BLOCK_ID;
            }
        }
    }

    /* Go through each block */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        int blk_x = place_ctx.block_locs[blk_id].loc.x;
        int blk_y = place_ctx.block_locs[blk_id].loc.y;
        int blk_z = place_ctx.block_locs[blk_id].loc.z;

        auto type = physical_tile_type(blk_id);

        /* Check range of block coords */
        if (blk_x < 0 || blk_y < 0
            || (blk_x + type->width - 1) > int(device_ctx.grid.width() - 1)
            || (blk_y + type->height - 1) > int(device_ctx.grid.height() - 1)
            || blk_z < 0 || blk_z > (type->capacity)) {
            VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Block %zu is at invalid location (%d, %d, %d).\n",
                            size_t(blk_id), blk_x, blk_y, blk_z);
        }

        /* Check types match */
        if (type != device_ctx.grid[blk_x][blk_y].type) {
            VPR_FATAL_ERROR(VPR_ERROR_PLACE, "A block is in a grid location (%d x %d) with a conflicting types '%s' and '%s' .\n",
                            blk_x, blk_y,
                            type->name,
                            device_ctx.grid[blk_x][blk_y].type->name);
        }

        /* Check already in use */
        if ((EMPTY_BLOCK_ID != place_ctx.grid_blocks[blk_x][blk_y].blocks[blk_z])
            && (INVALID_BLOCK_ID != place_ctx.grid_blocks[blk_x][blk_y].blocks[blk_z])) {
            VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Location (%d, %d, %d) is used more than once.\n",
                            blk_x, blk_y, blk_z);
        }

        if (device_ctx.grid[blk_x][blk_y].width_offset != 0 || device_ctx.grid[blk_x][blk_y].height_offset != 0) {
            VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Large block not aligned in placment for cluster_ctx.blocks %lu at (%d, %d, %d).",
                            size_t(blk_id), blk_x, blk_y, blk_z);
        }

        /* Set the block */
        for (int width = 0; width < type->width; ++width) {
            for (int height = 0; height < type->height; ++height) {
                place_ctx.grid_blocks[blk_x + width][blk_y + height].blocks[blk_z] = blk_id;
                place_ctx.grid_blocks[blk_x + width][blk_y + height].usage++;
                VTR_ASSERT(device_ctx.grid[blk_x + width][blk_y + height].width_offset == width);
                VTR_ASSERT(device_ctx.grid[blk_x + width][blk_y + height].height_offset == height);
            }
        }
    }
}

std::string block_type_pin_index_to_name(t_physical_tile_type_ptr type, int pin_index) {
    VTR_ASSERT(pin_index < type->num_pins);

    std::string pin_name = type->name;

    if (type->capacity > 1) {
        int pins_per_inst = type->num_pins / type->capacity;
        int inst_num = pin_index / pins_per_inst;
        pin_index %= pins_per_inst;

        pin_name += "[" + std::to_string(inst_num) + "]";
    }

    pin_name += ".";

    int curr_index = 0;
    for (auto const& port : type->ports) {
        if (curr_index + port.num_pins > pin_index) {
            //This port contains the desired pin index
            int index_in_port = pin_index - curr_index;
            pin_name += port.name;
            pin_name += "[" + std::to_string(index_in_port) + "]";
            return pin_name;
        }

        curr_index += port.num_pins;
    }

    return "<UNKOWN>";
}

std::vector<std::string> block_type_class_index_to_pin_names(t_physical_tile_type_ptr type, int class_index) {
    VTR_ASSERT(class_index < type->num_class);

    //TODO: unsure if classes are modulo capacity or not... so this may be unnessesary
    int classes_per_inst = type->num_class / type->capacity;
    class_index %= classes_per_inst;

    t_class& class_inf = type->class_inf[class_index];

    std::vector<std::string> pin_names;
    for (int ipin = 0; ipin < class_inf.num_pins; ++ipin) {
        pin_names.push_back(block_type_pin_index_to_name(type, class_inf.pinlist[ipin]));
    }

    return pin_names;
}

std::string rr_node_arch_name(int inode) {
    auto& device_ctx = g_vpr_ctx.device();

    const t_rr_node& rr_node = device_ctx.rr_nodes[inode];

    std::string rr_node_arch_name;
    if (rr_node.type() == OPIN || rr_node.type() == IPIN) {
        //Pin names
        auto type = device_ctx.grid[rr_node.xlow()][rr_node.ylow()].type;
        rr_node_arch_name += block_type_pin_index_to_name(type, rr_node.ptc_num());
    } else if (rr_node.type() == SOURCE || rr_node.type() == SINK) {
        //Set of pins associated with SOURCE/SINK
        auto type = device_ctx.grid[rr_node.xlow()][rr_node.ylow()].type;
        auto pin_names = block_type_class_index_to_pin_names(type, rr_node.ptc_num());
        if (pin_names.size() > 1) {
            rr_node_arch_name += rr_node.type_string();
            rr_node_arch_name += " connected to ";
            rr_node_arch_name += "{";
            rr_node_arch_name += vtr::join(pin_names, ", ");
            rr_node_arch_name += "}";
        } else {
            rr_node_arch_name += pin_names[0];
        }
    } else {
        VTR_ASSERT(rr_node.type() == CHANX || rr_node.type() == CHANY);
        //Wire segment name
        auto cost_index = rr_node.cost_index();
        int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;

        rr_node_arch_name += device_ctx.arch->Segments[seg_index].name;
    }

    return rr_node_arch_name;
}

IntraLbPbPinLookup::IntraLbPbPinLookup(const std::vector<t_logical_block_type>& block_types)
    : block_types_(block_types) {
    intra_lb_pb_pin_lookup_ = new t_pb_graph_pin**[block_types_.size()];
    for (unsigned int itype = 0; itype < block_types_.size(); ++itype) {
        intra_lb_pb_pin_lookup_[itype] = alloc_and_load_pb_graph_pin_lookup_from_index(&block_types[itype]);
    }
}

IntraLbPbPinLookup::IntraLbPbPinLookup(const IntraLbPbPinLookup& rhs)
    : IntraLbPbPinLookup(rhs.block_types_) {
    //nop
}

IntraLbPbPinLookup& IntraLbPbPinLookup::operator=(IntraLbPbPinLookup rhs) {
    //Copy-swap idiom
    swap(*this, rhs);

    return *this;
}

IntraLbPbPinLookup::~IntraLbPbPinLookup() {
    auto& device_ctx = g_vpr_ctx.device();
    for (unsigned int itype = 0; itype < device_ctx.logical_block_types.size(); itype++) {
        free_pb_graph_pin_lookup_from_index(intra_lb_pb_pin_lookup_[itype]);
    }

    delete[] intra_lb_pb_pin_lookup_;
}

const t_pb_graph_pin* IntraLbPbPinLookup::pb_gpin(unsigned int itype, int ipin) const {
    VTR_ASSERT(itype < block_types_.size());

    return intra_lb_pb_pin_lookup_[itype][ipin];
}

void swap(IntraLbPbPinLookup& lhs, IntraLbPbPinLookup& rhs) {
    std::swap(lhs.block_types_, rhs.block_types_);
    std::swap(lhs.intra_lb_pb_pin_lookup_, rhs.intra_lb_pb_pin_lookup_);
}

//Returns the set of pins which are connected to the top level clb pin
//  The pin(s) may be input(s) or and output (returning the connected sinks or drivers respectively)
std::vector<AtomPinId> find_clb_pin_connected_atom_pins(ClusterBlockId clb, int log_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    std::vector<AtomPinId> atom_pins;
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    auto logical_block = clb_nlist.block_type(clb);

    if (is_opin(log_pin, pick_random_physical_type(logical_block))) {
        //output
        AtomPinId driver = find_clb_pin_driver_atom_pin(clb, log_pin, pb_gpin_lookup);
        if (driver) {
            atom_pins.push_back(driver);
        }
    } else {
        //input
        atom_pins = find_clb_pin_sink_atom_pins(clb, log_pin, pb_gpin_lookup);
    }

    return atom_pins;
}

//Returns the atom pin which drives the top level clb output pin
AtomPinId find_clb_pin_driver_atom_pin(ClusterBlockId clb, int log_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    if (log_pin < 0) {
        //CLB output pin has no internal driver
        return AtomPinId::INVALID();
    }
    const t_pb_routes& pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;
    AtomNetId atom_net = pb_routes[log_pin].atom_net_id;

    int pb_pin_id = log_pin;
    //Trace back until the driver is reached
    while (pb_routes[pb_pin_id].driver_pb_pin_id >= 0) {
        pb_pin_id = pb_routes[pb_pin_id].driver_pb_pin_id;
    }
    VTR_ASSERT(pb_pin_id >= 0);

    VTR_ASSERT(atom_net == pb_routes[pb_pin_id].atom_net_id);

    //Map the pb_pin_id to AtomPinId
    AtomPinId atom_pin = find_atom_pin_for_pb_route_id(clb, pb_pin_id, pb_gpin_lookup);
    VTR_ASSERT(atom_pin);

    VTR_ASSERT_MSG(atom_ctx.nlist.pin_net(atom_pin) == atom_net, "Driver atom pin should drive the same net");

    return atom_pin;
}

//Returns the set of atom sink pins associated with the top level clb input pin
std::vector<AtomPinId> find_clb_pin_sink_atom_pins(ClusterBlockId clb, int log_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    const t_pb_routes& pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;

    VTR_ASSERT_MSG(log_pin < cluster_ctx.clb_nlist.block_type(clb)->pb_type->num_pins, "Must be a valid tile pin");

    VTR_ASSERT(cluster_ctx.clb_nlist.block_pb(clb));
    VTR_ASSERT_MSG(log_pin < cluster_ctx.clb_nlist.block_pb(clb)->pb_graph_node->num_pins(), "Pin must map to a top-level pb pin");

    VTR_ASSERT_MSG(pb_routes[log_pin].driver_pb_pin_id < 0, "CLB input pin should have no internal drivers");

    AtomNetId atom_net = pb_routes[log_pin].atom_net_id;
    VTR_ASSERT(atom_net);

    std::vector<int> connected_sink_pb_pins = find_connected_internal_clb_sink_pins(clb, log_pin);

    std::vector<AtomPinId> sink_atom_pins;
    for (int sink_pb_pin : connected_sink_pb_pins) {
        //Map the log_pin_id to AtomPinId
        AtomPinId atom_pin = find_atom_pin_for_pb_route_id(clb, sink_pb_pin, pb_gpin_lookup);
        VTR_ASSERT(atom_pin);

        VTR_ASSERT_MSG(atom_ctx.nlist.pin_net(atom_pin) == atom_net, "Sink atom pins should be driven by the same net");

        sink_atom_pins.push_back(atom_pin);
    }

    return sink_atom_pins;
}

//Find sinks internal to the given clb which are connected to the specified pb_pin
static std::vector<int> find_connected_internal_clb_sink_pins(ClusterBlockId clb, int pb_pin) {
    std::vector<int> connected_sink_pb_pins;
    find_connected_internal_clb_sink_pins_recurr(clb, pb_pin, connected_sink_pb_pins);

    return connected_sink_pb_pins;
}

//Recursive helper for find_connected_internal_clb_sink_pins()
static void find_connected_internal_clb_sink_pins_recurr(ClusterBlockId clb, int pb_pin, std::vector<int>& connected_sink_pb_pins) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.block_pb(clb)->pb_route[pb_pin].sink_pb_pin_ids.empty()) {
        //No more sinks => primitive input
        connected_sink_pb_pins.push_back(pb_pin);
    }
    for (int sink_pb_pin : cluster_ctx.clb_nlist.block_pb(clb)->pb_route[pb_pin].sink_pb_pin_ids) {
        find_connected_internal_clb_sink_pins_recurr(clb, sink_pb_pin, connected_sink_pb_pins);
    }
}

//Maps from a CLB's pb_route ID to it's matching AtomPinId (if the pb_route is a primitive pin)
static AtomPinId find_atom_pin_for_pb_route_id(ClusterBlockId clb, int pb_route_id, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    VTR_ASSERT_MSG(cluster_ctx.clb_nlist.block_pb(clb)->pb_route[pb_route_id].atom_net_id, "PB route should correspond to a valid atom net");

    //Find the graph pin associated with this pb_route
    const t_pb_graph_pin* gpin = pb_gpin_lookup.pb_gpin(cluster_ctx.clb_nlist.block_type(clb)->index, pb_route_id);
    VTR_ASSERT(gpin);

    //Get the PB associated with this block
    const t_pb* pb = cluster_ctx.clb_nlist.block_pb(clb);

    //Find the graph node containing the pin
    const t_pb_graph_node* gnode = gpin->parent_node;

    //Find the pb matching the gnode
    const t_pb* child_pb = pb->find_pb(gnode);

    VTR_ASSERT_MSG(child_pb, "Should find pb containing pb route");

    //Check if this is a leaf/atom pb
    if (child_pb->child_pbs == nullptr) {
        //It is a leaf, and hence should map to an atom

        //Find the associated atom
        AtomBlockId atom_block = atom_ctx.lookup.pb_atom(child_pb);
        VTR_ASSERT(atom_block);

        //Now find the matching pin by seeing which pin maps to the gpin
        for (AtomPinId atom_pin : atom_ctx.nlist.block_pins(atom_block)) {
            const t_pb_graph_pin* atom_pin_gpin = atom_ctx.lookup.atom_pin_pb_graph_pin(atom_pin);
            if (atom_pin_gpin == gpin) {
                //Match
                return atom_pin;
            }
        }
    }

    //No match
    return AtomPinId::INVALID();
}

/* Return the net pin which drive the CLB input connected to sink_pb_pin_id, or nullptr if none (i.e. driven internally)
 *   clb: Block in which the the sink pin is located on
 *   sink_pb_pin_id: The physical pin index of the sink pin on the block
 *
 *  Returns a tuple containing
 *   ClusterNetId: Corresponds to the net connected to the sink pin
 *                 INVALID if not an external CLB pin, or if it's an output (driver pin)
 *   clb_pin: Physical pin index, same as sink_pb_pin_id but potentially with an offset (if z is defined)
 *            -1 if not an external CLB pin, or if it's an output (driver pin)
 *   clb_net_pin: Index of the pin relative to the net, (i.e. 0 = driver, +1 = sink)
 *                -1 if not an external CLB pin, or if it's an output (driver pin)
 */
std::tuple<ClusterNetId, int, int> find_pb_route_clb_input_net_pin(ClusterBlockId clb, int sink_pb_pin_id) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    VTR_ASSERT(sink_pb_pin_id < cluster_ctx.clb_nlist.block_pb(clb)->pb_graph_node->total_pb_pins);
    VTR_ASSERT(clb != ClusterBlockId::INVALID());
    VTR_ASSERT(sink_pb_pin_id >= 0);

    const t_pb_routes& pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;

    VTR_ASSERT_MSG(pb_routes[sink_pb_pin_id].atom_net_id, "PB route should be associated with a net");

    //Walk back from the sink to the CLB input pin
    int curr_pb_pin_id = sink_pb_pin_id;
    int next_pb_pin_id = pb_routes[curr_pb_pin_id].driver_pb_pin_id;
    while (next_pb_pin_id >= 0) {
        //Advance back towards the input
        curr_pb_pin_id = next_pb_pin_id;

        VTR_ASSERT_MSG(pb_routes[next_pb_pin_id].atom_net_id == pb_routes[sink_pb_pin_id].atom_net_id,
                       "Connected pb_routes should connect the same net");

        next_pb_pin_id = pb_routes[curr_pb_pin_id].driver_pb_pin_id;
    }

    bool is_output_pin = (pb_routes[curr_pb_pin_id].pb_graph_pin->port->type == OUT_PORT);

    if (!is_clb_external_pin(clb, curr_pb_pin_id) || is_output_pin) {
        return std::make_tuple(ClusterNetId::INVALID(), -1, -1);
    }

    //To account for capacity > 1 blocks we need to convert the pb_pin to the clb pin
    int clb_pin = find_pb_pin_clb_pin(clb, curr_pb_pin_id);
    VTR_ASSERT(clb_pin >= 0);

    //clb_pin should be a top-level CLB input
    ClusterNetId clb_net_idx = cluster_ctx.clb_nlist.block_net(clb, clb_pin);
    int clb_net_pin_idx = cluster_ctx.clb_nlist.block_pin_net_index(clb, clb_pin);
    VTR_ASSERT(clb_net_idx != ClusterNetId::INVALID());
    VTR_ASSERT(clb_net_pin_idx >= 0);

    return std::tuple<ClusterNetId, int, int>(clb_net_idx, clb_pin, clb_net_pin_idx);
}

//Return the pb pin index corresponding to the pin clb_pin on block clb
// Given a clb_pin index on a this function will return the corresponding
// pin index on the pb_type (accounting for the possible z-coordinate offset).
int find_clb_pb_pin(ClusterBlockId clb, int clb_pin) {
    auto& place_ctx = g_vpr_ctx.placement();

    auto type = physical_tile_type(clb);
    VTR_ASSERT_MSG(clb_pin < type->num_pins, "Must be a valid top-level pin");

    int pb_pin = -1;
    if (place_ctx.block_locs[clb].nets_and_pins_synced_to_z_coordinate) {
        //Pins have been offset by z-coordinate, need to remove offset

        VTR_ASSERT(type->num_pins % type->capacity == 0);
        int num_basic_block_pins = type->num_pins / type->capacity;
        /* Logical location and physical location is offset by z * max_num_block_pins */

        pb_pin = clb_pin - place_ctx.block_locs[clb].loc.z * num_basic_block_pins;
    } else {
        pb_pin = clb_pin;
    }

    VTR_ASSERT(pb_pin >= 0);

    return pb_pin;
}

//Inverse of find_clb_pb_pin()
int find_pb_pin_clb_pin(ClusterBlockId clb, int pb_pin) {
    auto& place_ctx = g_vpr_ctx.placement();

    auto type = physical_tile_type(clb);

    int clb_pin = -1;
    if (place_ctx.block_locs[clb].nets_and_pins_synced_to_z_coordinate) {
        //Pins have been offset by z-coordinate, need to remove offset
        VTR_ASSERT(type->num_pins % type->capacity == 0);
        int num_basic_block_pins = type->num_pins / type->capacity;
        /* Logical location and physical location is offset by z * max_num_block_pins */

        clb_pin = pb_pin + place_ctx.block_locs[clb].loc.z * num_basic_block_pins;
    } else {
        //No offset
        clb_pin = pb_pin;
    }
    VTR_ASSERT(clb_pin >= 0);

    return clb_pin;
}

bool is_clb_external_pin(ClusterBlockId blk_id, int pb_pin_id) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const t_pb_graph_pin* gpin = cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route[pb_pin_id].pb_graph_pin;
    VTR_ASSERT(gpin);

    //If the gpin's parent graph node is the same as the pb's graph node
    //this must be a top level pin
    const t_pb_graph_node* gnode = gpin->parent_node;
    bool is_top_level_pin = (gnode == cluster_ctx.clb_nlist.block_pb(blk_id)->pb_graph_node);

    return is_top_level_pin;
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

bool is_input_type(t_physical_tile_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return device_ctx.input_types.count(type);
}

bool is_output_type(t_physical_tile_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return device_ctx.output_types.count(type);
}

bool is_io_type(t_physical_tile_type_ptr type) {
    return is_input_type(type)
           || is_output_type(type);
}

bool is_empty_type(t_physical_tile_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE;
}

bool is_empty_type(t_logical_block_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return type == device_ctx.EMPTY_LOGICAL_BLOCK_TYPE;
}

t_physical_tile_type_ptr physical_tile_type(ClusterBlockId blk) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    auto block_loc = place_ctx.block_locs[blk];
    auto loc = block_loc.loc;

    return device_ctx.grid[loc.x][loc.y].type;
}

t_logical_block_type_ptr logical_block_type(t_physical_tile_type_ptr physical_tile_type) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& logical_blocks = device_ctx.logical_block_types;

    // Loop through the ordered map to get tiles in a decreasing priority order
    for (auto& logical_blocks_ids : physical_tile_type->logical_blocks_priority) {
        for (auto block_id : logical_blocks_ids.second) {
            return &logical_blocks[block_id];
        }
    }

    VPR_THROW(VPR_ERROR_OTHER, "No corresponding logical block type found for physical tile type %s\n", physical_tile_type->name);
}

/* Each node in the pb_graph for a top-level pb_type can be uniquely identified
 * by its pins. Since the pins in a cluster of a certain type are densely indexed,
 * this function will find the pin index (int pin_count_in_cluster) of the first
 * pin for a given pb_graph_node, and use this index value as unique identifier
 * for the node.
 */
int get_unique_pb_graph_node_id(const t_pb_graph_node* pb_graph_node) {
    t_pb_graph_pin first_input_pin;
    t_pb_graph_pin first_output_pin;
    int node_id;

    if (pb_graph_node->num_input_pins != nullptr) {
        /* If input port exists on this node, return the index of the first
         * input pin as node_id.
         */
        first_input_pin = pb_graph_node->input_pins[0][0];
        node_id = first_input_pin.pin_count_in_cluster;
        return node_id;
    } else {
        /* If no input port exists on node, then return the index of the first
         * output pin. Every pb_node is guaranteed to have at least an input or
         * output pin.
         */
        first_output_pin = pb_graph_node->output_pins[0][0];
        node_id = first_output_pin.pin_count_in_cluster;
        return node_id;
    }
}

void get_class_range_for_block(const ClusterBlockId blk_id,
                               int* class_low,
                               int* class_high) {
    /* Assumes that the placement has been done so each block has a set of pins allocated to it */
    auto& place_ctx = g_vpr_ctx.placement();

    auto type = physical_tile_type(blk_id);
    VTR_ASSERT(type->num_class % type->capacity == 0);
    *class_low = place_ctx.block_locs[blk_id].loc.z * (type->num_class / type->capacity);
    *class_high = (place_ctx.block_locs[blk_id].loc.z + 1) * (type->num_class / type->capacity) - 1;
}

void get_pin_range_for_block(const ClusterBlockId blk_id,
                             int* pin_low,
                             int* pin_high) {
    /* Assumes that the placement has been done so each block has a set of pins allocated to it */
    auto& place_ctx = g_vpr_ctx.placement();

    auto type = physical_tile_type(blk_id);
    VTR_ASSERT(type->num_pins % type->capacity == 0);
    *pin_low = place_ctx.block_locs[blk_id].loc.z * (type->num_pins / type->capacity);
    *pin_high = (place_ctx.block_locs[blk_id].loc.z + 1) * (type->num_pins / type->capacity) - 1;
}

t_physical_tile_type_ptr find_tile_type_by_name(std::string name, const std::vector<t_physical_tile_type>& types) {
    for (auto const& type : types) {
        if (type.name == name) {
            return &type;
        }
    }
    return nullptr; //Not found
}

t_logical_block_type_ptr infer_logic_block_type(const DeviceGrid& grid) {
    auto& device_ctx = g_vpr_ctx.device();
    //Collect candidate blocks which contain LUTs
    std::vector<t_logical_block_type_ptr> logic_block_candidates;

    for (const auto& type : device_ctx.logical_block_types) {
        if (block_type_contains_blif_model(&type, LOGIC_MODEL_REGEX)) {
            logic_block_candidates.push_back(&type);
        }
    }

    if (logic_block_candidates.empty()) {
        //No LUTs, fallback to looking for which contain FFs
        for (const auto& type : device_ctx.logical_block_types) {
            if (block_type_contains_blif_model(&type, REGISTER_MODEL_REGEX)) {
                logic_block_candidates.push_back(&type);
            }
        }
    }

    //Sort the candidates by the most common block type
    auto by_desc_grid_count = [&](t_logical_block_type_ptr lhs, t_logical_block_type_ptr rhs) {
        int lhs_num_instances = 0;
        int rhs_num_instances = 0;
        // Count number of instances for each type
        for (auto type : lhs->equivalent_tiles)
            lhs_num_instances += grid.num_instances(type);
        for (auto type : rhs->equivalent_tiles)
            rhs_num_instances += grid.num_instances(type);
        return lhs_num_instances > rhs_num_instances;
    };
    std::stable_sort(logic_block_candidates.begin(), logic_block_candidates.end(), by_desc_grid_count);

    if (!logic_block_candidates.empty()) {
        return logic_block_candidates.front();
    } else {
        //Otherwise assume it is the most common block type
        auto most_common_type = find_most_common_block_type(grid);
        if (most_common_type == nullptr) {
            VTR_LOG_WARN("Unable to infer which block type is a logic block\n");
        }
        return most_common_type;
    }
}

t_logical_block_type_ptr find_most_common_block_type(const DeviceGrid& grid) {
    auto& device_ctx = g_vpr_ctx.device();

    t_logical_block_type_ptr max_type = nullptr;
    size_t max_count = 0;
    for (const auto& logical_block : device_ctx.logical_block_types) {
        size_t inst_cnt = 0;
        for (const auto& equivalent_tile : logical_block.equivalent_tiles) {
            inst_cnt += grid.num_instances(equivalent_tile);
        }

        if (max_count < inst_cnt) {
            max_count = inst_cnt;
            max_type = &logical_block;
        }
    }

    if (max_type == nullptr) {
        VTR_LOG_WARN("Unable to determine most common block type (perhaps the device grid was empty?)\n");
    }

    return max_type;
}

t_physical_tile_type_ptr find_most_common_tile_type(const DeviceGrid& grid) {
    auto& device_ctx = g_vpr_ctx.device();

    t_physical_tile_type_ptr max_type = nullptr;
    size_t max_count = 0;
    for (const auto& physical_tile : device_ctx.physical_tile_types) {
        size_t inst_cnt = grid.num_instances(&physical_tile);

        if (max_count < inst_cnt) {
            max_count = inst_cnt;
            max_type = &physical_tile;
        }
    }

    if (max_type == nullptr) {
        VTR_LOG_WARN("Unable to determine most common block type (perhaps the device grid was empty?)\n");
    }

    return max_type;
}

InstPort parse_inst_port(std::string str) {
    InstPort inst_port(str);

    auto& device_ctx = g_vpr_ctx.device();
    auto blk_type = find_tile_type_by_name(inst_port.instance_name(), device_ctx.physical_tile_types);
    if (blk_type == nullptr) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find block type named %s", inst_port.instance_name().c_str());
    }

    int num_pins = OPEN;
    for (auto physical_port : blk_type->ports) {
        if (0 == strcmp(inst_port.port_name().c_str(), physical_port.name)) {
            num_pins = physical_port.num_pins;
            break;
        }
    }

    if (num_pins == OPEN) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find port %s on block type %s", inst_port.port_name().c_str(), inst_port.instance_name().c_str());
    }

    if (inst_port.port_low_index() == InstPort::UNSPECIFIED) {
        VTR_ASSERT(inst_port.port_high_index() == InstPort::UNSPECIFIED);

        inst_port.set_port_low_index(0);
        inst_port.set_port_high_index(num_pins - 1);

    } else {
        if (inst_port.port_low_index() < 0 || inst_port.port_low_index() >= num_pins
            || inst_port.port_high_index() < 0 || inst_port.port_high_index() >= num_pins) {
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Pin indices [%d:%d] on port %s of block type %s out of expected range [%d:%d]",
                            inst_port.port_low_index(), inst_port.port_high_index(),
                            inst_port.port_name().c_str(), inst_port.instance_name().c_str(),
                            0, num_pins - 1);
        }
    }
    return inst_port;
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

    for (auto port : type->ports) {
        if (port.name == port_name) {
            num_pins = port.num_pins;
            break;
        }
        port_base_ipin += port.num_pins;
    }

    if (num_pins != OPEN) {
        VTR_ASSERT(pin_index_in_port < num_pins);

        ipin = port_base_ipin + pin_index_in_port;
    }

    return ipin;
}

//Returns true if the specified block type contains the specified blif model name
bool block_type_contains_blif_model(t_logical_block_type_ptr type, std::string blif_model_name) {
    return pb_type_contains_blif_model(type->pb_type, blif_model_name);
}

static bool block_type_contains_blif_model(t_logical_block_type_ptr type, const std::regex& blif_model_regex) {
    return pb_type_contains_blif_model(type->pb_type, blif_model_regex);
}

//Returns true of a pb_type (or it's children) contain the specified blif model name
bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::string& blif_model_name) {
    if (!pb_type) {
        return false;
    }

    if (pb_type->blif_model != nullptr) {
        //Leaf pb_type
        VTR_ASSERT(pb_type->num_modes == 0);
        if (blif_model_name == pb_type->blif_model
            || ".subckt " + blif_model_name == pb_type->blif_model) {
            return true;
        } else {
            return false;
        }
    } else {
        for (int imode = 0; imode < pb_type->num_modes; ++imode) {
            const t_mode* mode = &pb_type->modes[imode];

            for (int ichild = 0; ichild < mode->num_pb_type_children; ++ichild) {
                const t_pb_type* pb_type_child = &mode->pb_type_children[ichild];
                if (pb_type_contains_blif_model(pb_type_child, blif_model_name)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::regex& blif_model_regex) {
    if (!pb_type) {
        return false;
    }

    if (pb_type->blif_model != nullptr) {
        //Leaf pb_type
        VTR_ASSERT(pb_type->num_modes == 0);
        if (std::regex_match(pb_type->blif_model, blif_model_regex)) {
            return true;
        } else {
            return false;
        }
    } else {
        for (int imode = 0; imode < pb_type->num_modes; ++imode) {
            const t_mode* mode = &pb_type->modes[imode];

            for (int ichild = 0; ichild < mode->num_pb_type_children; ++ichild) {
                const t_pb_type* pb_type_child = &mode->pb_type_children[ichild];
                if (pb_type_contains_blif_model(pb_type_child, blif_model_regex)) {
                    return true;
                }
            }
        }
    }
    return false;
}

int get_max_primitives_in_pb_type(t_pb_type* pb_type) {
    int i, j;
    int max_size, temp_size;
    if (pb_type->modes == nullptr) {
        max_size = 1;
    } else {
        max_size = 0;
        temp_size = 0;
        for (i = 0; i < pb_type->num_modes; i++) {
            for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
                temp_size += pb_type->modes[i].pb_type_children[j].num_pb
                             * get_max_primitives_in_pb_type(
                                   &pb_type->modes[i].pb_type_children[j]);
            }
            if (temp_size > max_size) {
                max_size = temp_size;
            }
        }
    }
    return max_size;
}

/* finds maximum number of nets that can be contained in pb_type, this is bounded by the number of driving pins */
int get_max_nets_in_pb_type(const t_pb_type* pb_type) {
    int i, j;
    int max_nets, temp_nets;
    if (pb_type->modes == nullptr) {
        max_nets = pb_type->num_output_pins;
    } else {
        max_nets = 0;
        for (i = 0; i < pb_type->num_modes; i++) {
            temp_nets = 0;
            for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
                temp_nets += pb_type->modes[i].pb_type_children[j].num_pb
                             * get_max_nets_in_pb_type(
                                   &pb_type->modes[i].pb_type_children[j]);
            }
            if (temp_nets > max_nets) {
                max_nets = temp_nets;
            }
        }
    }
    if (pb_type->parent_mode == nullptr) {
        max_nets += pb_type->num_input_pins + pb_type->num_output_pins
                    + pb_type->num_clock_pins;
    }
    return max_nets;
}

int get_max_depth_of_pb_type(t_pb_type* pb_type) {
    int i, j;
    int max_depth, temp_depth;
    max_depth = pb_type->depth;
    for (i = 0; i < pb_type->num_modes; i++) {
        for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
            temp_depth = get_max_depth_of_pb_type(
                &pb_type->modes[i].pb_type_children[j]);
            if (temp_depth > max_depth) {
                max_depth = temp_depth;
            }
        }
    }
    return max_depth;
}

/**
 * given an atom block and physical primitive type, is the mapping legal
 */
bool primitive_type_feasible(const AtomBlockId blk_id, const t_pb_type* cur_pb_type) {
    if (cur_pb_type == nullptr) {
        return false;
    }

    auto& atom_ctx = g_vpr_ctx.atom();
    if (cur_pb_type->model != atom_ctx.nlist.block_model(blk_id)) {
        //Primitive and atom do not match
        return false;
    }

    VTR_ASSERT_MSG(atom_ctx.nlist.is_compressed(), "This function assumes a compressed/non-dirty netlist");

    //Keep track of how many atom ports were checked.
    //
    //We need to do this since we iterate over the pb's ports and
    //may miss some atom ports if there is a mismatch
    size_t checked_ports = 0;

    //Look at each port on the pb and find the associated port on the
    //atom. To be feasible the pb must have as many pins on each port
    //as the atom requires
    for (int iport = 0; iport < cur_pb_type->num_ports; ++iport) {
        const t_port* pb_port = &cur_pb_type->ports[iport];
        const t_model_ports* pb_model_port = pb_port->model_port;

        //Find the matching port on the atom
        auto port_id = atom_ctx.nlist.find_atom_port(blk_id, pb_model_port);

        if (port_id) { //Port is used by the atom

            //In compressed form the atom netlist stores only in-use pins,
            //so we can query the number of required pins directly
            int required_atom_pins = atom_ctx.nlist.port_pins(port_id).size();

            int available_pb_pins = pb_port->num_pins;

            if (available_pb_pins < required_atom_pins) {
                //Too many pins required
                return false;
            }

            //Note that this port was checked
            ++checked_ports;
        }
    }

    //Similarly to pins, only in-use ports are stored in the compressed
    //atom netlist, so we can figure out how many ports should have been
    //checked directly
    size_t atom_ports = atom_ctx.nlist.block_ports(blk_id).size();

    //See if all the atom ports were checked
    if (checked_ports != atom_ports) {
        VTR_ASSERT(checked_ports < atom_ports);
        //Required atom port was missing from physical primitive
        return false;
    }

    //Feasible
    return true;
}

//Returns the sibling atom of a memory slice pb
//  Note that the pb must be part of a MEMORY_CLASS
AtomBlockId find_memory_sibling(const t_pb* pb) {
    auto& atom_ctx = g_vpr_ctx.atom();

    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;

    VTR_ASSERT(pb_type->class_type == MEMORY_CLASS);

    const t_pb* memory_class_pb = pb->parent_pb;

    for (int isibling = 0; isibling < pb_type->parent_mode->num_pb_type_children; ++isibling) {
        const t_pb* sibling_pb = &memory_class_pb->child_pbs[pb->mode][isibling];

        if (sibling_pb->name != nullptr) {
            return atom_ctx.lookup.pb_atom(sibling_pb);
        }
    }
    return AtomBlockId::INVALID();
}

/**
 * Return pb_graph_node pin from model port and pin
 *  NULL if not found
 */
t_pb_graph_pin* get_pb_graph_node_pin_from_model_port_pin(const t_model_ports* model_port, const int model_pin, const t_pb_graph_node* pb_graph_node) {
    int i;

    if (model_port->dir == IN_PORT) {
        if (model_port->is_clock == false) {
            for (i = 0; i < pb_graph_node->num_input_ports; i++) {
                if (pb_graph_node->input_pins[i][0].port->model_port == model_port) {
                    if (pb_graph_node->num_input_pins[i] > model_pin) {
                        return &pb_graph_node->input_pins[i][model_pin];
                    } else {
                        return nullptr;
                    }
                }
            }
        } else {
            for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
                if (pb_graph_node->clock_pins[i][0].port->model_port == model_port) {
                    if (pb_graph_node->num_clock_pins[i] > model_pin) {
                        return &pb_graph_node->clock_pins[i][model_pin];
                    } else {
                        return nullptr;
                    }
                }
            }
        }
    } else {
        VTR_ASSERT(model_port->dir == OUT_PORT);
        for (i = 0; i < pb_graph_node->num_output_ports; i++) {
            if (pb_graph_node->output_pins[i][0].port->model_port == model_port) {
                if (pb_graph_node->num_output_pins[i] > model_pin) {
                    return &pb_graph_node->output_pins[i][model_pin];
                } else {
                    return nullptr;
                }
            }
        }
    }
    return nullptr;
}

//Retrieves the atom pin associated with a specific CLB and pb_graph_pin
AtomPinId find_atom_pin(ClusterBlockId blk_id, const t_pb_graph_pin* pb_gpin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    int pb_route_id = pb_gpin->pin_count_in_cluster;
    AtomNetId atom_net = cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route[pb_route_id].atom_net_id;

    VTR_ASSERT(atom_net);

    AtomPinId atom_pin;

    //Look through all the pins on this net, looking for the matching pin
    for (AtomPinId pin : atom_ctx.nlist.net_pins(atom_net)) {
        AtomBlockId blk = atom_ctx.nlist.pin_block(pin);
        if (atom_ctx.lookup.atom_clb(blk) == blk_id) {
            //Part of the same CLB
            if (atom_ctx.lookup.atom_pin_pb_graph_pin(pin) == pb_gpin)
                //The same pin
                atom_pin = pin;
        }
    }

    VTR_ASSERT(atom_pin);

    return atom_pin;
}

//Retrieves the pb_graph_pin associated with an AtomPinId
//  Currently this function just wraps get_pb_graph_node_pin_from_model_port_pin()
//  in a more convenient interface.
const t_pb_graph_pin* find_pb_graph_pin(const AtomNetlist& netlist, const AtomLookup& netlist_lookup, const AtomPinId pin_id) {
    VTR_ASSERT(pin_id);

    //Get the graph node
    AtomBlockId blk_id = netlist.pin_block(pin_id);
    const t_pb_graph_node* pb_gnode = netlist_lookup.atom_pb_graph_node(blk_id);
    VTR_ASSERT(pb_gnode);

    //The graph node and pin/block should agree on the model they represent
    VTR_ASSERT(netlist.block_model(blk_id) == pb_gnode->pb_type->model);

    //Get the pin index
    AtomPortId port_id = netlist.pin_port(pin_id);
    int ipin = netlist.pin_port_bit(pin_id);

    //Get the model port
    const t_model_ports* model_port = netlist.port_model(port_id);
    VTR_ASSERT(model_port);

    return get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb_gnode);
}

t_pb_graph_pin* get_pb_graph_node_pin_from_block_pin(ClusterBlockId iblock, int ipin) {
    int i, count;
    const t_pb_type* pb_type;
    t_pb_graph_node* pb_graph_node;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    pb_graph_node = cluster_ctx.clb_nlist.block_pb(iblock)->pb_graph_node;
    pb_type = pb_graph_node->pb_type;

    /* If this is post-placed, then the ipin may have been shuffled up by the z * num_pins,
     * bring it back down to 0..num_pins-1 range for easier analysis */
    ipin %= (pb_type->num_input_pins + pb_type->num_output_pins + pb_type->num_clock_pins);

    if (ipin < pb_type->num_input_pins) {
        count = ipin;
        for (i = 0; i < pb_graph_node->num_input_ports; i++) {
            if (count - pb_graph_node->num_input_pins[i] >= 0) {
                count -= pb_graph_node->num_input_pins[i];
            } else {
                return &pb_graph_node->input_pins[i][count];
            }
        }
    } else if (ipin < pb_type->num_input_pins + pb_type->num_output_pins) {
        count = ipin - pb_type->num_input_pins;
        for (i = 0; i < pb_graph_node->num_output_ports; i++) {
            if (count - pb_graph_node->num_output_pins[i] >= 0) {
                count -= pb_graph_node->num_output_pins[i];
            } else {
                return &pb_graph_node->output_pins[i][count];
            }
        }
    } else {
        count = ipin - pb_type->num_input_pins - pb_type->num_output_pins;
        for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
            if (count - pb_graph_node->num_clock_pins[i] >= 0) {
                count -= pb_graph_node->num_clock_pins[i];
            } else {
                return &pb_graph_node->clock_pins[i][count];
            }
        }
    }
    VTR_ASSERT(0);
    return nullptr;
}

const t_port* find_pb_graph_port(const t_pb_graph_node* pb_gnode, std::string port_name) {
    const t_pb_graph_pin* gpin = find_pb_graph_pin(pb_gnode, port_name, 0);

    if (gpin != nullptr) {
        return gpin->port;
    }
    return nullptr;
}

const t_pb_graph_pin* find_pb_graph_pin(const t_pb_graph_node* pb_gnode, std::string port_name, int index) {
    for (int iport = 0; iport < pb_gnode->num_input_ports; iport++) {
        if (pb_gnode->num_input_pins[iport] < index) continue;

        const t_pb_graph_pin* gpin = &pb_gnode->input_pins[iport][index];

        if (gpin->port->name == port_name) {
            return gpin;
        }
    }
    for (int iport = 0; iport < pb_gnode->num_output_ports; iport++) {
        if (pb_gnode->num_output_pins[iport] < index) continue;

        const t_pb_graph_pin* gpin = &pb_gnode->output_pins[iport][index];

        if (gpin->port->name == port_name) {
            return gpin;
        }
    }
    for (int iport = 0; iport < pb_gnode->num_clock_ports; iport++) {
        if (pb_gnode->num_clock_pins[iport] < index) continue;

        const t_pb_graph_pin* gpin = &pb_gnode->clock_pins[iport][index];

        if (gpin->port->name == port_name) {
            return gpin;
        }
    }

    //Not found
    return nullptr;
}

/* Recusively visit through all pb_graph_nodes to populate pb_graph_pin_lookup_from_index */
static void load_pb_graph_pin_lookup_from_index_rec(t_pb_graph_pin** pb_graph_pin_lookup_from_index, t_pb_graph_node* pb_graph_node) {
    for (int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
            t_pb_graph_pin* pb_pin = &pb_graph_node->input_pins[iport][ipin];
            VTR_ASSERT(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == nullptr);
            pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
        }
    }
    for (int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
            t_pb_graph_pin* pb_pin = &pb_graph_node->output_pins[iport][ipin];
            VTR_ASSERT(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == nullptr);
            pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
        }
    }
    for (int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
            t_pb_graph_pin* pb_pin = &pb_graph_node->clock_pins[iport][ipin];
            VTR_ASSERT(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == nullptr);
            pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
        }
    }

    for (int imode = 0; imode < pb_graph_node->pb_type->num_modes; imode++) {
        for (int ipb_type = 0; ipb_type < pb_graph_node->pb_type->modes[imode].num_pb_type_children; ipb_type++) {
            for (int ipb = 0; ipb < pb_graph_node->pb_type->modes[imode].pb_type_children[ipb_type].num_pb; ipb++) {
                load_pb_graph_pin_lookup_from_index_rec(pb_graph_pin_lookup_from_index, &pb_graph_node->child_pb_graph_nodes[imode][ipb_type][ipb]);
            }
        }
    }
}

/* Create a lookup that returns a pb_graph_pin pointer given the pb_graph_pin index */
t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_logical_block_type_ptr type) {
    t_pb_graph_pin** pb_graph_pin_lookup_from_type = nullptr;

    t_pb_graph_node* pb_graph_head = type->pb_graph_head;
    if (pb_graph_head == nullptr) {
        return nullptr;
    }
    int num_pins = pb_graph_head->total_pb_pins;

    VTR_ASSERT(num_pins > 0);

    pb_graph_pin_lookup_from_type = new t_pb_graph_pin*[num_pins];
    for (int id = 0; id < num_pins; id++) {
        pb_graph_pin_lookup_from_type[id] = nullptr;
    }

    VTR_ASSERT(pb_graph_pin_lookup_from_type);

    load_pb_graph_pin_lookup_from_index_rec(pb_graph_pin_lookup_from_type, pb_graph_head);

    for (int id = 0; id < num_pins; id++) {
        VTR_ASSERT(pb_graph_pin_lookup_from_type[id] != nullptr);
    }

    return pb_graph_pin_lookup_from_type;
}

/* Free pb_graph_pin lookup array */
void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type) {
    if (pb_graph_pin_lookup_from_type == nullptr) {
        return;
    }
    delete[] pb_graph_pin_lookup_from_type;
}

/**
 * Create lookup table that returns a pointer to the pb given [index to block][pin_id].
 */
vtr::vector<ClusterBlockId, t_pb**> alloc_and_load_pin_id_to_pb_mapping() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    vtr::vector<ClusterBlockId, t_pb**> pin_id_to_pb_mapping(cluster_ctx.clb_nlist.blocks().size());
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        pin_id_to_pb_mapping[blk_id] = new t_pb*[cluster_ctx.clb_nlist.block_type(blk_id)->pb_graph_head->total_pb_pins];
        for (int j = 0; j < cluster_ctx.clb_nlist.block_type(blk_id)->pb_graph_head->total_pb_pins; j++) {
            pin_id_to_pb_mapping[blk_id][j] = nullptr;
        }
        load_pin_id_to_pb_mapping_rec(cluster_ctx.clb_nlist.block_pb(blk_id), pin_id_to_pb_mapping[blk_id]);
    }
    return pin_id_to_pb_mapping;
}

/**
 * Recursively create lookup table that returns a pointer to the pb given a pb_graph_pin id.
 */
static void load_pin_id_to_pb_mapping_rec(t_pb* cur_pb, t_pb** pin_id_to_pb_mapping) {
    t_pb_graph_node* pb_graph_node = cur_pb->pb_graph_node;
    t_pb_type* pb_type = pb_graph_node->pb_type;
    int mode = cur_pb->mode;

    for (int i = 0; i < pb_graph_node->num_input_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
            pin_id_to_pb_mapping[pb_graph_node->input_pins[i][j].pin_count_in_cluster] = cur_pb;
        }
    }
    for (int i = 0; i < pb_graph_node->num_output_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
            pin_id_to_pb_mapping[pb_graph_node->output_pins[i][j].pin_count_in_cluster] = cur_pb;
        }
    }
    for (int i = 0; i < pb_graph_node->num_clock_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
            pin_id_to_pb_mapping[pb_graph_node->clock_pins[i][j].pin_count_in_cluster] = cur_pb;
        }
    }

    if (pb_type->num_modes == 0 || cur_pb->child_pbs == nullptr) {
        return;
    }

    for (int i = 0; i < pb_type->modes[mode].num_pb_type_children; i++) {
        for (int j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb; j++) {
            load_pin_id_to_pb_mapping_rec(&cur_pb->child_pbs[i][j], pin_id_to_pb_mapping);
        }
    }
}

/**
 * free pin_index_to_pb_mapping lookup table
 */
void free_pin_id_to_pb_mapping(vtr::vector<ClusterBlockId, t_pb**>& pin_id_to_pb_mapping) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        delete[] pin_id_to_pb_mapping[blk_id];
    }
    pin_id_to_pb_mapping.clear();
}

/**
 * Determine cost for using primitive within a complex block, should use primitives of low cost before selecting primitives of high cost
 * For now, assume primitives that have a lot of pins are scarcer than those without so use primitives with less pins before those with more
 */
float compute_primitive_base_cost(const t_pb_graph_node* primitive) {
    return (primitive->pb_type->num_input_pins
            + primitive->pb_type->num_output_pins
            + primitive->pb_type->num_clock_pins);
}

int num_ext_inputs_atom_block(AtomBlockId blk_id) {
    /* Returns the number of input pins on this atom block that must be hooked *
     * up through external interconnect. That is, the number of input          *
     * pins used - the number which connect (internally) to the outputs.       */

    int ext_inps = 0;

    /* TODO: process to get ext_inps is slow, should cache in lookup table */

    std::unordered_set<AtomNetId> input_nets;

    //Record the unique input nets
    auto& atom_ctx = g_vpr_ctx.atom();
    for (auto pin_id : atom_ctx.nlist.block_input_pins(blk_id)) {
        auto net_id = atom_ctx.nlist.pin_net(pin_id);
        input_nets.insert(net_id);
    }

    ext_inps = input_nets.size();

    //Look through the output nets for any duplicates of the input nets
    for (auto pin_id : atom_ctx.nlist.block_output_pins(blk_id)) {
        auto net_id = atom_ctx.nlist.pin_net(pin_id);
        if (input_nets.count(net_id)) {
            --ext_inps;
        }
    }

    VTR_ASSERT(ext_inps >= 0);

    return (ext_inps);
}

void free_pb(t_pb* pb) {
    if (pb == nullptr) {
        return;
    }

    const t_pb_type* pb_type;
    int i, j, mode;

    pb_type = pb->pb_graph_node->pb_type;

    if (pb->name) {
        free(pb->name);
        pb->name = nullptr;
    }

    if (pb_type->blif_model == nullptr) {
        mode = pb->mode;
        for (i = 0; i < pb_type->modes[mode].num_pb_type_children && pb->child_pbs != nullptr; i++) {
            for (j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb && pb->child_pbs[i] != nullptr; j++) {
                if (pb->child_pbs[i][j].name != nullptr || pb->child_pbs[i][j].child_pbs != nullptr) {
                    free_pb(&pb->child_pbs[i][j]);
                }
            }
            if (pb->child_pbs[i]) {
                //Free children (num_pb)
                delete[] pb->child_pbs[i];
            }
        }
        if (pb->child_pbs) {
            //Free child pointers (modes)
            delete[] pb->child_pbs;
        }

        pb->child_pbs = nullptr;

    } else {
        /* Primitive */
        auto& atom_ctx = g_vpr_ctx.mutable_atom();
        auto blk_id = atom_ctx.lookup.pb_atom(pb);
        if (blk_id) {
            //Update atom netlist mapping
            atom_ctx.lookup.set_atom_clb(blk_id, ClusterBlockId::INVALID());
            atom_ctx.lookup.set_atom_pb(blk_id, nullptr);
        }
        atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), pb);
    }
    free_pb_stats(pb);
}

void revalid_molecules(const t_pb* pb, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules) {
    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;

    if (pb_type->blif_model == nullptr) {
        int mode = pb->mode;
        for (int i = 0; i < pb_type->modes[mode].num_pb_type_children && pb->child_pbs != nullptr; i++) {
            for (int j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb && pb->child_pbs[i] != nullptr; j++) {
                if (pb->child_pbs[i][j].name != nullptr || pb->child_pbs[i][j].child_pbs != nullptr) {
                    revalid_molecules(&pb->child_pbs[i][j], atom_molecules);
                }
            }
        }
    } else {
        //Primitive
        auto& atom_ctx = g_vpr_ctx.mutable_atom();

        auto blk_id = atom_ctx.lookup.pb_atom(pb);
        if (blk_id) {
            /* If any molecules were marked invalid because of this logic block getting packed, mark them valid */

            //Update atom netlist mapping
            atom_ctx.lookup.set_atom_clb(blk_id, ClusterBlockId::INVALID());
            atom_ctx.lookup.set_atom_pb(blk_id, nullptr);

            auto rng = atom_molecules.equal_range(blk_id);
            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                t_pack_molecule* cur_molecule = kv.second;
                if (cur_molecule->valid == false) {
                    int i;
                    for (i = 0; i < get_array_size_of_molecule(cur_molecule); i++) {
                        if (cur_molecule->atom_block_ids[i]) {
                            if (atom_ctx.lookup.atom_clb(cur_molecule->atom_block_ids[i]) != ClusterBlockId::INVALID()) {
                                break;
                            }
                        }
                    }
                    /* All atom blocks are open for this molecule, place back in queue */
                    if (i == get_array_size_of_molecule(cur_molecule)) {
                        cur_molecule->valid = true;
                        // when invalidating a molecule check if it's a chain molecule
                        // that is part of a long chain. If so, check if this molecule
                        // have modified the chain_id value based on the stale packing
                        // then reset the chain id and the first packed molecule pointer
                        // this is packing is being reset
                        if (cur_molecule->is_chain() && cur_molecule->chain_info->is_long_chain && cur_molecule->chain_info->first_packed_molecule == cur_molecule) {
                            cur_molecule->chain_info->first_packed_molecule = nullptr;
                            cur_molecule->chain_info->chain_id = -1;
                        }
                    }
                }
            }
        }
    }
}

void free_pb_stats(t_pb* pb) {
    if (pb) {
        if (pb->pb_stats == nullptr) {
            return;
        }

        pb->pb_stats->gain.clear();
        pb->pb_stats->timinggain.clear();
        pb->pb_stats->sharinggain.clear();
        pb->pb_stats->hillgain.clear();
        pb->pb_stats->connectiongain.clear();
        pb->pb_stats->num_pins_of_net_in_pb.clear();

        if (pb->pb_stats->feasible_blocks) {
            free(pb->pb_stats->feasible_blocks);
        }
        if (!pb->parent_pb) {
            pb->pb_stats->transitive_fanout_candidates.clear();
        }
        delete pb->pb_stats;
        pb->pb_stats = nullptr;
    }
}

/***************************************************************************************
 * Y.G.THIEN
 * 29 AUG 2012
 *
 * The following functions maps the block pins indices for all block types to the      *
 * corresponding port indices and port_pin indices. This is necessary since there are  *
 * different netlist conventions - in the cluster level, ports and port pins are used  *
 * while in the post-pack level, block pins are used.                                  *
 *                                                                                     *
 ***************************************************************************************/

void get_blk_pin_from_port_pin(int blk_type_index, int port, int port_pin, int* blk_pin) {
    /* This mapping is needed since there are two different netlist                         *
     * conventions - in the cluster level, ports and port pins are used                     *
     * while in the post-pack level, block pins are used. The reason block                  *
     * type is used instead of blocks is to save memories.                                  *
     *                                                                                      *
     * f_port_pin_to_block_pin array allows us to quickly find what block                   *
     * pin a port pin corresponds to.                                                       *
     * [0...device_ctx.logical_block_types.size()-1][0...num_ports-1][0...num_port_pins-1]  */

    /* If the array is not allocated and loaded, allocate it.                */
    if (f_blk_pin_from_port_pin == nullptr) {
        alloc_and_load_blk_pin_from_port_pin();
    }

    /* Return the port and port_pin for the pin.                             */
    *blk_pin = f_blk_pin_from_port_pin[blk_type_index][port][port_pin];
}

void free_blk_pin_from_port_pin() {
    /* Frees the f_blk_pin_from_port_pin array.               *
     *                                                        *
     * This function is called when the arrays are freed in   *
     * free_placement_structs()                               */

    int iport, num_ports;
    auto& device_ctx = g_vpr_ctx.device();

    if (f_blk_pin_from_port_pin != nullptr) {
        for (const auto& type : device_ctx.physical_tile_types) {
            int itype = type.index;

            // Avoid EMPTY_PHYSICAL_TILE_TYPE
            if (itype == 0) {
                continue;
            }

            num_ports = type.ports.size();
            for (iport = 0; iport < num_ports; iport++) {
                free(f_blk_pin_from_port_pin[itype][iport]);
            }
            free(f_blk_pin_from_port_pin[itype]);
        }
        free(f_blk_pin_from_port_pin);

        f_blk_pin_from_port_pin = nullptr;
    }
}

static void alloc_and_load_blk_pin_from_port_pin() {
    /* Allocates and loads blk_pin_from_port_pin array.                      *
     *                                                                       *
     * The arrays are freed in free_placement_structs()                      */

    int*** temp_blk_pin_from_port_pin = nullptr;
    unsigned int itype;
    int iport, iport_pin;
    int blk_pin_count, num_port_pins, num_ports;
    auto& device_ctx = g_vpr_ctx.device();

    /* Allocate and initialize the values to OPEN (-1). */
    temp_blk_pin_from_port_pin = (int***)vtr::malloc(device_ctx.physical_tile_types.size() * sizeof(int**));
    for (itype = 1; itype < device_ctx.physical_tile_types.size(); itype++) {
        num_ports = device_ctx.physical_tile_types[itype].ports.size();
        temp_blk_pin_from_port_pin[itype] = (int**)vtr::malloc(num_ports * sizeof(int*));
        for (iport = 0; iport < num_ports; iport++) {
            num_port_pins = device_ctx.physical_tile_types[itype].ports[iport].num_pins;
            temp_blk_pin_from_port_pin[itype][iport] = (int*)vtr::malloc(num_port_pins * sizeof(int));

            for (iport_pin = 0; iport_pin < num_port_pins; iport_pin++) {
                temp_blk_pin_from_port_pin[itype][iport][iport_pin] = OPEN;
            }
        }
    }

    /* Load the values */
    /* itype starts from 1 since device_ctx.block_types[0] is the EMPTY_PHYSICAL_TILE_TYPE. */
    for (itype = 1; itype < device_ctx.physical_tile_types.size(); itype++) {
        blk_pin_count = 0;
        num_ports = device_ctx.physical_tile_types[itype].ports.size();
        for (iport = 0; iport < num_ports; iport++) {
            num_port_pins = device_ctx.physical_tile_types[itype].ports[iport].num_pins;
            for (iport_pin = 0; iport_pin < num_port_pins; iport_pin++) {
                temp_blk_pin_from_port_pin[itype][iport][iport_pin] = blk_pin_count;
                blk_pin_count++;
            }
        }
    }

    /* Sets the file_scope variables to point at the arrays. */
    f_blk_pin_from_port_pin = temp_blk_pin_from_port_pin;
}

/***************************************************************************************
 * Y.G.THIEN
 * 30 AUG 2012
 *
 * The following functions parses the direct connections' information obtained from    *
 * the arch file. Then, the functions map the block pins indices for all block types   *
 * to the corresponding idirect (the index of the direct connection as specified in    *
 * the arch file) and direct type (whether this pin is a SOURCE or a SINK for the      *
 * direct connection). If a pin is not part of any direct connections, the value       *
 * OPEN (-1) is stored in both entries.                                                *
 *                                                                                     *
 * The mapping arrays are freed by the caller. Currently, this mapping is only used to *
 * load placement macros in place_macro.c                                              *
 *                                                                                     *
 ***************************************************************************************/

void parse_direct_pin_name(char* src_string, int line, int* start_pin_index, int* end_pin_index, char* pb_type_name, char* port_name) {
    /* Parses out the pb_type_name and port_name from the direct passed in.   *
     * If the start_pin_index and end_pin_index is specified, parse them too. *
     * Return the values parsed by reference.                                 */

    char source_string[MAX_STRING_LEN + 1];
    char* find_format = nullptr;
    int ichar, match_count;

    if (vtr::split(src_string).size() > 1) {
        VPR_THROW(VPR_ERROR_ARCH,
                  "Only a single port pin range specification allowed for direct connect (was: '%s')", src_string);
    }

    // parse out the pb_type and port name, possibly pin_indices
    find_format = strstr(src_string, "[");
    if (find_format == nullptr) {
        /* Format "pb_type_name.port_name" */
        *start_pin_index = *end_pin_index = -1;

        if (strlen(src_string) + 1 <= MAX_STRING_LEN + 1) {
            strcpy(source_string, src_string);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                            "Pin name exceeded buffer size of %zu characters", MAX_STRING_LEN + 1);
        }
        for (ichar = 0; ichar < (int)(strlen(source_string)); ichar++) {
            if (source_string[ichar] == '.')
                source_string[ichar] = ' ';
        }

        match_count = sscanf(source_string, "%s %s", pb_type_name, port_name);
        if (match_count != 2) {
            VTR_LOG_ERROR(
                "[LINE %d] Invalid pin - %s, name should be in the format "
                "\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
                "The end_pin_index and start_pin_index can be the same.\n",
                line, src_string);
            exit(1);
        }
    } else {
        /* Format "pb_type_name.port_name[end_pin_index:start_pin_index]" */
        strcpy(source_string, src_string);
        for (ichar = 0; ichar < (int)(strlen(source_string)); ichar++) {
            //Need white space between the components when using %s with
            //sscanf
            if (source_string[ichar] == '.')
                source_string[ichar] = ' ';
            if (source_string[ichar] == '[')
                source_string[ichar] = ' ';
        }

        match_count = sscanf(source_string, "%s %s %d:%d]",
                             pb_type_name, port_name,
                             end_pin_index, start_pin_index);
        if (match_count != 4) {
            VTR_LOG_ERROR(
                "[LINE %d] Invalid pin - %s, name should be in the format "
                "\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
                "The end_pin_index and start_pin_index can be the same.\n",
                line, src_string);
            exit(1);
        }
        if (*end_pin_index < 0 || *start_pin_index < 0) {
            VTR_LOG_ERROR(
                "[LINE %d] Invalid pin - %s, the pin_index in "
                "[end_pin_index:start_pin_index] should not be a negative value.\n",
                line, src_string);
            exit(1);
        }
        if (*end_pin_index < *start_pin_index) {
            VTR_LOG_ERROR(
                "[LINE %d] Invalid from_pin - %s, the end_pin_index in "
                "[end_pin_index:start_pin_index] should not be less than start_pin_index.\n",
                line, src_string);
            exit(1);
        }
    }
}

static void mark_direct_of_pins(int start_pin_index, int end_pin_index, int itype, int iport, int** idirect_from_blk_pin, int idirect, int** direct_type_from_blk_pin, int direct_type, int line, char* src_string) {
    /* Mark the pin entry in idirect_from_blk_pin with idirect and the pin entry in    *
     * direct_type_from_blk_pin with direct_type from start_pin_index to               *
     * end_pin_index.                                                                  */

    int iport_pin, iblk_pin;
    auto& device_ctx = g_vpr_ctx.device();

    // Mark pins with indices from start_pin_index to end_pin_index, inclusive
    for (iport_pin = start_pin_index; iport_pin <= end_pin_index; iport_pin++) {
        get_blk_pin_from_port_pin(itype, iport, iport_pin, &iblk_pin);

        //iterate through all segment connections and check if all Fc's are 0
        bool all_fcs_0 = true;
        for (const auto& fc_spec : device_ctx.physical_tile_types[itype].fc_specs) {
            for (int ipin : fc_spec.pins) {
                if (iblk_pin == ipin && fc_spec.fc_value > 0) {
                    all_fcs_0 = false;
                    break;
                }
            }
            if (!all_fcs_0) break;
        }

        // Check the fc for the pin, direct chain link only if fc == 0
        if (all_fcs_0) {
            idirect_from_blk_pin[itype][iblk_pin] = idirect;

            // Check whether the pins are marked, errors out if so
            if (direct_type_from_blk_pin[itype][iblk_pin] != OPEN) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "[LINE %d] Invalid pin - %s, this pin is in more than one direct connection.\n",
                                line, src_string);
            } else {
                direct_type_from_blk_pin[itype][iblk_pin] = direct_type;
            }
        }
    } // Finish marking all the pins
}

static void mark_direct_of_ports(int idirect, int direct_type, char* pb_type_name, char* port_name, int end_pin_index, int start_pin_index, char* src_string, int line, int** idirect_from_blk_pin, int** direct_type_from_blk_pin) {
    /* Go through all the ports in all the blocks to find the port that has the same   *
     * name as port_name and belongs to the block type that has the name pb_type_name. *
     * Then, check that whether start_pin_index and end_pin_index are specified. If    *
     * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
     * Otherwise, mark down all the pins in that port.                                 */

    int num_ports, num_port_pins;
    unsigned int itype;
    int iport;
    auto& device_ctx = g_vpr_ctx.device();

    // Go through all the block types
    for (itype = 1; itype < device_ctx.physical_tile_types.size(); itype++) {
        auto& physical_tile = device_ctx.physical_tile_types[itype];
        // Find blocks with the same pb_type_name
        if (strcmp(physical_tile.name, pb_type_name) == 0) {
            num_ports = physical_tile.ports.size();
            for (iport = 0; iport < num_ports; iport++) {
                // Find ports with the same port_name
                if (strcmp(physical_tile.ports[iport].name, port_name) == 0) {
                    num_port_pins = physical_tile.ports[iport].num_pins;

                    // Check whether the end_pin_index is valid
                    if (end_pin_index > num_port_pins) {
                        VTR_LOG_ERROR(
                            "[LINE %d] Invalid pin - %s, the end_pin_index in "
                            "[end_pin_index:start_pin_index] should "
                            "be less than the num_port_pins %d.\n",
                            line, src_string, num_port_pins);
                        exit(1);
                    }

                    // Check whether the pin indices are specified
                    if (start_pin_index >= 0 || end_pin_index >= 0) {
                        mark_direct_of_pins(start_pin_index, end_pin_index, itype,
                                            iport, idirect_from_blk_pin, idirect,
                                            direct_type_from_blk_pin, direct_type, line, src_string);
                    } else {
                        mark_direct_of_pins(0, num_port_pins - 1, itype,
                                            iport, idirect_from_blk_pin, idirect,
                                            direct_type_from_blk_pin, direct_type, line, src_string);
                    }
                } // Do nothing if port_name does not match
            }     // Finish going through all the ports
        }         // Do nothing if pb_type_name does not match
    }             // Finish going through all the blocks
}

void alloc_and_load_idirect_from_blk_pin(t_direct_inf* directs, int num_directs, int*** idirect_from_blk_pin, int*** direct_type_from_blk_pin) {
    /* Allocates and loads idirect_from_blk_pin and direct_type_from_blk_pin arrays.    *
     *                                                                                  *
     * For a bus (multiple bits) direct connection, all the pins in the bus are marked. *
     *                                                                                  *
     * idirect_from_blk_pin array allow us to quickly find pins that could be in a      *
     * direct connection. Values stored is the index of the possible direct connection  *
     * as specified in the arch file, OPEN (-1) is stored for pins that could not be    *
     * part of a direct chain conneciton.                                               *
     *                                                                                  *
     * direct_type_from_blk_pin array stores the value SOURCE if the pin is the         *
     * from_pin, SINK if the pin is the to_pin in the direct connection as specified in *
     * the arch file, OPEN (-1) is stored for pins that could not be part of a direct   *
     * chain conneciton.                                                                *
     *                                                                                  *
     * Stores the pointers to the two 2D arrays in the addresses passed in.             *
     *                                                                                  *
     * The two arrays are freed by the caller(s).                                       */

    int iblk_pin, idirect, num_type_pins;
    int **temp_idirect_from_blk_pin, **temp_direct_type_from_blk_pin;

    char to_pb_type_name[MAX_STRING_LEN + 1], to_port_name[MAX_STRING_LEN + 1],
        from_pb_type_name[MAX_STRING_LEN + 1], from_port_name[MAX_STRING_LEN + 1];
    int to_start_pin_index = -1, to_end_pin_index = -1;
    int from_start_pin_index = -1, from_end_pin_index = -1;
    auto& device_ctx = g_vpr_ctx.device();

    /* Allocate and initialize the values to OPEN (-1). */
    temp_idirect_from_blk_pin = (int**)vtr::malloc(device_ctx.physical_tile_types.size() * sizeof(int*));
    temp_direct_type_from_blk_pin = (int**)vtr::malloc(device_ctx.physical_tile_types.size() * sizeof(int*));
    for (const auto& type : device_ctx.physical_tile_types) {
        if (is_empty_type(&type)) continue;

        int itype = type.index;
        num_type_pins = type.num_pins;

        temp_idirect_from_blk_pin[itype] = (int*)vtr::malloc(num_type_pins * sizeof(int));
        temp_direct_type_from_blk_pin[itype] = (int*)vtr::malloc(num_type_pins * sizeof(int));

        /* Initialize values to OPEN */
        for (iblk_pin = 0; iblk_pin < num_type_pins; iblk_pin++) {
            temp_idirect_from_blk_pin[itype][iblk_pin] = OPEN;
            temp_direct_type_from_blk_pin[itype][iblk_pin] = OPEN;
        }
    }

    /* Load the values */
    // Go through directs and find pins with possible direct connections
    for (idirect = 0; idirect < num_directs; idirect++) {
        // Parse out the pb_type and port name, possibly pin_indices from from_pin
        parse_direct_pin_name(directs[idirect].from_pin, directs[idirect].line,
                              &from_end_pin_index, &from_start_pin_index, from_pb_type_name, from_port_name);

        // Parse out the pb_type and port name, possibly pin_indices from to_pin
        parse_direct_pin_name(directs[idirect].to_pin, directs[idirect].line,
                              &to_end_pin_index, &to_start_pin_index, to_pb_type_name, to_port_name);

        /* Now I have all the data that I need, I could go through all the block pins   *
         * in all the blocks to find all the pins that could have possible direct       *
         * connections. Mark all down all those pins with the idirect the pins belong   *
         * to and whether it is a source or a sink of the direct connection.            */

        // Find blocks with the same name as from_pb_type_name and from_port_name
        mark_direct_of_ports(idirect, SOURCE, from_pb_type_name, from_port_name,
                             from_end_pin_index, from_start_pin_index, directs[idirect].from_pin,
                             directs[idirect].line,
                             temp_idirect_from_blk_pin, temp_direct_type_from_blk_pin);

        // Then, find blocks with the same name as to_pb_type_name and from_port_name
        mark_direct_of_ports(idirect, SINK, to_pb_type_name, to_port_name,
                             to_end_pin_index, to_start_pin_index, directs[idirect].to_pin,
                             directs[idirect].line,
                             temp_idirect_from_blk_pin, temp_direct_type_from_blk_pin);

    } // Finish going through all the directs

    /* Returns the pointer to the 2D arrays by reference. */
    *idirect_from_blk_pin = temp_idirect_from_blk_pin;
    *direct_type_from_blk_pin = temp_direct_type_from_blk_pin;
}

/*
 * this function is only called by print_switch_usage()
 * at the point of this function call, every switch type / fanin combination
 * has a unique index.
 * but for switch usage analysis, we need to convert the index back to the
 * type / fanin combination
 */
static int convert_switch_index(int* switch_index, int* fanin) {
    if (*switch_index == -1)
        return 1;

    auto& device_ctx = g_vpr_ctx.device();

    for (int iswitch = 0; iswitch < device_ctx.num_arch_switches; iswitch++) {
        for (auto itr = device_ctx.switch_fanin_remap[iswitch].begin(); itr != device_ctx.switch_fanin_remap[iswitch].end(); itr++) {
            if (itr->second == *switch_index) {
                *switch_index = iswitch;
                *fanin = itr->first;
                return 0;
            }
        }
    }
    *switch_index = -1;
    *fanin = -1;
    VTR_LOG("\n\nerror converting switch index ! \n\n");
    return -1;
}

/*
 * print out number of usage for every switch (type / fanin combination)
 * (referring to rr_graph.c: alloc_rr_switch_inf())
 * NOTE: to speed up this function, for XXX uni-directional arch XXX, the most efficient
 * way is to change the device_ctx.rr_nodes data structure (let it store the inward switch index,
 * instead of outward switch index list): --> instead of using a nested loop of
 *     for (inode in rr_nodes) {
 *         for (iedges in edges) {
 *             get switch type;
 *             get fanin;
 *         }
 *     }
 * as is done in rr_graph.c: alloc_rr_switch_inf()
 * we can just use a single loop
 *     for (inode in rr_nodes) {
 *         get switch type of inode;
 *         get fanin of inode;
 *     }
 * now since device_ctx.rr_nodes does not contain the switch type inward to the current node,
 * we have to use an extra loop to setup the information of inward switch first.
 */
void print_switch_usage() {
    auto& device_ctx = g_vpr_ctx.device();

    if (device_ctx.switch_fanin_remap.empty()) {
        VTR_LOG_WARN("Cannot print switch usage stats: device_ctx.switch_fanin_remap is empty\n");
        return;
    }
    std::map<int, int>* switch_fanin_count;
    std::map<int, float>* switch_fanin_delay;
    switch_fanin_count = new std::map<int, int>[device_ctx.num_arch_switches];
    switch_fanin_delay = new std::map<int, float>[device_ctx.num_arch_switches];
    // a node can have multiple inward switches, so
    // map key: switch index; map value: count (fanin)
    std::map<int, int>* inward_switch_inf = new std::map<int, int>[device_ctx.rr_nodes.size()];
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        const t_rr_node& from_node = device_ctx.rr_nodes[inode];
        int num_edges = from_node.num_edges();
        for (int iedge = 0; iedge < num_edges; iedge++) {
            int switch_index = from_node.edge_switch(iedge);
            int to_node_index = from_node.edge_sink_node(iedge);
            // Assumption: suppose for a L4 wire (bi-directional): ----+----+----+----, it can be driven from any point (0, 1, 2, 3).
            //             physically, the switch driving from point 1 & 3 should be the same. But we will assign then different switch
            //             index; or there is no way to differentiate them after abstracting a 2D wire into a 1D node
            if (inward_switch_inf[to_node_index].count(switch_index) == 0)
                inward_switch_inf[to_node_index][switch_index] = 0;
            //VTR_ASSERT(from_node.type != OPIN);
            inward_switch_inf[to_node_index][switch_index]++;
        }
    }
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        std::map<int, int>::iterator itr;
        for (itr = inward_switch_inf[inode].begin(); itr != inward_switch_inf[inode].end(); itr++) {
            int switch_index = itr->first;
            int fanin = itr->second;
            float Tdel = device_ctx.rr_switch_inf[switch_index].Tdel;
            int status = convert_switch_index(&switch_index, &fanin);
            if (status == -1) {
                delete[] switch_fanin_count;
                delete[] switch_fanin_delay;
                delete[] inward_switch_inf;
                return;
            }
            if (switch_fanin_count[switch_index].count(fanin) == 0) {
                switch_fanin_count[switch_index][fanin] = 0;
            }
            switch_fanin_count[switch_index][fanin]++;
            switch_fanin_delay[switch_index][fanin] = Tdel;
        }
    }
    VTR_LOG("\n=============== switch usage stats ===============\n");
    for (int iswitch = 0; iswitch < device_ctx.num_arch_switches; iswitch++) {
        char* s_name = device_ctx.arch_switch_inf[iswitch].name;
        float s_area = device_ctx.arch_switch_inf[iswitch].mux_trans_size;
        VTR_LOG(">>>>> switch index: %d, name: %s, mux trans size: %g\n", iswitch, s_name, s_area);

        std::map<int, int>::iterator itr;
        for (itr = switch_fanin_count[iswitch].begin(); itr != switch_fanin_count[iswitch].end(); itr++) {
            VTR_LOG("\t\tnumber of fanin: %d", itr->first);
            VTR_LOG("\t\tnumber of wires driven by this switch: %d", itr->second);
            VTR_LOG("\t\tTdel: %g\n", switch_fanin_delay[iswitch][itr->first]);
        }
    }
    VTR_LOG("\n==================================================\n\n");
    delete[] switch_fanin_count;
    delete[] switch_fanin_delay;
    delete[] inward_switch_inf;
}

/*
 * Motivation:
 *     to see what portion of long wires are utilized
 *     potentially a good measure for router look ahead quality
 */
/*
 * void print_usage_by_wire_length() {
 * map<int, int> used_wire_count;
 * map<int, int> total_wire_count;
 * auto& device_ctx = g_vpr_ctx.device();
 * for (int inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
 * if (device_ctx.rr_nodes[inode].type() == CHANX || rr_node[inode].type() == CHANY) {
 * //int length = abs(device_ctx.rr_nodes[inode].get_xhigh() + rr_node[inode].get_yhigh()
 * //             - device_ctx.rr_nodes[inode].get_xlow() - rr_node[inode].get_ylow());
 * int length = device_ctx.rr_nodes[inode].get_length();
 * if (rr_node_route_inf[inode].occ() > 0) {
 * if (used_wire_count.count(length) == 0)
 * used_wire_count[length] = 0;
 * used_wire_count[length] ++;
 * }
 * if (total_wire_count.count(length) == 0)
 * total_wire_count[length] = 0;
 * total_wire_count[length] ++;
 * }
 * }
 * int total_wires = 0;
 * map<int, int>::iterator itr;
 * for (itr = total_wire_count.begin(); itr != total_wire_count.end(); itr++) {
 * total_wires += itr->second;
 * }
 * VTR_LOG("\n\t-=-=-=-=-=-=-=-=-=-=- wire usage stats -=-=-=-=-=-=-=-=-=-=-\n");
 * for (itr = total_wire_count.begin(); itr != total_wire_count.end(); itr++)
 * VTR_LOG("\ttotal number: wire of length %d, ratio to all length of wires: %g\n", itr->first, ((float)itr->second) / total_wires);
 * for (itr = used_wire_count.begin(); itr != used_wire_count.end(); itr++) {
 * float ratio_to_same_type_total = ((float)itr->second) / total_wire_count[itr->first];
 * float ratio_to_all_type_total = ((float)itr->second) / total_wires;
 * VTR_LOG("\t\tratio to same type of wire: %g\tratio to all types of wire: %g\n", ratio_to_same_type_total, ratio_to_all_type_total);
 * }
 * VTR_LOG("\n\t-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n");
 * used_wire_count.clear();
 * total_wire_count.clear();
 * }
 */

void place_sync_external_block_connections(ClusterBlockId iblk) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    VTR_ASSERT_MSG(place_ctx.block_locs[iblk].nets_and_pins_synced_to_z_coordinate == false, "Block net and pins must not be already synced");

    auto type = physical_tile_type(iblk);
    VTR_ASSERT(type->num_pins % type->capacity == 0);
    int max_num_block_pins = type->num_pins / type->capacity;
    /* Logical location and physical location is offset by z * max_num_block_pins */

    auto& clb_nlist = cluster_ctx.clb_nlist;
    for (auto pin : clb_nlist.block_pins(iblk)) {
        int orig_phys_pin_index = clb_nlist.pin_physical_index(pin);
        int new_phys_pin_index = orig_phys_pin_index + place_ctx.block_locs[iblk].loc.z * max_num_block_pins;
        clb_nlist.set_pin_physical_index(pin, new_phys_pin_index);
    }

    //Mark the block as synced
    place_ctx.block_locs[iblk].nets_and_pins_synced_to_z_coordinate = true;
}

int get_max_num_pins(t_logical_block_type_ptr logical_block) {
    int max_num_pins = 0;

    for (auto physical_tile : logical_block->equivalent_tiles) {
        max_num_pins = std::max(max_num_pins, physical_tile->num_pins);
    }

    return max_num_pins;
}

int max_pins_per_grid_tile() {
    auto& device_ctx = g_vpr_ctx.device();
    int max_pins = 0;
    for (auto& type : device_ctx.physical_tile_types) {
        int pins_per_grid_tile = type.num_pins / (type.width * type.height);
        //Use the maximum number of pins normalized by block area
        max_pins = std::max(max_pins, pins_per_grid_tile);
    }
    return max_pins;
}

bool is_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block) {
    auto equivalent_tiles = logical_block->equivalent_tiles;
    return std::find(equivalent_tiles.begin(), equivalent_tiles.end(), physical_tile) != equivalent_tiles.end();
}

t_physical_tile_type_ptr pick_random_physical_type(t_logical_block_type_ptr logical_block) {
    auto equivalent_tiles = logical_block->equivalent_tiles;

    size_t num_equivalent_tiles = equivalent_tiles.size();
    int index = 0;

    if (num_equivalent_tiles > 1) {
        index = vtr::irand((int)equivalent_tiles.size() - 1);
    }

    return equivalent_tiles[index];
}

int get_logical_pin(t_physical_tile_type_ptr physical_tile,
                    t_logical_block_type_ptr logical_block,
                    int pin) {
    t_physical_pin physical_pin(pin);

    auto direct_map = physical_tile->tile_block_pin_directs_map.at(logical_block->index);
    auto result = direct_map.find(physical_pin);

    if (result == direct_map.inverse_end()) {
        VTR_LOG_WARN(
            "Couldn't find the corresponding logical pin of the physical pin %d."
            "Physical Tile: %s, Logical Block: %s.\n",
            pin, physical_tile->name, logical_block->name);
        return OPEN;
    }

    return result->second.pin;
}

int get_physical_pin(t_physical_tile_type_ptr physical_tile,
                     t_logical_block_type_ptr logical_block,
                     int pin) {
    t_logical_pin logical_pin(pin);

    auto direct_map = physical_tile->tile_block_pin_directs_map.at(logical_block->index);
    auto result = direct_map.find(logical_pin);

    if (result == direct_map.end()) {
        VTR_LOG_WARN(
            "Couldn't find the corresponding physical pin of the logical pin %d."
            "Physical Tile: %s, Logical Block: %s.\n",
            pin, physical_tile->name, logical_block->name);
        return OPEN;
    }

    return result->second.pin;
}

void pretty_print_uint(const char* prefix, size_t value, int num_digits, int scientific_precision) {
    //Print as integer if it will fit in the width, other wise scientific
    if (value <= std::pow(10, num_digits) - 1) {
        //Direct
        VTR_LOG("%s%*zu", prefix, num_digits, value);
    } else {
        //Scientific
        VTR_LOG("%s%#*.*g", prefix, num_digits, scientific_precision, float(value));
    }
}

void pretty_print_float(const char* prefix, double value, int num_digits, int scientific_precision) {
    //Print as float if it will fit in the width, other wise scientific

    //How many whole digits are there in non-scientific style?
    size_t whole_digits = num_digits - scientific_precision - 1; //-1 for decimal point
    if (value <= std::pow(10, whole_digits) - 1) {
        //Direct
        VTR_LOG("%s%*.*f", prefix, num_digits, scientific_precision, value);
    } else {
        //Scientific
        VTR_LOG("%s%#*.*g", prefix, num_digits, scientific_precision + 1, value);
    }
}
