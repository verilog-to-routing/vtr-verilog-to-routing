/*This function loads in a routing resource graph written in xml format
 into vpr when the option --read_rr_graph <file name> is specified.
 * When it is not specified the build_rr_graph function is then called.
 * This is done using the libpugixml library. This is useful
 * when specific routing resources should remain constant or when
 * some information left out in the architecture can be specified
 * in the routing resource graph. The routing resource graph file is
 * contained in a <rr_graph> tag. Its subtags describe the rr graph's
 * various features such as nodes, edges, and segments. Information such
 * as blocks, grids, and segments are verified with the architecture
 * to ensure it matches. An error will through if any feature does not match.
 * Other elements such as edges, nodes, and switches
 * are overwritten by the rr graph file if one is specified. If an optional
 * identifier such as capacitance is not specified, it is set to 0*/

#include <string.h>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <utility>

#include "vtr_version.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_matrix.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "vtr_time.h"

#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "read_xml_arch_file.h"
#include "read_xml_util.h"
#include "globals.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_indexed_data.h"
#include "rr_graph_writer.h"
#include "check_rr_graph.h"
#include "echo_files.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"

#include "rr_graph_reader.h"


using namespace std;
using namespace pugiutil;

/*********************** Subroutines local to this module *******************/
void process_switches(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void verify_segments(pugi::xml_node parent, const pugiutil::loc_data & loc_data, const t_segment_inf *segment_inf);
void verify_blocks(pugi::xml_node parent, const pugiutil::loc_data & loc_data);
void process_blocks(pugi::xml_node parent, const pugiutil::loc_data & loc_data);
void verify_grid(pugi::xml_node parent, const pugiutil::loc_data& loc_data, const DeviceGrid& grid);
void process_nodes(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_edges(pugi::xml_node parent, const pugiutil::loc_data& loc_data, int *wire_to_rr_ipin_switch, const int num_rr_switches);
void process_channels(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_rr_node_indices(const DeviceGrid& grid);
void process_seg_id(pugi::xml_node parent, const pugiutil::loc_data & loc_data);
void set_cost_indices(pugi::xml_node parent, const pugiutil::loc_data& loc_data,
        const bool is_global_graph, const int num_seg_types);

/************************ Subroutine definitions ****************************/

/*loads the given RR_graph file into the appropriate data structures
 * as specified by read_rr_graph_name. Set up correct routing data
 * structures as well*/
void load_rr_file(const t_graph_type graph_type,
        const DeviceGrid& grid,
        t_chan_width *nodes_per_chan,
        const int num_seg_types,
        const t_segment_inf * segment_inf,
        const enum e_base_cost_type base_cost_type,
        int *wire_to_rr_ipin_switch,
        int *num_rr_switches,
        const char* read_rr_graph_name) {
    vtr::ScopedStartFinishTimer timer("Loading routing resource graph");

    const char *Prop;
    pugi::xml_node next_component;

    pugi::xml_document doc;
    pugiutil::loc_data loc_data;

    if (vtr::check_file_name_extension(read_rr_graph_name, ".xml") == false) {
        VTR_LOG_WARN(
                "RR graph file '%s' may be in incorrect format. "
                "Expecting .xml format\n",
                read_rr_graph_name);
    }
    try {

        //parse the file
        loc_data = pugiutil::load_xml(doc, read_rr_graph_name);

        auto& device_ctx = g_vpr_ctx.mutable_device();

        auto rr_graph = get_single_child(doc, "rr_graph", loc_data);

        //Check for errors
        Prop = get_attribute(rr_graph, "tool_version", loc_data, OPTIONAL).as_string(nullptr);
        if (Prop != nullptr) {
            if (strcmp(Prop, vtr::VERSION) != 0) {
                VTR_LOG("\n");
                VTR_LOG_WARN(
                        "This architecture version is for VPR %s while your current VPR version is %s compatability issues may arise\n",
                        vtr::VERSION, Prop);
                VTR_LOG("\n");
            }
        }
        Prop = get_attribute(rr_graph, "tool_comment", loc_data, OPTIONAL).as_string(nullptr);
        string correct_string = "Generated from arch file ";
        correct_string += get_arch_file_name();
        if (Prop != nullptr) {
            if (Prop != correct_string) {
                VTR_LOG("\n");
                VTR_LOG_WARN(
                        "This RR graph file is based on %s while your input architecture file is %s compatability issues may arise\n"
                        , get_arch_file_name(), Prop);
                VTR_LOG("\n");
            }
        }

        //Compare with the architecture file to ensure consistency
        next_component = get_single_child(rr_graph, "grid", loc_data);
        verify_grid(next_component, loc_data, grid);

        next_component = get_single_child(rr_graph, "block_types", loc_data);
        verify_blocks(next_component, loc_data);

        next_component = get_single_child(rr_graph, "segments", loc_data);
        verify_segments(next_component, loc_data, segment_inf);

        VTR_LOG("Starting build routing resource graph...\n");

        next_component = get_first_child(rr_graph, "channels", loc_data);
        process_channels(next_component, loc_data);

        /* Decode the graph_type */
        bool is_global_graph = (GRAPH_GLOBAL == graph_type ? true : false);

        /* Global routing uses a single longwire track */
        int max_chan_width = (is_global_graph ? 1 : nodes_per_chan->max);
        VTR_ASSERT(max_chan_width > 0);

        /* Alloc rr nodes and count count nodes */
        next_component = get_single_child(rr_graph, "rr_nodes", loc_data);

        int num_rr_nodes = count_children(next_component, "node", loc_data);

        device_ctx.rr_nodes.resize(num_rr_nodes);
        process_nodes(next_component, loc_data);

        /* Loads edges, switches, and node look up tables*/
        next_component = get_single_child(rr_graph, "switches", loc_data);

        int numSwitches = count_children(next_component, "switch", loc_data);
        *num_rr_switches = numSwitches;
        device_ctx.rr_switch_inf = new t_rr_switch_inf[numSwitches];

        process_switches(next_component, loc_data);

        next_component = get_single_child(rr_graph, "rr_edges", loc_data);
        process_edges(next_component, loc_data, wire_to_rr_ipin_switch, *num_rr_switches);

        //Partition the rr graph edges for efficient access to configurable/non-configurable
        //edge subsets. Must be done after RR switches have been allocated
        partition_rr_graph_edges(device_ctx);

        process_rr_node_indices(grid);

        init_fan_in(device_ctx.rr_nodes, device_ctx.rr_nodes.size());

        //sets the cost index and seg id information
        next_component = get_single_child(rr_graph, "rr_nodes", loc_data);
        set_cost_indices(next_component, loc_data, is_global_graph, num_seg_types);

        alloc_and_load_rr_indexed_data(segment_inf, num_seg_types, device_ctx.rr_node_indices,
                max_chan_width, *wire_to_rr_ipin_switch, base_cost_type);

        process_seg_id(next_component, loc_data);

        if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH)) {
            dump_rr_graph(getEchoFileName(E_ECHO_RR_GRAPH));
        }

        check_rr_graph(graph_type, grid, *num_rr_switches, device_ctx.block_types);

#ifdef USE_MAP_LOOKAHEAD
        compute_router_lookahead(num_seg_types);
#endif

    } catch (XmlError& e) {

        vpr_throw(VPR_ERROR_ROUTE, read_rr_graph_name, e.line(), "%s", e.what());
    }

}

/* Reads in the switch information and adds it to device_ctx.rr_switch_inf as specified*/
void process_switches(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.device();
    pugi::xml_node Switch, SwitchSubnode;

    Switch = get_first_child(parent, "switch", loc_data);

    while (Switch) {
        int iSwitch = get_attribute(Switch, "id", loc_data).as_int();
        auto& rr_switch = device_ctx.rr_switch_inf[iSwitch];
        rr_switch.name = vtr::strdup(get_attribute(Switch, "name", loc_data, OPTIONAL).as_string(nullptr));

        std::string switch_type_str = get_attribute(Switch, "type", loc_data).as_string();
        SwitchType switch_type = SwitchType::INVALID;
        if (switch_type_str == "tristate") {
            switch_type = SwitchType::TRISTATE;
        } else if (switch_type_str == "mux") {
            switch_type = SwitchType::MUX;
        } else if (switch_type_str == "pass_gate") {
            switch_type = SwitchType::PASS_GATE;
        } else if (switch_type_str == "short") {
            switch_type = SwitchType::SHORT;
        } else if (switch_type_str == "buffer") {
            switch_type = SwitchType::BUFFER;
        } else {
            VPR_THROW(VPR_ERROR_ROUTE, "Invalid switch type '%s'\n", switch_type_str.c_str());
        }
        rr_switch.set_type(switch_type);
        SwitchSubnode = get_single_child(Switch, "timing", loc_data, OPTIONAL);
        if (SwitchSubnode) {
            rr_switch.R = get_attribute(SwitchSubnode, "R", loc_data).as_float();
            rr_switch.Cin = get_attribute(SwitchSubnode, "Cin", loc_data).as_float();
            rr_switch.Cout = get_attribute(SwitchSubnode, "Cout", loc_data).as_float();
            rr_switch.Tdel = get_attribute(SwitchSubnode, "Tdel", loc_data).as_float();
        } else {
            rr_switch.R = 0;
            rr_switch.Cin = 0;
            rr_switch.Cout = 0;
            rr_switch.Tdel = 0;
        }
        SwitchSubnode = get_single_child(Switch, "sizing", loc_data);
        rr_switch.mux_trans_size = get_attribute(SwitchSubnode, "mux_trans_size", loc_data).as_float();
        rr_switch.buf_size = get_attribute(SwitchSubnode, "buf_size", loc_data).as_float();

        Switch = Switch.next_sibling(Switch.name());
    }

}

/*Only CHANX and CHANY components have a segment id. This function
 *reads in the segment id of each node*/
void process_seg_id(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node segmentSubnode;
    pugi::xml_node rr_node;
    pugi::xml_attribute attribute;
    int id;

    rr_node = get_first_child(parent, "node", loc_data);

    while (rr_node) {
        id = get_attribute(rr_node, "id", loc_data).as_int();
        auto& node = device_ctx.rr_nodes[id];

        segmentSubnode = get_single_child(rr_node, "segment", loc_data, OPTIONAL);
        if (segmentSubnode) {
            attribute = get_attribute(segmentSubnode, "segment_id", loc_data, OPTIONAL);
            if (attribute) {
                int seg_id = get_attribute(segmentSubnode, "segment_id", loc_data).as_int(0);
                device_ctx.rr_indexed_data[node.cost_index()].seg_index = seg_id;
            } else {
                //-1 for non chanx or chany nodes
                device_ctx.rr_indexed_data[node.cost_index()].seg_index = -1;
            }
        }
        rr_node = rr_node.next_sibling(rr_node.name());
    }
}

/* Node info are processed. Seg_id of nodes are processed separately when rr_index_data is allocated*/
void process_nodes(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node locSubnode, timingSubnode, segmentSubnode;
    pugi::xml_node rr_node;

    rr_node = get_first_child(parent, "node", loc_data);

    while (rr_node) {
        int i = get_attribute(rr_node, "id", loc_data).as_int();
        auto& node = device_ctx.rr_nodes[i];

        const char* node_type = get_attribute(rr_node, "type", loc_data).as_string();
        if (strcmp(node_type, "CHANX") == 0) {
            node.set_type(CHANX);
        } else if (strcmp(node_type, "CHANY") == 0) {
            node.set_type(CHANY);
        } else if (strcmp(node_type, "SOURCE") == 0) {
            node.set_type(SOURCE);
        } else if (strcmp(node_type, "SINK") == 0) {
            node.set_type(SINK);
        } else if (strcmp(node_type, "OPIN") == 0) {
            node.set_type(OPIN);
        } else if (strcmp(node_type, "IPIN") == 0) {
            node.set_type(IPIN);
        } else {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Valid inputs for class types are \"CHANX\", \"CHANY\",\"SOURCE\", \"SINK\",\"OPIN\", and \"IPIN\".");
        }

        if (node.type() == CHANX || node.type() == CHANY) {
            const char* correct_direction = get_attribute(rr_node, "direction", loc_data).as_string();
            if (strcmp(correct_direction, "INC_DIR") == 0) {
                node.set_direction(INC_DIRECTION);
            } else if (strcmp(correct_direction, "DEC_DIR") == 0) {
                node.set_direction(DEC_DIRECTION);
            } else if (strcmp(correct_direction, "BI_DIR") == 0) {
                node.set_direction(BI_DIRECTION);
            } else {
                VTR_ASSERT((strcmp(correct_direction, "NO_DIR") == 0));
                node.set_direction(NO_DIRECTION);
            }
        }

        node.set_capacity(get_attribute(rr_node, "capacity", loc_data).as_float());

        //--------------
        locSubnode = get_single_child(rr_node, "loc", loc_data);

        short x1, x2, y1, y2;
        x1 = get_attribute(locSubnode, "xlow", loc_data).as_float();
        x2 = get_attribute(locSubnode, "xhigh", loc_data).as_float();
        y1 = get_attribute(locSubnode, "ylow", loc_data).as_float();
        y2 = get_attribute(locSubnode, "yhigh", loc_data).as_float();

        if (node.type() == IPIN || node.type() == OPIN) {
            e_side side;
            std::string side_str = get_attribute(locSubnode, "side", loc_data).as_string();
            if (side_str == "LEFT") {
                side = LEFT;
            } else if (side_str == "RIGHT") {
                side = RIGHT;
            } else if (side_str == "TOP") {
                side = TOP;
            } else {
                VTR_ASSERT(side_str == "BOTTOM");
                side = BOTTOM;
            }
            node.set_side(side);
        }

        node.set_coordinates(x1, y1, x2, y2);
        node.set_ptc_num(get_attribute(locSubnode, "ptc", loc_data).as_int());

        //-------
        timingSubnode = get_single_child(rr_node, "timing", loc_data, OPTIONAL);

        float R = 0.;
        float C = 0.;
        if (timingSubnode) {
            R = get_attribute(timingSubnode, "R", loc_data).as_float();
            C = get_attribute(timingSubnode, "C", loc_data).as_float();
        }
        node.set_rc_index(find_create_rr_rc_data(R, C));

        //clear each node edge
        node.set_num_edges(0);

        rr_node = rr_node.next_sibling(rr_node.name());
    }
}

