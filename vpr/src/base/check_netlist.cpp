/**
 * Simple checks to make sure netlist data structures are consistent.  These include checking for duplicated names, dangling links, etc.
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "hash.h"
#include "vpr_utils.h"
#include "check_netlist.h"
#include "read_xml_arch_file.h"

#define ERROR_THRESHOLD 100

/**************** Subroutines local to this module **************************/

static int check_connections_to_global_clb_pins(ClusterNetId net_id, int verbosity);

static int check_for_duplicated_names();

static int check_clb_conn(ClusterBlockId iblk, int num_conn);

static int check_clb_internal_nets(ClusterBlockId iblk);

/*********************** Subroutine definitions *****************************/

void check_netlist(int verbosity) {
    int error = 0;
    int num_conn;
    t_hash **net_hash_table, *h_net_ptr;

    /* This routine checks that the netlist makes sense. */
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    net_hash_table = alloc_hash_table();

    /* Check that nets fanout and have a driver. */
    int global_to_non_global_connection_count = 0;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        h_net_ptr = insert_in_hash_table(net_hash_table, cluster_ctx.clb_nlist.net_name(net_id).c_str(), size_t(net_id));
        if (h_net_ptr->count != 1) {
            VTR_LOG_ERROR("Net %s has multiple drivers.\n",
                          cluster_ctx.clb_nlist.net_name(net_id).c_str());
            error++;
        }
        global_to_non_global_connection_count += check_connections_to_global_clb_pins(net_id, verbosity);
        if (error >= ERROR_THRESHOLD) {
            VTR_LOG_ERROR("Too many errors in netlist, exiting.\n");
        }
    }
    free_hash_table(net_hash_table);
    VTR_LOG_WARN("Netlist contains %d global net to non-global architecture pin connections\n", global_to_non_global_connection_count);

    /* Check that each block makes sense. */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        num_conn = (int)cluster_ctx.clb_nlist.block_pins(blk_id).size();
        error += check_clb_conn(blk_id, num_conn);
        error += check_clb_internal_nets(blk_id);
        if (error >= ERROR_THRESHOLD) {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                      "Too many errors in netlist, exiting.\n");
        }
    }

    error += check_for_duplicated_names();

    if (error != 0) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                  "Found %d fatal Errors in the input netlist.\n", error);
    }
}

/* Checks that a global net (net_id) connects only to global CLB input pins  *
 * and that non-global nets never connects to a global CLB pin.  Either       *
 * global or non-global nets are allowed to connect to pads.                  */
static int check_connections_to_global_clb_pins(ClusterNetId net_id, int verbosity) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    bool net_is_ignored = cluster_ctx.clb_nlist.net_is_ignored(net_id);

    /* For now global signals can be driven by an I/O pad or any CLB output       *
     * although a CLB output generates a warning.  I could make a global CLB      *
     * output pin type to allow people to make architectures that didn't have     *
     * this warning.                                                              */
    int global_to_non_global_connection_count = 0;
    for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
        ClusterBlockId blk_id = cluster_ctx.clb_nlist.pin_block(pin_id);
        int pin_index = cluster_ctx.clb_nlist.pin_physical_index(pin_id);

        if (cluster_ctx.clb_nlist.block_type(blk_id)->is_ignored_pin[pin_index] != net_is_ignored
            && !is_io_type(cluster_ctx.clb_nlist.block_type(blk_id))) {
            VTR_LOGV_WARN(verbosity > 2,
                          "Global net '%s' connects to non-global architecture pin '%s' (netlist pin '%s')\n",
                          cluster_ctx.clb_nlist.net_name(net_id).c_str(),
                          block_type_pin_index_to_name(cluster_ctx.clb_nlist.block_type(blk_id), pin_index).c_str(),
                          cluster_ctx.clb_nlist.pin_name(pin_id).c_str());

            ++global_to_non_global_connection_count;
        }
    }

    return global_to_non_global_connection_count;
}

