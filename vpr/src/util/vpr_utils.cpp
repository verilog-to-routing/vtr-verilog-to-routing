#include <cstring>
#include <unordered_set>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "physical_types.h"
#include "path_delay.h"
#include "globals.h"
#include "vpr_utils.h"
#include "cluster_placement.h"
#include "place_macro.h"
#include "string.h"
#include "pack_types.h"
#include "device_grid.h"
#include <algorithm>

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

 /* f_port_from_blk_pin array allow us to quickly find what port a block *
 * pin corresponds to.                                                   *
 * [0...device_ctx.num_block_types-1][0...blk_pin_count-1]                                *
 *                                                                       */
static int ** f_port_from_blk_pin = nullptr;

/* f_port_pin_from_blk_pin array allow us to quickly find what port pin a*
 * block pin corresponds to.                                             *
 * [0...device_ctx.num_block_types-1][0...blk_pin_count-1]                                */
static int ** f_port_pin_from_blk_pin = nullptr;

/* f_port_pin_to_block_pin array allows us to quickly find what block    *
 * pin a port pin corresponds to.                                        *
 * [0...device_ctx.num_block_types-1][0...num_ports-1][0...num_port_pins-1]               */
static int *** f_blk_pin_from_port_pin = nullptr;


/******************** Subroutine declarations ********************************/

/* Allocates and loads f_port_from_blk_pin and f_port_pin_from_blk_pin   *
 * arrays.                                                               *
 * The arrays are freed in free_placement_structs()                      */
static void alloc_and_load_port_pin_from_blk_pin();

/* Allocates and loads blk_pin_from_port_pin array.                      *
 * The arrays are freed in free_placement_structs()                      */
static void alloc_and_load_blk_pin_from_port_pin();

/* Go through all the ports in all the blocks to find the port that has the same   *
 * name as port_name and belongs to the block type that has the name pb_type_name. *
 * Then, check that whether start_pin_index and end_pin_index are specified. If    *
 * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
 * Otherwise, mark down all the pins in that port.                                 */
static void mark_direct_of_ports (int idirect, int direct_type, char * pb_type_name,
		char * port_name, int end_pin_index, int start_pin_index, char * src_string,
		int line, int ** idirect_from_blk_pin, int ** direct_type_from_blk_pin);

/* Mark the pin entry in idirect_from_blk_pin with idirect and the pin entry in    *
 * direct_type_from_blk_pin with direct_type from start_pin_index to               *
 * end_pin_index.                                                                  */
static void mark_direct_of_pins(int start_pin_index, int end_pin_index, int itype,
		int iport, int ** idirect_from_blk_pin, int idirect,
		int ** direct_type_from_blk_pin, int direct_type, int line, char * src_string);

static void load_pb_graph_pin_lookup_from_index_rec(t_pb_graph_pin ** pb_graph_pin_lookup_from_index, t_pb_graph_node *pb_graph_node);

static void load_pin_id_to_pb_mapping_rec(t_pb *cur_pb, t_pb **pin_id_to_pb_mapping);

static std::vector<int> find_connected_internal_clb_sink_pins(ClusterBlockId clb, int pb_pin);
static void find_connected_internal_clb_sink_pins_recurr(ClusterBlockId clb, int pb_pin, std::vector<int>& connected_sink_pb_pins);
static AtomPinId find_atom_pin_for_pb_route_id(ClusterBlockId clb, int pb_route_id, const IntraLbPbPinLookup& pb_gpin_lookup);

/******************** Subroutine definitions *********************************/

const t_model* find_model(const t_model* models, const std::string& name, bool required) {

    for(const t_model* model = models; model != nullptr; model = model->next) {
        if(name == model->name) {
            return model;
        }
    }

    if(required) {
        VPR_THROW(VPR_ERROR_ARCH, "Failed to find architecture modedl '%s'\n", name.c_str());
    }

    return nullptr;
}

const t_model_ports* find_model_port(const t_model* model, const std::string& name, bool required) {
    VTR_ASSERT(model);

    for(const t_model_ports* model_ports : {model->inputs, model->outputs}) {
        for(const t_model_ports* port = model_ports; port != nullptr; port = port->next) {
            if (port->name == name) {
                return port;
            }
        }
    }

    if(required) {
        VPR_THROW(VPR_ERROR_ARCH, "Failed to find port '%s; on architecture modedl '%s'\n", name.c_str(), model->name);
    }


    return nullptr;
}


/**
 * print tabs given number of tabs to file
 */
void print_tabs(FILE * fpout, int num_tab) {

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
        int blk_x = place_ctx.block_locs[blk_id].x;
        int blk_y = place_ctx.block_locs[blk_id].y;
        int blk_z = place_ctx.block_locs[blk_id].z;

		/* Check range of block coords */
		if (blk_x < 0 || blk_y < 0
			|| (blk_x + cluster_ctx.clb_nlist.block_type(blk_id)->width - 1) > int(device_ctx.grid.width() - 1)
				|| (blk_y + cluster_ctx.clb_nlist.block_type(blk_id)->height - 1) > int(device_ctx.grid.height() - 1)
				|| blk_z < 0 || blk_z > (cluster_ctx.clb_nlist.block_type(blk_id)->capacity)) {
			VPR_THROW(VPR_ERROR_PLACE, "Block %zu is at invalid location (%d, %d, %d).\n",
					size_t(blk_id), blk_x, blk_y, blk_z);
		}

		/* Check types match */
		if (cluster_ctx.clb_nlist.block_type(blk_id) != device_ctx.grid[blk_x][blk_y].type) {
            VPR_THROW(VPR_ERROR_PLACE, "A block is in a grid location (%d x %d) with a conflicting types '%s' and '%s' .\n",
					blk_x, blk_y,
					cluster_ctx.clb_nlist.block_type(blk_id)->name,
					device_ctx.grid[blk_x][blk_y].type->name);
		}

		/* Check already in use */
		if ((EMPTY_BLOCK_ID != place_ctx.grid_blocks[blk_x][blk_y].blocks[blk_z])
				&& (INVALID_BLOCK_ID != place_ctx.grid_blocks[blk_x][blk_y].blocks[blk_z])) {
            VPR_THROW(VPR_ERROR_PLACE, "Location (%d, %d, %d) is used more than once.\n",
					blk_x, blk_y, blk_z);
		}

		if (device_ctx.grid[blk_x][blk_y].width_offset != 0 || device_ctx.grid[blk_x][blk_y].height_offset != 0) {
            VPR_THROW(VPR_ERROR_PLACE, "Large block not aligned in placment for cluster_ctx.blocks %lu at (%d, %d, %d).",
				size_t(blk_id), blk_x, blk_y, blk_z);
		}

		/* Set the block */
		for (int width = 0; width < cluster_ctx.clb_nlist.block_type(blk_id)->width; ++width) {
			for (int height = 0; height < cluster_ctx.clb_nlist.block_type(blk_id)->height; ++height) {
				place_ctx.grid_blocks[blk_x + width][blk_y + height].blocks[blk_z] = blk_id;
				place_ctx.grid_blocks[blk_x + width][blk_y + height].usage++;
				VTR_ASSERT(device_ctx.grid[blk_x + width][blk_y + height].width_offset == width);
				VTR_ASSERT(device_ctx.grid[blk_x + width][blk_y + height].height_offset == height);
			}
		}
	}
}

