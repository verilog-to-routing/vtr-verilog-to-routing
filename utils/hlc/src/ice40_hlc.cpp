#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "globals.h"

#include "vtr_assert.h"
#include "vtr_logic.h"
#include "vtr_version.h"

#include "atom_netlist_utils.h"
#include "netlist_writer.h"

#include "ice40_hlc.h"

/** FIXME: Remove these -- they belong in the architecture definition. */
std::array<std::pair<t_hlc_coord,int>,8> global_drivers{ {
    {{13, 8}, 1}, // global 0
    {{ 0, 8}, 1}, // global 1
    {{ 7,17}, 0}, // global 2
    {{ 7, 0}, 0}, // global 3
    {{ 0, 9}, 0}, // global 4
    {{13, 9}, 0}, // global 5
    {{ 6, 0}, 1}, // global 6
    {{ 6,17}, 1}  // global 7
} };

static std::string drives_global_net(t_hlc_coord c, int index) {
    int i = 0;
    for (auto d : global_drivers) {
        if (d.first == c && d.second == index) {
            return "glb_netwk_"+std::to_string(i);
        }
        i++;
    }
    return "";
}

static int _find_pb_index(const t_pb_graph_node *n) {
    for (; n; n = n->parent_pb_graph_node)
        if (n->pb_type->num_pb > 1)
            return n->placement_index;
    return -1;
}

// join a vector of elements by a delimiter object.  ostream<< must be defined
// for both class S and T and an ostream, as it is e.g. in the case of strings
// and character arrays
template<class S, class T>
std::string join(std::vector<T>& elems, S& delim) {
    std::stringstream ss;
    typename std::vector<T>::iterator e = elems.begin();
	if (e == elems.end())
		return "";
    ss << *e++;
    for (; e != elems.end(); ++e) {
        ss << delim << *e;
    }
    return ss.str();
}

static std::string get_fasm_name(const t_pb_type *pb) {
	if (pb->name == nullptr) {
		return "unknown";
	}
    return pb->name;
}

static ClusterBlockId blk_id_from_pb_graph_node(const t_pb *pb) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for(auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        const auto cpb = cluster_ctx.clb_nlist.block_pb(blk_id);
        if (cpb == pb) {
            return blk_id;
        }
    }
    return ClusterBlockId::INVALID();
}

static int get_index(const t_pb *pb) {
    if (!pb->is_root()) {
        if (pb->pb_graph_node->pb_type->num_pb > 0) {
            return pb->pb_graph_node->placement_index;
        } else {
            return -1;
        }
    } else {
        auto blk_id = blk_id_from_pb_graph_node(pb);
        if (blk_id == ClusterBlockId::INVALID()) {
            return -1;
        }
        auto& place_ctx = g_vpr_ctx.placement();
        return place_ctx.block_locs[blk_id].z;
    }
}

static std::string get_index_string(const t_pb *pb) {
    if (!pb->is_root()) {
        if (pb->pb_graph_node->pb_type->num_pb > 0) {
            return "i-"+std::to_string(pb->pb_graph_node->placement_index);
        } else {
            return "";
        }
    } else {
        auto blk_id = blk_id_from_pb_graph_node(pb);
        if (blk_id == ClusterBlockId::INVALID()) {
            return "";
        }
        auto& place_ctx = g_vpr_ctx.placement();
        return "t-"+std::to_string(place_ctx.block_locs[blk_id].z);
    }
}

static std::string get_fasm_fullname(const t_pb *pb) {
	const t_pb* curr = pb;
	std::vector<std::string> v;
	for (; curr != nullptr; curr = curr->parent_pb) {
        if (curr->has_modes()) {
            auto mode_name = curr->get_mode()->name;
            if (std::string(mode_name) != "default") {
                v.insert(v.begin(), "mode-"+std::string(curr->get_mode()->name));
            }
        }
        auto index = get_index_string(curr);
        if (index.size() > 0) {
            v.insert(v.begin(), index);
        }
		v.insert(v.begin(), "type-"+get_fasm_name(curr->pb_graph_node->pb_type));
	}

	return join(v, ".");
}

/**
 * Get the HLC "tile type" string from a block ID.
 */
static t_hlc_tile_type _tile_type_from_name(std::string s) {
    if (s.size() <= 7) {
        return HLC_TILE_NULL;
    }
    auto name = s.substr(7);
    const std::map<std::string, t_hlc_tile_type> block_types = {
        {"PLB", HLC_TILE_LOGIC},
        {"PIO_A", HLC_TILE_IO},
        {"RAMT", HLC_TILE_RAM_TOP},
        {"RAMB", HLC_TILE_RAM_BOTTOM},
    };
    try {
        return block_types.at(name);
    } catch (const std::out_of_range&) {
        return HLC_TILE_NULL;
    }
}

