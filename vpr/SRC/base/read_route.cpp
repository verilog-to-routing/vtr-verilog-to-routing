#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <unordered_set>
using namespace std;

#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "rr_graph.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "tatum/echo_writer.hpp"
#include "vtr_log.h"
#include "check_route.h"
#include "route_common.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_api.h"
#include "read_place.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"
#include "place_and_route.h"
#include "route_export.h"
#include "echo_files.h"
#include "route_common.h"
#include "read_route.h"

static void process_route(ifstream &fp);
static void process_nodes(ifstream &fp, int inet);
static void process_nets(ifstream &fp, int inet, string name, std::vector<std::string> input_tokens);
static void process_global_blocks(ifstream &fp, int inet);
static void format_coordinates(int &x, int &y, string coord);
static void format_pin_info(string &pb_name, string & port_name, int & pb_pin_num, string input);
static string format_name(string name);

void read_route(const char* placement_file, const char* route_file, t_vpr_setup& vpr_setup) {

    /* Reads in the routing file to fill in the trace_head and t_clb_opins_used data structure. 
     * Perform a series of verification tests to ensure the netlist, placement, and routing
     * files match */
    auto& device_ctx = g_vpr_ctx.mutable_device();
    /* Begin parsing the file */
    vtr::printf_info("Begin loading packed FPGA routing file.\n");

    string header_str;

    ifstream fp;
    fp.open(route_file);

    if (!fp.is_open()) {
        vpr_throw(VPR_ERROR_ROUTE, route_file, __LINE__,
                "Cannot open %s routing file", route_file);
    }

    getline(fp, header_str);
    std::vector<std::string> header = vtr::split(header_str);
    if (header[0] == "Placement_File:" && header[1] != placement_file) {
        vpr_throw(VPR_ERROR_ROUTE, route_file, __LINE__,
                "Placement files %s specified in the routing file does not match given %s", header[1].c_str(), placement_file);
    }

    /*Allocate necessary routing structures*/
    alloc_and_load_rr_node_route_structs();
    t_clb_opins_used clb_opins_used_locally = alloc_route_structs();
    init_route_structs(vpr_setup.RouterOpts.bb_factor);

    /*Check dimensions*/
    getline(fp, header_str);
    header.clear();
    header = vtr::split(header_str);
    if (header[0] == "Array" && header[1] == "size:" &&
            (atoi(header[2].c_str()) != device_ctx.nx || atoi(header[4].c_str()) != device_ctx.ny)) {
        vpr_throw(VPR_ERROR_ROUTE, route_file, __LINE__,
                "Device dimensions %sx%s specified in the routing file does not match given %dx%d ",
                header[2].c_str(), header[4].c_str(), device_ctx.nx, device_ctx.ny);
    }

    /* Read in every net */
    process_route(fp);

    fp.close();

    /*Correctly set up the clb opins*/
    recompute_occupancy_from_scratch(clb_opins_used_locally);

    /* Note: This pres_fac is not necessarily correct since it isn't the first routing iteration*/
    pathfinder_update_cost(vpr_setup.RouterOpts.initial_pres_fac, vpr_setup.RouterOpts.acc_fac);

    reserve_locally_used_opins(vpr_setup.RouterOpts.initial_pres_fac,
            vpr_setup.RouterOpts.acc_fac, true, clb_opins_used_locally);

    /* Finished loading in the routing, now check it*/
    check_route(vpr_setup.RouterOpts.route_type, device_ctx.num_rr_switches, clb_opins_used_locally, vpr_setup.Segments);
    get_serial_num();

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ROUTING_SINK_DELAYS)) {
        print_sink_delays(getEchoFileName(E_ECHO_ROUTING_SINK_DELAYS));
    }

    vtr::printf_info("Finished loading route file\n");
}

static void process_route(ifstream &fp) {
    /*Walks through every net and add the routing appropriately*/
    string input;
    unsigned inet;
    std::vector<std::string> tokens;
    while (getline(fp, input)) {
        tokens.clear();
        tokens = vtr::split(input);
        if (tokens.empty()) {
            continue; //Skip blank lines
        } else if (tokens[0][0] == '#') {
            continue; //Skip commented lines
        } else if (tokens[0] == "Net") {
            inet = atoi(tokens[1].c_str());
            process_nets(fp, inet, tokens[2], tokens);
        }
    }

    tokens.clear();
}