std::string block_type_pin_index_to_name(t_type_ptr type, int pin_index) {
    std::string pin_name = type->name;

    if (type->capacity > 1) {
        int pins_per_inst = type->num_pins / type->capacity;
        int inst_num = pin_index / pins_per_inst;
        pin_index %= pins_per_inst;

        pin_name += "[" + std::to_string(inst_num) + "]";
    }

    pin_name += ".";

    t_pb_type* pb_type = type->pb_type;
    int curr_index = 0;
    for (int iport = 0; iport < pb_type->num_ports; ++iport) {
        t_port* port = &pb_type->ports[iport];

        if (curr_index + port->num_pins > pin_index) {
            //This port contains the desired pin index
            int index_in_port = pin_index - curr_index;
            pin_name += port->name;
            pin_name += "[" + std::to_string(index_in_port) + "]";
            return pin_name;
        }

        curr_index += port->num_pins;
    }

    return "<UNKOWN>";
}

IntraLbPbPinLookup::IntraLbPbPinLookup(t_type_descriptor* block_types, int ntypes)
    : block_types_(block_types)
    , num_types_(ntypes) {
	intra_lb_pb_pin_lookup_ = new t_pb_graph_pin**[num_types_];
    for(int itype = 0; itype < num_types_; ++itype) {
		intra_lb_pb_pin_lookup_[itype] = alloc_and_load_pb_graph_pin_lookup_from_index(&block_types[itype]);
    }
}

IntraLbPbPinLookup::IntraLbPbPinLookup(const IntraLbPbPinLookup& rhs)
    : IntraLbPbPinLookup(rhs.block_types_, rhs.num_types_) {
    //nop
}

IntraLbPbPinLookup& IntraLbPbPinLookup::operator=(IntraLbPbPinLookup rhs) {
    //Copy-swap idiom
    swap(*this, rhs);

    return *this;
}

IntraLbPbPinLookup::~IntraLbPbPinLookup() {
    auto& device_ctx = g_vpr_ctx.device();
	for (int itype = 0; itype < device_ctx.num_block_types; itype++) {
		free_pb_graph_pin_lookup_from_index(intra_lb_pb_pin_lookup_[itype]);
	}

	delete[] intra_lb_pb_pin_lookup_;
}

const t_pb_graph_pin* IntraLbPbPinLookup::pb_gpin(int itype, int ipin) const {
    VTR_ASSERT(itype < num_types_);

    return intra_lb_pb_pin_lookup_[itype][ipin];
}

void swap(IntraLbPbPinLookup& lhs, IntraLbPbPinLookup& rhs) {
    std::swap(lhs.num_types_, rhs.num_types_);
    std::swap(lhs.block_types_, rhs.block_types_);
    std::swap(lhs.intra_lb_pb_pin_lookup_, rhs.intra_lb_pb_pin_lookup_);
}

//Returns the set of pins which are connected to the top level clb pin
//  The pin(s) may be input(s) or and output (returning the connected sinks or drivers respectively)
std::vector<AtomPinId> find_clb_pin_connected_atom_pins(ClusterBlockId clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    std::vector<AtomPinId> atom_pins;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    if(is_opin(clb_pin, cluster_ctx.clb_nlist.block_type(clb))) {
        //output
        AtomPinId driver = find_clb_pin_driver_atom_pin(clb, clb_pin, pb_gpin_lookup);
        if(driver) {
            atom_pins.push_back(driver);
        }
    } else {
        //input
        atom_pins = find_clb_pin_sink_atom_pins(clb, clb_pin, pb_gpin_lookup);
    }

    return atom_pins;
}

