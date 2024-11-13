#include <unordered_set>
#include <regex>
#include <algorithm>
#include <sstream>

#include "pack_types.h"
#include "prepack.h"
#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "physical_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "cluster_placement.h"
#include "device_grid.h"
#include "user_route_constraints.h"
#include "placer_state.h"
#include "grid_block.h"

/* This module contains subroutines that are used in several unrelated parts *
 * of VPR.  They are VPR-specific utility routines.                          */

/******************** File-scope variables declarations **********************/
//Regular expressions used to determine register and logic primitives
//TODO: Make this set-able from command-line?
const std::regex REGISTER_MODEL_REGEX("(.subckt\\s+)?.*(latch|dff).*", std::regex::icase);
const std::regex LOGIC_MODEL_REGEX("(.subckt\\s+)?.*(lut|names|lcell).*", std::regex::icase);

/******************** Subroutine declarations ********************************/
static void load_pb_graph_pin_lookup_from_index_rec(t_pb_graph_pin** pb_graph_pin_lookup_from_index, t_pb_graph_node* pb_graph_node);

static void load_pin_id_to_pb_mapping_rec(t_pb* cur_pb, t_pb** pin_id_to_pb_mapping);

static std::vector<int> find_connected_internal_clb_sink_pins(ClusterBlockId clb, int pb_pin);
static void find_connected_internal_clb_sink_pins_recurr(ClusterBlockId clb, int pb_pin, std::vector<int>& connected_sink_pb_pins);
static AtomPinId find_atom_pin_for_pb_route_id(ClusterBlockId clb, int pb_route_id, const IntraLbPbPinLookup& pb_gpin_lookup);

static bool block_type_contains_blif_model(t_logical_block_type_ptr type, const std::regex& blif_model_regex);
static bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::regex& blif_model_regex);
static t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_logical_block_type_ptr type);
static void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type);

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
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find port '%s; on architecture model '%s'\n", name.c_str(), model->name);
    }

    return nullptr;
}

/**
 * print tabs given number of tabs to file
 */
void print_tabs(FILE* fpout, int num_tab) {
    for (int i = 0; i < num_tab; i++) {
        fprintf(fpout, "\t");
    }
}

std::string rr_node_arch_name(RRNodeId inode, bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    auto rr_node = inode;

    std::string rr_node_arch_name;
    if (rr_graph.node_type(inode) == OPIN || rr_graph.node_type(inode) == IPIN) {
        //Pin names
        auto type = device_ctx.grid.get_physical_type({rr_graph.node_xlow(rr_node),
                                                       rr_graph.node_ylow(rr_node),
                                                       rr_graph.node_layer(rr_node)});
        rr_node_arch_name += block_type_pin_index_to_name(type, rr_graph.node_pin_num(rr_node), is_flat);
    } else if (rr_graph.node_type(inode) == SOURCE || rr_graph.node_type(inode) == SINK) {
        //Set of pins associated with SOURCE/SINK
        auto type = device_ctx.grid.get_physical_type({rr_graph.node_xlow(rr_node),
                                                       rr_graph.node_ylow(rr_node),
                                                       rr_graph.node_layer(rr_node)});
        auto pin_names = block_type_class_index_to_pin_names(type, rr_graph.node_class_num(rr_node), is_flat);
        if (pin_names.size() > 1) {
            rr_node_arch_name += rr_graph.node_type_string(inode);
            rr_node_arch_name += " connected to ";
            rr_node_arch_name += "{";
            rr_node_arch_name += vtr::join(pin_names, ", ");
            rr_node_arch_name += "}";
        } else {
            rr_node_arch_name += pin_names[0];
        }
    } else {
        VTR_ASSERT(rr_graph.node_type(inode) == CHANX || rr_graph.node_type(inode) == CHANY);
        //Wire segment name
        auto cost_index = rr_graph.node_cost_index(inode);
        int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;

        rr_node_arch_name += rr_graph.rr_segments(RRSegmentId(seg_index)).name;
    }

    return rr_node_arch_name;
}