static t_hlc_sw_type _sw_type_from_id(int iswitch) {
    if (iswitch < 0) {
        return HLC_SW_END;
    }
    auto& device_ctx = g_vpr_ctx.device();
    auto sw = device_ctx.rr_switch_inf[iswitch];
    const std::map<std::string, t_hlc_sw_type> sw_name_map = {
        {"buffer",                      HLC_SW_BUFFER},
        {"mux",                         HLC_SW_BUFFER},
        {"routing",                     HLC_SW_ROUTING},
        {"short",                       HLC_SW_SHORT},
        {"__vpr_delayless_switch__",    HLC_SW_SHORT},
    };
    try {
        return sw_name_map.at(sw.name);
    } catch (const std::out_of_range&) {}

    if (sw.configurable()) {
        if (sw.buffered()) {
            return HLC_SW_BUFFER;
        } else {
            return HLC_SW_ROUTING;
        }
    }

    return HLC_SW_SHORT;
}

static std::string _sw_name_from_id(int iswitch) {
    return hlc_sw_str(_sw_type_from_id(iswitch));
}

/**
 * Translate a block's coordinates into HLC coordinates.
 */
static t_hlc_coord _translate_coords(int x, int y) {
    return t_hlc_coord(x - 2, y - 2);
}
static std::pair<int, int> _translate_coords(t_hlc_coord c) {
    return std::make_pair(c.x + 2, c.y + 2);
}

ICE40HLCWriterVisitor::ICE40HLCWriterVisitor(std::ostream& os)
        : os_(os)
        , output_()
        , current_tile_(nullptr)
        , current_cell_(nullptr) {
}

static t_offset hlc_coord_offset(t_hlc_coord a, t_hlc_coord b) {
    t_offset o;
    o.x = a.x-b.x;
    o.y = a.y-b.y;
    return o;
}

static std::pair<int, int> overlap(short low_a, short high_a, short low_b, short high_b) {
    auto overlap = std::make_pair(std::max(low_a, low_b), std::min(high_a, high_b));
    if (overlap.first <= overlap.second) {
        return overlap;
    } else {
        return std::make_pair(-1, -1);
    }
}

static std::pair<t_hlc_coord,t_hlc_coord> overlap(t_hlc_coord a, t_hlc_coord b) {
    auto overlap_x = overlap(a.x, a.x, b.x, b.x);
    auto overlap_y = overlap(a.y, a.y, b.y, b.y);

    t_hlc_coord low;
    t_hlc_coord high;
    low.x = overlap_x.first;
    low.y = overlap_y.first;
    high.x = overlap_x.second;
    high.y = overlap_y.second;
    return std::make_pair(low, high);
}

static t_hlc_coord parse_hlc_coord(std::string s) {
    std::stringstream ss(s);
    t_hlc_coord pos(-1, -1);
    ss >> pos.x;
    ss.ignore(1, ',');
    ss >> pos.y;
    if (pos.x == -1 || pos.y == -1) {
        return t_hlc_coord(-2, -2);
    }
    return pos;
}

static t_hlc_coord metadata_hlc_coord(const t_rr_node& src_node) {
    auto hlc_coordp = src_node.metadata("hlc_coord");
    if (hlc_coordp == nullptr) {
        return t_hlc_coord(-4, -4);
    }
    return parse_hlc_coord(hlc_coordp->as_string());
}

static t_hlc_coord metadata_hlc_coord(const t_rr_node& src_node, int sink_id, short switch_id) {
    auto iedge = src_node.get_iedge(sink_id, switch_id);
    VTR_ASSERT(iedge != -1);
    auto hlc_coordp = src_node.edge_metadata(sink_id, switch_id, "hlc_coord");
    if (hlc_coordp == nullptr) {
        return t_hlc_coord(-3, -3);
    }
    return parse_hlc_coord(hlc_coordp->as_string());
}

static void node_output(std::ostream& os, int node_id, const t_rr_node& node) {
    os << "(" << std::setw(6) << node_id << ") ";
    os << std::setw(6) << node.type_string();
}

static const t_pb* _find_top_cb(const t_pb* curr) {
    //Walk up through the pb graph until curr
    //has no parent, at which point it will be the top pb
    const t_pb* parent = curr->parent_pb;
    while(parent != nullptr) {
        curr = parent;
        parent = curr->parent_pb;
    }
    return curr;
}

