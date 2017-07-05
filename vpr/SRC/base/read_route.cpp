#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <unordered_set>
using namespace std;

#include "blifparse.hpp"
#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "rr_graph.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "tatum/echo_writer.hpp"
#include "vtr_list.h"
#include "vtr_log.h"
#include "vtr_logic.h"
#include "vtr_time.h"
#include "vtr_digest.h"
#include "check_route.h"
#include "route_common.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_api.h"
#include "read_place.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"
#include "globals.h"
#include "place_and_route.h"
#include "place.h"
#include "read_place.h"
#include "route_export.h"
#include "rr_graph.h"
#include "path_delay.h"
#include "net_delay.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "route_common.h"
#include "place_macro.h"
#include "RoutingDelayCalculator.h"
#include "timing_info.h"
#include "read_route.h"

static void process_route(ifstream &fp, int inet, string name, stringstream &input_stream);

void read_route(const char* placement_file, const char* route_file, t_vpr_setup& vpr_setup, const t_arch& Arch) {

    /* Prints out the routing to file route_file.  */
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    vpr_init_pre_place_and_route(vpr_setup, Arch);

    /* Parse the file */
    vtr::printf_info("Begin loading packed FPGA routing file.\n");

    std::string route_id = vtr::secure_digest_file(route_file);

    ifstream fp;
    fp.open(route_file);

    if (!fp.is_open()) {
        vpr_throw(VPR_ERROR_ROUTE, route_file, __LINE__,
                "Cannot open %s routing file", route_file);
    }

    string temp, temp2, input_placement_file, input_id;
    int nx, ny;

    fp >> temp >> input_placement_file;
    if (input_placement_file != placement_file) {
        vpr_throw(VPR_ERROR_ROUTE, route_file, __LINE__,
                "Placement files %s specified in the routing file does not match given %s", input_placement_file.c_str(), placement_file);
    }

    fp >> temp >> input_id;

    //    if (input_id != place_ctx.placement_id.c_str()) {
    //        vpr_throw(VPR_ERROR_ROUTE, route_file, __LINE__,
    //                "Placement id %s specified in the routing file does not match given %s", input_id.c_str(), place_ctx.placement_id.c_str());
    //    }


    read_place(vpr_setup.FileNameOpts.NetFile.c_str(), vpr_setup.FileNameOpts.PlaceFile.c_str(), vpr_setup.FileNameOpts.verify_file_digests, device_ctx.nx, device_ctx.ny, cluster_ctx.num_blocks, cluster_ctx.blocks);
    sync_grid_to_blocks();

    post_place_sync(cluster_ctx.num_blocks);

    t_graph_type graph_type;
    if (vpr_setup.RouterOpts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
    } else {
        graph_type = (vpr_setup.RoutingArch.directionality == BI_DIRECTIONAL ?
                GRAPH_BIDIR : GRAPH_UNIDIR);
    }
    free_rr_graph();


    init_chan(vpr_setup.RouterOpts.fixed_channel_width, Arch.Chans);

    /* Set up the routing resource graph defined by this FPGA architecture. */
    int warning_count;
    create_rr_graph(graph_type, device_ctx.num_block_types, device_ctx.block_types, device_ctx.nx, device_ctx.ny, device_ctx.grid,
            &device_ctx.chan_width, vpr_setup.RoutingArch.switch_block_type,
            vpr_setup.RoutingArch.Fs, vpr_setup.RoutingArch.switchblocks,
            vpr_setup.RoutingArch.num_segment,
            device_ctx.num_arch_switches, vpr_setup.Segments,
            vpr_setup.RoutingArch.global_route_switch,
            vpr_setup.RoutingArch.delayless_switch,
            vpr_setup.RoutingArch.wire_to_arch_ipin_switch,
            vpr_setup.RouterOpts.base_cost_type,
            vpr_setup.RouterOpts.trim_empty_channels,
            vpr_setup.RouterOpts.trim_obs_channels,
            Arch.Directs, Arch.num_directs,
            vpr_setup.RoutingArch.dump_rr_structs_file,
            &vpr_setup.RoutingArch.wire_to_rr_ipin_switch,
            &device_ctx.num_rr_switches,
            &warning_count,
            vpr_setup.RouterOpts.write_rr_graph_name.c_str(),
            vpr_setup.RouterOpts.read_rr_graph_name.c_str(), false);

    alloc_and_load_rr_node_route_structs();

    t_clb_opins_used clb_opins_used_locally = alloc_route_structs();
    init_route_structs(vpr_setup.RouterOpts.bb_factor);

    fp >> temp >> temp2 >> nx >> temp >> ny;

    if (nx != device_ctx.nx || ny != device_ctx.ny) {
        vpr_throw(VPR_ERROR_ROUTE, route_file, __LINE__,
                "Device dimensions %d x %d specified in the routing file does not match given %d x %d ",
                nx, ny, device_ctx.nx, device_ctx.ny);
    }

    string input, identifier, name;
    unsigned inet;

    while (getline(fp, input)) {

        stringstream input_stream(input);
        getline(input_stream, identifier, ' ');
        if (identifier == "Net") {
            input_stream >> inet >> name;
            process_route(fp, inet, name, input_stream);
        }
    }

    fp.close();

    /*Correctly set up the clb opins*/
    recompute_occupancy_from_scratch(clb_opins_used_locally);

    float pres_fac = vpr_setup.RouterOpts.initial_pres_fac;

    /* Avoid overflow for high iteration counts, even if acc_cost is big */
    pres_fac = min(pres_fac, static_cast<float> (HUGE_POSITIVE_FLOAT / 1e5));

    pathfinder_update_cost(pres_fac, vpr_setup.RouterOpts.acc_fac);

    reserve_locally_used_opins(pres_fac, vpr_setup.RouterOpts.acc_fac, true, clb_opins_used_locally);
    
    check_route(vpr_setup.RouterOpts.route_type, device_ctx.num_rr_switches, clb_opins_used_locally, vpr_setup.Segments);
    get_serial_num();

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ROUTING_SINK_DELAYS)) {
        print_sink_delays(getEchoFileName(E_ECHO_ROUTING_SINK_DELAYS));
    }

    vtr::printf_info("Finished loading route file\n");

}