IntraLbPbPinLookup::IntraLbPbPinLookup(const std::vector<t_logical_block_type>& block_types)
    : block_types_(block_types)
    , intra_lb_pb_pin_lookup_(block_types.size(), nullptr) {
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
    for (unsigned int itype = 0; itype < intra_lb_pb_pin_lookup_.size(); ++itype) {
        free_pb_graph_pin_lookup_from_index(intra_lb_pb_pin_lookup_[itype]);
    }
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
std::vector<AtomPinId> find_clb_pin_connected_atom_pins(ClusterBlockId clb, int logical_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    std::vector<AtomPinId> atom_pins;
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    auto logical_block = clb_nlist.block_type(clb);
    auto physical_tile = pick_physical_type(logical_block);

    int physical_pin = get_physical_pin(physical_tile, logical_block, logical_pin);
    VTR_ASSERT(physical_pin >= 0);

    if (is_opin(physical_pin, physical_tile)) {
        //output
        AtomPinId driver = find_clb_pin_driver_atom_pin(clb, logical_pin, pb_gpin_lookup);
        if (driver) {
            atom_pins.push_back(driver);
        }
    } else {
        //input
        atom_pins = find_clb_pin_sink_atom_pins(clb, logical_pin, pb_gpin_lookup);
    }

    return atom_pins;
}

//Returns the atom pin which drives the top level clb output pin
AtomPinId find_clb_pin_driver_atom_pin(ClusterBlockId clb, int logical_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    if (logical_pin < 0) {
        //CLB output pin has no internal driver
        return AtomPinId::INVALID();
    }
    const t_pb_routes& pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;
    AtomNetId atom_net = pb_routes[logical_pin].atom_net_id;

    int pb_pin_id = logical_pin;
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
std::vector<AtomPinId> find_clb_pin_sink_atom_pins(ClusterBlockId clb, int logical_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    const t_pb_routes& pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;

    VTR_ASSERT_MSG(logical_pin < cluster_ctx.clb_nlist.block_type(clb)->pb_type->num_pins, "Must be a valid tile pin");

    VTR_ASSERT(cluster_ctx.clb_nlist.block_pb(clb));
    VTR_ASSERT_MSG(logical_pin < cluster_ctx.clb_nlist.block_pb(clb)->pb_graph_node->num_pins(), "Pin must map to a top-level pb pin");

    VTR_ASSERT_MSG(pb_routes[logical_pin].driver_pb_pin_id < 0, "CLB input pin should have no internal drivers");

    AtomNetId atom_net = pb_routes[logical_pin].atom_net_id;
    VTR_ASSERT(atom_net);

    std::vector<int> connected_sink_pb_pins = find_connected_internal_clb_sink_pins(clb, logical_pin);

    std::vector<AtomPinId> sink_atom_pins;
    for (int sink_pb_pin : connected_sink_pb_pins) {
        //Map the logical_pin_id to AtomPinId
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

/* Return the net pin which drives the CLB input connected to sink_pb_pin_id, or nullptr if none (i.e. driven internally)
 *   clb: Block on which the sink pin is located
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

    //curr_pb_pin should be a top-level CLB input
    ClusterNetId clb_net_idx = cluster_ctx.clb_nlist.block_net(clb, curr_pb_pin_id);
    int clb_net_pin_idx = cluster_ctx.clb_nlist.block_pin_net_index(clb, curr_pb_pin_id);

    /* The net could be remapped, we should verify the remapped location */
    auto remapped_clb = cluster_ctx.post_routing_clb_pin_nets.find(clb);
    if (remapped_clb != cluster_ctx.post_routing_clb_pin_nets.end()) {
        auto remapped_result = remapped_clb->second.find(curr_pb_pin_id);
        if ((remapped_result != remapped_clb->second.end())
            && (remapped_result->second != clb_net_idx)) {
            clb_net_idx = remapped_result->second;
            VTR_ASSERT(clb_net_idx);
            clb_net_pin_idx = cluster_ctx.clb_nlist.block_pin_net_index(clb, cluster_ctx.pre_routing_net_pin_mapping.at(clb).at(curr_pb_pin_id));
        }
    }
    VTR_ASSERT(clb_net_pin_idx >= 0);

    return std::tuple<ClusterNetId, int, int>(clb_net_idx, curr_pb_pin_id, clb_net_pin_idx);
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

bool is_empty_type(t_physical_tile_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE;
}

bool is_empty_type(t_logical_block_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return type == device_ctx.EMPTY_LOGICAL_BLOCK_TYPE;
}

t_physical_tile_type_ptr physical_tile_type(t_pl_loc loc) {
    auto& device_ctx = g_vpr_ctx.device();

    return device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});
}

t_physical_tile_type_ptr physical_tile_type(AtomBlockId atom_blk) {
    auto& atom_look_up = g_vpr_ctx.atom().lookup;
    auto& block_locs = g_vpr_ctx.placement().block_locs();

    ClusterBlockId cluster_blk = atom_look_up.atom_clb(atom_blk);
    VTR_ASSERT(cluster_blk != ClusterBlockId::INVALID());

    return physical_tile_type(block_locs[cluster_blk].loc);
}

t_physical_tile_type_ptr physical_tile_type(ParentBlockId blk_id, bool is_flat) {
    auto& block_locs = g_vpr_ctx.placement().block_locs();

    if (is_flat) {
        return physical_tile_type(convert_to_atom_block_id(blk_id));
    } else {
        ClusterBlockId cluster_blk_id = convert_to_cluster_block_id(blk_id);
        t_pl_loc block_loc = block_locs[cluster_blk_id].loc;
        return physical_tile_type(block_loc);
    }
}

int get_sub_tile_index(ClusterBlockId blk,
                       const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto logical_block = cluster_ctx.clb_nlist.block_type(blk);
    auto block_loc = block_locs[blk];
    auto loc = block_loc.loc;
    int sub_tile_coordinate = loc.sub_tile;

    auto type = device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});

    for (const auto& sub_tile : type->sub_tiles) {
        if (sub_tile.capacity.is_in_range(sub_tile_coordinate)) {
            auto result = std::find(sub_tile.equivalent_sites.begin(), sub_tile.equivalent_sites.end(), logical_block);
            if (result == sub_tile.equivalent_sites.end()) {
                VPR_THROW(VPR_ERROR_PLACE, "The Block Id %d has been placed in an incompatible sub tile location.\n", blk);
            }

            return sub_tile.index;
        }
    }

    VPR_THROW(VPR_ERROR_PLACE, "The Block Id %d has been placed in an impossible sub tile location.\n", blk);
}

int get_sub_tile_index(ClusterBlockId blk) {
    auto& block_locs = g_vpr_ctx.placement().block_locs();
    return get_sub_tile_index(blk, block_locs);
}

/* Each node in the pb_graph for a top-level pb_type can be uniquely identified
 * by its pins. Since the pins in a cluster of a certain type are densely indexed,
 * this function will find the pin index (int pin_count_in_cluster) of the first
 * pin for a given pb_graph_node, and use this index value as unique identifier
 * for the node.
 */
int get_unique_pb_graph_node_id(const t_pb_graph_node* pb_graph_node) {
    if (pb_graph_node->num_input_pins != nullptr) {
        /* If input port exists on this node, return the index of the first
         * input pin as node_id.
         */
        t_pb_graph_pin first_input_pin = pb_graph_node->input_pins[0][0];
        int node_id = first_input_pin.pin_count_in_cluster;
        return node_id;
    } else {
        /* If no input port exists on node, then return the index of the first
         * output pin. Every pb_node is guaranteed to have at least an input or
         * output pin.
         */
        t_pb_graph_pin first_output_pin = pb_graph_node->output_pins[0][0];
        int node_id = first_output_pin.pin_count_in_cluster;
        return node_id;
    }
}

t_class_range get_class_range_for_block(const ClusterBlockId blk_id) {
    /* Assumes that the placement has been done so each block has a set of pins allocated to it */
    auto& block_locs = g_vpr_ctx.placement().block_locs();

    t_pl_loc block_loc = block_locs[blk_id].loc;
    auto type = physical_tile_type(block_loc);
    auto sub_tile = type->sub_tiles[get_sub_tile_index(blk_id)];
    int sub_tile_capacity = sub_tile.capacity.total();
    auto class_range = sub_tile.class_range;
    int class_range_total = class_range.high - class_range.low + 1;

    VTR_ASSERT((class_range_total) % sub_tile_capacity == 0);
    int rel_capacity = block_locs[blk_id].loc.sub_tile - sub_tile.capacity.low;

    t_class_range abs_class_range;
    abs_class_range.low = rel_capacity * (class_range_total / sub_tile_capacity) + class_range.low;
    abs_class_range.high = (rel_capacity + 1) * (class_range_total / sub_tile_capacity) - 1 + class_range.low;

    return abs_class_range;
}