/*Loads the edges information from file into vpr. Nodes and switches must be loaded
 before calling this function*/
void process_edges(pugi::xml_node parent, const pugiutil::loc_data & loc_data,
        int *wire_to_rr_ipin_switch, const int num_rr_switches) {

    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node edges;

    edges = get_first_child(parent, "edge", loc_data);
    int source_node;
    //count the number of edges and store it in a vector
    vector<int> num_edges_for_node;
    num_edges_for_node.resize(device_ctx.rr_nodes.size(), 0);

    while (edges) {
        source_node = get_attribute(edges, "src_node", loc_data).as_int(0);
        num_edges_for_node[source_node]++;
        device_ctx.rr_nodes[source_node].set_num_edges(num_edges_for_node[source_node]);
        edges = edges.next_sibling(edges.name());
    }

    //reset this vector in order to start count for num edges again
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        num_edges_for_node[inode] = 0;
    }

    int sink_node, switch_id;
    edges = get_first_child(parent, "edge", loc_data);
    /*initialize a vector that keeps track of the number of wire to ipin switches
    There should be only one wire to ipin switch. In case there are more, make sure to
    store the most frequent switch */
    std::vector<int> count_for_wire_to_ipin_switches;
    count_for_wire_to_ipin_switches.resize(num_rr_switches, 0);
    //first is index, second is count
    pair <int, int> most_frequent_switch(-1, 0);

    while (edges) {
        source_node = get_attribute(edges, "src_node", loc_data).as_int(0);
        sink_node = get_attribute(edges, "sink_node", loc_data).as_int(0);
        switch_id = get_attribute(edges, "switch_id", loc_data).as_int(0);

        /*Keeps track of the number of the specific type of switch that connects a wire to an ipin
         * use the pair data structure to keep the maximum*/
        if (device_ctx.rr_nodes[source_node].type() == CHANX || device_ctx.rr_nodes[source_node].type() == CHANY) {
            if (device_ctx.rr_nodes[sink_node].type() == IPIN) {
                count_for_wire_to_ipin_switches[switch_id]++;
                if (count_for_wire_to_ipin_switches[switch_id] > most_frequent_switch.second) {
                    most_frequent_switch.first = switch_id;
                    most_frequent_switch.second = count_for_wire_to_ipin_switches[switch_id];
                }
            }
        }
        //set edge in correct rr_node data structure
        device_ctx.rr_nodes[source_node].set_edge_sink_node(num_edges_for_node[source_node], sink_node);
        device_ctx.rr_nodes[source_node].set_edge_switch(num_edges_for_node[source_node], switch_id);
        num_edges_for_node[source_node]++;

        edges = edges.next_sibling(edges.name()); //Next edge
    }
    *wire_to_rr_ipin_switch = most_frequent_switch.first;
    num_edges_for_node.clear();
    count_for_wire_to_ipin_switches.clear();
}

