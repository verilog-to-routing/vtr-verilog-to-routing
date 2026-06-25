/**
 * @file
 * @brief Simple checks to make sure netlist data structures are consistent.
 *
 * These include checking for duplicated names, dangling links, etc.
 */
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "atom_lookup.h"
#include "atom_netlist.h"
#include "direct_connection_legality.h"
#include "logic_types.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "hash.h"
#include "vpr_utils.h"
#include "check_netlist.h"

#define ERROR_THRESHOLD 100

/**************** Subroutines local to this module **************************/

static int check_connections_to_global_clb_pins(ClusterNetId net_id, int verbosity, bool is_flat);

static int check_for_duplicated_names();

static int check_clb_conn(ClusterBlockId iblk, int num_conn);

static int check_clb_internal_nets(ClusterBlockId iblk, const IntraLbPbPinLookup& intra_lb_pb_pini_lookup);

/** @brief Checks Fc_out = 0 top-level OUT pins are only used by legal external <direct>s. */
static int check_external_directs_legality(const t_arch& arch);

/** @brief True if the given top-level OUT pin has Fc_out == 0 (no general-routing access). */
static bool pin_has_fc_out_zero(t_logical_block_type_ptr lb, int pin_count_in_cluster);

/**
 * @brief Walks pb_route up from a primitive sink pin to the top-level IN pin it enters
 * through, returning that pin's pin_count_in_cluster, or -1 if none is reached.
 */
static int find_top_level_input_pin_of_sink(const t_pb_routes& pb_route,
                                            const t_pb_graph_node* pb_head,
                                            int sink_primitive_pin_id);

/** @brief True if the given sink pin is routed entirely within its cluster. */
static bool is_sink_routed_within_cluster(const t_pb_routes& pb_route,
                                          const t_pb_graph_node* pb_head,
                                          int sink_primitive_pin_id);

/** @brief A single illegal use of an Fc_out = 0 OUT pin: the source net/cluster/pin and the sink it couldn't legally reach. */
struct t_external_directs_legality_violation {
    AtomNetId net_id;               ///< The offending source net.
    ClusterBlockId cluster_id;      ///< Cluster driving the Fc_out = 0 OUT pin.
    const t_pb_graph_pin* out_pin;  ///< The Fc_out = 0 top-level OUT pin.
    ClusterBlockId sink_cluster_id; ///< Cluster of the sink that couldn't be reached legally.
    const t_pb_graph_pin* sink_pin; ///< The sink pin within sink_cluster_id.
};

/*********************** Subroutine definitions *****************************/

void check_netlist(int verbosity, const t_arch& arch) {
    int error = 0;
    int num_conn;
    t_hash **net_hash_table, *h_net_ptr;

    /* This routine checks that the netlist makes sense. */
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    // Return internal netlist verification first.
    cluster_ctx.clb_nlist.verify();

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
        // This function is called during packing - For the time being, we only use a flat netlist during routing
        global_to_non_global_connection_count += check_connections_to_global_clb_pins(net_id, verbosity, false);
        if (error >= ERROR_THRESHOLD) {
            VTR_LOG_ERROR("Too many errors in netlist, exiting.\n");
        }
    }
    free_hash_table(net_hash_table);
    if (global_to_non_global_connection_count > 0) {
        VTR_LOG("Netlist contains %d global net to non-global architecture pin connections\n", global_to_non_global_connection_count);
    }

    auto& device_ctx = g_vpr_ctx.device();
    IntraLbPbPinLookup intra_lb_pb_pin_lookup(device_ctx.logical_block_types);

    /* Check that each block makes sense. */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        num_conn = (int)cluster_ctx.clb_nlist.block_pins(blk_id).size();
        error += check_clb_conn(blk_id, num_conn);
        error += check_clb_internal_nets(blk_id, intra_lb_pb_pin_lookup);
        if (error >= ERROR_THRESHOLD) {
            VPR_ERROR(VPR_ERROR_OTHER,
                      "Too many errors in netlist, exiting.\n");
        }
    }

    error += check_for_duplicated_names();

    error += check_external_directs_legality(arch);

    if (error != 0) {
        VPR_ERROR(VPR_ERROR_OTHER,
                  "Found %d fatal Errors in the input netlist.\n", error);
    }
}