static const t_pb_routes *_find_top_pb_route(const t_pb* curr) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const t_pb* top_pb = _find_top_cb(curr);
    for(auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if(cluster_ctx.clb_nlist.block_pb(blk_id) == top_pb) {
            return &cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route;
        }
    }
    VTR_ASSERT(false);
    return nullptr;
}

static AtomNetId _find_atom_input_logical_net(const t_pb* atom, int atom_input_idx) {
    const t_pb_graph_node* pb_node = atom->pb_graph_node;
    const int cluster_pin_idx = pb_node->input_pins[0][atom_input_idx].pin_count_in_cluster;
    const auto *top_pb_route = _find_top_pb_route(atom);
    if(top_pb_route->count(cluster_pin_idx) > 0) {
        return top_pb_route->at(cluster_pin_idx).atom_net_id;
    } else {
        return AtomNetId::INVALID();
    }
}

//Helper function for load_lut_mask() which determines how the LUT inputs were
//permuted compared to the input BLIF
//
//  Since the LUT inputs may have been rotated from the input blif specification we need to
//  figure out this permutation to reflect the physical implementation connectivity.
//
//  We return a permutation map (which is a list of swaps from index to index)
//  which is then applied to do the rotation of the lutmask.
//
//  The net in the atom netlist which was originally connected to pin i, is connected
//  to pin permute[i] in the implementation.
static std::vector<int> _determine_lut_permutation(size_t num_inputs, const t_pb* atom_pb) {
    auto& atom_ctx = g_vpr_ctx.atom();

    std::vector<int> permute(num_inputs, OPEN);

    //Determine the permutation
    //
    //We walk through the logical inputs to this atom (i.e. in the original truth table/netlist)
    //and find the corresponding input in the implementation atom (i.e. in the current netlist)
    auto ports = atom_ctx.nlist.block_input_ports(atom_ctx.lookup.pb_atom(atom_pb));
    if(ports.size() == 1) {
        const t_pb_graph_node* gnode = atom_pb->pb_graph_node;
        VTR_ASSERT(gnode->num_input_ports == 1);
        VTR_ASSERT(gnode->num_input_pins[0] >= (int) num_inputs);

        AtomPortId port_id = *ports.begin();

        for(size_t ipin = 0; ipin < num_inputs; ++ipin) {
            //The net currently connected to input j
            const AtomNetId impl_input_net_id = _find_atom_input_logical_net(atom_pb, ipin);

            //Find the original pin index
            const t_pb_graph_pin* gpin = &gnode->input_pins[0][ipin];
            const BitIndex orig_index = atom_pb->atom_pin_bit_index(gpin);

            if(impl_input_net_id) {
                //If there is a valid net connected in the implementation
                AtomNetId logical_net_id = atom_ctx.nlist.port_net(port_id, orig_index);
                VTR_ASSERT(impl_input_net_id == logical_net_id);

                //Mark the permutation.
                //  The net originally located at orig_index in the atom netlist
                //  was moved to ipin in the implementation
                permute[orig_index] = ipin;
            }
        }
    } else {
        //May have no inputs on a constant generator
        VTR_ASSERT(ports.size() == 0);
    }

    //Fill in any missing values in the permutation (i.e. zeros)
    std::set<int> perm_indicies(permute.begin(), permute.end());
    size_t unused_index = 0;
    for(size_t i = 0; i < permute.size(); i++) {
        if(permute[i] == OPEN) {
            while(perm_indicies.count(unused_index)) {
                unused_index++;
            }
            permute[i] = unused_index;
            perm_indicies.insert(unused_index);
        }
    }

    return permute;
}

static std::string hlc_tile(const t_pb_type* t, int xoff, int yoff) {
    std::string hlc_tile_key = "hlc_tile";
    std::string hlc_tile_name = "UNKNOWN";
    t_offset offset = {xoff, yoff, 0};

    if (t->meta != nullptr && t->meta->has(offset, hlc_tile_key)) {
      for (auto hlc_tile : *(t->meta->get(offset, hlc_tile_key))) {
        hlc_tile_name = hlc_tile.as_string();
      }
    }

    std::cerr << "hlc_tile:" << hlc_tile_name << " from: " << t->name << std::endl;

    return hlc_tile_name;
}