/* All channel info is read in and loaded into device_ctx.chan_width*/
void process_channels(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node channel, channelLists;
    int index;

    channel = get_first_child(parent, "channel", loc_data);

    device_ctx.chan_width.max = get_attribute(channel, "chan_width_max", loc_data).as_uint();
    device_ctx.chan_width.x_min = get_attribute(channel, "x_min", loc_data).as_uint();
    device_ctx.chan_width.y_min = get_attribute(channel, "y_min", loc_data).as_uint();
    device_ctx.chan_width.x_max = get_attribute(channel, "x_max", loc_data).as_uint();
    device_ctx.chan_width.y_max = get_attribute(channel, "y_max", loc_data).as_uint();

    channelLists = get_first_child(parent, "x_list", loc_data);
    while (channelLists) {
        index = get_attribute(channelLists, "index", loc_data).as_int(0);
        device_ctx.chan_width.x_list[index] = get_attribute(channelLists, "info", loc_data).as_float();
        channelLists = channelLists.next_sibling(channelLists.name());
    }
    channelLists = get_first_child(parent, "y_list", loc_data);
    while (channelLists) {
        index = get_attribute(channelLists, "index", loc_data).as_int(0);
        device_ctx.chan_width.y_list[index] = get_attribute(channelLists, "info", loc_data).as_float();
        channelLists = channelLists.next_sibling(channelLists.name());
    }

}