static void process_route(ifstream &fp, int inet, string name, stringstream &input_stream) {

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& place_ctx = g_vpr_ctx.placement();

    t_trace *tptr;

    string identifier, temp, newline, block;
    int inode, x, y, x2, y2, ptc, switch_id;
    string type, coordinates, next;

    getline(fp, temp);
    //get rid of brackets
    name.erase(name.begin());
    if (name.back() == ':') { /* Global net.  Never routed. */
        getline(input_stream, temp);
        if (temp == " global net connecting:") {
            cluster_ctx.clbs_nlist.net[inet].is_global = true;
            name.erase(name.end() - 1);
            name.erase(name.end() - 1);

            if (cluster_ctx.clbs_nlist.net[inet].name != name) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Net name %s for net number %d specified in the routing file does not match given %s",
                        name.c_str(), inet, cluster_ctx.clbs_nlist.net[inet].name);
            }
        }
        int pin_counter = 0;
        int iclass, bnum;
        string x_str, y_str, temp2, bnum_str;

        while (getline(fp, block)) {
            stringstream global_nets(block);

            getline(global_nets, identifier, ' ');

            if (identifier != "Block") {
                return;
            }

            global_nets >> name >> bnum_str >> temp >> x_str >> y_str;

            global_nets >> ws;
            getline(global_nets, temp, ' ');
            global_nets >> ws;
            getline(global_nets, temp2, ' ');
            global_nets >> ws;
            global_nets >> iclass;

            y_str.erase(y_str.end() - 1);
            y = atoi(y_str.c_str());

            x_str.erase(x_str.begin());
            x_str.erase(x_str.end() - 1);
            x = atoi(x_str.c_str());

            bnum_str.erase(bnum_str.begin());
            bnum_str.erase(bnum_str.begin());
            bnum_str.erase(bnum_str.end() - 1);
            bnum = atoi(bnum_str.c_str());

            if (cluster_ctx.blocks[bnum].name != name) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Block %s for block number %d specified in the routing file does not match given %s",
                        name.c_str(), bnum, cluster_ctx.blocks[bnum].name);
            }

            if (place_ctx.block_locs[bnum].x != x || place_ctx.block_locs[bnum].y != y) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "The placement coordinates (%d, %d) of %d block does not match given (%d, %d)",
                        x, y, place_ctx.block_locs[bnum].x, place_ctx.block_locs[bnum].y);
            }
            //            int node_block_pin = cluster_ctx.clbs_nlist.net[inet].pins[pin_counter].block_pin;
            //            cluster_ctx.blocks[bnum].type->pin_class[node_block_pin] = iclass;
            pin_counter++;
        }

    } else {

        cluster_ctx.clbs_nlist.net[inet].is_global = false;
        name.erase(name.end() - 1);


        if (cluster_ctx.clbs_nlist.net[inet].name != name) {
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Net name %s for net number %d specified in the routing file does not match given %s",
                    name.c_str(), inet, cluster_ctx.clbs_nlist.net[inet].name);
        }
        //removes a space after net:
        getline(fp, newline);

        if (newline == "\n\nUsed in local cluster only, reserved one CLB pin\n\n") {
            if (cluster_ctx.clbs_nlist.net[inet].num_sinks() != false) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Net %d should be used in local cluster only, reserved one CLB pin");
            }
            return;
        } else if (newline == "") {
            return;
        } else {
            stringstream ss(newline);
            ss >> temp >> inode >> type >> coordinates >> next;
            if (temp != "Node:") {
                return;
            }

            coordinates.erase(coordinates.begin());
            coordinates.erase(coordinates.end() - 1);

            stringstream coord_stream1(coordinates);
            coord_stream1 >> x;
            coord_stream1.ignore(1, ' ');
            coord_stream1 >> y;

            auto& node = device_ctx.rr_nodes[inode];
            if (next == "to") {
                ss >> coordinates >> next >> ptc;
                coordinates.erase(coordinates.begin());
                coordinates.erase(coordinates.end() - 1);

                stringstream coord_stream2(coordinates);
                coord_stream2 >> x2;
                coord_stream2.ignore(1, ' ');
                coord_stream2 >> y2;
                if (node.xlow() != x || node.xhigh() != x2 || node.yhigh() != y2 || node.ylow() != y) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "The coordinates of node %d does not match the rr graph", inode);
                }
            } else {
                if (node.xlow() != x || node.xhigh() != x || node.yhigh() != y || node.ylow() != y) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "The coordinates of node %d does not match the rr graph", inode);
                }
                ss >> ptc;
            }
            ss >> temp >> switch_id;
            route_ctx.trace_head[inet] = alloc_trace_data();
            route_ctx.trace_head[inet]->index = inode;
            route_ctx.trace_head[inet]->iswitch = switch_id;
            route_ctx.trace_head[inet]->next = NULL;
        }


        tptr = route_ctx.trace_head[inet];
        while (getline(fp, newline)) {
            if (newline == "\n\nUsed in local cluster only, reserved one CLB pin\n\n") {
                if (cluster_ctx.clbs_nlist.net[inet].num_sinks() != false)
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "Net %d should be used in local cluster only, reserved one CLB pin");
            } else if (newline.empty()) {
                route_ctx.trace_tail[inet] = tptr;
                return;
            } else {
                stringstream ss(newline);
                ss >> temp >> inode >> type >> coordinates >> next;

                if (temp != "Node:") {
                    route_ctx.trace_tail[inet] = tptr;
                    return;
                }

                tptr->next = alloc_trace_data();
                tptr = tptr -> next;
                tptr->index = inode;
                tptr->next = NULL;

                coordinates.erase(coordinates.begin());
                coordinates.erase(coordinates.end() - 1);

                stringstream coord_stream1(coordinates);
                coord_stream1 >> x;
                coord_stream1.ignore(1, ' ');
                coord_stream1 >> y;

                auto& node = device_ctx.rr_nodes[inode];

                if (next == "to") {
                    ss >> coordinates >> next >> ptc;
                    coordinates.erase(coordinates.begin());
                    coordinates.erase(coordinates.end() - 1);

                    stringstream coord_stream2(coordinates);
                    coord_stream2 >> x2;
                    coord_stream2.ignore(1, ' ');
                    coord_stream2 >> y2;
                    if (node.xlow() != x || node.xhigh() != x2 || node.yhigh() != y2 || node.ylow() != y) {
                        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                                "The coordinates of node %d does not match the rr graph", inode);
                    }
                } else {
                    if (node.xlow() != x || node.xhigh() != x || node.yhigh() != y || node.ylow() != y) {
                        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                                "The coordinates of node %d does not match the rr graph", inode);
                    }
                    ss >> ptc;
                }


                if (node.ptc_num() != ptc) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "The ptc of node %d does not match the rr graph", inode);
                }

                ss >> temp;
                if (temp == "Switch:") {
                    ss >> switch_id;
                } else {

                    if (device_ctx.grid[x][y].type != device_ctx.IO_TYPE && (type == "IPIN" || type == "OPIN")) {
                        int pin_num = device_ctx.rr_nodes[inode].ptc_num();
                        int offset = device_ctx.grid[x][y].height_offset;
                        int iblock = place_ctx.grid_blocks[x][y - offset].blocks[0];
                        VTR_ASSERT(iblock != OPEN);
                        t_pb_graph_pin *pb_pin = get_pb_graph_node_pin_from_block_pin(iblock, pin_num);
                        t_pb_type *pb_type = pb_pin->parent_node->pb_type;

                        string pb_name, port_name;
                        int pb_pin_num;
                        stringstream pb_info(temp);
                        getline(pb_info, pb_name, '.');
                        coord_stream1.ignore(1, ' ');
                        getline(pb_info, port_name, '[');
                        coord_stream1.ignore(1, ' ');
                        pb_info >> pb_pin_num;

                        if (pb_name != pb_type->name || port_name != pb_pin->port->name || pb_pin_num != pb_pin->pin_number) {
                            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                                    "%d node does not have correct pins", inode);
                        }
                        ss >> temp >>switch_id;
                    } else {
                        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                                "%d node does not have correct pins", inode);
                    }
                }
                tptr->iswitch = switch_id;

                if (node.type_string() != type) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "%d node does not have the correct type, should be %s", inode, node.type_string());
                }
                //                if (next == "Pad:") {
                //                    device_ctx.grid[x][y].type = device_ctx.IO_TYPE;
                //                }else{
                //                    device_ctx.grid[x][y].type = device_ctx.FILL_TYPE;
                //                }
            }
        }
    }
    return;
}