static void process_nets(ifstream &fp, int inet, string name, std::vector<std::string> input_tokens) {
    /* Check if the net is global or not, and process appropriately */
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    if (input_tokens.size() > 3 && input_tokens[3] == "global"
            && input_tokens[4] == "net" && input_tokens[5] == "connecting:") {
        /* Global net.  Never routed. */
        if (!cluster_ctx.clb_nlist.net_global((NetId)inet)) {
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Net %d should be a global net", inet);
        }
        //erase an extra colon for global nets
        name.erase(name.end() - 1);
        name = format_name(name);

        if (0 != cluster_ctx.clb_nlist.net_name((NetId)inet).compare(name)) {
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Net name %s for net number %d specified in the routing file does not match given %s",
                    name.c_str(), inet, cluster_ctx.clb_nlist.net_name((NetId)inet));
        }

        process_global_blocks(fp, inet);
    } else {
        /* Not a global net */
        if (cluster_ctx.clb_nlist.net_global((NetId)inet)) {
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Net %d is not a global net", inet);
        }

        name = format_name(name);

        if (0 != cluster_ctx.clb_nlist.net_name((NetId)inet).compare(name)) {
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Net name %s for net number %d specified in the routing file does not match given %s",
                    name.c_str(), inet, cluster_ctx.clb_nlist.net_name((NetId)inet));
        }

        process_nodes(fp, inet);
    }
    input_tokens.clear();
    return;
}

static void process_nodes(ifstream & fp, int inet) {
    /* Not a global net. Goes through every node and add it into trace_head*/

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& place_ctx = g_vpr_ctx.placement();

    t_trace *tptr = route_ctx.trace_head[inet];

    /*remember the position of the last line in order to go back*/
    streampos oldpos = fp.tellg();
    int inode, x, y, x2, y2, ptc, switch_id, offset;
    bool last_node_sink = false;
    int node_count = 0;
    string input;
    std::vector<std::string> tokens;

    /*Walk through every line that begins with Node:*/
    while (getline(fp, input)) {
        tokens.clear();
        tokens = vtr::split(input);

        if (tokens.empty()) {
            continue; /*Skip blank lines*/
        } else if (tokens[0][0] == '#') {
            continue; /*Skip commented lines*/
        } else if (tokens[0] == "Net") {
            /*End of the nodes list,
             *  return by moving the position of next char of input stream to be before net*/
            fp.seekg(oldpos);
            if (!last_node_sink) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Last node in routing has to be a sink type");
            }
            return;
        } else if (input == "\n\nUsed in local cluster only, reserved one CLB pin\n\n") {
            if (cluster_ctx.clbs_nlist.net[inet].num_sinks() != false) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Net %d should be used in local cluster only, reserved one CLB pin");
            }
            return;
        } else if (tokens[0] == "Node:") {
            /*An actual line, go through each node and add it to the route tree*/
            inode = atoi(tokens[1].c_str());
            auto& node = device_ctx.rr_nodes[inode];

            /*First node needs to be source. It is isolated to correctly set heap head.*/
            if (node_count == 0 && tokens[2] != "SOURCE") {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "First node in routing has to be a source type");
            }

            /*Check node types if match rr graph*/
            if (tokens[2] != node.type_string()) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Node %d has a type that does not match the RR graph", inode);
            }

            /*Keep track of the sink since last node has to be sink type*/
            if (tokens[2] == "SINK") {
                last_node_sink = true;
            } else {
                last_node_sink = false;
            }

            format_coordinates(x, y, tokens[3]);

            if (tokens[4] == "to") {
                format_coordinates(x2, y2, tokens[5]);
                if (node.xlow() != x || node.xhigh() != x2 || node.yhigh() != y2 || node.ylow() != y) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "The coordinates of node %d does not match the rr graph", inode);
                }
                offset = 2;
            } else {
                if (node.xlow() != x || node.xhigh() != x || node.yhigh() != y || node.ylow() != y) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "The coordinates of node %d does not match the rr graph", inode);
                }
                offset = 0;
            }

            /* Verify types and ptc*/
            if (tokens[2] == "SOURCE" || tokens[2] == "SINK" || tokens[2] == "OPIN" || tokens[2] == "IPIN") {
                if (tokens[4 + offset] == "Pad:" && device_ctx.grid[x][y].type != device_ctx.IO_TYPE) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "Node %d is of the wrong type", inode);
                }
            } else if (tokens[2] == "CHANX" || tokens[2] == "CHANY") {
                if (tokens[4 + offset] != "Track:") {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "A %s node have to have track info", tokens[2].c_str());
                }
            }

            ptc = atoi(tokens[5 + offset].c_str());
            if (node.ptc_num() != ptc) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "The ptc num of node %d does not match the rr graph", inode);
            }

            /*Process switches and pb pin info if it is ipin or opin type*/
            if (tokens[6 + offset] != "Switch:") {
                /*This is an opin or ipin, process its pin nums*/
                if (device_ctx.grid[x][y].type != device_ctx.IO_TYPE && (tokens[2] == "IPIN" || tokens[2] == "OPIN")) {
                    int pin_num = device_ctx.rr_nodes[inode].ptc_num();
                    int height_offset = device_ctx.grid[x][y].height_offset;
                    int iblock = place_ctx.grid_blocks[x][y - height_offset].blocks[0];
                    VTR_ASSERT(iblock != OPEN);
                    t_pb_graph_pin *pb_pin = get_pb_graph_node_pin_from_block_pin(iblock, pin_num);
                    t_pb_type *pb_type = pb_pin->parent_node->pb_type;

                    string pb_name, port_name;
                    int pb_pin_num;

                    format_pin_info(pb_name, port_name, pb_pin_num, tokens[6 + offset]);

                    if (pb_name != pb_type->name || port_name != pb_pin->port->name ||
                            pb_pin_num != pb_pin->pin_number) {
                        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                                "%d node does not have correct pins", inode);
                    }
                } else {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "%d node does not have correct pins", inode);
                }
                switch_id = atoi(tokens[8 + offset].c_str());
            } else {
                switch_id = atoi(tokens[7 + offset].c_str());
            }

            /* Allocate and load correct values to trace_head*/
            if (node_count == 0) {
                route_ctx.trace_head[inet] = alloc_trace_data();
                route_ctx.trace_head[inet]->index = inode;
                route_ctx.trace_head[inet]->iswitch = switch_id;
                route_ctx.trace_head[inet]->next = NULL;
                tptr = route_ctx.trace_head[inet];
                node_count++;
            } else {
                tptr->next = alloc_trace_data();
                tptr = tptr -> next;
                tptr->index = inode;
                tptr->iswitch = switch_id;
                tptr->next = NULL;
                node_count++;
            }
        }
        //stores last line so can easily go back to read
        oldpos = fp.tellg();
    }
}