/* Grid was initialized from the architecture file. This function checks
 * if it corresponds to the RR graph. Errors out if it doesn't correspond*/
void verify_grid(pugi::xml_node parent, const pugiutil::loc_data & loc_data, const DeviceGrid& grid) {
    pugi::xml_node grid_node;
    int num_grid_node = count_children(parent, "grid_loc", loc_data);

    grid_node = get_first_child(parent, "grid_loc", loc_data);
    for (int i = 0; i < num_grid_node; i++) {
        int x = get_attribute(grid_node, "x", loc_data).as_float();
        int y = get_attribute(grid_node, "y", loc_data).as_float();

        const t_grid_tile& grid_tile = grid[x][y];

        if (grid_tile.type->index != get_attribute(grid_node, "block_type_id", loc_data).as_int(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's block_type_id at (%d, %d): arch used ID %d, RR graph used ID %d.", x, y,
                     (grid_tile.type->index), get_attribute(grid_node, "block_type_id", loc_data).as_int(0));
        }
        if (grid_tile.width_offset != get_attribute(grid_node, "width_offset", loc_data).as_float(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's width_offset at (%d, %d)", x, y);
        }

        if (grid_tile.height_offset != get_attribute(grid_node, "height_offset", loc_data).as_float(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's height_offset at (%d, %d)", x, y);
        }
        grid_node = grid_node.next_sibling(grid_node.name());
    }
}

static int pin_index_by_num(const t_class &class_inf, int num) {
	for (int index = 0; index < class_inf.num_pins; ++index) {
		if (num == class_inf.pinlist[index]) {
			return index;
		}
	}
	return -1;
}

/* Blocks were initialized from the architecture file. This function checks
 * if it corresponds to the RR graph. Errors out if it doesn't correspond*/
void verify_blocks(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {

    pugi::xml_node Block;
    pugi::xml_node pin_class;
    pugi::xml_node pin;
    Block = get_first_child(parent, "block_type", loc_data);
    auto& device_ctx = g_vpr_ctx.mutable_device();
    while (Block) {

        auto block_info = device_ctx.block_types[get_attribute(Block, "id", loc_data).as_int(0)];

        const char* name = get_attribute(Block, "name", loc_data).as_string(nullptr);

        if (strcmp(block_info.name, name) != 0) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's block name: arch uses name %s, RR graph uses name %s", block_info.name, name);
        }

        if (block_info.width != get_attribute(Block, "width", loc_data).as_float(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's block width");
        }
        if (block_info.height != get_attribute(Block, "height", loc_data).as_float(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's block height");
        }

        pin_class = get_first_child(Block, "pin_class", loc_data, OPTIONAL);

        block_info.num_class = count_children(Block, "pin_class", loc_data, OPTIONAL);

        for (int classNum = 0; classNum < block_info.num_class; classNum++) {
            auto& class_inf = block_info.class_inf[classNum];
            e_pin_type type;

            /*Verify types and pins*/
            const char* typeInfo = get_attribute(pin_class, "type", loc_data).value();
            if (strcmp(typeInfo, "OPEN") == 0) {
                type = OPEN;
            } else if (strcmp(typeInfo, "OUTPUT") == 0) {
                type = DRIVER;
            } else if (strcmp(typeInfo, "INPUT") == 0) {
                type = RECEIVER;
            } else {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                        "Valid inputs for class types are \"OPEN\", \"OUTPUT\", and \"INPUT\".");
                type = OPEN;
            }

            if (class_inf.type != type) {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                        "Architecture file does not match RR graph's block type");
            }

            if (class_inf.num_pins != (int) count_children(pin_class, "pin", loc_data)) {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                        "Incorrect number of pins in %d pin_class in block %s", classNum, block_info.name);
            }

            pin = get_first_child(pin_class, "pin", loc_data, OPTIONAL);
            while (pin) {
                auto num = get_attribute(pin, "ptc", loc_data).as_uint();
                auto index = pin_index_by_num(class_inf, num);

                if (index < 0) {
                    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                            "Architecture file does not match RR graph's block pin list: invalid ptc for pin class");
                }

                if (pin.child_value() != block_type_pin_index_to_name(&block_info, class_inf.pinlist[index])) {
                    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                            "Architecture file does not match RR graph's block pin list");
                }
                pin = pin.next_sibling("pin");
            }
            pin_class = pin_class.next_sibling(pin_class.name());
        }
        Block = Block.next_sibling(Block.name());
    }
}

