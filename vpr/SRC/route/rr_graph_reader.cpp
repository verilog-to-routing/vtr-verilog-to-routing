#include <string.h>
#include <string>
#include <algorithm>

#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "read_xml_arch_file.h"
#include "read_xml_util.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "globals.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_indexed_data.h"
#include "vtr_version.h"
#include <sstream>
#include "dump_rr_structs.h"

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_matrix.h"
#include "vtr_math.h"
#include "vtr_log.h"
#include "rr_graph_writer.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"
#include "check_rr_graph.h"

#include "rr_graph_reader.h"


using namespace std;
using namespace pugiutil;

/*********************** Subroutines local to this module *******************/
void process_switches(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void verify_segments(pugi::xml_node parent, const pugiutil::loc_data & loc_data, const t_segment_inf *segment_inf);
void verify_blocks(pugi::xml_node parent, const pugiutil::loc_data & loc_data);
void process_blocks(pugi::xml_node parent, const pugiutil::loc_data & loc_data);
void verify_grid(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_nodes(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_edges(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_channels(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_rr_node_indices(const int L_nx, const int L_ny);
void process_seg_id(pugi::xml_node parent, const pugiutil::loc_data & loc_data);


/************************ Subroutine definitions ****************************/

/*loads the given RR_graph file into the appropriate data structures
 * as specified by read_rr_graph_name*/
void load_rr_file(const t_graph_type graph_type,
        const int L_nx,
        const int L_ny,
        t_chan_width *nodes_per_chan,
        const int num_seg_types,
        const t_segment_inf * segment_inf,
        const enum e_base_cost_type base_cost_type,
        int *wire_to_rr_ipin_switch,
        int *num_rr_switches, 
        const char* write_rr_graph_name,
        const char* read_rr_graph_name) {

    const char *Prop;
    pugi::xml_node next_component;

    pugi::xml_document doc;
    pugiutil::loc_data loc_data;


    if (vtr::check_file_name_extension(read_rr_graph_name, ".xml") == false) {
        vtr::printf_warning(__FILE__, __LINE__,
                "Architecture file '%s' may be in incorrect format. "
                "Expecting .xml format for RR graph files.\n",
                read_rr_graph_name);
    }
    try {
        //parse the file
        loc_data = pugiutil::load_xml(doc, read_rr_graph_name);

        auto& device_ctx = g_vpr_ctx.mutable_device();

        auto rr_graph = get_single_child(doc, "rr_graph", loc_data);

        //Check for errors
        Prop = get_attribute(rr_graph, "tool_version", loc_data, OPTIONAL).as_string(NULL);
        if (Prop != NULL) {
            if (strcmp(Prop, vtr::VERSION) != 0) {
                cout << endl;
                vtr::printf_warning(__FILE__, __LINE__,
                        "This architecture version is for VPR %s while your current VPR version is %s compatability issues may arise\n",
                        vtr::VERSION, Prop);
                cout << endl;
            }
        }
        Prop = get_attribute(rr_graph, "tool_comment", loc_data, OPTIONAL).as_string(NULL);
        string correct_string = "Generated from arch file ";
        correct_string += get_arch_file_name();
        if (Prop != NULL) {
            if (Prop != correct_string) {
                cout << endl;
                vtr::printf_warning(__FILE__, __LINE__,
                        "This RR graph file is based on %s while your input architecture file is %s compatability issues may arise\n"
                        , get_arch_file_name(), Prop);
                cout << endl;
            }
        }

        //Compare with the architecture file to ensure consistency
        next_component = get_single_child(rr_graph, "grid", loc_data);
        verify_grid(next_component, loc_data);

        next_component = get_single_child(rr_graph, "block_types", loc_data);
        verify_blocks(next_component, loc_data);

        next_component = get_single_child(rr_graph, "segments", loc_data);
        verify_segments(next_component, loc_data, segment_inf);

        vtr::printf_info("Starting build routing resource graph...\n");
        clock_t begin = clock();

        next_component = get_first_child(rr_graph, "channels", loc_data);
        process_channels(next_component, loc_data);


        /* Decode the graph_type */
        bool is_global_graph = (GRAPH_GLOBAL == graph_type ? true : false);

        /* Global routing uses a single longwire track */
        int max_chan_width = (is_global_graph ? 1 : nodes_per_chan->max);
        VTR_ASSERT(max_chan_width > 0);

        /* Alloc  rr nodes and count count nodes */
        next_component = get_single_child(rr_graph, "rr_nodes", loc_data);

        device_ctx.num_rr_nodes = count_children(next_component, "node", loc_data);

        device_ctx.rr_nodes = new t_rr_node[device_ctx.num_rr_nodes];
        process_nodes(next_component, loc_data);

        /* Loads edges, switches, and node look up tables*/
        next_component = get_single_child(rr_graph, "rr_edges", loc_data);
        process_edges(next_component, loc_data);

        next_component = get_single_child(rr_graph, "switches", loc_data);

        int numSwitches = count_children(next_component, "switch", loc_data);
        *num_rr_switches = numSwitches;
        device_ctx.rr_switch_inf = new t_rr_switch_inf[numSwitches];

        process_switches(next_component, loc_data);

        process_rr_node_indices(L_nx, L_ny);

        init_fan_in(L_nx, L_ny, device_ctx.rr_nodes, device_ctx.rr_node_indices, device_ctx.grid, device_ctx.num_rr_nodes);

        alloc_and_load_rr_indexed_data(segment_inf, num_seg_types, device_ctx.rr_node_indices,
                max_chan_width, *wire_to_rr_ipin_switch, base_cost_type);

        //seg_id is read in from the rr_graph file 
        next_component = get_single_child(rr_graph, "rr_nodes", loc_data);
        process_seg_id(next_component, loc_data);

        //Load all the external routing data structures
        alloc_net_rr_terminals();
        load_net_rr_terminals(device_ctx.rr_node_indices);
        alloc_and_load_rr_clb_source(device_ctx.rr_node_indices);


#ifdef USE_MAP_LOOKAHEAD
        compute_router_lookahead(num_seg_types);
#endif
        auto& route_ctx = g_vpr_ctx.mutable_routing();

        route_ctx.rr_node_state = new t_rr_node_state[device_ctx.num_rr_nodes];


        float elapsed_time = (float) (clock() - begin) / CLOCKS_PER_SEC;
        vtr::printf_info("Build routing resource graph took %g seconds\n", elapsed_time);

        //Write out rr graph file if needed
        if (write_rr_graph_name) {
            write_rr_graph(write_rr_graph_name, segment_inf, num_seg_types);
        }
//        dump_rr_structs("structs2.txt");
//        
//        dump_rr_graph("rr_graph1.echo");

    } catch (XmlError& e) {

        archfpga_throw(read_rr_graph_name, e.line(), "%s", e.what());
    }

}

/* Reads in the switch information and adds it to device_ctx.rr_switch_inf as specified*/
void process_switches(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.device();
    pugi::xml_node Switch, SwitchSubnode;

    Switch = get_first_child(parent, "switch", loc_data);
    int numSwitches = count_children(parent, "switch", loc_data);

    for (int iSwitch = 0; iSwitch < numSwitches; iSwitch++) {
        auto& rr_switch = device_ctx.rr_switch_inf[iSwitch];
        rr_switch.name = get_attribute(Switch, "name", loc_data, OPTIONAL).as_string(NULL);
        rr_switch.buffered = get_attribute(Switch, "buffered", loc_data).as_bool();
        SwitchSubnode = get_single_child(Switch, "timing", loc_data, OPTIONAL);
        if (SwitchSubnode) {
            rr_switch.R = get_attribute(SwitchSubnode, "R", loc_data).as_float();
            rr_switch.Cin = get_attribute(SwitchSubnode, "Cin", loc_data).as_float();
            rr_switch.Cout = get_attribute(SwitchSubnode, "Cout", loc_data).as_float();
            rr_switch.Tdel = get_attribute(SwitchSubnode, "Tdel", loc_data).as_float();
        }
        SwitchSubnode = get_single_child(Switch, "sizing", loc_data);
        rr_switch.mux_trans_size = get_attribute(SwitchSubnode, "mux_trans_size", loc_data).as_float();
        rr_switch.buf_size = get_attribute(SwitchSubnode, "buf_size", loc_data).as_float();

        Switch = Switch.next_sibling(Switch.name());
    }

}

/*Reads in the segment id of each node*/
void process_seg_id(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node segmentSubnode;
    pugi::xml_node rr_node;
    pugi::xml_attribute attribute;

    rr_node = get_first_child(parent, "node", loc_data);

    for (int i = 0; i < device_ctx.num_rr_nodes; i++) {
        auto& node = device_ctx.rr_nodes[i];

        segmentSubnode = get_single_child(rr_node, "segment", loc_data);
        attribute = get_attribute(segmentSubnode, "segment_id", loc_data, OPTIONAL);
        if (attribute) {
            device_ctx.rr_indexed_data[node.cost_index()].seg_index = get_attribute(segmentSubnode, "segment_id", loc_data).as_int(0);
        } else {
            device_ctx.rr_indexed_data[node.cost_index()].seg_index = -1;
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

    int numNode = count_children(parent, "node", loc_data);
    e_direction direction;
    for (int i = 0; i < numNode; i++) {

        auto& node = device_ctx.rr_nodes[i];

        const char* node_type = get_attribute(rr_node, "type", loc_data).as_string();
        t_rr_type newType;
        if (strcmp(node_type, "CHANX") == 0) {
            newType = CHANX;
            node.set_type(newType);
        } else if (strcmp(node_type, "CHANY") == 0) {
            newType = CHANY;
            node.set_type(newType);
        } else if (strcmp(node_type, "SOURCE") == 0) {
            newType = SOURCE;
            node.set_type(newType);
        } else if (strcmp(node_type, "SINK") == 0) {
            newType = SINK;
            node.set_type(newType);
        } else if (strcmp(node_type, "OPIN") == 0) {
            newType = OPIN;
            node.set_type(newType);
        } else if (strcmp(node_type, "IPIN") == 0) {
            newType = IPIN;
            node.set_type(newType);
        }

        const char* correct_direction = get_attribute(rr_node, "direction", loc_data).as_string(0);
        if (strcmp(correct_direction, "INC") == 0) {
            direction = INC_DIRECTION;
            node.set_direction(direction);
        } else if (strcmp(correct_direction, "DEC") == 0) {

            direction = DEC_DIRECTION;
            node.set_direction(direction);
        } else if (strcmp(correct_direction, "BI") == 0) {

            direction = BI_DIRECTION;
            node.set_direction(direction);
        } else if (strcmp(correct_direction, "NONE") == 0) {

            direction = NONE;
            node.set_direction(direction);
        }

        node.set_capacity(get_attribute(rr_node, "capacity", loc_data).as_float());

        //--------------
        locSubnode = get_single_child(rr_node, "loc", loc_data);

        short x1, x2, y1, y2;
        x1 = get_attribute(locSubnode, "xlow", loc_data).as_float();
        x2 = get_attribute(locSubnode, "xhigh", loc_data).as_float();
        y1 = get_attribute(locSubnode, "ylow", loc_data).as_float();
        y2 = get_attribute(locSubnode, "yhigh", loc_data).as_float();

        node.set_coordinates(x1, y1, x2, y2);
        node.set_ptc_num(get_attribute(locSubnode, "ptc", loc_data).as_float());

        //-------
        timingSubnode = get_single_child(rr_node, "timing", loc_data, OPTIONAL);

        if (timingSubnode) {
            node.set_R(get_attribute(timingSubnode, "R", loc_data).as_float());
            node.set_C(get_attribute(timingSubnode, "C", loc_data).as_float());
        }
        //-------

        segmentSubnode = get_single_child(rr_node, "segment", loc_data);
        node.set_cost_index(get_attribute(segmentSubnode, "cost_index", loc_data).as_int());
        node.set_num_edges(0);

        rr_node = rr_node.next_sibling(rr_node.name());
    }
}

void process_edges(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node edges;

    edges = get_first_child(parent, "edge", loc_data);
    int counter = 0;
    int source_node;
    int previous_source = -1;

    //count the number of edges
    while (edges) {
        source_node = get_attribute(edges, "src_node", loc_data).as_int(0);
        if (source_node != previous_source) {
            if (previous_source != -1) {
                device_ctx.rr_nodes[previous_source].set_num_edges(counter);
            }
            counter = 1;
        } else {
            counter++;
        }
        previous_source = source_node;
        edges = edges.next_sibling(edges.name());
    }
    device_ctx.rr_nodes[previous_source].set_num_edges(counter);

    int sink_node, switch_id;
    edges = get_first_child(parent, "edge", loc_data);
    previous_source = -1;
    counter = 0;
    //set edge in correct rr_node data structure
    while (edges) {
        source_node = get_attribute(edges, "src_node", loc_data).as_int(0);
        sink_node = get_attribute(edges, "sink_node", loc_data).as_int(0);
        switch_id = get_attribute(edges, "switch_id", loc_data).as_int(0);
        if (previous_source != source_node) {
            counter = 0;
        } else {
            counter++;
        }
        device_ctx.rr_nodes[source_node].set_edge_sink_node(counter, sink_node);
        device_ctx.rr_nodes[source_node].set_edge_switch(counter, switch_id);
        edges = edges.next_sibling(edges.name());
        previous_source = source_node;
    }
}

/* All channel info is read in and loaded into device_ctx.chan_width*/
void process_channels(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node channel, channelLists;
    int index;

    channel = get_first_child(parent, "channel", loc_data);

    device_ctx.chan_width.max = get_attribute(channel, "chan_width_max", loc_data).as_float();
    device_ctx.chan_width.x_min = get_attribute(channel, "x_min", loc_data).as_float();
    device_ctx.chan_width.y_min = get_attribute(channel, "y_min", loc_data).as_float();
    device_ctx.chan_width.x_max = get_attribute(channel, "x_max", loc_data).as_float();
    device_ctx.chan_width.y_max = get_attribute(channel, "y_max", loc_data).as_float();

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
void verify_grid(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node Grid;
    int numGrid = count_children(parent, "grid_loc", loc_data);

    Grid = get_first_child(parent, "grid_loc", loc_data);
    for (int i = 0; i < numGrid; i++) {
        int x = get_attribute(Grid, "x", loc_data).as_float();
        int y = get_attribute(Grid, "y", loc_data).as_float();

        t_grid_tile grid_tile = device_ctx.grid[x][y];


        if (grid_tile.type->index != get_attribute(Grid, "block_type_id", loc_data).as_float(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's block_type_id");
        }
        if (grid_tile.width_offset != get_attribute(Grid, "width_offset", loc_data).as_float(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's width_offset");
        }

        if (grid_tile.height_offset != get_attribute(Grid, "height_offset", loc_data).as_float(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's height_offset");
        }
        Grid = Grid.next_sibling(Grid.name());
    }
}

/* Blocks were initialized from the architecture file. This function checks 
 * if it corresponds to the RR graph. Errors out if it doesn't correspond*/
void verify_blocks(pugi::xml_node parent, const pugiutil::loc_data & loc_data) {

    pugi::xml_node Block;
    pugi::xml_node pin_class;
    Block = get_first_child(parent, "block_type", loc_data);
    int numBlocks = count_children(parent, "block_type", loc_data);
    auto& device_ctx = g_vpr_ctx.mutable_device();
    for (int i = 0; i < numBlocks; ++i) {

        auto block_info = device_ctx.block_types[i];
        if (block_info.index != get_attribute(Block, "id", loc_data).as_int(0)) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's block id");
        }

        const char* name;
        if (strcmp(get_attribute(Block, "name", loc_data).as_string(NULL), "EMPTY") == 0) {
            name = "<EMPTY>";
        } else {
            name = get_attribute(Block, "name", loc_data).as_string(NULL);
        }
        if (strcmp(block_info.name, name) != 0) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's block name");
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

            const char* typeInfo = get_attribute(pin_class, "type", loc_data).value();
            if (strcmp(typeInfo, "OPEN") == 0) {
                type = OPEN;
            } else if (strcmp(typeInfo, "OUTPUT") == 0) {
                type = DRIVER;
            } else if (strcmp(typeInfo, "INPUT") == 0) {
                type = RECEIVER;
            } else {
                type = OPEN;
            }

            if (class_inf.type != type) {
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                        "Architecture file does not match RR graph's block type");
            }

            //Go through each pin of the pin list
            string temp = vtr::strdup(pin_class.child_value());
            stringstream longString(temp);

            int eachInt;
            longString.str(temp);
            int pinNum = 0;
            longString >> eachInt;
            while (longString) {
                if (class_inf.pinlist[pinNum] != eachInt) {
                    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                            "Architecture file does not match RR graph's block pin list");
                }
                pinNum++;
                longString>>eachInt;
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
    int numSegment = count_children(parent, "segment", loc_data);

    Segment = get_first_child(parent, "segment", loc_data);
    for (int segNum = 0; segNum < numSegment; segNum++) {
        const char* name = get_attribute(Segment, "name", loc_data).as_string();
        if (strcmp(segment_inf[segNum].name, name) != 0) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                    "Architecture file does not match RR graph's segment name");
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
void process_rr_node_indices(const int L_nx, const int L_ny) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    /* Alloc the lookup table */

    auto& indices = device_ctx.rr_node_indices;

    indices = (vtr::t_ivec ***) vtr::calloc(NUM_RR_TYPES, sizeof (vtr::t_ivec **));
    for (int itype = 0; itype < NUM_RR_TYPES; ++itype) {
        indices[itype] = (vtr::t_ivec **) vtr::calloc((L_nx + 2), sizeof (vtr::t_ivec *));
        for (int ilength = 0; ilength <= (L_nx + 1); ++ilength) {
            indices[itype][ilength] = (vtr::t_ivec *) vtr::calloc((L_ny + 2), sizeof (vtr::t_ivec));
        }
    }

    /*Counts the length of list
     * CHANX and CHANY 's x and y are swapped due to the chan and seg convention*/
    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
        auto& node = device_ctx.rr_nodes[inode];
        if (node.type() == SOURCE || node.type() == SINK) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    indices[SINK][ix][iy].nelem++;
                }
            }
        } else if (node.type() == IPIN || node.type() == OPIN) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    indices[IPIN][ix][iy].nelem++;
                }
            }
        } else if (node.type() == CHANX) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    indices[node.type()][iy][ix].nelem++;
                }
            }
        } else if (node.type() == CHANY) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    indices[node.type()][ix][iy].nelem++;
                }
            }
        }
    }

    /* Allocates the memory for list */
    for (int itype = 0; itype < NUM_RR_TYPES; itype++) {
        for (int x = 0; x <= (L_nx + 1); x++) {
            for (int y = 0; y <= (L_ny + 1); y++) {
                if (indices[itype][x][y].nelem != 0) {
                    indices[itype][x][y].list = (int*) vtr::calloc(indices[itype][x][y].nelem, sizeof (int));
                    indices[itype][x][y].nelem = 0;
                }
            }
        }
    }
    int count;
    /* Assigns the right node number for each look up list*/
    for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
        auto& node = device_ctx.rr_nodes[inode];

        if (node.type() == SOURCE || node.type() == SINK) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    count = indices[SINK][ix][iy].nelem;
                    indices[SINK][ix][iy].list[count] = inode;
                    indices[SINK][ix][iy].nelem++;
                }
            }
        } else if (node.type() == IPIN || node.type() == OPIN) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    count = indices[IPIN][ix][iy].nelem;
                    indices[IPIN][ix][iy].list[count] = inode;
                    indices[IPIN][ix][iy].nelem++;
                }
            }
        }
        if (node.type() == CHANX) {
            for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                    count = node.ptc_num();
                    indices[CHANX][iy][ix].list[count] = inode;
                    indices[CHANX][iy][ix].nelem++;
                }
            }
        } else if (node.type() == CHANY) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    count = node.ptc_num();
                    indices[CHANY][ix][iy].list[count] = inode;
                    indices[CHANY][ix][iy].nelem++;
                }
            }
        }
    }
    indices[SOURCE] = indices[SINK];
    indices[OPIN] = indices[IPIN];
    device_ctx.rr_node_indices = indices;
}