static void process_global_blocks(ifstream &fp, int inet) {
    /*This function goes through all the blocks in a global net and verify it with the
     *clustered netlist and the placement*/

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    string block, bnum_str;
    int x, y, bnum;
    std::vector<std::string> tokens;
    int pin_counter = 0;

    streampos oldpos = fp.tellg();
    /*Walk through every block line*/
    while (getline(fp, block)) {
        tokens.clear();
        tokens = vtr::split(block);

        if (tokens.empty()) {
            continue; /*Skip blank lines*/
        } else if (tokens[0][0] == '#') {
            continue; /*Skip commented lines*/
        } else if (tokens[0] != "Block") {
            /*End of blocks, go back to reading nets*/
            fp.seekg(oldpos);
            return;
        } else {
            format_coordinates(x, y, tokens[4]);

            /*remove ()*/
            bnum_str = format_name(tokens[2]);
            /*remove #*/
            bnum_str.erase(bnum_str.begin());
            bnum = atoi(bnum_str.c_str());

            /*Check for name, coordinate, and pins*/
            if (0 != cluster_ctx.clb_nlist.block_name((BlockId)bnum).compare(tokens[1])) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Block %s for block number %d specified in the routing file does not match given %s",
                        tokens[1].c_str(), bnum, cluster_ctx.clb_nlist.block_name((BlockId)bnum));
            }
            if (place_ctx.block_locs[bnum].x != x || place_ctx.block_locs[bnum].y != y) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "The placement coordinates (%d, %d) of %d block does not match given (%d, %d)",
                        x, y, place_ctx.block_locs[bnum].x, place_ctx.block_locs[bnum].y);
            }
            int node_block_pin = cluster_ctx.clbs_nlist.net[inet].pins[pin_counter].block_pin;
            if (cluster_ctx.clb_nlist.block_type((BlockId)bnum)->pin_class[node_block_pin] != atoi(tokens[7].c_str())) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "The pin class %d of %d net does not match given ",
                        atoi(tokens[7].c_str()), inet, cluster_ctx.clb_nlist.block_type((BlockId)bnum)->pin_class[node_block_pin]);
            }
            pin_counter++;
        }
        oldpos = fp.tellg();
    }
}

static void format_coordinates(int &x, int &y, string coord) {
    /*Parse coordinates in the form of (x,y) into correct x and y values*/
    coord = format_name(coord);
    stringstream coord_stream(coord);
    coord_stream >> x;
    coord_stream.ignore(1, ' ');
    coord_stream >> y;
}

static void format_pin_info(string &pb_name, string & port_name, int & pb_pin_num, string input) {
    /*Parse the pin info in the form of pb_name.port_name[pb_pin_num]
     *into its appropriate variables*/
    stringstream pb_info(input);
    getline(pb_info, pb_name, '.');
    getline(pb_info, port_name, '[');
    pb_info >> pb_pin_num;
    if (!pb_info) {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                "Format of this pin info %s is incorrect",
                input.c_str());
    }
}

static string format_name(string name) {

    /*Return actual name by extracting it out of the form of (name)*/
    if (name.length() > 2) {
        name.erase(name.begin());
        name.erase(name.end() - 1);
        return name;
    } else {
        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                "%s should be enclosed by parenthesis",
                name.c_str());
        return NULL;
    }
}