/* Segments was initialized already. This function checks
 * if it corresponds to the RR graph. Errors out if it doesn't correspond*/
void verify_segments(pugi::xml_node parent, const pugiutil::loc_data & loc_data, const t_segment_inf * segment_inf) {
    pugi::xml_node Segment, subnode;

    Segment = get_first_child(parent, "segment", loc_data);
    while (Segment) {
        int segNum = get_attribute(Segment, "id", loc_data).as_int();
        const char* name = get_attribute(Segment, "name", loc_data).as_string();
        if (strcmp(segment_inf[segNum].name, name) != 0) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's segment name: arch uses %s, RR graph uses %s", segment_inf[segNum].name, name);
        }

        subnode = get_single_child(parent, "timing", loc_data, OPTIONAL);

        if (subnode) {
            if (segment_inf[segNum].Rmetal != get_attribute(subnode, "R_per_meter", loc_data).as_float(0)) {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                        "Architecture file does not match RR graph's segment R_per_meter");
            }
            if (segment_inf[segNum].Cmetal != get_attribute(subnode, "C_per_meter", loc_data).as_float(0)) {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                        "Architecture file does not match RR graph's segment C_per_meter");
            }
        }
        Segment = Segment.next_sibling(Segment.name());
    }
}

/*Allocates and load the rr_node look up table. SINK and SOURCE, IPIN and OPIN
 *share the same look up table. CHANX and CHANY have individual look ups */
