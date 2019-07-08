/*This function loads in a routing resource graph written in xml format
 * into vpr when the option --read_rr_graph <file name> is specified.
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
#include "rr_metadata.h"
#include "rr_graph_indexed_data.h"
#include "rr_graph_writer.h"
#include "check_rr_graph.h"
#include "echo_files.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"

#include "rr_graph_reader.h"

/*********************** Subroutines local to this module *******************/
void process_switches(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void verify_segments(pugi::xml_node parent, const pugiutil::loc_data& loc_data, const std::vector<t_segment_inf>& segment_inf);
void verify_blocks(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_blocks(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void verify_grid(pugi::xml_node parent, const pugiutil::loc_data& loc_data, const DeviceGrid& grid);
void process_nodes(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_connection_boxes(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_edges(pugi::xml_node parent, const pugiutil::loc_data& loc_data, int* wire_to_rr_ipin_switch, const int num_rr_switches);
void process_channels(t_chan_width& chan_width, const DeviceGrid& grid, pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void process_rr_node_indices(const DeviceGrid& grid);
void process_seg_id(pugi::xml_node parent, const pugiutil::loc_data& loc_data);
void set_cost_indices(pugi::xml_node parent, const pugiutil::loc_data& loc_data, const bool is_global_graph, const int num_seg_types);

/************************ Subroutine definitions ****************************/

/*loads the given RR_graph file into the appropriate data structures
 * as specified by read_rr_graph_name. Set up correct routing data
 * structures as well*/
void load_rr_file(const t_graph_type graph_type,
                  const DeviceGrid& grid,
                  const std::vector<t_segment_inf>& segment_inf,
                  const enum e_base_cost_type base_cost_type,
                  int* wire_to_rr_ipin_switch,
                  const char* read_rr_graph_name) {
    vtr::ScopedStartFinishTimer timer("Loading routing resource graph");

    const char* Prop;
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
        Prop = get_attribute(rr_graph, "tool_version", loc_data, pugiutil::OPTIONAL).as_string(nullptr);
        if (Prop != nullptr) {
            if (strcmp(Prop, vtr::VERSION) != 0) {
                VTR_LOG("\n");
                VTR_LOG_WARN("This architecture version is for VPR %s while your current VPR version is %s compatability issues may arise\n",
                             vtr::VERSION, Prop);
                VTR_LOG("\n");
            }
        }
        Prop = get_attribute(rr_graph, "tool_comment", loc_data, pugiutil::OPTIONAL).as_string(nullptr);
        std::string correct_string = "Generated from arch file ";
        correct_string += get_arch_file_name();
        if (Prop != nullptr) {
            if (Prop != correct_string) {
                VTR_LOG("\n");
                VTR_LOG_WARN("This RR graph file is based on %s while your input architecture file is %s compatability issues may arise\n",
                             get_arch_file_name(), Prop);
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
        t_chan_width nodes_per_chan;
        process_channels(nodes_per_chan, grid, next_component, loc_data);

        next_component = get_first_child(rr_graph, "connection_boxes", loc_data, OPTIONAL);
        if (next_component != nullptr) {
            process_connection_boxes(next_component, loc_data);
        } else {
            device_ctx.connection_boxes.clear();
        }

        /* Decode the graph_type */
        bool is_global_graph = (GRAPH_GLOBAL == graph_type ? true : false);

        /* Global routing uses a single longwire track */
        int max_chan_width = (is_global_graph ? 1 : nodes_per_chan.max);
        VTR_ASSERT(max_chan_width > 0);

        /* Alloc rr nodes and count count nodes */
        next_component = get_single_child(rr_graph, "rr_nodes", loc_data);

        int num_rr_nodes = count_children(next_component, "node", loc_data);

        device_ctx.rr_nodes.resize(num_rr_nodes);
        device_ctx.connection_boxes.resize_nodes(num_rr_nodes);
        process_nodes(next_component, loc_data);

        /* Loads edges, switches, and node look up tables*/
        next_component = get_single_child(rr_graph, "switches", loc_data);

        int numSwitches = count_children(next_component, "switch", loc_data);
        device_ctx.rr_switch_inf.resize(numSwitches);

        process_switches(next_component, loc_data);

        next_component = get_single_child(rr_graph, "rr_edges", loc_data);
        process_edges(next_component, loc_data, wire_to_rr_ipin_switch, numSwitches);

        //Partition the rr graph edges for efficient access to configurable/non-configurable
        //edge subsets. Must be done after RR switches have been allocated
        partition_rr_graph_edges(device_ctx);

        process_rr_node_indices(grid);

        init_fan_in(device_ctx.rr_nodes, device_ctx.rr_nodes.size());

        //sets the cost index and seg id information
        next_component = get_single_child(rr_graph, "rr_nodes", loc_data);
        set_cost_indices(next_component, loc_data, is_global_graph, segment_inf.size());

        alloc_and_load_rr_indexed_data(segment_inf, device_ctx.rr_node_indices,
                                       max_chan_width, *wire_to_rr_ipin_switch, base_cost_type);

        process_seg_id(next_component, loc_data);

        device_ctx.chan_width = nodes_per_chan;
        device_ctx.read_rr_graph_filename = std::string(read_rr_graph_name);

        check_rr_graph(graph_type, grid, device_ctx.physical_tile_types);
        device_ctx.connection_boxes.create_sink_back_ref();

    } catch (pugiutil::XmlError& e) {
        vpr_throw(VPR_ERROR_ROUTE, read_rr_graph_name, e.line(), "%s", e.what());
    }
}

/* Reads in the switch information and adds it to device_ctx.rr_switch_inf as specified*/
void process_switches(pugi::xml_node parent, const pugiutil::loc_data& loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node Switch, SwitchSubnode;

    Switch = get_first_child(parent, "switch", loc_data);

    while (Switch) {
        int iSwitch = get_attribute(Switch, "id", loc_data).as_int();
        auto& rr_switch = device_ctx.rr_switch_inf[iSwitch];
        const char* name = get_attribute(Switch, "name", loc_data, pugiutil::OPTIONAL).as_string(nullptr);
        bool found_arch_name = false;
        if (name != nullptr) {
            for (int i = 0; i < device_ctx.num_arch_switches; ++i) {
                if (strcmp(name, device_ctx.arch_switch_inf[i].name) == 0) {
                    name = device_ctx.arch_switch_inf[i].name;
                    found_arch_name = true;
                    break;
                }
            }
        }

        if (name != nullptr && !found_arch_name) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Switch name '%s' not found in architecture\n", name);
        }

        rr_switch.name = name;

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
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Invalid switch type '%s'\n", switch_type_str.c_str());
        }
        rr_switch.set_type(switch_type);
        SwitchSubnode = get_single_child(Switch, "timing", loc_data, pugiutil::OPTIONAL);
        if (SwitchSubnode) {
            rr_switch.R = get_attribute(SwitchSubnode, "R", loc_data).as_float();
            rr_switch.Cin = get_attribute(SwitchSubnode, "Cin", loc_data).as_float();
            rr_switch.Cout = get_attribute(SwitchSubnode, "Cout", loc_data).as_float();
            rr_switch.Cinternal = get_attribute(SwitchSubnode, "Cinternal", loc_data).as_float();
            rr_switch.Tdel = get_attribute(SwitchSubnode, "Tdel", loc_data).as_float();
        } else {
            rr_switch.R = 0;
            rr_switch.Cin = 0;
            rr_switch.Cout = 0;
            rr_switch.Cinternal = 0;
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
void process_seg_id(pugi::xml_node parent, const pugiutil::loc_data& loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node segmentSubnode;
    pugi::xml_node rr_node;
    pugi::xml_attribute attribute;
    int id;

    rr_node = get_first_child(parent, "node", loc_data);

    while (rr_node) {
        id = get_attribute(rr_node, "id", loc_data).as_int();
        auto& node = device_ctx.rr_nodes[id];

        segmentSubnode = get_single_child(rr_node, "segment", loc_data, pugiutil::OPTIONAL);
        if (segmentSubnode) {
            attribute = get_attribute(segmentSubnode, "segment_id", loc_data, pugiutil::OPTIONAL);
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
void process_nodes(pugi::xml_node parent, const pugiutil::loc_data& loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node locSubnode, timingSubnode, segmentSubnode;
    pugi::xml_node rr_node;

    rr_node = get_first_child(parent, "node", loc_data);

    while (rr_node) {
        int inode = get_attribute(rr_node, "id", loc_data).as_int();
        auto& node = device_ctx.rr_nodes[inode];

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

            pugi::xml_node connection_boxSubnode = get_single_child(rr_node, "connection_box", loc_data, OPTIONAL);
            if (connection_boxSubnode) {
                int x = get_attribute(connection_boxSubnode, "x", loc_data).as_int();
                int y = get_attribute(connection_boxSubnode, "y", loc_data).as_int();
                int id = get_attribute(connection_boxSubnode, "id", loc_data).as_int();

                device_ctx.connection_boxes.add_connection_box(inode,
                                                               ConnectionBoxId(id),
                                                               std::make_pair(x, y));
            }

        } else {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
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

        pugi::xml_node connection_boxSubnode = get_single_child(rr_node, "canonical_loc", loc_data, OPTIONAL);
        if (connection_boxSubnode) {
            int x = get_attribute(connection_boxSubnode, "x", loc_data).as_int();
            int y = get_attribute(connection_boxSubnode, "y", loc_data).as_int();

            device_ctx.connection_boxes.add_canonical_loc(inode,
                                                          std::make_pair(x, y));
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
        timingSubnode = get_single_child(rr_node, "timing", loc_data, pugiutil::OPTIONAL);

        float R = 0.;
        float C = 0.;
        if (timingSubnode) {
            R = get_attribute(timingSubnode, "R", loc_data).as_float();
            C = get_attribute(timingSubnode, "C", loc_data).as_float();
        }
        node.set_rc_index(find_create_rr_rc_data(R, C));

        //clear each node edge
        node.set_num_edges(0);

        //  <metadata>
        //    <meta name='grid_prefix' >CLBLL_L_</meta>
        //  </metadata>
        auto metadata = get_single_child(rr_node, "metadata", loc_data, pugiutil::OPTIONAL);
        if (metadata) {
            auto rr_node_meta = get_first_child(metadata, "meta", loc_data);
            while (rr_node_meta) {
                auto key = get_attribute(rr_node_meta, "name", loc_data).as_string();

                vpr::add_rr_node_metadata(inode, key, rr_node_meta.child_value());

                rr_node_meta = rr_node_meta.next_sibling(rr_node_meta.name());
            }
        }

        rr_node = rr_node.next_sibling(rr_node.name());
    }
}

/*Loads the edges information from file into vpr. Nodes and switches must be loaded
 * before calling this function*/
void process_edges(pugi::xml_node parent, const pugiutil::loc_data& loc_data, int* wire_to_rr_ipin_switch, const int num_rr_switches) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    pugi::xml_node edges;

    edges = get_first_child(parent, "edge", loc_data);
    //count the number of edges and store it in a vector
    std::vector<size_t> num_edges_for_node;
    num_edges_for_node.resize(device_ctx.rr_nodes.size(), 0);

    while (edges) {
        size_t source_node = get_attribute(edges, "src_node", loc_data).as_uint();
        if (source_node >= device_ctx.rr_nodes.size()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "source_node %d is larger than rr_nodes.size() %d",
                            source_node, device_ctx.rr_nodes.size());
        }

        num_edges_for_node[source_node]++;
        edges = edges.next_sibling(edges.name());
    }

    //reset this vector in order to start count for num edges again
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        if (num_edges_for_node[inode] > std::numeric_limits<t_edge_size>::max()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "source node %d edge count %d is too high",
                            inode, num_edges_for_node[inode]);
        }
        device_ctx.rr_nodes[inode].set_num_edges(num_edges_for_node[inode]);
        num_edges_for_node[inode] = 0;
    }

    edges = get_first_child(parent, "edge", loc_data);
    /*initialize a vector that keeps track of the number of wire to ipin switches
     * There should be only one wire to ipin switch. In case there are more, make sure to
     * store the most frequent switch */
    std::vector<int> count_for_wire_to_ipin_switches;
    count_for_wire_to_ipin_switches.resize(num_rr_switches, 0);
    //first is index, second is count
    std::pair<int, int> most_frequent_switch(-1, 0);

    while (edges) {
        size_t source_node = get_attribute(edges, "src_node", loc_data).as_uint();
        size_t sink_node = get_attribute(edges, "sink_node", loc_data).as_uint();
        int switch_id = get_attribute(edges, "switch_id", loc_data).as_int();

        if (sink_node >= device_ctx.rr_nodes.size()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "sink_node %d is larger than rr_nodes.size() %d",
                            sink_node, device_ctx.rr_nodes.size());
        }

        if (switch_id >= num_rr_switches) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "switch_id %d is larger than num_rr_switches %d",
                            switch_id, num_rr_switches);
        }

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

        // Read the metadata for the edge
        auto metadata = get_single_child(edges, "metadata", loc_data, pugiutil::OPTIONAL);
        if (metadata) {
            auto edges_meta = get_first_child(metadata, "meta", loc_data);
            while (edges_meta) {
                auto key = get_attribute(edges_meta, "name", loc_data).as_string();

                vpr::add_rr_edge_metadata(source_node, sink_node, switch_id,
                                          key, edges_meta.child_value());

                edges_meta = edges_meta.next_sibling(edges_meta.name());
            }
        }
        num_edges_for_node[source_node]++;

        edges = edges.next_sibling(edges.name()); //Next edge
    }
    *wire_to_rr_ipin_switch = most_frequent_switch.first;
    num_edges_for_node.clear();
    count_for_wire_to_ipin_switches.clear();
}

/* All channel info is read in and loaded into device_ctx.chan_width*/
void process_channels(t_chan_width& chan_width, const DeviceGrid& grid, pugi::xml_node parent, const pugiutil::loc_data& loc_data) {
    pugi::xml_node channel, channelLists;

    channel = get_first_child(parent, "channel", loc_data);

    chan_width.max = get_attribute(channel, "chan_width_max", loc_data).as_uint();
    chan_width.x_min = get_attribute(channel, "x_min", loc_data).as_uint();
    chan_width.y_min = get_attribute(channel, "y_min", loc_data).as_uint();
    chan_width.x_max = get_attribute(channel, "x_max", loc_data).as_uint();
    chan_width.y_max = get_attribute(channel, "y_max", loc_data).as_uint();
    chan_width.x_list.resize(grid.height());
    chan_width.y_list.resize(grid.width());

    channelLists = get_first_child(parent, "x_list", loc_data);
    while (channelLists) {
        size_t index = get_attribute(channelLists, "index", loc_data).as_uint();
        if (index >= chan_width.x_list.size()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "index %d on x_list exceeds x_list size %u",
                            index, chan_width.x_list.size());
        }
        chan_width.x_list[index] = get_attribute(channelLists, "info", loc_data).as_float();
        channelLists = channelLists.next_sibling(channelLists.name());
    }
    channelLists = get_first_child(parent, "y_list", loc_data);
    while (channelLists) {
        size_t index = get_attribute(channelLists, "index", loc_data).as_uint();
        if (index >= chan_width.y_list.size()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "index %d on y_list exceeds y_list size %u",
                            index, chan_width.y_list.size());
        }
        chan_width.y_list[index] = get_attribute(channelLists, "info", loc_data).as_float();
        channelLists = channelLists.next_sibling(channelLists.name());
    }
}

/* Grid was initialized from the architecture file. This function checks
 * if it corresponds to the RR graph. Errors out if it doesn't correspond*/
void verify_grid(pugi::xml_node parent, const pugiutil::loc_data& loc_data, const DeviceGrid& grid) {
    pugi::xml_node grid_node;
    int num_grid_node = count_children(parent, "grid_loc", loc_data);

    grid_node = get_first_child(parent, "grid_loc", loc_data);
    for (int i = 0; i < num_grid_node; i++) {
        int x = get_attribute(grid_node, "x", loc_data).as_float();
        int y = get_attribute(grid_node, "y", loc_data).as_float();

        const t_grid_tile& grid_tile = grid[x][y];

        if (grid_tile.type->index != get_attribute(grid_node, "block_type_id", loc_data).as_int(0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Architecture file does not match RR graph's block_type_id at (%d, %d): arch used ID %d, RR graph used ID %d.", x, y,
                            (grid_tile.type->index), get_attribute(grid_node, "block_type_id", loc_data).as_int(0));
        }
        if (grid_tile.width_offset != get_attribute(grid_node, "width_offset", loc_data).as_float(0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Architecture file does not match RR graph's width_offset at (%d, %d)", x, y);
        }

        if (grid_tile.height_offset != get_attribute(grid_node, "height_offset", loc_data).as_float(0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Architecture file does not match RR graph's height_offset at (%d, %d)", x, y);
        }
        grid_node = grid_node.next_sibling(grid_node.name());
    }
}

static int pin_index_by_num(const t_class& class_inf, int num) {
    for (int index = 0; index < class_inf.num_pins; ++index) {
        if (num == class_inf.pinlist[index]) {
            return index;
        }
    }
    return -1;
}

/* Blocks were initialized from the architecture file. This function checks
 * if it corresponds to the RR graph. Errors out if it doesn't correspond*/
void verify_blocks(pugi::xml_node parent, const pugiutil::loc_data& loc_data) {
    pugi::xml_node Block;
    pugi::xml_node pin_class;
    pugi::xml_node pin;
    Block = get_first_child(parent, "block_type", loc_data);
    auto& device_ctx = g_vpr_ctx.mutable_device();
    while (Block) {
        auto block_info = device_ctx.physical_tile_types[get_attribute(Block, "id", loc_data).as_int(0)];

        const char* name = get_attribute(Block, "name", loc_data).as_string(nullptr);

        if (strcmp(block_info.name, name) != 0) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Architecture file does not match RR graph's block name: arch uses name %s, RR graph uses name %s", block_info.name, name);
        }

        if (block_info.width != get_attribute(Block, "width", loc_data).as_float(0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Architecture file does not match RR graph's block width");
        }
        if (block_info.height != get_attribute(Block, "height", loc_data).as_float(0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Architecture file does not match RR graph's block height");
        }

        pin_class = get_first_child(Block, "pin_class", loc_data, pugiutil::OPTIONAL);

        block_info.num_class = count_children(Block, "pin_class", loc_data, pugiutil::OPTIONAL);

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
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Valid inputs for class types are \"OPEN\", \"OUTPUT\", and \"INPUT\".");
                type = OPEN;
            }

            if (class_inf.type != type) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Architecture file does not match RR graph's block type");
            }

            if (class_inf.num_pins != (int)count_children(pin_class, "pin", loc_data)) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Incorrect number of pins in %d pin_class in block %s", classNum, block_info.name);
            }

            pin = get_first_child(pin_class, "pin", loc_data, pugiutil::OPTIONAL);
            while (pin) {
                auto num = get_attribute(pin, "ptc", loc_data).as_uint();
                auto index = pin_index_by_num(class_inf, num);

                if (index < 0) {
                    VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                    "Architecture file does not match RR graph's block pin list: invalid ptc for pin class");
                }

                if (pin.child_value() != block_type_pin_index_to_name(&block_info, class_inf.pinlist[index])) {
                    VPR_FATAL_ERROR(VPR_ERROR_OTHER,
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
void verify_segments(pugi::xml_node parent, const pugiutil::loc_data& loc_data, const std::vector<t_segment_inf>& segment_inf) {
    pugi::xml_node Segment, subnode;

    Segment = get_first_child(parent, "segment", loc_data);
    while (Segment) {
        int segNum = get_attribute(Segment, "id", loc_data).as_int();
        const char* name = get_attribute(Segment, "name", loc_data).as_string();
        if (strcmp(segment_inf[segNum].name.c_str(), name) != 0) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Architecture file does not match RR graph's segment name: arch uses %s, RR graph uses %s", segment_inf[segNum].name.c_str(), name);
        }

        subnode = get_single_child(parent, "timing", loc_data, pugiutil::OPTIONAL);

        if (subnode) {
            if (segment_inf[segNum].Rmetal != get_attribute(subnode, "R_per_meter", loc_data).as_float(0)) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Architecture file does not match RR graph's segment R_per_meter");
            }
            if (segment_inf[segNum].Cmetal != get_attribute(subnode, "C_per_meter", loc_data).as_float(0)) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
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

    typedef struct max_ptc {
        short chanx_max_ptc = 0;
        short chany_max_ptc = 0;
    } t_max_ptc;

    /*
     * Local multi-dimensional vector to hold max_ptc for every coordinate.
     * It has same height and width as CHANY and CHANX are inverted
     */
    vtr::Matrix<t_max_ptc> coordinates_max_ptc; /* [x][y] */
    size_t max_coord_size = std::max(grid.width(), grid.height());
    coordinates_max_ptc.resize({max_coord_size, max_coord_size}, t_max_ptc());

    /* Alloc the lookup table */
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

    /*
     * Add the correct node into the vector
     * For CHANX and CHANY no node is added yet, but the maximum ptc is counted for each
     * x/y location. This is needed later to add the correct node corresponding to CHANX
     * and CHANY.
     *
     * Note that CHANX and CHANY 's x and y are swapped due to the chan and seg convention.
     */
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        auto& node = device_ctx.rr_nodes[inode];
        if (node.type() == SOURCE || node.type() == SINK) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    if (node.type() == SOURCE) {
                        indices[SOURCE][ix][iy][0].push_back(inode);
                        indices[SINK][ix][iy][0].push_back(OPEN);
                    } else {
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
                    coordinates_max_ptc[iy][ix].chanx_max_ptc = std::max(coordinates_max_ptc[iy][ix].chanx_max_ptc, node.ptc_num());
                }
            }
        } else if (node.type() == CHANY) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    coordinates_max_ptc[ix][iy].chany_max_ptc = std::max(coordinates_max_ptc[ix][iy].chany_max_ptc, node.ptc_num());
                }
            }
        }
    }

    /* Alloc the lookup table */
    for (t_rr_type rr_type : RR_TYPES) {
        if (rr_type == CHANX) {
            for (size_t y = 0; y < grid.height(); ++y) {
                for (size_t x = 0; x < grid.width(); ++x) {
                    indices[CHANX][y][x][0].resize(coordinates_max_ptc[y][x].chanx_max_ptc + 1, OPEN);
                }
            }
        } else if (rr_type == CHANY) {
            for (size_t x = 0; x < grid.width(); ++x) {
                for (size_t y = 0; y < grid.height(); ++y) {
                    indices[CHANY][x][y][0].resize(coordinates_max_ptc[x][y].chany_max_ptc + 1, OPEN);
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
                    if (count >= int(indices[CHANX][iy][ix][0].size())) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                        "Ptc index %d for CHANX (%d, %d) is out of bounds, size = %zu",
                                        count, ix, iy, indices[CHANX][iy][ix][0].size());
                    }
                    indices[CHANX][iy][ix][0][count] = inode;
                }
            }
        } else if (node.type() == CHANY) {
            for (int ix = node.xlow(); ix <= node.xhigh(); ix++) {
                for (int iy = node.ylow(); iy <= node.yhigh(); iy++) {
                    count = node.ptc_num();
                    if (count >= int(indices[CHANY][ix][iy][0].size())) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                        "Ptc index %d for CHANY (%d, %d) is out of bounds, size = %zu",
                                        count, ix, iy, indices[CHANY][ix][iy][0].size());
                    }
                    indices[CHANY][ix][iy][0][count] = inode;
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
void set_cost_indices(pugi::xml_node parent, const pugiutil::loc_data& loc_data, const bool is_global_graph, const int num_seg_types) {
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

    while (rr_node) {
        int inode = get_attribute(rr_node, "id", loc_data).as_int();
        auto& node = device_ctx.rr_nodes[inode];

        /*CHANX and CHANY cost index is dependent on the segment id*/

        segmentSubnode = get_single_child(rr_node, "segment", loc_data, pugiutil::OPTIONAL);
        if (segmentSubnode) {
            attribute = get_attribute(segmentSubnode, "segment_id", loc_data, pugiutil::OPTIONAL);
            if (attribute) {
                int seg_id = get_attribute(segmentSubnode, "segment_id", loc_data).as_int(0);
                if (is_global_graph) {
                    node.set_cost_index(0);
                } else if (node.type() == CHANX) {
                    node.set_cost_index(CHANX_COST_INDEX_START + seg_id);
                } else if (node.type() == CHANY) {
                    node.set_cost_index(CHANX_COST_INDEX_START + num_seg_types + seg_id);
                }
            }
        }
        rr_node = rr_node.next_sibling(rr_node.name());
    }
}

void process_connection_boxes(pugi::xml_node parent, const pugiutil::loc_data& loc_data) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    int x_dim = get_attribute(parent, "x_dim", loc_data).as_int(0);
    int y_dim = get_attribute(parent, "y_dim", loc_data).as_int(0);
    int num_boxes = get_attribute(parent, "num_boxes", loc_data).as_int(0);
    VTR_ASSERT(num_boxes >= 0);

    pugi::xml_node connection_box = get_first_child(parent, "connection_box", loc_data);
    std::vector<ConnectionBox> boxes(num_boxes);
    while (connection_box) {
        int id = get_attribute(connection_box, "id", loc_data).as_int(-1);
        const char* name = get_attribute(connection_box, "name", loc_data).as_string(nullptr);
        VTR_ASSERT(id >= 0 && id < num_boxes);
        VTR_ASSERT(boxes.at(id).name == "");
        boxes.at(id).name = std::string(name);

        connection_box = connection_box.next_sibling(connection_box.name());
    }

    device_ctx.connection_boxes.reset_boxes(std::make_pair(x_dim, y_dim), boxes);
}