//Returns the atom pin which drives the top level clb output pin
AtomPinId find_clb_pin_driver_atom_pin(ClusterBlockId clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    int pb_pin_id = find_clb_pb_pin(clb, clb_pin);
    if(pb_pin_id < 0) {
        //CLB output pin has no internal driver
        return AtomPinId::INVALID();
    }
    t_pb_route* pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;
    AtomNetId atom_net = pb_routes[pb_pin_id].atom_net_id;

    //Trace back until the driver is reached
    while(pb_routes[pb_pin_id].driver_pb_pin_id >= 0) {
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
std::vector<AtomPinId> find_clb_pin_sink_atom_pins(ClusterBlockId clb, int clb_pin, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    t_pb_route* pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;
    VTR_ASSERT(pb_routes);

    VTR_ASSERT_MSG(clb_pin < cluster_ctx.clb_nlist.block_type(clb)->num_pins, "Must be a valid top-level pin");

    int pb_pin = find_clb_pb_pin(clb, clb_pin);

    VTR_ASSERT(cluster_ctx.clb_nlist.block_pb(clb));
    VTR_ASSERT_MSG(pb_pin < cluster_ctx.clb_nlist.block_pb(clb)->pb_graph_node->num_pins(), "Pin must map to a top-level pb pin");

    VTR_ASSERT_MSG(pb_routes[pb_pin].driver_pb_pin_id < 0, "CLB input pin should have no internal drivers");

    AtomNetId atom_net = pb_routes[pb_pin].atom_net_id;
    VTR_ASSERT(atom_net);

    std::vector<int> connected_sink_pb_pins = find_connected_internal_clb_sink_pins(clb, pb_pin);

    std::vector<AtomPinId> sink_atom_pins;
    for(int sink_pb_pin : connected_sink_pb_pins) {
        //Map the pb_pin_id to AtomPinId
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

    if(cluster_ctx.clb_nlist.block_pb(clb)->pb_route[pb_pin].sink_pb_pin_ids.empty()) {
        //No more sinks => primitive input
        connected_sink_pb_pins.push_back(pb_pin);
    }
    for(int sink_pb_pin : cluster_ctx.clb_nlist.block_pb(clb)->pb_route[pb_pin].sink_pb_pin_ids) {
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
    if(child_pb->child_pbs == nullptr) {
        //It is a leaf, and hence should map to an atom

        //Find the associated atom
        AtomBlockId atom_block = atom_ctx.lookup.pb_atom(child_pb);
        VTR_ASSERT(atom_block);

        //Now find the matching pin by seeing which pin maps to the gpin
        for(AtomPinId atom_pin : atom_ctx.nlist.block_pins(atom_block)) {
            const t_pb_graph_pin* atom_pin_gpin = atom_ctx.lookup.atom_pin_pb_graph_pin(atom_pin);
            if(atom_pin_gpin == gpin) {
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

    const t_pb_route* pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;

    VTR_ASSERT_MSG(pb_routes[sink_pb_pin_id].atom_net_id, "PB route should be associated with a net");

    //Walk back from the sink to the CLB input pin
    int curr_pb_pin_id = sink_pb_pin_id;
    int next_pb_pin_id = pb_routes[curr_pb_pin_id].driver_pb_pin_id;
    while(next_pb_pin_id >= 0) {
        //Advance back towards the input
        curr_pb_pin_id = next_pb_pin_id;

        VTR_ASSERT_MSG(pb_routes[next_pb_pin_id].atom_net_id == pb_routes[sink_pb_pin_id].atom_net_id,
                       "Connected pb_routes should connect the same net");

        next_pb_pin_id = pb_routes[curr_pb_pin_id].driver_pb_pin_id;
    }

    bool is_output_pin = (pb_routes[curr_pb_pin_id].pb_graph_pin->port->type == OUT_PORT);

    if(!is_clb_external_pin(clb, curr_pb_pin_id) || is_output_pin) {
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

	return std::tuple<ClusterNetId,int,int>(clb_net_idx, clb_pin, clb_net_pin_idx);
}

//Return the pb pin index corresponding to the pin clb_pin on block clb
// Given a clb_pin index on a this function will return the corresponding
// pin index on the pb_type (accounting for the possible z-coordinate offset).
int find_clb_pb_pin(ClusterBlockId clb, int clb_pin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    VTR_ASSERT_MSG(clb_pin < cluster_ctx.clb_nlist.block_type(clb)->num_pins, "Must be a valid top-level pin");

    int pb_pin = -1;
    if(place_ctx.block_locs[clb].nets_and_pins_synced_to_z_coordinate) {
        //Pins have been offset by z-coordinate, need to remove offset

        t_type_ptr type = cluster_ctx.clb_nlist.block_type(clb);
        VTR_ASSERT(type->num_pins % type->capacity == 0);
        int num_basic_block_pins = type->num_pins / type->capacity;
        /* Logical location and physical location is offset by z * max_num_block_pins */

        pb_pin = clb_pin - place_ctx.block_locs[clb].z * num_basic_block_pins;
    } else {
        //No offset
        pb_pin = clb_pin;
    }

    VTR_ASSERT(pb_pin >= 0);

    return pb_pin;
}

//Inverse of find_clb_pb_pin()
int find_pb_pin_clb_pin(ClusterBlockId clb, int pb_pin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    int clb_pin = -1;
    if(place_ctx.block_locs[clb].nets_and_pins_synced_to_z_coordinate) {
        //Pins have been offset by z-coordinate, need to remove offset
        t_type_ptr type = cluster_ctx.clb_nlist.block_type(clb);
        VTR_ASSERT(type->num_pins % type->capacity == 0);
        int num_basic_block_pins = type->num_pins / type->capacity;
        /* Logical location and physical location is offset by z * max_num_block_pins */

        clb_pin = pb_pin + place_ctx.block_locs[clb].z * num_basic_block_pins;
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

bool is_opin(int ipin, t_type_ptr type) {

	/* Returns true if this clb pin is an output, false otherwise. */

    if(ipin > type->num_pins) {
        //Not a top level pin
        return false;
    }

	int iclass = type->pin_class[ipin];

	if (type->class_inf[iclass].type == DRIVER)
		return true;
	else
		return false;
}

bool is_input_type(t_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return device_ctx.input_types.count(type);
}

bool is_output_type(t_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return device_ctx.output_types.count(type);
}

bool is_io_type(t_type_ptr type) {
    return is_input_type(type)
        || is_output_type(type);
}

bool is_empty_type(t_type_ptr type) {
    auto& device_ctx = g_vpr_ctx.device();

    return type == device_ctx.EMPTY_TYPE;
}

/* Each node in the pb_graph for a top-level pb_type can be uniquely identified
 * by its pins. Since the pins in a cluster of a certain type are densely indexed,
 * this function will find the pin index (int pin_count_in_cluster) of the first
 * pin for a given pb_graph_node, and use this index value as unique identifier
 * for the node.
 */
int get_unique_pb_graph_node_id(const t_pb_graph_node *pb_graph_node) {
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
	}
	else {
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
		int *class_low,
		int *class_high) {

	/* Assumes that the placement has been done so each block has a set of pins allocated to it */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

	t_type_ptr type = cluster_ctx.clb_nlist.block_type(blk_id);
	VTR_ASSERT(type->num_class % type->capacity == 0);
	*class_low = place_ctx.block_locs[blk_id].z * (type->num_class / type->capacity);
	*class_high = (place_ctx.block_locs[blk_id].z + 1) * (type->num_class / type->capacity) - 1;
}

void get_pin_range_for_block(const ClusterBlockId blk_id,
		int *pin_low,
		int *pin_high) {

	/* Assumes that the placement has been done so each block has a set of pins allocated to it */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

	t_type_ptr type = cluster_ctx.clb_nlist.block_type(blk_id);
	VTR_ASSERT(type->num_pins % type->capacity == 0);
	*pin_low = place_ctx.block_locs[blk_id].z * (type->num_pins / type->capacity);
	*pin_high = (place_ctx.block_locs[blk_id].z + 1) * (type->num_pins / type->capacity) - 1;
}

t_type_descriptor* find_block_type_by_name(std::string name, t_type_descriptor* types, int num_types) {

    for (int itype = 0; itype < num_types; ++itype) {
        if (types[itype].name == name) {
            return &types[itype];
        }
    }
    return nullptr; //Not found
}

t_type_ptr find_most_common_block_type(const DeviceGrid& grid) {
    auto& device_ctx = g_vpr_ctx.device();

    t_type_ptr max_type = nullptr;
    size_t max_count = 0;
    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        t_type_ptr type = &device_ctx.block_types[itype];

        size_t inst_cnt = grid.num_instances(type);
        if (max_count < inst_cnt) {
            max_count = inst_cnt;
            max_type = type;
        }
    }

    VTR_ASSERT(max_type);
    return max_type;
}

//Returns true if the specified block type contains the specified blif model name
bool block_type_contains_blif_model(t_type_ptr type, std::string blif_model_name) {
    return pb_type_contains_blif_model(type->pb_type, blif_model_name);
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

int get_max_primitives_in_pb_type(t_pb_type *pb_type) {

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
int get_max_nets_in_pb_type(const t_pb_type *pb_type) {

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

int get_max_depth_of_pb_type(t_pb_type *pb_type) {

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
bool primitive_type_feasible(const AtomBlockId blk_id, const t_pb_type *cur_pb_type) {

	if (cur_pb_type == nullptr) {
		return false;
	}

    auto& atom_ctx = g_vpr_ctx.atom();
    if(cur_pb_type->model != atom_ctx.nlist.block_model(blk_id)) {
        //Primitive and atom do not match
        return false;
    }

    VTR_ASSERT_MSG(atom_ctx.nlist.is_compressed(), "This function assumes a compresssed/non-dirty netlist");


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

        if(port_id) { //Port is used by the atom

            //In compressed form the atom netlist stores only in-use pins,
            //so we can query the number of required pins directly
            int required_atom_pins = atom_ctx.nlist.port_pins(port_id).size();

            int available_pb_pins = pb_port->num_pins;

            if(available_pb_pins < required_atom_pins) {
                //Too many pins required
                return false;
            }

            //Note that this port was checked
            ++checked_ports;
        }
    }

    //Similarily to pins, only in-use ports are stored in the compressed
    //atom netlist, so we can figure out how many ports should have been
    //checked directly
    size_t atom_ports = atom_ctx.nlist.block_ports(blk_id).size();

    //See if all the atom ports were checked
    if(checked_ports != atom_ports) {
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

    for(int isibling = 0; isibling < pb_type->parent_mode->num_pb_type_children; ++isibling) {
        const t_pb* sibling_pb = &memory_class_pb->child_pbs[pb->mode][isibling];

        if(sibling_pb->name != nullptr) {
            return atom_ctx.lookup.pb_atom(sibling_pb);
        }
    }
    return AtomBlockId::INVALID();
}


/**
 * Return pb_graph_node pin from model port and pin
 *  NULL if not found
 */
t_pb_graph_pin* get_pb_graph_node_pin_from_model_port_pin(const t_model_ports *model_port, const int model_pin, const t_pb_graph_node *pb_graph_node) {
	int i;

	if(model_port->dir == IN_PORT) {
		if(model_port->is_clock == false) {
			for (i = 0; i < pb_graph_node->num_input_ports; i++) {
				if (pb_graph_node->input_pins[i][0].port->model_port == model_port) {
					if(pb_graph_node->num_input_pins[i] > model_pin) {
						return &pb_graph_node->input_pins[i][model_pin];
					} else {
						return nullptr;
					}
				}
			}
		} else {
			for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
				if (pb_graph_node->clock_pins[i][0].port->model_port == model_port) {
					if(pb_graph_node->num_clock_pins[i] > model_pin) {
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
				if(pb_graph_node->num_output_pins[i] > model_pin) {
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
    for(AtomPinId pin : atom_ctx.nlist.net_pins(atom_net)) {
        AtomBlockId blk = atom_ctx.nlist.pin_block(pin);
        if(atom_ctx.lookup.atom_clb(blk) == blk_id) {
            //Part of the same CLB
            if(atom_ctx.lookup.atom_pin_pb_graph_pin(pin) == pb_gpin)
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
	const t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;
    auto& cluster_ctx = g_vpr_ctx.clustering();

	pb_graph_node = cluster_ctx.clb_nlist.block_pb(iblock)->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	/* If this is post-placed, then the ipin may have been shuffled up by the z * num_pins,
	bring it back down to 0..num_pins-1 range for easier analysis */
	ipin %= (pb_type->num_input_pins + pb_type->num_output_pins + pb_type->num_clock_pins);

	if(ipin < pb_type->num_input_pins) {
		count = ipin;
		for(i = 0; i < pb_graph_node->num_input_ports; i++) {
			if(count - pb_graph_node->num_input_pins[i] >= 0) {
				count -= pb_graph_node->num_input_pins[i];
			} else {
				return &pb_graph_node->input_pins[i][count];
			}
		}
	} else if (ipin < pb_type->num_input_pins + pb_type->num_output_pins) {
		count = ipin - pb_type->num_input_pins;
		for(i = 0; i < pb_graph_node->num_output_ports; i++) {
			if(count - pb_graph_node->num_output_pins[i] >= 0) {
				count -= pb_graph_node->num_output_pins[i];
			} else {
				return &pb_graph_node->output_pins[i][count];
			}
		}
	} else {
		count = ipin - pb_type->num_input_pins - pb_type->num_output_pins;
		for(i = 0; i < pb_graph_node->num_clock_ports; i++) {
			if(count - pb_graph_node->num_clock_pins[i] >= 0) {
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

    if(gpin != nullptr) {
        return gpin->port;
    }
    return nullptr;
}

const t_pb_graph_pin* find_pb_graph_pin(const t_pb_graph_node* pb_gnode, std::string port_name, int index) {
	for(int iport = 0; iport < pb_gnode->num_input_ports; iport++) {
        if(pb_gnode->num_input_pins[iport] < index) continue;

        const t_pb_graph_pin* gpin = &pb_gnode->input_pins[iport][index];

        if(gpin->port->name == port_name) {
            return gpin;
        }
    }
	for(int iport = 0; iport < pb_gnode->num_output_ports; iport++) {
        if(pb_gnode->num_output_pins[iport] < index) continue;

        const t_pb_graph_pin* gpin = &pb_gnode->output_pins[iport][index];

        if(gpin->port->name == port_name) {
            return gpin;
        }
    }
	for(int iport = 0; iport < pb_gnode->num_clock_ports; iport++) {
        if(pb_gnode->num_clock_pins[iport] < index) continue;

        const t_pb_graph_pin* gpin = &pb_gnode->clock_pins[iport][index];

        if(gpin->port->name == port_name) {
            return gpin;
        }
    }

    //Not found
    return nullptr;
}

/* Recusively visit through all pb_graph_nodes to populate pb_graph_pin_lookup_from_index */
static void load_pb_graph_pin_lookup_from_index_rec(t_pb_graph_pin ** pb_graph_pin_lookup_from_index, t_pb_graph_node *pb_graph_node) {
	for(int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
			t_pb_graph_pin * pb_pin = &pb_graph_node->input_pins[iport][ipin];
			VTR_ASSERT(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == nullptr);
			pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
		}
	}
	for(int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
			t_pb_graph_pin * pb_pin = &pb_graph_node->output_pins[iport][ipin];
			VTR_ASSERT(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == nullptr);
			pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
		}
	}
	for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
			t_pb_graph_pin * pb_pin = &pb_graph_node->clock_pins[iport][ipin];
			VTR_ASSERT(pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] == nullptr);
			pb_graph_pin_lookup_from_index[pb_pin->pin_count_in_cluster] = pb_pin;
		}
	}

	for(int imode = 0; imode < pb_graph_node->pb_type->num_modes; imode++) {
		for(int ipb_type = 0; ipb_type < pb_graph_node->pb_type->modes[imode].num_pb_type_children; ipb_type++) {
			for(int ipb = 0; ipb < pb_graph_node->pb_type->modes[imode].pb_type_children[ipb_type].num_pb; ipb++) {
				load_pb_graph_pin_lookup_from_index_rec(pb_graph_pin_lookup_from_index, &pb_graph_node->child_pb_graph_nodes[imode][ipb_type][ipb]);
			}
		}
	}
}

/* Create a lookup that returns a pb_graph_pin pointer given the pb_graph_pin index */
t_pb_graph_pin** alloc_and_load_pb_graph_pin_lookup_from_index(t_type_ptr type) {
	t_pb_graph_pin** pb_graph_pin_lookup_from_type = nullptr;

	t_pb_graph_node *pb_graph_head = type->pb_graph_head;
	if(pb_graph_head == nullptr) {
		return nullptr;
	}
	int num_pins = pb_graph_head->total_pb_pins;

    VTR_ASSERT(num_pins > 0);

	pb_graph_pin_lookup_from_type = new t_pb_graph_pin* [num_pins];
	for(int id = 0; id < num_pins; id++) {
		pb_graph_pin_lookup_from_type[id] = nullptr;
	}

    VTR_ASSERT(pb_graph_pin_lookup_from_type);

	load_pb_graph_pin_lookup_from_index_rec(pb_graph_pin_lookup_from_type, pb_graph_head);

	for(int id = 0; id < num_pins; id++) {
		VTR_ASSERT(pb_graph_pin_lookup_from_type[id] != nullptr);
	}

	return pb_graph_pin_lookup_from_type;
}

/* Free pb_graph_pin lookup array */
void free_pb_graph_pin_lookup_from_index(t_pb_graph_pin** pb_graph_pin_lookup_from_type) {
	if(pb_graph_pin_lookup_from_type == nullptr) {
		return;
	}
	delete [] pb_graph_pin_lookup_from_type;
}


/**
* Create lookup table that returns a pointer to the pb given [index to block][pin_id].
*/
vtr::vector_map<ClusterBlockId, t_pb **> alloc_and_load_pin_id_to_pb_mapping() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

	vtr::vector_map<ClusterBlockId, t_pb **> pin_id_to_pb_mapping(cluster_ctx.clb_nlist.blocks().size());
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
static void load_pin_id_to_pb_mapping_rec(t_pb *cur_pb, t_pb **pin_id_to_pb_mapping) {
	t_pb_graph_node *pb_graph_node = cur_pb->pb_graph_node;
	t_pb_type *pb_type = pb_graph_node->pb_type;
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

/*
* free pin_index_to_pb_mapping lookup table
*/
void free_pin_id_to_pb_mapping(vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		delete[] pin_id_to_pb_mapping[blk_id];
	}
	pin_id_to_pb_mapping.clear();
}



/**
 * Determine cost for using primitive within a complex block, should use primitives of low cost before selecting primitives of high cost
 For now, assume primitives that have a lot of pins are scarcer than those without so use primitives with less pins before those with more
 */
float compute_primitive_base_cost(const t_pb_graph_node *primitive) {

	return (primitive->pb_type->num_input_pins
			+ primitive->pb_type->num_output_pins
			+ primitive->pb_type->num_clock_pins);
}

int num_ext_inputs_atom_block(AtomBlockId blk_id) {

	/* Returns the number of input pins on this atom block that must be hooked *
	 * up through external interconnect.  That is, the number of input    *
	 * pins used - the number which connect (internally) to the outputs.   */

	int ext_inps = 0;

	/* TODO: process to get ext_inps is slow, should cache in lookup table */

    std::unordered_set<AtomNetId> input_nets;

    //Record the unique input nets
    auto& atom_ctx = g_vpr_ctx.atom();
    for(auto pin_id : atom_ctx.nlist.block_input_pins(blk_id)) {
        auto net_id = atom_ctx.nlist.pin_net(pin_id);
        input_nets.insert(net_id);
    }

    ext_inps = input_nets.size();

    //Look through the output nets for any duplicates of the input nets
    for(auto pin_id : atom_ctx.nlist.block_output_pins(blk_id)) {
        auto net_id = atom_ctx.nlist.pin_net(pin_id);
        if(input_nets.count(net_id)) {
            --ext_inps;
        }
    }

	VTR_ASSERT(ext_inps >= 0);

	return (ext_inps);
}

void free_pb(t_pb *pb) {
    if(pb == nullptr) {
        return;
    }

	const t_pb_type * pb_type;
	int i, j, mode;

	pb_type = pb->pb_graph_node->pb_type;

    if (pb->pb_route) {
        delete[] pb->pb_route;
        pb->pb_route = nullptr;
    }

    if (pb->name) {
        free(pb->name);
        pb->name = nullptr;
    }

	if (pb_type->blif_model == nullptr) {
		mode = pb->mode;
		for (i = 0; i < pb_type->modes[mode].num_pb_type_children && pb->child_pbs != nullptr; i++) {
			for (j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb	&& pb->child_pbs[i] != nullptr; j++) {
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

void revalid_molecules(const t_pb* pb, const std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules) {
	const t_pb_type* pb_type = pb->pb_graph_node->pb_type;

	if (pb_type->blif_model == nullptr) {
		int mode = pb->mode;
		for (int i = 0; i < pb_type->modes[mode].num_pb_type_children && pb->child_pbs != nullptr; i++) {
			for (int j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb	&& pb->child_pbs[i] != nullptr; j++) {
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
            for(const auto& kv : vtr::make_range(rng.first, rng.second)) {
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
                    }
                }
            }
        }
    }
}

void free_pb_stats(t_pb *pb) {

    if(pb) {
        if(pb->pb_stats == nullptr) {
            return;
        }

        pb->pb_stats->gain.clear();
        pb->pb_stats->timinggain.clear();
        pb->pb_stats->sharinggain.clear();
        pb->pb_stats->hillgain.clear();
        pb->pb_stats->connectiongain.clear();
        pb->pb_stats->num_pins_of_net_in_pb.clear();

        if(pb->pb_stats->feasible_blocks) {
            free(pb->pb_stats->feasible_blocks);
        }
        if(pb->pb_stats->transitive_fanout_candidates != nullptr) {
            delete pb->pb_stats->transitive_fanout_candidates;
        };
        delete pb->pb_stats;
        pb->pb_stats = nullptr;
    }
}

/***************************************************************************************
  Y.G.THIEN
  29 AUG 2012

 * The following functions maps the block pins indices for all block types to the      *
 * corresponding port indices and port_pin indices. This is necessary since there are  *
 * different netlist conventions - in the cluster level, ports and port pins are used  *
 * while in the post-pack level, block pins are used.                                  *
 *                                                                                     *
 ***************************************************************************************/

void get_port_pin_from_blk_pin(int blk_type_index, int blk_pin, int * port,
		int * port_pin) {

	/* These two mappings are needed since there are two different netlist   *
	 * conventions - in the cluster level, ports and port pins are used      *
	 * while in the post-pack level, block pins are used. The reason block   *
	 * type is used instead of blocks is that the mapping is the same for    *
	 * blocks belonging to the same block type.                              *
	 *                                                                       *
	 * f_port_from_blk_pin array allow us to quickly find what port a        *
	 * block pin corresponds to.                                             *
	 * [0...device_ctx.num_block_types-1][0...blk_pin_count-1]                                *
	 *                                                                       *
	 * f_port_pin_from_blk_pin array allow us to quickly find what port      *
	 * pin a block pin corresponds to.                                       *
	 * [0...device_ctx.num_block_types-1][0...blk_pin_count-1]                                */

	/* If either one of the arrays is not allocated and loaded, it is        *
	 * corrupted, so free both of them.                                      */
	if ((f_port_from_blk_pin == nullptr && f_port_pin_from_blk_pin != nullptr)
		|| (f_port_from_blk_pin != nullptr && f_port_pin_from_blk_pin == nullptr)){
		free_port_pin_from_blk_pin();
	}

	/* If the arrays are not allocated and loaded, allocate it.              */
	if (f_port_from_blk_pin == nullptr && f_port_pin_from_blk_pin == nullptr) {
		alloc_and_load_port_pin_from_blk_pin();
	}

	/* Return the port and port_pin for the pin.                             */
	*port = f_port_from_blk_pin[blk_type_index][blk_pin];
	*port_pin = f_port_pin_from_blk_pin[blk_type_index][blk_pin];

}

void free_port_pin_from_blk_pin() {

	/* Frees the f_port_from_blk_pin and f_port_pin_from_blk_pin arrays.     *
	 *                                                                       *
	 * This function is called when the file-scope arrays are corrupted.     *
	 * Otherwise, the arrays are freed in free_placement_structs()           */

	int itype;
    auto& device_ctx = g_vpr_ctx.device();

	if (f_port_from_blk_pin != nullptr) {
		for (itype = 1; itype < device_ctx.num_block_types; itype++) {
			free(f_port_from_blk_pin[itype]);
		}
		free(f_port_from_blk_pin);

		f_port_from_blk_pin = nullptr;
	}

	if (f_port_pin_from_blk_pin != nullptr) {
		for (itype = 1; itype < device_ctx.num_block_types; itype++) {
			free(f_port_pin_from_blk_pin[itype]);
		}
		free(f_port_pin_from_blk_pin);

		f_port_pin_from_blk_pin = nullptr;
	}

}

static void alloc_and_load_port_pin_from_blk_pin() {

	/* Allocates and loads f_port_from_blk_pin and f_port_pin_from_blk_pin   *
	 * arrays.                                                               *
	 *                                                                       *
	 * The arrays are freed in free_placement_structs()                      */

	int ** temp_port_from_blk_pin = nullptr;
	int ** temp_port_pin_from_blk_pin = nullptr;
	int itype, iblk_pin, iport, iport_pin;
	int blk_pin_count, num_port_pins, num_ports;
    auto& device_ctx = g_vpr_ctx.device();

	/* Allocate and initialize the values to OPEN (-1). */
	temp_port_from_blk_pin = (int **) vtr::malloc(device_ctx.num_block_types* sizeof(int*));
	temp_port_pin_from_blk_pin = (int **) vtr::malloc(device_ctx.num_block_types* sizeof(int*));
	for (itype = 1; itype < device_ctx.num_block_types; itype++) {

		blk_pin_count = device_ctx.block_types[itype].num_pins;

		temp_port_from_blk_pin[itype] = (int *) vtr::malloc(blk_pin_count* sizeof(int));
		temp_port_pin_from_blk_pin[itype] = (int *) vtr::malloc(blk_pin_count* sizeof(int));

		for (iblk_pin = 0; iblk_pin < blk_pin_count; iblk_pin++) {
			temp_port_from_blk_pin[itype][iblk_pin] = OPEN;
			temp_port_pin_from_blk_pin[itype][iblk_pin] = OPEN;
		}
	}

	/* Load the values */
	/* itype starts from 1 since device_ctx.block_types[0] is the EMPTY_TYPE. */
	for (itype = 1; itype < device_ctx.num_block_types; itype++) {

		blk_pin_count = 0;
		num_ports = device_ctx.block_types[itype].pb_type->num_ports;

		for (iport = 0; iport < num_ports; iport++) {

			num_port_pins = device_ctx.block_types[itype].pb_type->ports[iport].num_pins;

			for (iport_pin = 0; iport_pin < num_port_pins; iport_pin++) {

				temp_port_from_blk_pin[itype][blk_pin_count] = iport;
				temp_port_pin_from_blk_pin[itype][blk_pin_count] = iport_pin;
				blk_pin_count++;
			}
		}
	}

	/* Sets the file_scope variables to point at the arrays. */
	f_port_from_blk_pin = temp_port_from_blk_pin;
	f_port_pin_from_blk_pin = temp_port_pin_from_blk_pin;
}

void get_blk_pin_from_port_pin(int blk_type_index, int port,int port_pin,
		int * blk_pin) {

	/* This mapping is needed since there are two different netlist          *
	 * conventions - in the cluster level, ports and port pins are used      *
	 * while in the post-pack level, block pins are used. The reason block   *
	 * type is used instead of blocks is to save memories.                   *
	 *                                                                       *
	 * f_port_pin_to_block_pin array allows us to quickly find what block    *
	 * pin a port pin corresponds to.                                        *
	 * [0...device_ctx.num_block_types-1][0...num_ports-1][0...num_port_pins-1]               */

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

	int itype, iport, num_ports;
    auto& device_ctx = g_vpr_ctx.device();

	if (f_blk_pin_from_port_pin != nullptr) {

		for (itype = 1; itype < device_ctx.num_block_types; itype++) {
			num_ports = device_ctx.block_types[itype].pb_type->num_ports;
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

	int *** temp_blk_pin_from_port_pin = nullptr;
	int itype, iport, iport_pin;
	int blk_pin_count, num_port_pins, num_ports;
    auto& device_ctx = g_vpr_ctx.device();

	/* Allocate and initialize the values to OPEN (-1). */
	temp_blk_pin_from_port_pin = (int ***) vtr::malloc(device_ctx.num_block_types * sizeof(int**));
	for (itype = 1; itype < device_ctx.num_block_types; itype++) {
		num_ports = device_ctx.block_types[itype].pb_type->num_ports;
		temp_blk_pin_from_port_pin[itype] = (int **) vtr::malloc(num_ports * sizeof(int*));
		for (iport = 0; iport < num_ports; iport++) {
			num_port_pins = device_ctx.block_types[itype].pb_type->ports[iport].num_pins;
			temp_blk_pin_from_port_pin[itype][iport] = (int *) vtr::malloc(num_port_pins * sizeof(int));

			for(iport_pin = 0; iport_pin < num_port_pins; iport_pin ++) {
				temp_blk_pin_from_port_pin[itype][iport][iport_pin] = OPEN;
			}
		}
	}

	/* Load the values */
	/* itype starts from 1 since device_ctx.block_types[0] is the EMPTY_TYPE. */
	for (itype = 1; itype < device_ctx.num_block_types; itype++) {
		blk_pin_count = 0;
		num_ports = device_ctx.block_types[itype].pb_type->num_ports;
		for (iport = 0; iport < num_ports; iport++) {
			num_port_pins = device_ctx.block_types[itype].pb_type->ports[iport].num_pins;
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
  Y.G.THIEN
  30 AUG 2012

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

void parse_direct_pin_name(char * src_string, int line, int * start_pin_index,
		int * end_pin_index, char * pb_type_name, char * port_name){

	/* Parses out the pb_type_name and port_name from the direct passed in.   *
	 * If the start_pin_index and end_pin_index is specified, parse them too. *
	 * Return the values parsed by reference.                                 */

	char source_string[MAX_STRING_LEN+1];
	char * find_format = nullptr;
	int ichar, match_count;

	// parse out the pb_type and port name, possibly pin_indices
	find_format = strstr(src_string,"[");
	if (find_format == nullptr) {
		/* Format "pb_type_name.port_name" */
		*start_pin_index = *end_pin_index = -1;

        if(strlen(src_string) + 1 <= MAX_STRING_LEN + 1) {
            strcpy (source_string, src_string);
        } else {
            vpr_throw(VPR_ERROR_ARCH, __FILE__, __LINE__,
                      "Pin name exceeded buffer size of %zu characters", MAX_STRING_LEN + 1);

        }
		for (ichar = 0; ichar < (int)(strlen(source_string)); ichar++) {
			if (source_string[ichar] == '.')
				source_string[ichar] = ' ';
		}

		match_count = sscanf(source_string, "%s %s", pb_type_name, port_name);
		if (match_count != 2){
			vtr::printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid pin - %s, name should be in the format "
					"\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
					"The end_pin_index and start_pin_index can be the same.\n",
					line, src_string);
			exit(1);
		}
	} else {
		/* Format "pb_type_name.port_name[end_pin_index:start_pin_index]" */
		strcpy (source_string, src_string);
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
		if (match_count != 4){
			vtr::printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid pin - %s, name should be in the format "
					"\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
					"The end_pin_index and start_pin_index can be the same.\n",
					line, src_string);
			exit(1);
		}
		if (*end_pin_index < 0 || *start_pin_index < 0) {
			vtr::printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid pin - %s, the pin_index in "
					"[end_pin_index:start_pin_index] should not be a negative value.\n",
					line, src_string);
			exit(1);
		}
		if ( *end_pin_index < *start_pin_index) {
			vtr::printf_error(__FILE__, __LINE__,
					"[LINE %d] Invalid from_pin - %s, the end_pin_index in "
					"[end_pin_index:start_pin_index] should not be less than start_pin_index.\n",
					line, src_string);
			exit(1);
		}
	}
}

static void mark_direct_of_pins(int start_pin_index, int end_pin_index, int itype,
		int iport, int ** idirect_from_blk_pin, int idirect,
		int ** direct_type_from_blk_pin, int direct_type, int line, char * src_string) {

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
        for (const auto& fc_spec : device_ctx.block_types[itype].fc_specs) {
            for(int ipin : fc_spec.pins) {
                if(iblk_pin == ipin && fc_spec.fc_value > 0) {
                    all_fcs_0 = false;
                    break;
                }
            }
            if(!all_fcs_0) break;
        }

		// Check the fc for the pin, direct chain link only if fc == 0
		if (all_fcs_0) {
			idirect_from_blk_pin[itype][iblk_pin] = idirect;

			// Check whether the pins are marked, errors out if so
			if (direct_type_from_blk_pin[itype][iblk_pin] != OPEN) {
				vpr_throw(VPR_ERROR_ARCH, __FILE__, __LINE__,
						"[LINE %d] Invalid pin - %s, this pin is in more than one direct connection.\n",
						line, src_string);
			} else {
				direct_type_from_blk_pin[itype][iblk_pin] = direct_type;
			}
		}
	} // Finish marking all the pins

}

static void mark_direct_of_ports (int idirect, int direct_type, char * pb_type_name,
		char * port_name, int end_pin_index, int start_pin_index, char * src_string,
		int line, int ** idirect_from_blk_pin, int ** direct_type_from_blk_pin) {

	/* Go through all the ports in all the blocks to find the port that has the same   *
	 * name as port_name and belongs to the block type that has the name pb_type_name. *
	 * Then, check that whether start_pin_index and end_pin_index are specified. If    *
	 * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
	 * Otherwise, mark down all the pins in that port.                                 */

	int num_ports, num_port_pins;
	int itype, iport;
    auto& device_ctx = g_vpr_ctx.device();

	// Go through all the block types
	for (itype = 1; itype < device_ctx.num_block_types; itype++) {

		// Find blocks with the same pb_type_name
		if (strcmp(device_ctx.block_types[itype].pb_type->name, pb_type_name) == 0) {
			num_ports = device_ctx.block_types[itype].pb_type->num_ports;
			for (iport = 0; iport < num_ports; iport++) {
				// Find ports with the same port_name
				if (strcmp(device_ctx.block_types[itype].pb_type->ports[iport].name, port_name) == 0) {
					num_port_pins = device_ctx.block_types[itype].pb_type->ports[iport].num_pins;

					// Check whether the end_pin_index is valid
					if (end_pin_index > num_port_pins) {
						vtr::printf_error(__FILE__, __LINE__,
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
						mark_direct_of_pins(0, num_port_pins-1, itype,
								iport, idirect_from_blk_pin, idirect,
								direct_type_from_blk_pin, direct_type, line, src_string);
					}
				} // Do nothing if port_name does not match
			} // Finish going through all the ports
		} // Do nothing if pb_type_name does not match
	} // Finish going through all the blocks

}

void alloc_and_load_idirect_from_blk_pin(t_direct_inf* directs, int num_directs,
		int *** idirect_from_blk_pin, int *** direct_type_from_blk_pin) {

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

	int itype, iblk_pin, idirect, num_type_pins;
	int ** temp_idirect_from_blk_pin, ** temp_direct_type_from_blk_pin;

	char to_pb_type_name[MAX_STRING_LEN+1], to_port_name[MAX_STRING_LEN+1],
			from_pb_type_name[MAX_STRING_LEN+1], from_port_name[MAX_STRING_LEN+1];
	int to_start_pin_index = -1, to_end_pin_index = -1;
	int from_start_pin_index = -1, from_end_pin_index = -1;
    auto& device_ctx = g_vpr_ctx.device();

	/* Allocate and initialize the values to OPEN (-1). */
	temp_idirect_from_blk_pin = (int **) vtr::malloc(device_ctx.num_block_types * sizeof(int *));
	temp_direct_type_from_blk_pin = (int **) vtr::malloc(device_ctx.num_block_types * sizeof(int *));
	for (itype = 1; itype < device_ctx.num_block_types; itype++) {

		num_type_pins = device_ctx.block_types[itype].num_pins;

		temp_idirect_from_blk_pin[itype] = (int *) vtr::malloc(num_type_pins * sizeof(int));
		temp_direct_type_from_blk_pin[itype] = (int *) vtr::malloc(num_type_pins * sizeof(int));

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
		mark_direct_of_ports (idirect, SOURCE, from_pb_type_name, from_port_name,
				from_end_pin_index, from_start_pin_index, directs[idirect].from_pin,
				directs[idirect].line,
				temp_idirect_from_blk_pin, temp_direct_type_from_blk_pin);

		// Then, find blocks with the same name as to_pb_type_name and from_port_name
		mark_direct_of_ports (idirect, SINK, to_pb_type_name, to_port_name,
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
static int convert_switch_index(int *switch_index, int *fanin) {
    if (*switch_index == -1)
        return 1;

    auto& device_ctx = g_vpr_ctx.device();

    for (int iswitch = 0; iswitch < device_ctx.num_arch_switches; iswitch ++ ) {
        map<int, int>::iterator itr;
        for (itr = device_ctx.switch_fanin_remap[iswitch].begin(); itr != device_ctx.switch_fanin_remap[iswitch].end(); itr++) {
            if (itr->second == *switch_index) {
                *switch_index = iswitch;
                *fanin = itr->first;
                return 0;
            }
        }
    }
    *switch_index = -1;
    *fanin = -1;
    vtr::printf_info("\n\nerror converting switch index ! \n\n");
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

    if (device_ctx.switch_fanin_remap == nullptr) {
        vtr::printf_warning(__FILE__, __LINE__, "Cannot print switch usage stats: device_ctx.switch_fanin_remap is NULL\n");
        return;
    }
    map<int, int> *switch_fanin_count;
    map<int, float> *switch_fanin_delay;
    switch_fanin_count = new map<int, int>[device_ctx.num_arch_switches];
    switch_fanin_delay = new map<int, float>[device_ctx.num_arch_switches];
    // a node can have multiple inward switches, so
    // map key: switch index; map value: count (fanin)
    map<int, int> *inward_switch_inf = new map<int, int>[device_ctx.num_rr_nodes];
    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
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
            inward_switch_inf[to_node_index][switch_index] ++;
        }
    }
    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
        map<int, int>::iterator itr;
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
            switch_fanin_count[switch_index][fanin] ++;
            switch_fanin_delay[switch_index][fanin] = Tdel;
        }
    }
    vtr::printf_info("\n=============== switch usage stats ===============\n");
    for (int iswitch = 0; iswitch < device_ctx.num_arch_switches; iswitch ++ ) {
        char *s_name = device_ctx.arch_switch_inf[iswitch].name;
        float s_area = device_ctx.arch_switch_inf[iswitch].mux_trans_size;
        vtr::printf_info(">>>>> switch index: %d, name: %s, mux trans size: %g\n", iswitch, s_name, s_area);

        map<int, int>::iterator itr;
        for (itr = switch_fanin_count[iswitch].begin(); itr != switch_fanin_count[iswitch].end(); itr ++ ) {
            vtr::printf_info("\t\tnumber of fanin: %d", itr->first);
            vtr::printf_info("\t\tnumber of wires driven by this switch: %d", itr->second);
            vtr::printf_info("\t\tTdel: %g\n", switch_fanin_delay[iswitch][itr->first]);
        }
    }
    vtr::printf_info("\n==================================================\n\n");
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
void print_usage_by_wire_length() {
    map<int, int> used_wire_count;
    map<int, int> total_wire_count;
    auto& device_ctx = g_vpr_ctx.device();
    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
        if (device_ctx.rr_nodes[inode].type() == CHANX || rr_node[inode].type() == CHANY) {
            //int length = abs(device_ctx.rr_nodes[inode].get_xhigh() + rr_node[inode].get_yhigh()
            //             - device_ctx.rr_nodes[inode].get_xlow() - rr_node[inode].get_ylow());
            int length = device_ctx.rr_nodes[inode].get_length();
            if (rr_node_route_inf[inode].occ() > 0) {
                if (used_wire_count.count(length) == 0)
                    used_wire_count[length] = 0;
                used_wire_count[length] ++;
            }
            if (total_wire_count.count(length) == 0)
                total_wire_count[length] = 0;
            total_wire_count[length] ++;
        }
    }
    int total_wires = 0;
    map<int, int>::iterator itr;
    for (itr = total_wire_count.begin(); itr != total_wire_count.end(); itr++) {
        total_wires += itr->second;
    }
    vtr::printf_info("\n\t-=-=-=-=-=-=-=-=-=-=- wire usage stats -=-=-=-=-=-=-=-=-=-=-\n");
    for (itr = total_wire_count.begin(); itr != total_wire_count.end(); itr++)
        vtr::printf_info("\ttotal number: wire of length %d, ratio to all length of wires: %g\n", itr->first, ((float)itr->second) / total_wires);
    for (itr = used_wire_count.begin(); itr != used_wire_count.end(); itr++) {
        float ratio_to_same_type_total = ((float)itr->second) / total_wire_count[itr->first];
        float ratio_to_all_type_total = ((float)itr->second) / total_wires;
        vtr::printf_info("\t\tratio to same type of wire: %g\tratio to all types of wire: %g\n", ratio_to_same_type_total, ratio_to_all_type_total);
    }
    vtr::printf_info("\n\t-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n");
    used_wire_count.clear();
    total_wire_count.clear();
}
*/

AtomBlockId find_tnode_atom_block(int inode) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& timing_ctx = g_vpr_ctx.timing();

    AtomBlockId blk_id;
    AtomPinId pin_id;
    auto type = timing_ctx.tnodes[inode].type;
    if(type == TN_INPAD_SOURCE || type == TN_FF_SOURCE) {
        //A source does not map directly to a netlist pin,
        //so we walk to it's assoicated OPIN
        VTR_ASSERT_MSG(timing_ctx.tnodes[inode].num_edges == 1, "Source nodes must have a single output edge");
        int i_opin_node = timing_ctx.tnodes[inode].out_edges[0].to_node;

        VTR_ASSERT(timing_ctx.tnodes[i_opin_node].type == TN_INPAD_OPIN ||timing_ctx.tnodes[i_opin_node].type == TN_FF_OPIN);

        pin_id = atom_ctx.lookup.classic_tnode_atom_pin(i_opin_node);

    } else if (type == TN_OUTPAD_SINK || type == TN_FF_SINK) {
        //A sink does not map directly to a netlist pin,
        //so we go back to its input pin

        //By convention the sink pin is at one index before the sink itself
        int i_ipin_node = inode - 1;
        VTR_ASSERT(timing_ctx.tnodes[i_ipin_node].type == TN_OUTPAD_IPIN || timing_ctx.tnodes[i_ipin_node].type == TN_FF_IPIN);
        VTR_ASSERT(timing_ctx.tnodes[i_ipin_node].num_edges == 1);
        VTR_ASSERT(timing_ctx.tnodes[i_ipin_node].out_edges[0].to_node == inode);

        pin_id = atom_ctx.lookup.classic_tnode_atom_pin(i_ipin_node);
    } else {
        pin_id = atom_ctx.lookup.classic_tnode_atom_pin(inode);
    }

    blk_id = atom_ctx.nlist.pin_block(pin_id);

    return blk_id;
}

void place_sync_external_block_connections(ClusterBlockId iblk) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    VTR_ASSERT_MSG(place_ctx.block_locs[iblk].nets_and_pins_synced_to_z_coordinate == false, "Block net and pins must not be already synced");

    t_type_ptr type = cluster_ctx.clb_nlist.block_type(iblk);
    VTR_ASSERT(type->num_pins % type->capacity == 0);
    int max_num_block_pins = type->num_pins / type->capacity;
    /* Logical location and physical location is offset by z * max_num_block_pins */

    auto& clb_nlist = cluster_ctx.clb_nlist;
    for (auto pin : clb_nlist.block_pins(iblk)) {
        int orig_phys_pin_index = clb_nlist.pin_physical_index(pin);
        int new_phys_pin_index = orig_phys_pin_index + place_ctx.block_locs[iblk].z * max_num_block_pins;
        clb_nlist.set_pin_physical_index(pin, new_phys_pin_index);
    }

    //Mark the block as synced
    place_ctx.block_locs[iblk].nets_and_pins_synced_to_z_coordinate = true;
}

int max_pins_per_grid_tile() {
    auto& device_ctx = g_vpr_ctx.device();
    int max_pins = 0;
    for (int i = 0; i < device_ctx.num_block_types; i++) {
        t_type_ptr type = &device_ctx.block_types[i];

        int pins_per_grid_tile = type->num_pins / (type->width * type->height);
        //Use the maximum number of pins normalized by block area
        max_pins = max(max_pins, pins_per_grid_tile);
    }
    return max_pins;
}