void process_rr_node_indices(const DeviceGrid& grid) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    /* Alloc the lookup table */

    auto& indices = device_ctx.rr_node_indices;

    indices.resize(NUM_RR_TYPES);

    /* Alloc the lookup table */
    indices.resize(NUM_RR_TYPES);
    for (t_rr_type rr_type : RR_TYPES) {
        if (rr_type == CHANX) {
            indices[rr_type].resize(grid.height());
            for (size_t y = 0; y < grid.height(); ++y) {
                indices[rr_type][y].resize(grid.width());
                for (size_t x = 0; x < grid.width(); ++x) {
                    indices[rr_type][y][x].resize(NUM_SIDES);
                }
            }
        } else {
            indices[rr_type].resize(grid.width());
            for (size_t x = 0; x < grid.width(); ++x) {
                indices[rr_type][x].resize(grid.height());
                for (size_t y = 0; y < grid.height(); ++y) {
                    indices[rr_type][x][y].resize(NUM_SIDES);
                }
            }
        }
    }

    /*Add the correct node into the vector
     * Note that CHANX and CHANY 's x and y are swapped due to the chan and seg convention.
     * Push back temporary incorrect nodes for CHANX and CHANY to set the length of the vector*/

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        auto& node = device_ctx.rr_nodes[inode];
        if (node.type() == SOURCE || node.type() == SINK) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    if (node.type() == SOURCE) {
                        indices[SOURCE][ix][iy][0].push_back(inode);
                        indices[SINK][ix][iy][0].push_back(OPEN);
                    } else  {
                        VTR_ASSERT(node.type() == SINK);
                        indices[SINK][ix][iy][0].push_back(inode);
                        indices[SOURCE][ix][iy][0].push_back(OPEN);
                    }
                }
            }
        } else if (node.type() == IPIN || node.type() == OPIN) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {

                    if (node.type() == OPIN) {
                        indices[OPIN][ix][iy][node.side()].push_back(inode);
                        indices[IPIN][ix][iy][node.side()].push_back(OPEN);
                    } else {
                        VTR_ASSERT(node.type() == IPIN);
                        indices[IPIN][ix][iy][node.side()].push_back(inode);
                        indices[OPIN][ix][iy][node.side()].push_back(OPEN);
                    }
                }
            }
        } else if (node.type() == CHANX) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    indices[CHANX][iy][ix][0].push_back(inode);
                }
            }
        } else if (node.type() == CHANY) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    indices[CHANY][ix][iy][0].push_back(inode);
                }
            }
        }
    }

    int count;
    /* CHANX and CHANY need to reevaluated with its ptc num as the correct index*/
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        auto& node = device_ctx.rr_nodes[inode];
        if (node.type() == CHANX) {
            for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                    count = node.ptc_num();
                    indices[CHANX][iy][ix][0].at(count) = inode;
                }
            }
        } else if (node.type() == CHANY) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    count = node.ptc_num();
                    indices[CHANY][ix][iy][0].at(count) = inode;
                }
            }
        }
    }
    //Copy the SOURCE/SINK nodes to all offset positions for blocks with width > 1 and/or height > 1
    // This ensures that look-ups on non-root locations will still find the correct SOURCE/SINK
    for (size_t x = 0; x < grid.width(); x++) {
        for (size_t y = 0; y < grid.height(); y++) {
            int width_offset = grid[x][y].width_offset;
            int height_offset = grid[x][y].height_offset;
            if (width_offset != 0 || height_offset != 0) {
                int root_x = x - width_offset;
                int root_y = y - height_offset;

                indices[SOURCE][x][y] = indices[SOURCE][root_x][root_y];
                indices[SINK][x][y] = indices[SINK][root_x][root_y];
            }
        }
    }

    device_ctx.rr_node_indices = indices;
}