t_class_range get_class_range_for_block(const AtomBlockId atom_blk) {
    auto& atom_look_up = g_vpr_ctx.atom().lookup;

    ClusterBlockId cluster_blk = atom_look_up.atom_clb(atom_blk);

    auto [physical_tile, sub_tile, sub_tile_cap, logical_block] = get_cluster_blk_physical_spec(cluster_blk);
    const t_pb_graph_node* pb_graph_node = atom_look_up.atom_pb_graph_node(atom_blk);
    VTR_ASSERT(pb_graph_node != nullptr);
    return get_pb_graph_node_class_physical_range(physical_tile,
                                                  sub_tile,
                                                  logical_block,
                                                  sub_tile_cap,
                                                  pb_graph_node);
}

t_class_range get_class_range_for_block(const ParentBlockId blk_id, bool is_flat) {
    if (is_flat) {
        return get_class_range_for_block(convert_to_atom_block_id(blk_id));
    } else {
        return get_class_range_for_block(convert_to_cluster_block_id(blk_id));
    }
}

std::pair<int, int> get_pin_range_for_block(const ClusterBlockId blk_id) {
    /* Assumes that the placement has been done so each block has a set of pins allocated to it */
    auto& block_locs = g_vpr_ctx.placement().block_locs();

    t_pl_loc block_loc = block_locs[blk_id].loc;
    auto type = physical_tile_type(block_loc);
    auto sub_tile = type->sub_tiles[get_sub_tile_index(blk_id)];
    int sub_tile_capacity = sub_tile.capacity.total();

    VTR_ASSERT(sub_tile.num_phy_pins % sub_tile_capacity == 0);
    int rel_capacity = block_loc.sub_tile - sub_tile.capacity.low;
    int rel_pin_low = rel_capacity * (sub_tile.num_phy_pins / sub_tile_capacity);
    int rel_pin_high = (rel_capacity + 1) * (sub_tile.num_phy_pins / sub_tile_capacity) - 1;

    int pin_low = sub_tile.sub_tile_to_tile_pin_indices[rel_pin_low];
    int pin_high = sub_tile.sub_tile_to_tile_pin_indices[rel_pin_high];

    return {pin_low, pin_high};
}

t_physical_tile_type_ptr find_tile_type_by_name(const std::string& name, const std::vector<t_physical_tile_type>& types) {
    for (auto const& type : types) {
        if (type.name == name) {
            return &type;
        }
    }
    return nullptr; //Not found
}

t_block_loc get_block_loc(const ParentBlockId& block_id, bool is_flat) {
    auto& place_ctx = g_vpr_ctx.placement();
    ClusterBlockId cluster_block_id = ClusterBlockId::INVALID();

    if (is_flat) {
        AtomBlockId atom_block_id = convert_to_atom_block_id(block_id);
        auto& atom_look_up = g_vpr_ctx.atom().lookup;
        cluster_block_id = atom_look_up.atom_clb(atom_block_id);
    } else {
        cluster_block_id = convert_to_cluster_block_id(block_id);
    }

    VTR_ASSERT(cluster_block_id != ClusterBlockId::INVALID());
    auto blk_loc = place_ctx.block_locs()[cluster_block_id];

    return blk_loc;
}

int get_block_num_class(const ParentBlockId& block_id, bool is_flat) {
    auto type = physical_tile_type(block_id, is_flat);
    return get_tile_class_max_ptc(type, is_flat);
}