/**
 * @brief Checks that a global net (net_id) connects only to global CLB input pin
 *        and that non-global nets never connects to a global CLB pin.
 *
 * Either global or non-global nets are allowed to connect to pads.
 */
static int check_connections_to_global_clb_pins(ClusterNetId net_id, int verbosity, bool is_flat) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    bool net_is_ignored = cluster_ctx.clb_nlist.net_is_ignored(net_id);

    /* For now global signals can be driven by an I/O pad or any CLB output       *
     * although a CLB output generates a warning.  I could make a global CLB      *
     * output pin type to allow people to make architectures that didn't have     *
     * this warning.                                                              */
    int global_to_non_global_connection_count = 0;
    for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
        ClusterBlockId blk_id = cluster_ctx.clb_nlist.pin_block(pin_id);
        auto logical_type = cluster_ctx.clb_nlist.block_type(blk_id);
        auto physical_type = pick_physical_type(logical_type);

        int log_index = cluster_ctx.clb_nlist.pin_logical_index(pin_id);
        int pin_index = get_physical_pin(physical_type, logical_type, log_index);

        if (physical_type->is_ignored_pin[pin_index] != net_is_ignored && !physical_type->is_io()) {
            VTR_LOGV_WARN(verbosity > 2,
                          "Global net '%s' connects to non-global architecture pin '%s' (netlist pin '%s')\n",
                          cluster_ctx.clb_nlist.net_name(net_id).c_str(),
                          block_type_pin_index_to_name(physical_type, pin_index, is_flat).c_str(),
                          cluster_ctx.clb_nlist.pin_name(pin_id).c_str());

            ++global_to_non_global_connection_count;
        }
    }

    return global_to_non_global_connection_count;
}

///@brief Checks that the connections into and out of the clb make sense.
static int check_clb_conn(ClusterBlockId iblk, int num_conn) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;

    int error = 0;
    t_logical_block_type_ptr type = clb_nlist.block_type(iblk);

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

    if (num_conn > type->pb_type->num_pins) {
        VTR_LOG_ERROR("logic block #%d with output %s has %d pins.\n",
                      iblk, cluster_ctx.clb_nlist.block_name(iblk).c_str(), num_conn);
        error++;
    }

    return (error);
}