/* This function sets the Source pins, sink pins, ipin, and opin
 * to their unique cost index identifier. CHANX and CHANY cost indicies are set after the
 * seg_id is read in from the rr graph*/
void set_cost_indices(pugi::xml_node parent, const pugiutil::loc_data& loc_data,
        const bool is_global_graph, const int num_seg_types) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    //set the cost index in order to load the segment information, rr nodes should be set already
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        if (device_ctx.rr_nodes[inode].type() == SOURCE) {
            device_ctx.rr_nodes[inode].set_cost_index(SOURCE_COST_INDEX);
        } else if (device_ctx.rr_nodes[inode].type() == SINK) {
            device_ctx.rr_nodes[inode].set_cost_index(SINK_COST_INDEX);
        } else if (device_ctx.rr_nodes[inode].type() == IPIN) {
            device_ctx.rr_nodes[inode].set_cost_index(IPIN_COST_INDEX);
        } else if (device_ctx.rr_nodes[inode].type() == OPIN) {
            device_ctx.rr_nodes[inode].set_cost_index(OPIN_COST_INDEX);
        }
    }

    pugi::xml_node segmentSubnode;
    pugi::xml_node rr_node;
    pugi::xml_attribute attribute;

    /*Go through each rr_node and use the segment ids to set CHANX and CHANY cost index*/
    rr_node = get_first_child(parent, "node", loc_data);

    for (size_t i = 0; i < device_ctx.rr_nodes.size(); i++) {
        auto& node = device_ctx.rr_nodes[i];

        /*CHANX and CHANY cost index is dependent on the segment id*/

        segmentSubnode = get_single_child(rr_node, "segment", loc_data, OPTIONAL);
        if (segmentSubnode) {
            attribute = get_attribute(segmentSubnode, "segment_id", loc_data, OPTIONAL);
            if (attribute) {
                int seg_id = get_attribute(segmentSubnode, "segment_id", loc_data).as_int(0);
                if (is_global_graph) {
                    node.set_cost_index(0);
                } else if (device_ctx.rr_nodes[i].type() == CHANX) {
                    node.set_cost_index(CHANX_COST_INDEX_START + seg_id);
                } else if (device_ctx.rr_nodes[i].type() == CHANY) {
                    node.set_cost_index(CHANX_COST_INDEX_START + num_seg_types + seg_id);

                }
            }
        }
        rr_node = rr_node.next_sibling(rr_node.name());
    }
}