void ICE40HLCWriterVisitor::visit_top_impl(const char* top_level_name) {
    using std::endl;

    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    // Generate a header with useful creation information.
    output_.comments << " HLC generated by VPR " << vtr::VERSION << " from" << std::endl;
    output_.comments << " - architecture_id = " << device_ctx.arch.architecture_id << std::endl;
    output_.comments << " - atom_netlist_id = " << atom_ctx.nlist.netlist_id().c_str() << std::endl;
    output_.comments << " -      netlist_id = " << cluster_ctx.clb_nlist.netlist_id().c_str() << std::endl;
    output_.comments << " -    placement_id = " << place_ctx.placement_id.c_str() << std::endl;
    output_.comments << std::endl;
    output_.comments << "top: " << top_level_name << std::endl;

    // Generate the device name
    std::string device_name = device_ctx.grid.name().substr(2);
    std::transform(device_name.begin(), device_name.end(), device_name.begin(), ::tolower);
    output_.device = device_name;

    output_.size.x = device_ctx.grid.width() - 4;
    output_.size.y = device_ctx.grid.height() - 4;
    output_.warmboot = true;
}

void ICE40HLCWriterVisitor::set_cell(std::string name, int index) {
    current_cell_ = current_tile_->get_cell(name, index);
}

void ICE40HLCWriterVisitor::set_tile(t_hlc_coord pos, std::string name) {
    current_tile_ = output_.get_tile(pos);
    if (name.size() > 0) {
        current_tile_->name = name;
    }
    current_cell_ = nullptr;
}

void ICE40HLCWriterVisitor::visit_clb_impl(ClusterBlockId blk_id, const t_pb* clb) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    current_blk_id_ = blk_id;

	VTR_ASSERT(clb->pb_graph_node != nullptr);
	VTR_ASSERT(clb->pb_graph_node->pb_type);

    auto pb_type = clb->pb_graph_node->pb_type;
    int x = place_ctx.block_locs[blk_id].x;
    int y = place_ctx.block_locs[blk_id].y;
    int x_offset = device_ctx.grid[x][y].width_offset;;
    int y_offset = device_ctx.grid[x][y].height_offset;;

    set_tile(_translate_coords(x, y), hlc_tile(pb_type, x_offset, y_offset));

    if (pb_type->meta != nullptr && pb_type->meta->has("hlc_cell")) {
        std::string cell_name = pb_type->meta->get("hlc_cell")->front().as_string();
        set_cell(cell_name, get_index(clb));
    }
}

static const t_pb_graph_pin* is_node_used(const t_pb_routes &top_pb_route, const t_pb_graph_node* pb_graph_node) {
    // Is the node used at all?
    const t_pb_graph_pin* pin = nullptr;
    for(int port_index = 0; port_index < pb_graph_node->num_output_ports; ++port_index) {
        for(int pin_index = 0; pin_index < pb_graph_node->num_output_pins[port_index]; ++pin_index) {
            pin = &pb_graph_node->output_pins[port_index][pin_index];
            if (top_pb_route.find(pin->pin_count_in_cluster) != top_pb_route.end() &&
                top_pb_route[pin->pin_count_in_cluster].atom_net_id != AtomNetId::INVALID()) {
                return pin;
            }
        }
    }
    for(int port_index = 0; port_index < pb_graph_node->num_input_ports; ++port_index) {
        for(int pin_index = 0; pin_index < pb_graph_node->num_input_pins[port_index]; ++pin_index) {
            pin = &pb_graph_node->input_pins[port_index][pin_index];
            if (top_pb_route.find(pin->pin_count_in_cluster) != top_pb_route.end() &&
                top_pb_route[pin->pin_count_in_cluster].atom_net_id != AtomNetId::INVALID()) {
                return pin;
            }
        }
    }
    return nullptr;
}

static const t_pb_graph_pin* is_node_clocked(const t_pb_routes &top_pb_route, const t_pb_graph_node* pb_graph_node) {
    // Is the node used at all?
    const t_pb_graph_pin* pin = nullptr;
    for(int port_index = 0; port_index < pb_graph_node->num_clock_ports; ++port_index) {
        for(int pin_index = 0; pin_index < pb_graph_node->num_clock_pins[port_index]; ++pin_index) {
            pin = &pb_graph_node->clock_pins[port_index][pin_index];
            if (top_pb_route.find(pin->pin_count_in_cluster) != top_pb_route.end() &&
                top_pb_route[pin->pin_count_in_cluster].atom_net_id != AtomNetId::INVALID()) {
                return pin;
            }
        }
    }
    return nullptr;
}