///@brief Check that internal-to-logic-block connectivity is continuous and logically consistent
static int check_clb_internal_nets(ClusterBlockId iblk, const IntraLbPbPinLookup& pb_graph_pin_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int error = 0;
    const auto& pb_route = cluster_ctx.clb_nlist.block_pb(iblk)->pb_route;
    int num_pins_in_block = cluster_ctx.clb_nlist.block_pb(iblk)->pb_graph_node->total_pb_pins;

    t_logical_block_type_ptr type = cluster_ctx.clb_nlist.block_type(iblk);

    for (int i = 0; i < num_pins_in_block; i++) {
        if (!pb_route.count(i)) continue;

        VTR_ASSERT(pb_route.count(i));

        if (pb_route[i].atom_net_id || pb_route[i].driver_pb_pin_id != UNDEFINED) {
            const t_pb_graph_pin* pb_gpin = pb_graph_pin_lookup.pb_gpin(type->index, i);
            if ((pb_gpin->port->type == IN_PORT && pb_gpin->is_root_block_pin())
                || (pb_gpin->port->type == OUT_PORT && pb_gpin->parent_node->is_primitive())) {
                if (pb_route[i].driver_pb_pin_id != UNDEFINED) {
                    VTR_LOG_ERROR(
                        "Internal connectivity error in logic block #%d with output %s."
                        " Internal node %d driven when it shouldn't be driven \n",
                        iblk, cluster_ctx.clb_nlist.block_name(iblk).c_str(), i);
                    error++;
                }
            } else {
                if (!pb_route[i].atom_net_id || pb_route[i].driver_pb_pin_id == UNDEFINED) {
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
    return error;
}

static int check_external_directs_legality(const t_arch& arch) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const AtomNetlist& atom_nlist = atom_ctx.netlist();
    const AtomLookup& atom_lookup = atom_ctx.lookup();
    const DirectConnectionLegality direct_legality(arch.directs, g_vpr_ctx.device());

    std::vector<t_external_directs_legality_violation> violations;

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        t_logical_block_type_ptr cluster_type = cluster_ctx.clb_nlist.block_type(blk_id);
        const t_pb* cluster_pb = cluster_ctx.clb_nlist.block_pb(blk_id);
        if (cluster_type == nullptr || cluster_pb == nullptr) continue;

        const t_pb_graph_node* pb_head = cluster_pb->pb_graph_node;
        if (pb_head == nullptr) continue;

        const t_pb_routes& pb_route = cluster_pb->pb_route;

        // Iterate every used pin in the cluster (entries in pb_route are by
        // definition used). Filter to top-level OUT pins.
        for (const auto& pb_route_entry : pb_route) {
            int out_pin_id = pb_route_entry.first;
            const t_pb_graph_pin* pb_pin = pb_route_entry.second.pb_graph_pin;
            if (pb_pin == nullptr || pb_pin->port == nullptr) continue;
            if (pb_pin->parent_node != pb_head || pb_pin->port->type != OUT_PORT) continue;

            AtomNetId net_id = pb_route_entry.second.atom_net_id;
            if (!net_id) continue;

            // Fc_out > 0 pins have general-routing access; nothing to check.
            if (!pin_has_fc_out_zero(cluster_type, out_pin_id)) continue;
            // Bypass global nets under clock modeling ideal mode, it is not going to be routed. No need to ensure routing path feasibility
            if (atom_nlist.net_is_global(net_id) && atom_nlist.net_is_ignored(net_id)) {
                continue;
            }

            bool net_has_unsatisfied_sink = false;
            ClusterBlockId offending_sink_cluster_id;
            const t_pb_graph_pin* offending_sink_pin = nullptr;
            for (AtomPinId sink_atom_pin : atom_nlist.net_sinks(net_id)) {
                AtomBlockId sink_blk = atom_nlist.pin_block(sink_atom_pin);
                if (!sink_blk) continue;

                ClusterBlockId sink_cluster_id = atom_lookup.atom_clb(sink_blk);
                if (!sink_cluster_id.is_valid()) continue;

                const t_pb* sink_cluster_pb = cluster_ctx.clb_nlist.block_pb(sink_cluster_id);
                t_logical_block_type_ptr sink_cluster_type = cluster_ctx.clb_nlist.block_type(sink_cluster_id);
                if (sink_cluster_pb == nullptr || sink_cluster_type == nullptr) continue;

                const t_pb_graph_pin* sink_pb_pin = find_pb_graph_pin(
                    atom_nlist,
                    atom_lookup.atom_pb_bimap(),
                    sink_atom_pin);
                if (sink_pb_pin == nullptr) continue;

                if (sink_cluster_id == blk_id) {
                    // Source and sink share a cluster. The connection is legal
                    // if it routes entirely with intra-cluster resources. If
                    // instead it leaves and re-enters through a top-level pin,
                    // that feedback path needs general routing, which an
                    // Fc_out = 0 pin cannot provide.
                    if (!is_sink_routed_within_cluster(sink_cluster_pb->pb_route,
                                                       sink_cluster_pb->pb_graph_node,
                                                       sink_pb_pin->pin_count_in_cluster)) {
                        net_has_unsatisfied_sink = true;
                        offending_sink_cluster_id = sink_cluster_id;
                        offending_sink_pin = sink_pb_pin;
                        break;
                    }
                } else {
                    // Source and sink are in different clusters: the connection
                    // can only exist if a <direct> wires this OUT pin to the
                    // sink's top-level IN pin.
                    int sink_top_in_pin = find_top_level_input_pin_of_sink(
                        sink_cluster_pb->pb_route,
                        sink_cluster_pb->pb_graph_node,
                        sink_pb_pin->pin_count_in_cluster);
                    if (!direct_legality.is_direct_legal(cluster_type, out_pin_id,
                                                         sink_cluster_type, sink_top_in_pin)) {
                        net_has_unsatisfied_sink = true;
                        offending_sink_cluster_id = sink_cluster_id;
                        offending_sink_pin = sink_pb_pin;
                        break;
                    }
                }
            }

            if (net_has_unsatisfied_sink) {
                violations.push_back({net_id, blk_id, pb_pin, offending_sink_cluster_id, offending_sink_pin});
            }
        }
    }

    if (violations.empty()) return 0;

    VTR_LOG_ERROR("Clustered netlist <direct>-list legality check failed: "
                  "%zu cluster/net pair(s) drive top-level OUT pin(s) with Fc_out = 0 "
                  "without a matching <direct> entry.\n",
                  violations.size());
    for (const t_external_directs_legality_violation& violation : violations) {
        VTR_LOG_ERROR("    Offending net: %s, cluster: %s, Fc_out = 0 pin: %s, sink cluster: %s, sink pin: %s\n",
                      atom_nlist.net_name(violation.net_id).c_str(),
                      cluster_ctx.clb_nlist.block_name(violation.cluster_id).c_str(),
                      violation.out_pin->to_string().c_str(),
                      cluster_ctx.clb_nlist.block_name(violation.sink_cluster_id).c_str(),
                      violation.sink_pin->to_string().c_str());
    }

    return violations.size();
}

static bool pin_has_fc_out_zero(t_logical_block_type_ptr lb, int pin_count_in_cluster) {
    // LIMITATION: only the first equivalent tile is considered. A logical
    // block placeable in multiple physical tiles with differing Fc is not
    // supported.
    if (lb == nullptr || lb->equivalent_tiles.empty()) return true;

    const t_physical_tile_type* tile = lb->equivalent_tiles.front();
    int phys_pin = get_physical_pin(tile, lb, pin_count_in_cluster);
    for (const t_fc_specification& fc_spec : tile->fc_specs) {
        if (fc_spec.fc_value <= 0) continue;
        if (std::find(fc_spec.pins.begin(), fc_spec.pins.end(), phys_pin) != fc_spec.pins.end()) {
            return false;
        }
    }

    return true;
}

static int find_top_level_input_pin_of_sink(const t_pb_routes& pb_route,
                                            const t_pb_graph_node* pb_head,
                                            int sink_primitive_pin_id) {
    int current_pin_id = sink_primitive_pin_id;

    // Follow driver links upstream. The step count bounds the walk in case a
    // malformed pb_route contains a cycle.
    for (size_t remaining_steps = pb_route.size(); remaining_steps > 0; --remaining_steps) {
        auto it = pb_route.find(current_pin_id);
        if (it == pb_route.end()) return -1;

        const t_pb_route& entry = it->second;
        const t_pb_graph_pin* pb_pin = entry.pb_graph_pin;
        if (pb_pin == nullptr) return -1;

        if (pb_pin->pin_count_in_cluster != current_pin_id) return -1;

        if (pb_pin->parent_node == pb_head
            && pb_pin->port != nullptr
            && pb_pin->port->type == IN_PORT) {
            return current_pin_id;
        }

        if (entry.driver_pb_pin_id < 0) return -1;
        current_pin_id = entry.driver_pb_pin_id;
    }

    return -1;
}

static bool is_sink_routed_within_cluster(const t_pb_routes& pb_route,
                                          const t_pb_graph_node* pb_head,
                                          int sink_primitive_pin_id) {
    // find_top_level_input_pin_of_sink returns >= 0 only when the sink's driver chain
    // reaches a top-level input pin (i.e. the signal enters from outside). A negative
    // result means the chain ends on an internal driver, so the sink is fed within the cluster.
    return find_top_level_input_pin_of_sink(pb_route, pb_head, sink_primitive_pin_id) < 0;
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