int get_block_pin_class_num(const ParentBlockId block_id, const ParentPinId pin_id, bool is_flat) {
    const auto& blk_loc_registry = g_vpr_ctx.placement().blk_loc_registry();
    auto& block_locs = blk_loc_registry.block_locs();

    int class_num;

    if (is_flat) {
        AtomPinId atom_pin_id = convert_to_atom_pin_id(pin_id);
        class_num = get_atom_pin_class_num(atom_pin_id);
    } else {
        ClusterBlockId cluster_block_id = convert_to_cluster_block_id(block_id);
        t_pl_loc block_loc = block_locs[cluster_block_id].loc;
        ClusterPinId cluster_pin_id = convert_to_cluster_pin_id(pin_id);
        auto type = physical_tile_type(block_loc);
        int phys_pin = blk_loc_registry.tile_pin_index(cluster_pin_id);
        class_num = get_class_num_from_pin_physical_num(type, phys_pin);
    }

    return class_num;
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
            lhs_num_instances += grid.num_instances(type, -1);
        for (auto type : rhs->equivalent_tiles)
            rhs_num_instances += grid.num_instances(type, -1);
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
            inst_cnt += grid.num_instances(equivalent_tile, -1);
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
        size_t inst_cnt = grid.num_instances(&physical_tile, -1);

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

InstPort parse_inst_port(const std::string& str) {
    InstPort inst_port(str);

    auto& device_ctx = g_vpr_ctx.device();
    auto blk_type = find_tile_type_by_name(inst_port.instance_name(), device_ctx.physical_tile_types);
    if (blk_type == nullptr) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find block type named %s", inst_port.instance_name().c_str());
    }

    int num_pins = find_tile_port_by_name(blk_type, inst_port.port_name().c_str()).num_pins;

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

static bool block_type_contains_blif_model(t_logical_block_type_ptr type, const std::regex& blif_model_regex) {
    return pb_type_contains_blif_model(type->pb_type, blif_model_regex);
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
    int max_size;
    if (pb_type->modes == nullptr) {
        max_size = 1;
    } else {
        max_size = 0;
        int temp_size = 0;
        for (int i = 0; i < pb_type->num_modes; i++) {
            for (int j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
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
    int max_nets;
    if (pb_type->modes == nullptr) {
        max_nets = pb_type->num_output_pins;
    } else {
        max_nets = 0;
        for (int i = 0; i < pb_type->num_modes; i++) {
            int temp_nets = 0;
            for (int j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
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
    int max_depth = pb_type->depth;
    for (int i = 0; i < pb_type->num_modes; i++) {
        for (int j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
            int temp_depth = get_max_depth_of_pb_type(&pb_type->modes[i].pb_type_children[j]);
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
    if (model_port->dir == IN_PORT) {
        if (!model_port->is_clock) {
            for (int i = 0; i < pb_graph_node->num_input_ports; i++) {
                if (pb_graph_node->input_pins[i][0].port->model_port == model_port) {
                    if (pb_graph_node->num_input_pins[i] > model_pin) {
                        return &pb_graph_node->input_pins[i][model_pin];
                    } else {
                        return nullptr;
                    }
                }
            }
        } else {
            for (int i = 0; i < pb_graph_node->num_clock_ports; i++) {
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
        for (int i = 0; i < pb_graph_node->num_output_ports; i++) {
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

t_pb_graph_pin* get_pb_graph_node_pin_from_pb_graph_node(t_pb_graph_node* pb_graph_node,
                                                         int ipin) {
    int i, count;

    const t_pb_type* pb_type = pb_graph_node->pb_type;

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

t_pb_graph_pin* get_pb_graph_node_pin_from_block_pin(ClusterBlockId iblock, int ipin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_pb_graph_node* pb_graph_node = cluster_ctx.clb_nlist.block_pb(iblock)->pb_graph_node;
    return get_pb_graph_node_pin_from_pb_graph_node(pb_graph_node, ipin);
}

const t_port* find_pb_graph_port(const t_pb_graph_node* pb_gnode, const std::string& port_name) {
    const t_pb_graph_pin* gpin = find_pb_graph_pin(pb_gnode, port_name, 0);

    if (gpin != nullptr) {
        return gpin->port;
    }
    return nullptr;
}

const t_pb_graph_pin* find_pb_graph_pin(const t_pb_graph_node* pb_gnode, const std::string& port_name, int index) {
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
static t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_logical_block_type_ptr type) {
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
static void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type) {
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
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        delete[] pin_id_to_pb_mapping[blk_id];
    }
    pin_id_to_pb_mapping.clear();
}

std::tuple<t_physical_tile_type_ptr, const t_sub_tile*, int, t_logical_block_type_ptr> get_cluster_blk_physical_spec(ClusterBlockId cluster_blk_id) {
    auto& grid = g_vpr_ctx.device().grid;
    auto& block_locs = g_vpr_ctx.placement().block_locs();
    auto& loc = block_locs[cluster_blk_id].loc;
    int cap = loc.sub_tile;
    const auto& physical_type = grid.get_physical_type({loc.x, loc.y, loc.layer});
    VTR_ASSERT(grid.get_width_offset({loc.x, loc.y, loc.layer}) == 0 && grid.get_height_offset(t_physical_tile_loc(loc.x, loc.y, loc.layer)) == 0);
    VTR_ASSERT(cap < physical_type->capacity);

    auto& cluster_net_list = g_vpr_ctx.clustering().clb_nlist;
    auto sub_tile = &physical_type->sub_tiles[get_sub_tile_index(cluster_blk_id)];
    int rel_cap = cap - sub_tile->capacity.low;
    VTR_ASSERT(rel_cap >= 0);
    auto logical_block = cluster_net_list.block_type(cluster_blk_id);

    return std::make_tuple(physical_type, sub_tile, rel_cap, logical_block);
}

std::vector<int> get_cluster_internal_class_pairs(const AtomLookup& atom_lookup,
                                                  ClusterBlockId cluster_block_id) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    std::vector<int> class_num_vec;

    auto [physical_tile, sub_tile, rel_cap, logical_block] = get_cluster_blk_physical_spec(cluster_block_id);
    class_num_vec.reserve(physical_tile->primitive_class_inf.size());

    const auto& cluster_atoms = cluster_ctx.atoms_lookup[cluster_block_id];
    for (AtomBlockId atom_blk_id : cluster_atoms) {
        auto atom_pb_graph_node = atom_lookup.atom_pb_graph_node(atom_blk_id);
        auto class_range = get_pb_graph_node_class_physical_range(physical_tile,
                                                                  sub_tile,
                                                                  logical_block,
                                                                  rel_cap,
                                                                  atom_pb_graph_node);
        for (int class_num = class_range.low; class_num <= class_range.high; class_num++) {
            class_num_vec.push_back(class_num);
        }
    }

    return class_num_vec;
}

std::vector<int> get_cluster_internal_pins(ClusterBlockId cluster_blk_id) {
    std::vector<int> internal_pins;

    auto& cluster_net_list = g_vpr_ctx.clustering().clb_nlist;

    auto [physical_tile, sub_tile, rel_cap, logical_block] = get_cluster_blk_physical_spec(cluster_blk_id);
    internal_pins.reserve(logical_block->pin_logical_num_to_pb_pin_mapping.size());

    std::list<const t_pb*> internal_pbs;
    const t_pb* pb = cluster_net_list.block_pb(cluster_blk_id);
    // Pins on the tile are already added. Thus, we should ** not ** at the top-level block's classes.
    add_pb_child_to_list(internal_pbs, pb);

    while (!internal_pbs.empty()) {
        pb = internal_pbs.front();
        internal_pbs.pop_front();

        auto pin_num_range = get_pb_pins(physical_tile,
                                         sub_tile,
                                         logical_block,
                                         pb,
                                         rel_cap);
        for (int pin_num = pin_num_range.low; pin_num <= pin_num_range.high; pin_num++) {
            internal_pins.push_back(pin_num);
        }

        add_pb_child_to_list(internal_pbs, pb);
    }
    internal_pins.shrink_to_fit();
    VTR_ASSERT(internal_pins.size() <= logical_block->pin_logical_num_to_pb_pin_mapping.size());
    return internal_pins;
}

t_pin_range get_pb_pins(t_physical_tile_type_ptr physical_type,
                        const t_sub_tile* sub_tile,
                        t_logical_block_type_ptr logical_block,
                        const t_pb* pb,
                        int rel_cap) {
    /* If pb is the root block, pins on the tile is returned */
    VTR_ASSERT(pb->pb_graph_node != nullptr);

    //TODO: This is not working if there is a custom mapping between tile pins and the root-level
    // pb-block.
    if (pb->pb_graph_node->is_root()) {
        int num_pins = sub_tile->num_phy_pins / sub_tile->capacity.total();
        int first_num_node = sub_tile->sub_tile_to_tile_pin_indices[0] + num_pins * rel_cap;

        return {first_num_node, first_num_node + num_pins - 1};
    } else {
        return get_pb_graph_node_pins(physical_type,
                                      sub_tile,
                                      logical_block,
                                      rel_cap,
                                      pb->pb_graph_node);
    }
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
            delete[] pb->pb_stats->feasible_blocks;
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
std::tuple<int, int, std::string, std::string> parse_direct_pin_name(std::string_view src_string, int line) {

    if (vtr::split(src_string).size() > 1) {
        VPR_THROW(VPR_ERROR_ARCH,
                  "Only a single port pin range specification allowed for direct connect (was: '%s')", src_string);
    }

    // parse out the pb_type and port name, possibly pin_indices
    if (src_string.find('[') == std::string_view::npos) {
        /* Format "pb_type_name.port_name" */
        const int start_pin_index = -1;
        const int end_pin_index = -1;

        std::string source_string{src_string};
        // replace '.' characters with space
        std::replace(source_string.begin(), source_string.end(), '.', ' ');

        std::istringstream source_iss(source_string);
        std::string pb_type_name, port_name;

        if (source_iss >> pb_type_name >> port_name) {
            return {start_pin_index, end_pin_index, pb_type_name, port_name};
        } else {
            VTR_LOG_ERROR(
                "[LINE %d] Invalid pin - %s, name should be in the format "
                "\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
                "The end_pin_index and start_pin_index can be the same.\n",
                line, src_string);
            exit(1);
        }
    } else {
        /* Format "pb_type_name.port_name[end_pin_index:start_pin_index]" */
        std::string source_string{src_string};

        // Replace '.' and '[' characters with ' '
        std::replace_if(source_string.begin(), source_string.end(),
            [](char c) { return c == '.' || c == '[' || c == ':' || c == ']'; },
            ' ');

        std::istringstream source_iss(source_string);
        int start_pin_index, end_pin_index;
        std::string pb_type_name, port_name;

        if (source_iss >> pb_type_name >> port_name >> end_pin_index >> start_pin_index) {

            if (end_pin_index < 0 || start_pin_index < 0) {
                VTR_LOG_ERROR(
                    "[LINE %d] Invalid pin - %s, the pin_index in "
                    "[end_pin_index:start_pin_index] should not be a negative value.\n",
                    line, src_string);
                exit(1);
            }

            if (end_pin_index < start_pin_index) {
                VTR_LOG_ERROR(
                    "[LINE %d] Invalid from_pin - %s, the end_pin_index in "
                    "[end_pin_index:start_pin_index] should not be less than start_pin_index.\n",
                    line, src_string);
                exit(1);
            }

            return {start_pin_index, end_pin_index, pb_type_name, port_name};
        } else {
            VTR_LOG_ERROR(
                "[LINE %d] Invalid pin - %s, name should be in the format "
                "\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
                "The end_pin_index and start_pin_index can be the same.\n",
                line, src_string);
            exit(1);
        }
    }
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

    for (int iswitch = 0; iswitch < (int)device_ctx.arch_switch_inf.size(); iswitch++) {
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
    const auto& rr_graph = device_ctx.rr_graph;
    if (device_ctx.switch_fanin_remap.empty()) {
        VTR_LOG_WARN("Cannot print switch usage stats: device_ctx.switch_fanin_remap is empty\n");
        return;
    }
    std::map<int, int>* switch_fanin_count;
    std::map<int, float>* switch_fanin_delay;
    switch_fanin_count = new std::map<int, int>[device_ctx.all_sw_inf.size()];
    switch_fanin_delay = new std::map<int, float>[device_ctx.all_sw_inf.size()];
    // a node can have multiple inward switches, so
    // map key: switch index; map value: count (fanin)
    std::map<int, int>* inward_switch_inf = new std::map<int, int>[rr_graph.num_nodes()];
    for (const RRNodeId& inode : rr_graph.nodes()) {
        int num_edges = rr_graph.num_edges(inode);
        for (int iedge = 0; iedge < num_edges; iedge++) {
            int switch_index = rr_graph.edge_switch(inode, iedge);
            int to_node_index = size_t(rr_graph.edge_sink_node(inode, iedge));
            // Assumption: suppose for a L4 wire (bi-directional): ----+----+----+----, it can be driven from any point (0, 1, 2, 3).
            //             physically, the switch driving from point 1 & 3 should be the same. But we will assign then different switch
            //             index; or there is no way to differentiate them after abstracting a 2D wire into a 1D node
            if (inward_switch_inf[to_node_index].count(switch_index) == 0)
                inward_switch_inf[to_node_index][switch_index] = 0;
            //VTR_ASSERT(from_node.type != OPIN);
            inward_switch_inf[to_node_index][switch_index]++;
        }
    }
    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        std::map<int, int>::iterator itr;
        for (itr = inward_switch_inf[(size_t)rr_id].begin(); itr != inward_switch_inf[(size_t)rr_id].end(); itr++) {
            int switch_index = itr->first;
            int fanin = itr->second;
            float Tdel = rr_graph.rr_switch_inf(RRSwitchId(switch_index)).Tdel;
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
    for (int iswitch = 0; iswitch < (int)device_ctx.all_sw_inf.size(); iswitch++) {
        std::string s_name = device_ctx.all_sw_inf.at(iswitch).name;
        float s_area = device_ctx.all_sw_inf.at(iswitch).mux_trans_size;
        VTR_LOG(">>>>> switch index: %d, name: %s, mux trans size: %g\n", iswitch, s_name.c_str(), s_area);

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

int get_atom_pin_class_num(const AtomPinId atom_pin_id) {
    auto& atom_look_up = g_vpr_ctx.atom().lookup;
    auto& atom_net_list = g_vpr_ctx.atom().nlist;

    auto atom_blk_id = atom_net_list.pin_block(atom_pin_id);
    auto cluster_block_id = atom_look_up.atom_clb(atom_blk_id);

    auto [physical_type, sub_tile, sub_tile_rel_cap, logical_block] = get_cluster_blk_physical_spec(cluster_block_id);
    auto pb_graph_pin = atom_look_up.atom_pin_pb_graph_pin(atom_pin_id);
    int pin_physical_num = -1;
    pin_physical_num = get_pb_pin_physical_num(physical_type, sub_tile, logical_block, sub_tile_rel_cap, pb_graph_pin);

    return get_class_num_from_pin_physical_num(physical_type, pin_physical_num);
}

t_physical_tile_port find_tile_port_by_name(t_physical_tile_type_ptr type, std::string_view port_name) {
    for (const t_sub_tile& sub_tile : type->sub_tiles) {
        for (const t_physical_tile_port& port : sub_tile.ports) {
            if (port_name == port.name) {
                return port;
            }
        }
    }

    // Port has not been found, throw an error.
    VPR_THROW(VPR_ERROR_ARCH, "Unable to find port %s (on block %s).\n", port_name, type->name.c_str());
}

void pretty_print_uint(const char* prefix, size_t value, int num_digits, int scientific_precision) {
    //Print as integer if it will fit in the width, otherwise scientific
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

void print_timing_stats(const std::string& name,
                        const t_timing_analysis_profile_info& current,
                        const t_timing_analysis_profile_info& past) {
    VTR_LOG("%s timing analysis took %g seconds (%g STA, %g slack) (%zu full updates: %zu setup, %zu hold, %zu combined).\n",
            name.c_str(),
            current.timing_analysis_wallclock_time() - past.timing_analysis_wallclock_time(),
            current.sta_wallclock_time - past.sta_wallclock_time,
            current.slack_wallclock_time - past.slack_wallclock_time,
            current.num_full_updates() - past.num_full_updates(),
            current.num_full_setup_updates - past.num_full_setup_updates,
            current.num_full_hold_updates - past.num_full_hold_updates,
            current.num_full_setup_hold_updates - past.num_full_setup_hold_updates);
}

std::vector<const t_pb_graph_node*> get_all_pb_graph_node_primitives(const t_pb_graph_node* pb_graph_node) {
    std::vector<const t_pb_graph_node*> primitives;
    if (pb_graph_node->is_primitive()) {
        primitives.push_back(pb_graph_node);
        return primitives;
    }

    auto pb_type = pb_graph_node->pb_type;
    for (int mode_idx = 0; mode_idx < pb_graph_node->pb_type->num_modes; mode_idx++) {
        for (int pb_type_idx = 0; pb_type_idx < (pb_type->modes[mode_idx]).num_pb_type_children; pb_type_idx++) {
            int num_pb = pb_type->modes[mode_idx].pb_type_children[pb_type_idx].num_pb;
            for (int pb_idx = 0; pb_idx < num_pb; pb_idx++) {
                const t_pb_graph_node* child_pb_graph_node = &(pb_graph_node->child_pb_graph_nodes[mode_idx][pb_type_idx][pb_idx]);
                auto tmp_primitives = get_all_pb_graph_node_primitives(child_pb_graph_node);
                primitives.insert(std::end(primitives), std::begin(tmp_primitives), std::end(tmp_primitives));
            }
        }
    }
    return primitives;
}

bool is_inter_cluster_node(const RRGraphView& rr_graph_view,
                           RRNodeId node_id) {
    auto node_type = rr_graph_view.node_type(node_id);
    if (node_type == CHANX || node_type == CHANY) {
        return true;
    } else {
        int x_low = rr_graph_view.node_xlow(node_id);
        int y_low = rr_graph_view.node_ylow(node_id);
        int layer = rr_graph_view.node_layer(node_id);
        int node_ptc = rr_graph_view.node_ptc_num(node_id);
        const t_physical_tile_type_ptr physical_tile = g_vpr_ctx.device().grid.get_physical_type({x_low, y_low, layer});
        if (node_type == IPIN || node_type == OPIN) {
            return is_pin_on_tile(physical_tile, node_ptc);
        } else {
            VTR_ASSERT_DEBUG(node_type == SINK || node_type == SOURCE);
            return is_class_on_tile(physical_tile, node_ptc);
        }
    }
}

int get_rr_node_max_ptc(const RRGraphView& rr_graph_view,
                        RRNodeId node_id,
                        bool is_flat) {
    auto node_type = rr_graph_view.node_type(node_id);

    VTR_ASSERT(node_type == IPIN || node_type == OPIN || node_type == SINK || node_type == SOURCE);

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    auto physical_type = device_ctx.grid.get_physical_type({rr_graph_view.node_xlow(node_id),
                                                            rr_graph_view.node_ylow(node_id),
                                                            rr_graph_view.node_layer(node_id)});

    if (node_type == SINK || node_type == SOURCE) {
        return get_tile_class_max_ptc(physical_type, is_flat);
    } else {
        return get_tile_pin_max_ptc(physical_type, is_flat);
    }
}

RRNodeId get_pin_rr_node_id(const RRSpatialLookup& rr_spatial_lookup,
                            t_physical_tile_type_ptr physical_tile,
                            const int layer,
                            const int root_i,
                            const int root_j,
                            int pin_physical_num) {
    auto pin_type = get_pin_type_from_pin_physical_num(physical_tile, pin_physical_num);
    t_rr_type node_type = (pin_type == e_pin_type::DRIVER) ? t_rr_type::OPIN : t_rr_type::IPIN;
    std::vector<int> x_offset;
    std::vector<int> y_offset;
    std::vector<e_side> pin_sides;
    std::tie(x_offset, y_offset, pin_sides) = get_pin_coordinates(physical_tile, pin_physical_num, std::vector<e_side>(TOTAL_2D_SIDES.begin(), TOTAL_2D_SIDES.end()));
    VTR_ASSERT(!x_offset.empty());
    RRNodeId node_id = RRNodeId::INVALID();
    for (int coord_idx = 0; coord_idx < (int)pin_sides.size(); coord_idx++) {
        node_id = rr_spatial_lookup.find_node(layer,
                                              root_i + x_offset[coord_idx],
                                              root_j + y_offset[coord_idx],
                                              node_type,
                                              pin_physical_num,
                                              pin_sides[coord_idx]);
        if (node_id != RRNodeId::INVALID())
            break;
    }
    return node_id;
}

RRNodeId get_class_rr_node_id(const RRSpatialLookup& rr_spatial_lookup,
                              t_physical_tile_type_ptr physical_tile,
                              const int layer,
                              const int i,
                              const int j,
                              int class_physical_num) {
    auto class_type = get_class_type_from_class_physical_num(physical_tile, class_physical_num);
    VTR_ASSERT(class_type == DRIVER || class_type == RECEIVER);
    t_rr_type node_type = (class_type == e_pin_type::DRIVER) ? t_rr_type::SOURCE : t_rr_type::SINK;
    return rr_spatial_lookup.find_node(layer, i, j, node_type, class_physical_num);
}

bool node_in_same_physical_tile(RRNodeId node_first, RRNodeId node_second) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto first_rr_type = rr_graph.node_type(node_first);
    auto second_rr_type = rr_graph.node_type(node_second);

    // If one of the given node's type is CHANX/Y nodes are definitely not in the same physical tile
    if (first_rr_type == t_rr_type::CHANX || first_rr_type == t_rr_type::CHANY || second_rr_type == t_rr_type::CHANX || second_rr_type == t_rr_type::CHANY) {
        return false;
    } else {
        VTR_ASSERT(first_rr_type == t_rr_type::IPIN || first_rr_type == t_rr_type::OPIN || first_rr_type == t_rr_type::SINK || first_rr_type == t_rr_type::SOURCE);
        VTR_ASSERT(second_rr_type == t_rr_type::IPIN || second_rr_type == t_rr_type::OPIN || second_rr_type == t_rr_type::SINK || second_rr_type == t_rr_type::SOURCE);
        int first_layer = rr_graph.node_layer(node_first);
        int first_x = rr_graph.node_xlow(node_first);
        int first_y = rr_graph.node_ylow(node_first);
        int sec_layer = rr_graph.node_layer(node_second);
        int sec_x = rr_graph.node_xlow(node_second);
        int sec_y = rr_graph.node_ylow(node_second);

        // Get the root-location of the pin's block
        int first_root_x = first_x - device_ctx.grid.get_width_offset({first_x, first_y, first_layer});
        int first_root_y = first_y - device_ctx.grid.get_height_offset({first_x, first_y, first_layer});

        int sec_root_x = sec_x - device_ctx.grid.get_width_offset({sec_x, sec_y, sec_layer});
        int sec_root_y = sec_y - device_ctx.grid.get_height_offset({sec_x, sec_y, sec_layer});

        // If the root-location of the nodes are similar, they should be located in the same tile
        if (first_root_x == sec_root_x && first_root_y == sec_root_y)
            return true;
        else
            return false;
    }
}

std::vector<int> get_cluster_netlist_intra_tile_classes_at_loc(int layer,
                                                               int i,
                                                               int j,
                                                               t_physical_tile_type_ptr physical_type) {
    std::vector<int> class_num_vec;

    const auto& place_ctx = g_vpr_ctx.placement();
    const auto& atom_lookup = g_vpr_ctx.atom().lookup;
    const auto& grid_block = place_ctx.grid_blocks();

    class_num_vec.reserve(physical_type->primitive_class_inf.size());

    //iterate over different sub tiles inside a tile
    for (int abs_cap = 0; abs_cap < physical_type->capacity; abs_cap++) {
        if (grid_block.is_sub_tile_empty({i, j, layer}, abs_cap)) {
            continue;
        }
        auto cluster_blk_id = grid_block.block_at_location({i, j, abs_cap, layer});
        VTR_ASSERT(cluster_blk_id != ClusterBlockId::INVALID());

        auto primitive_classes = get_cluster_internal_class_pairs(atom_lookup,
                                                                  cluster_blk_id);
        /* Initialize SINK/SOURCE nodes and connect them to their respective pins */
        for (auto class_num : primitive_classes) {
            class_num_vec.push_back(class_num);
        }
    }

    class_num_vec.shrink_to_fit();
    return class_num_vec;
}

std::vector<int> get_cluster_netlist_intra_tile_pins_at_loc(const int layer,
                                                            const int i,
                                                            const int j,
                                                            const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                            const vtr::vector<ClusterBlockId, std::unordered_set<int>>& pin_chains_num,
                                                            t_physical_tile_type_ptr physical_type) {
    const auto& place_ctx = g_vpr_ctx.placement();
    const auto& grid_block = place_ctx.grid_blocks();

    std::vector<int> pin_num_vec;
    pin_num_vec.reserve(get_tile_num_internal_pin(physical_type));

    for (int abs_cap = 0; abs_cap < physical_type->capacity; abs_cap++) {
        std::vector<int> cluster_internal_pins;

        if (grid_block.is_sub_tile_empty({i, j, layer}, abs_cap)) {
            continue;
        }
        auto cluster_blk_id = grid_block.block_at_location({i, j, abs_cap, layer});
        VTR_ASSERT(cluster_blk_id != ClusterBlockId::INVALID());

        cluster_internal_pins = get_cluster_internal_pins(cluster_blk_id);
        const auto& cluster_pin_chains = pin_chains_num[cluster_blk_id];
        const auto& cluster_chain_sinks = pin_chains[cluster_blk_id].chain_sink;
        const auto& cluster_pin_chain_idx = pin_chains[cluster_blk_id].pin_chain_idx;
        // remove common elements between cluster_pin_chains.
        for (auto pin : cluster_internal_pins) {
            auto it = cluster_pin_chains.find(pin);
            if (it == cluster_pin_chains.end()) {
                pin_num_vec.push_back(pin);
            } else {
                VTR_ASSERT(cluster_pin_chain_idx[pin] != OPEN);
                if (is_pin_on_tile(physical_type, pin) || is_primitive_pin(physical_type, pin) || cluster_chain_sinks[cluster_pin_chain_idx[pin]] == pin) {
                    pin_num_vec.push_back(pin);
                }
            }
        }
    }

    pin_num_vec.shrink_to_fit();
    return pin_num_vec;
}

std::vector<int> get_cluster_block_pins(t_physical_tile_type_ptr physical_tile,
                                        ClusterBlockId cluster_blk_id,
                                        int abs_cap) {
    int max_num_pin = get_tile_total_num_pin(physical_tile) / physical_tile->capacity;
    int num_tile_pin_per_inst = physical_tile->num_pins / physical_tile->capacity;
    std::vector<int> pin_num_vec(num_tile_pin_per_inst);
    std::iota(pin_num_vec.begin(), pin_num_vec.end(), abs_cap * num_tile_pin_per_inst);

    pin_num_vec.reserve(max_num_pin);

    auto internal_pins = get_cluster_internal_pins(cluster_blk_id);
    pin_num_vec.insert(pin_num_vec.end(), internal_pins.begin(), internal_pins.end());

    pin_num_vec.shrink_to_fit();
    return pin_num_vec;
}

t_arch_switch_inf create_internal_arch_sw(float delay) {
    t_arch_switch_inf arch_switch_inf;
    arch_switch_inf.set_type(SwitchType::MUX);
    std::ostringstream stream_obj;
    stream_obj << delay << std::scientific;
    arch_switch_inf.name = (std::string(VPR_INTERNAL_SWITCH_NAME) + "/" + stream_obj.str());
    arch_switch_inf.R = 0.;
    arch_switch_inf.Cin = 0.;
    arch_switch_inf.Cout = 0;
    arch_switch_inf.set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, delay);
    arch_switch_inf.power_buffer_type = POWER_BUFFER_TYPE_NONE;
    arch_switch_inf.mux_trans_size = 0.;
    arch_switch_inf.buf_size_type = BufferSize::ABSOLUTE;
    arch_switch_inf.buf_size = 0.;
    arch_switch_inf.intra_tile = true;

    return arch_switch_inf;
}

void add_pb_child_to_list(std::list<const t_pb*>& pb_list, const t_pb* parent_pb) {
    for (int child_pb_type_idx = 0; child_pb_type_idx < parent_pb->get_num_child_types(); child_pb_type_idx++) {
        int num_children = parent_pb->get_num_children_of_type(child_pb_type_idx);
        for (int child_idx = 0; child_idx < num_children; child_idx++) {
            const t_pb* child_pb = &parent_pb->child_pbs[child_pb_type_idx][child_idx];
            // We are adding a child, thus it is for sure not a root pb.
            // If parent_pb for a non-root pb is null, it means this pb doesn't contain
            // any atom block
            if (child_pb->parent_pb != nullptr) {
                pb_list.push_back(child_pb);
            }
        }
    }
}

void apply_route_constraints(const UserRouteConstraints& route_constraints) {
    ClusteringContext& mutable_cluster_ctx = g_vpr_ctx.mutable_clustering();

    // Iterate through all the nets 
    for (auto net_id : mutable_cluster_ctx.clb_nlist.nets()) {
        // Get the name of the current net
        std::string net_name = mutable_cluster_ctx.clb_nlist.net_name(net_id);

        // Check if a routing constraint is specified for the current net
        if (route_constraints.has_routing_constraint(net_name)) {
            // Mark the net as 'global' if there is a routing constraint for this net
            // as the routing constraints are used to set the net as global
            // and specify the routing model for it
            mutable_cluster_ctx.clb_nlist.set_net_is_global(net_id, true);

            // Mark the net as 'ignored' if the route model is 'ideal'
            if (route_constraints.get_route_model_by_net_name(net_name) == e_clock_modeling::IDEAL_CLOCK) {
                mutable_cluster_ctx.clb_nlist.set_net_is_ignored(net_id, true);
            } else {
                // Set the 'ignored' flag to false otherwise
                mutable_cluster_ctx.clb_nlist.set_net_is_ignored(net_id, false);
            }
        }
    }
}

float get_min_cross_layer_delay() {
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    float min_delay = std::numeric_limits<float>::max();

    for (const auto& driver_node : rr_graph.nodes()) {
        for (size_t edge_id = 0; edge_id < rr_graph.num_edges(driver_node); edge_id++) {
            const auto& sink_node = rr_graph.edge_sink_node(driver_node, edge_id);
            if (rr_graph.node_layer(driver_node) != rr_graph.node_layer(sink_node)) {
                int i_switch = rr_graph.edge_switch(driver_node, edge_id);
                float edge_delay = rr_graph.rr_switch_inf(RRSwitchId(i_switch)).Tdel;
                min_delay = std::min(min_delay, edge_delay);
            }
        }
    }

    return min_delay;
}

PortPinToBlockPinConverter::PortPinToBlockPinConverter() {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& types = device_ctx.physical_tile_types;

    // Resize and initialize the values to OPEN (-1).
    size_t num_types = types.size();
    blk_pin_from_port_pin_.resize(num_types);

    for (size_t itype = 1; itype < num_types; itype++) {
        int blk_pin_count = 0;
        auto& type = types[itype];
        size_t num_sub_tiles = type.sub_tiles.size();
        blk_pin_from_port_pin_[itype].resize(num_sub_tiles);
        for (size_t isub_tile = 0; isub_tile < num_sub_tiles; isub_tile++) {
            size_t num_ports = type.sub_tiles[isub_tile].ports.size();
            blk_pin_from_port_pin_[itype][isub_tile].resize(num_ports);
            for (size_t iport = 0; iport < num_ports; iport++) {
                int num_pins = type.sub_tiles[isub_tile].ports[iport].num_pins;
                for (int ipin = 0; ipin < num_pins; ipin++) {
                    blk_pin_from_port_pin_[itype][isub_tile][iport].push_back(blk_pin_count);
                    blk_pin_count++;
                }
            }
        }
    }
}

int PortPinToBlockPinConverter::get_blk_pin_from_port_pin(int blk_type_index, int sub_tile, int port, int port_pin) const {
    // Return the port and port_pin for the pin.
    int blk_pin = blk_pin_from_port_pin_[blk_type_index][sub_tile][port][port_pin];
    return blk_pin;
}