static std::string lut_outputs(const AtomNetlist::TruthTable& truth_table, const std::vector<int> permute, size_t num_inputs) {
    const auto unpermuted_truth_table = permute_truth_table(truth_table, num_inputs, {0, 1, 2, 3});
    const auto permuted_truth_table = permute_truth_table(truth_table, num_inputs, permute);
    LogicVec mask = truth_table_to_lut_mask(permuted_truth_table, num_inputs);
    std::stringstream ss("");
    ss << "out = " << mask << std::endl;
    return ss.str();
}

static std::string lut_outputs(const t_pb* atom_pb) {
    auto& atom_ctx = g_vpr_ctx.atom();
    // Write the LUT config
    const int num_inputs = atom_pb->pb_graph_node->total_input_pins();

    const std::vector<int> permute = _determine_lut_permutation(num_inputs, atom_pb);

    const auto& truth_table = atom_ctx.nlist.block_truth_table(atom_ctx.lookup.pb_atom(atom_pb));
    return lut_outputs(truth_table, permute, num_inputs);
}

void ICE40HLCWriterVisitor::visit_all_impl(const t_pb_routes &top_pb_route, const t_pb* pb,
        const t_pb_graph_node* pb_graph_node) {

    auto pb_type = pb_graph_node->pb_type;

    auto io_pin = is_node_used(top_pb_route, pb_graph_node);
    auto clk_pin = is_node_clocked(top_pb_route, pb_graph_node);
    std::cout << "-visit_all_impl " << get_fasm_fullname(pb) << " used:" << (io_pin != nullptr) << " clocked:" << (clk_pin != nullptr) << std::endl;

    if (io_pin == nullptr) {
        return;
    }

    if (pb != nullptr) {
        if (pb->has_modes()) {
            auto mode = pb->get_mode();
            /** Hack for "route through" LUTs */
            if (std::string(mode->name) == std::string("wire")) {
                const int num_inputs = pb_graph_node->total_input_pins();
                auto& route = top_pb_route[io_pin->pin_count_in_cluster];

                AtomNetlist::TruthTable truth_table(1);
                truth_table[0].push_back(vtr::LogicValue::TRUE);
                truth_table[0].push_back(vtr::LogicValue::TRUE);

                if (!current_cell_) {
                    std::cout << "ERROR: No current cell!" << std::endl;
                    return;
                }
                current_cell_->enable(lut_outputs(truth_table, {route.pb_graph_pin->pin_number}, 4));

                auto& trace = current_cell_->comments;
                trace << " route_through " << route.pb_graph_pin->port->name << " " << route.pb_graph_pin->pin_number << " of " << num_inputs << std::endl;
            }

            if (mode->meta != nullptr) {
                if (mode->meta->has("hlc_property")) {
                    for (auto v : *(mode->meta->get("hlc_property"))) {
                        if (current_cell_ == NULL) {
                            std::cout << "No cell with mode " << std::string(mode->name) << std::endl;
                            continue;
                        }
                        current_cell_->enable(v.as_string());
                    }
                }
            }
        }
    }

    if (pb_type->meta != nullptr) {
        if (pb_type->meta->has("hlc_cell")) {
            std::string cell_name = pb_type->meta->get("hlc_cell")->front().as_string();
            set_cell(cell_name, get_index(pb));
        }

        if (pb_type->meta->has("hlc_property")) {
            for (auto v : *(pb_type->meta->get("hlc_property"))) {
                if (current_cell_) {
                    current_cell_->enable(v.as_string());
                } else if (current_tile_) {
                    current_tile_->enable(v.as_string());
                } else {
                    std::cout << "ERROR: No current cell!" << std::endl;
                    return;
                }
            }
        }
    }

    if (clk_pin != nullptr) {
        auto& route = top_pb_route[clk_pin->pin_count_in_cluster];
        // netlist_id
        std::cout << "clock_pin " << route.pb_graph_pin->port->name << " " << route.pb_graph_pin->pin_number << std::endl;
    }
}

void ICE40HLCWriterVisitor::visit_open_impl(const t_pb* atom) {
	std::cout << "--visit_open_impl " << get_fasm_fullname(atom) << std::endl;
}