/* Checks that the connections into and out of the clb make sense.  */
static int check_clb_conn(ClusterBlockId iblk, int num_conn) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;

    int error = 0;
    t_type_ptr type = clb_nlist.block_type(iblk);

    if (num_conn == 1) {
        for (auto pin_id : clb_nlist.block_pins(iblk)) {
            auto pin_type = clb_nlist.pin_type(pin_id);

            if (pin_type == PinType::SINK && !clb_nlist.block_contains_primary_output(iblk)) {
                //Input only and not a Primary-Output block
                VTR_LOG_WARN(
                    "Logic block #%d (%s) has only 1 input pin '%s'"
                    " -- the whole block is hanging logic that should be swept.\n",
                    iblk, clb_nlist.block_name(iblk).c_str(),
                    clb_nlist.pin_name(pin_id).c_str());
            }
            if (pin_type == PinType::DRIVER && !clb_nlist.block_contains_primary_input(iblk)) {
                //Output only and not a Primary-Input block
                VTR_LOG_WARN(
                    "Logic block #%d (%s) has only 1 output pin '%s'."
                    " It may be a constant generator.\n",
                    iblk, clb_nlist.block_name(iblk).c_str(),
                    clb_nlist.pin_name(pin_id).c_str());
            }

            break;
        }
    }

    /* This case should already have been flagged as an error -- this is *
     * just a redundant double check.                                    */

    if (num_conn > type->num_pins) {
        VTR_LOG_ERROR("logic block #%d with output %s has %d pins.\n",
                      iblk, cluster_ctx.clb_nlist.block_name(iblk).c_str(), num_conn);
        error++;
    }

    return (error);
}

/* Check that internal-to-logic-block connectivity is continuous and logically consistent */
static int check_clb_internal_nets(ClusterBlockId iblk) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int error = 0;
    const auto& pb_route = cluster_ctx.clb_nlist.block_pb(iblk)->pb_route;
    int num_pins_in_block = cluster_ctx.clb_nlist.block_pb(iblk)->pb_graph_node->total_pb_pins;

    t_pb_graph_pin** pb_graph_pin_lookup = alloc_and_load_pb_graph_pin_lookup_from_index(cluster_ctx.clb_nlist.block_type(iblk));

    for (int i = 0; i < num_pins_in_block; i++) {
        if (!pb_route.count(i)) continue;

        VTR_ASSERT(pb_route.count(i));

        if (pb_route[i].atom_net_id || pb_route[i].driver_pb_pin_id != OPEN) {
            if ((pb_graph_pin_lookup[i]->port->type == IN_PORT && pb_graph_pin_lookup[i]->is_root_block_pin())
                || (pb_graph_pin_lookup[i]->port->type == OUT_PORT && pb_graph_pin_lookup[i]->parent_node->is_primitive())) {
                if (pb_route[i].driver_pb_pin_id != OPEN) {
                    VTR_LOG_ERROR(
                        "Internal connectivity error in logic block #%d with output %s."
                        " Internal node %d driven when it shouldn't be driven \n",
                        iblk, cluster_ctx.clb_nlist.block_name(iblk).c_str(), i);
                    error++;
                }
            } else {
                if (!pb_route[i].atom_net_id || pb_route[i].driver_pb_pin_id == OPEN) {
                    VTR_LOG_ERROR(
                        "Internal connectivity error in logic block #%d with output %s."
                        " Internal node %d dangling\n",
                        iblk, cluster_ctx.clb_nlist.block_name(iblk).c_str(), i);
                    error++;
                } else {
                    int prev_pin = pb_route[i].driver_pb_pin_id;
                    if (pb_route[prev_pin].atom_net_id != pb_route[i].atom_net_id) {
                        VTR_LOG_ERROR(
                            "Internal connectivity error in logic block #%d with output %s."
                            " Internal node %d driven by different net than internal node %d\n",
                            iblk, cluster_ctx.clb_nlist.block_name(iblk).c_str(), i, prev_pin);
                        error++;
                    }
                }
            }
        }
    }

    free_pb_graph_pin_lookup_from_index(pb_graph_pin_lookup);
    return error;
}

static int check_for_duplicated_names() {
    int error, clb_count;
    t_hash **clb_hash_table, *clb_h_ptr;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    clb_hash_table = alloc_hash_table();

    error = clb_count = 0;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        clb_h_ptr = insert_in_hash_table(clb_hash_table, cluster_ctx.clb_nlist.block_name(blk_id).c_str(), clb_count);
        if (clb_h_ptr->count > 1) {
            VTR_LOG_ERROR("Block %s has duplicated name.\n",
                          cluster_ctx.clb_nlist.block_name(blk_id).c_str());
            error++;
        } else {
            clb_count++;
        }
    }

    free_hash_table(clb_hash_table);

    return error;
}