void ICE40HLCWriterVisitor::visit_atom_impl(const t_pb* atom) {

    VTR_ASSERT(atom->name != nullptr);
    std::cout << "--visit_atom_impl " << get_fasm_fullname(atom) << " " << atom->name << std::endl;

    // Is this a LUT?
    auto& atom_ctx = g_vpr_ctx.atom();
    auto atom_blk_id = atom_ctx.lookup.pb_atom(atom);
    if (atom_blk_id == AtomBlockId::INVALID()) {
        return;
    }

    std::stringstream trace;

    const t_model* model = atom_ctx.nlist.block_model(atom_blk_id);
    if (model->name == std::string(MODEL_NAMES)) {
        std::string o = lut_outputs(atom);
        if (!current_cell_) {
            std::cout << "ERROR: No current cell!" << std::endl;
            return;
        }
        current_cell_->enable(o);
    }

    trace << " " << atom_ctx.nlist.block_name(atom_blk_id) << std::endl;

    auto attrs = atom_ctx.nlist.block_attrs(atom_blk_id);
    if (attrs.size() > 0) {
        trace << " - Attributes -------" << std::endl;
        for (auto attr : attrs) {
            trace << " " << attr.first << " = " << attr.second << std::endl;
        }
    }

    auto params = atom_ctx.nlist.block_params(atom_blk_id);
    if (params.size() > 0) {
        trace << " - Parameters -------" << std::endl;
        for (auto param : params) {
            if (param.first == "LUT_INIT") {
                std::string s(param.second);
                //std::reverse(s.begin(), s.end());
                VTR_ASSERT(current_cell_);
                current_cell_->enable("out = " + std::to_string(s.size()) + "'b" + s);
            }
            trace << " " << param.first << " = " << param.second << std::endl;
        }
    }

    if (current_cell_ != nullptr) {
        current_cell_->comments << trace.str();
    } else {
        current_tile_->comments << trace.str();
    }

}

void ICE40HLCWriterVisitor::finish_impl() {

    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    // FIXME: Make this an argument somewhere...
    bool verbose_ = true;

    /* Output the routing */
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        auto net_name = cluster_ctx.clb_nlist.net_name(net_id);
        std::cout << "Net name:" << net_name << " ";
        if (cluster_ctx.clb_nlist.net_is_global(net_id)) {
            std::cout << "Global net" << std::endl;

            std::cout << "->Net " << cluster_ctx.clb_nlist.net_name(net_id).c_str() << "(#" << size_t(net_id) << "): global net connecting:" << std::endl;

            // Find the driver for this global net
            std::string glb_net_name = "";
            for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
                ClusterBlockId block_id = cluster_ctx.clb_nlist.pin_block(pin_id);
                auto& block = place_ctx.block_locs[block_id];

                auto hlcpos = _translate_coords(block.x, block.y);
                glb_net_name = drives_global_net(hlcpos, block.z);
                if (glb_net_name.size() == 0) {
                    continue;
                }

                auto tile = output_.get_tile(hlcpos);
                VTR_ASSERT(tile != nullptr);
                auto cell = tile->get_cell("io", block.z);
                VTR_ASSERT(cell != nullptr);
                std::string name = cluster_ctx.clb_nlist.port_bit_name(pin_id);
                cell->enable_edge("GLOBAL_BUFFER_OUTPUT", glb_net_name, HLC_SW_BUFFER) << net_name;
                break;
            }
            for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
                ClusterBlockId block_id = cluster_ctx.clb_nlist.pin_block(pin_id);

                int pin_index = cluster_ctx.clb_nlist.pin_physical_index(pin_id);
                auto& block = place_ctx.block_locs[block_id];
                std::string name = cluster_ctx.clb_nlist.port_bit_name(pin_id);

                auto grid_tile = device_ctx.grid[block.x][block.y];
                int x_offset = grid_tile.type->pin_width_offset[pin_index];
                int y_offset = grid_tile.type->pin_height_offset[pin_index];

                auto hlcpos = _translate_coords(block.x + x_offset, block.y + y_offset);
                auto is_driver = drives_global_net(hlcpos, block.z);
                if (is_driver.size() == 0) {
                    auto tile = output_.get_tile(hlcpos);
                    VTR_ASSERT(tile != nullptr);
                    auto cell = tile->get_global_cell();
                    VTR_ASSERT(cell != nullptr);
                    if (glb_net_name.size() > 0) {
                        cell->enable_edge(glb_net_name, name, HLC_SW_BUFFER) << net_name;
                    }
                }
                std::cout << "-->Block " << cluster_ctx.clb_nlist.block_name(block_id).c_str() << " (#" << size_t(block_id) << ")";
                std::cout << " at " << place_ctx.block_locs[block_id].x << "," << place_ctx.block_locs[block_id].y << "[" << place_ctx.block_locs[block_id].z << "]";
                std::cout << " " << glb_net_name << " " << name << " driver? " << (is_driver.size() != 0);
                std::cout << std::endl;
            }
            continue;
        }
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() == false) {
            std::cout << "#  Used in local cluster only, reserved one CLB pin" << std::endl;
            continue;
        }
        std::cout << "Normal net" << std::endl;

        if (verbose_) {
            std::stringstream& trace = output_.comments;
            /** Output the block names connected to this net */
            std::unordered_set<std::string> block_names_set;
            for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
                ClusterBlockId block_id = cluster_ctx.clb_nlist.pin_block(pin_id);
                block_names_set.insert(cluster_ctx.clb_nlist.block_name(block_id));
            }
            trace << std::endl;
            trace << " Net name:" << cluster_ctx.clb_nlist.net_name(net_id) << std::endl;
            trace << "- Blocks -----------" << std::endl;
            for (auto block_name : block_names_set) {
                trace << " " << block_name << std::endl;
            }
            trace << " -----------------" << std::endl;
            std::string no_hlc_name = "No HLC name";
            /** Output the actual trace of the route */
            for (auto tptr = route_ctx.trace[net_id].head; tptr->next != nullptr; tptr = tptr->next) {
                VTR_ASSERT(tptr != nullptr);
                VTR_ASSERT(tptr->next != nullptr);
                // Source
                auto src_id = tptr->index;
                auto& src_node = device_ctx.rr_nodes[src_id];
                // Sink
                auto snk_id = tptr->next->index;
                auto& snk_node = device_ctx.rr_nodes[snk_id];

                if (tptr->iswitch == OPEN) {
                    trace << std::endl;
                } else {
                    // HLC Positions
                    auto hlcpos_edge = metadata_hlc_coord(src_node, snk_id, tptr->iswitch);
                    auto hlcpos_src = metadata_hlc_coord(src_node);
                    auto hlcpos_snk = metadata_hlc_coord(snk_node);
                    auto hlcpos_src_offset = hlc_coord_offset(hlcpos_edge, hlcpos_src);
                    auto hlcpos_snk_offset = hlc_coord_offset(hlcpos_edge, hlcpos_snk);

                    trace << "Edge: (" << std::setw(2) << hlcpos_edge.x << "," << std::setw(2) << hlcpos_edge.y << ") - ";
                    node_output(trace, src_id, src_node);
                    trace << " <" << std::setw(2) << std::to_string(tptr->iswitch) << "> ";
                    node_output(trace, snk_id, snk_node);

                    /* Output the HLC names */
                    auto sw = _sw_name_from_id(tptr->iswitch);

                    std::string src_name(no_hlc_name);
                    auto src_pname = src_node.metadata(hlcpos_src_offset, "hlc_name");
                    if (src_pname != nullptr) {
                        src_name = src_pname->as_string();
                    }

                    std::string snk_name(no_hlc_name);
                    auto snk_pname = snk_node.metadata(hlcpos_snk_offset, "hlc_name");
                    if (snk_pname != nullptr) {
                        snk_name = snk_pname->as_string();
                    }

                    trace << " " << std::setw(20) << std::right << src_name;
                    trace << " (" << std::setw(3) << hlcpos_src_offset.x << "," << std::setw(3) << hlcpos_src_offset.y << ")";
                    trace << " " << sw;
                    trace << " (" << std::setw(3) << hlcpos_snk_offset.x << "," << std::setw(3) << hlcpos_snk_offset.y << ")";
                    trace << " " << snk_name;
                    trace << std::endl;
                }
            }
        }

        // Set the edges needed for this route in the HLC file.
        for (auto tptr = route_ctx.trace[net_id].head; tptr != nullptr; tptr = tptr->next) {
            VTR_ASSERT(tptr != nullptr);
            // Source
            auto src_id = tptr->index;
            auto& src_node = device_ctx.rr_nodes[src_id];

            if (tptr->iswitch != OPEN) {
                VTR_ASSERT(tptr->next != nullptr);

                auto hlc_sw_type = _sw_type_from_id(tptr->iswitch);
                if (hlc_sw_type == HLC_SW_SHORT) {
                    continue;
                }

                // Sink
                auto snk_id = tptr->next->index;
                auto& snk_node = device_ctx.rr_nodes[snk_id];

                // HLC Positions
                auto hlcpos_edge = metadata_hlc_coord(src_node, snk_id, tptr->iswitch);
                auto hlcpos_src = metadata_hlc_coord(src_node);
                auto hlcpos_snk = metadata_hlc_coord(snk_node);
                auto hlcpos_src_offset = hlc_coord_offset(hlcpos_edge, hlcpos_src);
                auto hlcpos_snk_offset = hlc_coord_offset(hlcpos_edge, hlcpos_snk);

                auto src_pname = src_node.metadata(hlcpos_src_offset, "hlc_name");
                if (src_pname == nullptr) {
                    continue;
                }
                auto snk_pname = snk_node.metadata(hlcpos_snk_offset, "hlc_name");
                if (snk_pname == nullptr) {
                    continue;
                }

                auto tile = output_.get_tile(hlcpos_edge);

                // FIXME: Must be a better way to get the tile type
                auto grid_pos = _translate_coords(hlcpos_edge);
                auto type = device_ctx.grid[grid_pos.first][grid_pos.second].type;
                int x_offset = device_ctx.grid[grid_pos.first][grid_pos.second].width_offset;
                int y_offset = device_ctx.grid[grid_pos.first][grid_pos.second].height_offset;
                tile->name = hlc_tile(type->pb_type, x_offset, y_offset);

                auto& c = tile->enable_edge(src_pname->as_string(), snk_pname->as_string(), hlc_sw_type);
                if (verbose_) {
                    // Output a useful debugging about which rr_nodes this edge
                    // comes from.
                    c << " ";
                    c << std::setw(5) << std::right << src_id;
                    c << " => ";
                    c << std::setw(5) << std::right << snk_id;
                    c << "  ";
                    c << std::setw(6) << std::right << src_node.type_string();
                    c << " => ";
                    c << std::left << snk_node.type_string();
                }
            }
        }
    }

    // XXX(elms):
    for (auto clb_id : cluster_ctx.clb_nlist.blocks()) {
        auto& block = place_ctx.block_locs[clb_id];
        auto hlcpos = _translate_coords(block.x, block.y);

        auto tile = output_.get_tile(hlcpos);

        auto grid_pos = _translate_coords(hlcpos);
        auto type = device_ctx.grid[grid_pos.first][grid_pos.second].type;
        int x_offset = device_ctx.grid[grid_pos.first][grid_pos.second].width_offset;
        int y_offset = device_ctx.grid[grid_pos.first][grid_pos.second].height_offset;
        tile->name = hlc_tile(type->pb_type, x_offset, y_offset);

        auto atom_id = atom_ctx.lookup.clb_atom(clb_id);
        if (atom_id != AtomBlockId::INVALID()) {
            auto atom_pb = atom_ctx.lookup.atom_pb(atom_id);
            std::cout << "atomic " <<  atom_pb->name << std::endl;
            auto params = atom_ctx.nlist.block_params(atom_id);
            std::map<std::string, std::string> data;
            if (params.size() > 0) {
                std::cout << " - Parameters -------" << std::endl;
                for (auto param : params) {
                    std::cout << " " << param.first << " = " << param.second << std::endl;
                    if (param.first.find("INIT_") == 0) {
                        data[param.first] = param.second;
                    }
                    /*
                      SB_RAM40_4KRAMT Config Bit
                      WRITE_MODE[0]RamConfig CBIT_0
                      WRITE_MODE[1]RamConfig CBIT_1
                      READ_MODE[0]RamConfig CBIT_2
                      READ_MODE[1]RamConfig CBIT_3
                    */
                    if (param.first.find("WRITE_MODE") == 0 || param.first.find("READ_MODE") == 0) {
                        auto top_hlcpos = _translate_coords(block.x, block.y + 1);
                        auto top_tile = output_.get_tile(top_hlcpos);

                        if (param.first.find("WRITE_MODE") == 0) {
                            if (param.second[param.second.length()-1] == '1') {
                                top_tile->enable("RamConfig CBIT_0");
                            }
                            if (param.second[param.second.length()-2] == '1') {
                                top_tile->enable("RamConfig CBIT_1");
                            }
                        }
                        if (param.first.find("READ_MODE") == 0) {
                            if (param.second[param.second.length()-1] == '1') {
                                top_tile->enable("RamConfig CBIT_2");
                            }
                            if (param.second[param.second.length()-2] == '1') {
                                top_tile->enable("RamConfig CBIT_3");
                            }
                        }
                    }

                }

                if (data.size() > 0) {
                    tile->enable("power_up");
                    std::string data_str;
                    for (auto datum : data) {
                        data_str += "256'b" + datum.second + "\n";
                    }
                    tile->enable("data {\n" + data_str +" \n}");
                }
            }
        }
    }

    output_.print(os_);
}
