/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   The implementation of the Cluster Legalizer class.
 *
 * Most of the code in this file was original part of cluster_util.cpp and was
 * highly integrated with the clusterer in VPR. All code that was used for
 * legalizing the clusters was moved into this file and all the functionality
 * was moved into the ClusterLegalizer class.
 */

#include "cluster_legalizer.h"
#include <algorithm>
#include <string>
#include <tuple>
#include <vector>
#include "atom_lookup.h"
#include "atom_netlist.h"
#include "cluster_placement.h"
#include "cluster_router.h"
#include "cluster_util.h"
#include "globals.h"
#include "logic_types.h"
#include "netlist_utils.h"
#include "noc_aware_cluster_util.h"
#include "noc_data_types.h"
#include "pack_types.h"
#include "partition.h"
#include "partition_region.h"
#include "physical_types.h"
#include "prepack.h"
#include "user_place_constraints.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

/*
 * @brief Gets the max cluster size that any logical block can have.
 */
static size_t calc_max_cluster_size(const std::vector<t_logical_block_type>& logical_block_types) {
    size_t max_cluster_size = 0;
    for (const t_logical_block_type& blk_type : logical_block_types) {
        if (is_empty_type(&blk_type))
            continue;
        int cur_cluster_size = get_max_primitives_in_pb_type(blk_type.pb_type);
        max_cluster_size = std::max<int>(max_cluster_size, cur_cluster_size);
    }
    return max_cluster_size;
}

/*
 * @brief Allocates the stats stored within the pb of a cluster.
 *
 * Used to store information used during clustering.
 */
static void alloc_and_load_pb_stats(t_pb* pb, const int feasible_block_array_size) {
    /* Call this routine when starting to fill up a new cluster.  It resets *
     * the gain vector, etc.                                                */

    pb->pb_stats = new t_pb_stats;

    /* If statement below is for speed.  If nets are reasonably low-fanout,  *
     * only a relatively small number of blocks will be marked, and updating *
     * only those atom block structures will be fastest.  If almost all blocks    *
     * have been touched it should be faster to just run through them all    *
     * in order (less addressing and better cache locality).                 */
    pb->pb_stats->input_pins_used = std::vector<std::unordered_map<size_t, AtomNetId>>(pb->pb_graph_node->num_input_pin_class);
    pb->pb_stats->output_pins_used = std::vector<std::unordered_map<size_t, AtomNetId>>(pb->pb_graph_node->num_output_pin_class);
    pb->pb_stats->lookahead_input_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_input_pin_class);
    pb->pb_stats->lookahead_output_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_output_pin_class);
    pb->pb_stats->num_feasible_blocks = NOT_VALID;
    pb->pb_stats->feasible_blocks = new t_pack_molecule*[feasible_block_array_size];

    for (int i = 0; i < feasible_block_array_size; i++)
        pb->pb_stats->feasible_blocks[i] = nullptr;

    pb->pb_stats->tie_break_high_fanout_net = AtomNetId::INVALID();

    pb->pb_stats->pulled_from_atom_groups = 0;
    pb->pb_stats->num_att_group_atoms_used = 0;

    pb->pb_stats->gain.clear();
    pb->pb_stats->timinggain.clear();
    pb->pb_stats->connectiongain.clear();
    pb->pb_stats->sharinggain.clear();
    pb->pb_stats->hillgain.clear();
    pb->pb_stats->transitive_fanout_candidates.clear();

    pb->pb_stats->num_pins_of_net_in_pb.clear();

    pb->pb_stats->num_child_blocks_in_pb = 0;

    pb->pb_stats->explore_transitive_fanout = true;
}

/*
 * @brief Check the atom blocks of a cluster pb. Used in the verify method.
 */
/* TODO: May want to check that all atom blocks are actually reached */
static void check_cluster_atom_blocks(t_pb* pb, std::unordered_set<AtomBlockId>& blocks_checked) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;
    if (pb_type->num_modes == 0) {
        /* primitive */
        AtomBlockId blk_id = atom_ctx.lookup.pb_atom(pb);
        if (blk_id) {
            if (blocks_checked.count(blk_id)) {
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "pb %s contains atom block %s but atom block is already contained in another pb.\n",
                                pb->name, atom_ctx.nlist.block_name(blk_id).c_str());
            }
            blocks_checked.insert(blk_id);
            if (pb != atom_ctx.lookup.atom_pb(blk_id)) {
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "pb %s contains atom block %s but atom block does not link to pb.\n",
                                pb->name, atom_ctx.nlist.block_name(blk_id).c_str());
            }
        }
    } else {
        /* this is a container pb, all container pbs must contain children */
        bool has_child = false;
        for (int i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
            for (int j = 0; j < pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
                if (pb->child_pbs[i] != nullptr) {
                    if (pb->child_pbs[i][j].name != nullptr) {
                        has_child = true;
                        check_cluster_atom_blocks(&pb->child_pbs[i][j], blocks_checked);
                    }
                }
            }
        }
        VTR_ASSERT(has_child);
    }
}

/// @brief Recursively frees the pb stats of the given pb, without freeing the
///        pb itself.
static void free_pb_stats_recursive(t_pb* pb) {
    /* Releases all the memory used by clustering data structures.   */
    if (pb) {
        if (pb->pb_graph_node != nullptr) {
            if (!pb->pb_graph_node->is_primitive()) {
                for (int i = 0; i < pb->pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children; i++) {
                    for (int j = 0; j < pb->pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
                        if (pb->child_pbs && pb->child_pbs[i]) {
                            free_pb_stats_recursive(&pb->child_pbs[i][j]);
                        }
                    }
                }
            }
        }
        free_pb_stats(pb);
    }
}

/* Record the failure of the molecule in this cluster in the current pb stats.
 * If a molecule fails repeatedly, it's gain will be penalized if packing with
 * attraction groups on. */
static void record_molecule_failure(t_pack_molecule* molecule, t_pb* pb) {
    //Only have to record the failure for the first atom in the molecule.
    //The convention when checking if a molecule has failed to pack in the cluster
    //is to check whether the first atoms has been recorded as having failed

    auto got = pb->pb_stats->atom_failures.find(molecule->atom_block_ids[0]);
    if (got == pb->pb_stats->atom_failures.end()) {
        pb->pb_stats->atom_failures.insert({molecule->atom_block_ids[0], 1});
    } else {
        got->second++;
    }
}

/**
 * @brief Checks whether an atom block can be added to a clustered block
 *        without violating floorplanning constraints. It also updates the
 *        clustered block's floorplanning region by taking the intersection of
 *        its current region and the floorplanning region of the given atom block.
 *
 * @param atom_blk_id               A unique ID for the candidate atom block to
 *                                  be added to the growing cluster.
 * @param cluster_pr                The floorplanning regions of the clustered
 *                                  block. This function may update the given
 *                                  region.
 * @param constraints               The set of user-given place constraints.
 * @param log_verbosity             Controls the detail level of log information
 *                                  printed by this function.
 * @param cluster_pr_needs_update   Indicates whether the floorplanning region
 *                                  of the clustered block have updated.
 *
 * @return True if adding the given atom block to the clustered block does not
 *         violated any floorplanning constraints.
 */
static bool check_cluster_floorplanning(AtomBlockId atom_blk_id,
                                        PartitionRegion& cluster_pr,
                                        const UserPlaceConstraints& constraints,
                                        int log_verbosity,
                                        bool& cluster_pr_needs_update) {
    // Get the partition ID of the atom.
    PartitionId part_id = constraints.get_atom_partition(atom_blk_id);
    // If the partition ID is invalid, then it can be put in the cluster
    // regardless of what the cluster's PartitionRegion is since it is not
    // constrained.
    if (!part_id.is_valid()) {
        VTR_LOGV(log_verbosity > 3,
                 "\t\t\t Intersect: Atom block %d has no floorplanning constraints\n",
                 atom_blk_id);
        cluster_pr_needs_update = false;
        return true;
    }

    // Get the Atom and Cluster Partition Regions
    const PartitionRegion& atom_pr = constraints.get_partition_pr(part_id);

    // If the Cluster's PartitionRegion is empty, then this atom's PR becomes
    // the Cluster's new PartitionRegion.
    if (cluster_pr.empty()) {
        VTR_LOGV(log_verbosity > 3,
                "\t\t\t Intersect: Atom block %d has floorplanning constraints\n",
                atom_blk_id);
        cluster_pr = atom_pr;
        cluster_pr_needs_update = true;
        return true;
    }

    // The Cluster's new PartitionRegion is the intersection of the Cluster's
    // original PartitionRegion and the atom's PartitionRegion.
    update_cluster_part_reg(cluster_pr, atom_pr);

    // If the intersection is empty, then the atom cannot be placed in this
    // Cluster due to floorplanning constraints.
    if (cluster_pr.empty()) {
        VTR_LOGV(log_verbosity > 3,
                "\t\t\t Intersect: Atom block %d failed floorplanning check for cluster\n",
                atom_blk_id);
        cluster_pr_needs_update = false;
        return false;
    }

    // If the Cluster's new PartitionRegion is non-empty, then this atom passes
    // the floorplanning constraints and the cluster's PartitionRegion should be
    // updated.
    cluster_pr_needs_update = true;
    VTR_LOGV(log_verbosity > 3,
             "\t\t\t Intersect: Atom block %d passed cluster, cluster PR was updated with intersection result \n",
             atom_blk_id);
    return true;
}

/**
 * @brief Checks if an atom block can be added to a clustered block without
 *        violating NoC group constraints. For passing this check, either both
 *        clustered and atom blocks must belong to the same NoC group, or at
 *        least one of them should not belong to any NoC group. If the atom block
 *        is associated with a NoC group while the clustered block does not
 *        belong to any NoC groups, the NoC group ID of the atom block is assigned
 *        to the clustered block when the atom is added to it.
 *
 * @param atom_blk_id           A unique ID for the candidate atom block to be
 *                              added to the growing cluster.
 * @param cluster_noc_grp_id    The NoC group ID of the clustered block. This
 *                              function may update this ID.
 * @param atom_noc_grp_ids      A mapping from atoms to NoC group IDs.
 * @param log_verbosity         Controls the detail level of log information
 *                              printed by this function.
 *
 * @return True if adding the atom block the cluster does not violate NoC group
 *         constraints.
 */
static bool check_cluster_noc_group(AtomBlockId atom_blk_id,
                                    NocGroupId& cluster_noc_grp_id,
                                    const vtr::vector<AtomBlockId, NocGroupId>& atom_noc_grp_ids,
                                    int log_verbosity) {
    const NocGroupId atom_noc_grp_id = atom_noc_grp_ids.empty() ? NocGroupId::INVALID() : atom_noc_grp_ids[atom_blk_id];

    if (!cluster_noc_grp_id.is_valid()) {
        // If the cluster does not have a NoC group, assign the atom's NoC group
        // to the cluster.
        VTR_LOGV(log_verbosity > 3,
                 "\t\t\t NoC Group: Atom block %d passed cluster, cluster's NoC group was updated with the atom's group %d\n",
                 atom_blk_id, (size_t)atom_noc_grp_id);
        cluster_noc_grp_id = atom_noc_grp_id;
        return true;
    }

    if (cluster_noc_grp_id == atom_noc_grp_id) {
        // If the cluster has the same NoC group ID as the atom, they are
        // compatible.
        VTR_LOGV(log_verbosity > 3,
                 "\t\t\t NoC Group: Atom block %d passed cluster, cluster's NoC group was compatible with the atom's group %d\n",
                 atom_blk_id, (size_t)atom_noc_grp_id);
        return true;
    }

    // If the cluster belongs to a different NoC group than the atom's group,
    // they are incompatible.
    VTR_LOGV(log_verbosity > 3,
             "\t\t\t NoC Group: Atom block %d failed NoC group check for cluster. Cluster's NoC group: %d, atom's NoC group: %d\n",
             atom_blk_id, (size_t)cluster_noc_grp_id, (size_t)atom_noc_grp_id);
    return false;
}

/**
 * This function takes the root block of a chain molecule and a proposed
 * placement primitive for this block. The function then checks if this
 * chain root block has a placement constraint (such as being driven from
 * outside the cluster) and returns the status of the placement accordingly.
 */
static enum e_block_pack_status check_chain_root_placement_feasibility(const t_pb_graph_node* pb_graph_node,
                                                                const t_pack_molecule* molecule,
                                                                const AtomBlockId blk_id) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    enum e_block_pack_status block_pack_status = e_block_pack_status::BLK_PASSED;

    bool is_long_chain = molecule->chain_info->is_long_chain;

    const auto& chain_root_pins = molecule->pack_pattern->chain_root_pins;

    t_model_ports* root_port = chain_root_pins[0][0]->port->model_port;
    AtomNetId chain_net_id;
    auto port_id = atom_ctx.nlist.find_atom_port(blk_id, root_port);

    if (port_id) {
        chain_net_id = atom_ctx.nlist.port_net(port_id, chain_root_pins[0][0]->pin_number);
    }

    // if this block is part of a long chain or it is driven by a cluster
    // input pin we need to check the placement legality of this block
    // Depending on the logic synthesis even small chains that can fit within one
    // cluster might need to start at the top of the cluster as their input can be
    // driven by a global gnd or vdd. Therefore even if this is not a long chain
    // but its input pin is driven by a net, the placement legality is checked.
    if (is_long_chain || chain_net_id) {
        auto chain_id = molecule->chain_info->chain_id;
        // if this chain has a chain id assigned to it (implies is_long_chain too)
        if (chain_id != -1) {
            // the chosen primitive should be a valid starting point for the chain
            // long chains should only be placed at the top of the chain tieOff = 0
            if (pb_graph_node != chain_root_pins[chain_id][0]->parent_node) {
                block_pack_status = e_block_pack_status::BLK_FAILED_FEASIBLE;
            }
            // the chain doesn't have an assigned chain_id yet
        } else {
            block_pack_status = e_block_pack_status::BLK_FAILED_FEASIBLE;
            for (const auto& chain : chain_root_pins) {
                for (auto tieOff : chain) {
                    // check if this chosen primitive is one of the possible
                    // starting points for this chain.
                    if (pb_graph_node == tieOff->parent_node) {
                        // this location matches with the one of the dedicated chain
                        // input from outside logic block, therefore it is feasible
                        block_pack_status = e_block_pack_status::BLK_PASSED;
                        break;
                    }
                    // long chains should only be placed at the top of the chain tieOff = 0
                    if (is_long_chain) break;
                }
            }
        }
    }

    return block_pack_status;
}

/*
 * @brief Check that the two atom blocks blk_id and sibling_blk_id (which should
 *        both be memory slices) are feasible, in the sence that they have
 *        precicely the same net connections (with the exception of nets in data
 *        port classes).
 *
 * Note that this routine does not check pin feasibility against the cur_pb_type; so
 * primitive_type_feasible() should also be called on blk_id before concluding it is feasible.
 */
static bool primitive_memory_sibling_feasible(const AtomBlockId blk_id, const t_pb_type* cur_pb_type, const AtomBlockId sibling_blk_id) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    VTR_ASSERT(cur_pb_type->class_type == MEMORY_CLASS);

    //First, identify the 'data' ports by looking at the cur_pb_type
    std::unordered_set<t_model_ports*> data_ports;
    for (int iport = 0; iport < cur_pb_type->num_ports; ++iport) {
        const char* port_class = cur_pb_type->ports[iport].port_class;
        if (port_class && strstr(port_class, "data") == port_class) {
            //The port_class starts with "data", so it is a data port

            //Record the port
            data_ports.insert(cur_pb_type->ports[iport].model_port);
        }
    }

    //Now verify that all nets (except those connected to data ports) are equivalent
    //between blk_id and sibling_blk_id

    //Since the atom netlist stores only in-use ports, we iterate over the model to ensure
    //all ports are compared
    const t_model* model = cur_pb_type->model;
    for (t_model_ports* port : {model->inputs, model->outputs}) {
        for (; port; port = port->next) {
            if (data_ports.count(port)) {
                //Don't check data ports
                continue;
            }

            //Note: VPR doesn't support multi-driven nets, so all outputs
            //should be data ports, otherwise the siblings will both be
            //driving the output net

            //Get the ports from each primitive
            auto blk_port_id = atom_ctx.nlist.find_atom_port(blk_id, port);
            auto sib_port_id = atom_ctx.nlist.find_atom_port(sibling_blk_id, port);

            //Check that all nets (including unconnected nets) match
            for (int ipin = 0; ipin < port->size; ++ipin) {
                //The nets are initialized as invalid (i.e. disconnected)
                AtomNetId blk_net_id;
                AtomNetId sib_net_id;

                //We can get the actual net provided the port exists
                //
                //Note that if the port did not exist, the net is left
                //as invalid/disconneced
                if (blk_port_id) {
                    blk_net_id = atom_ctx.nlist.port_net(blk_port_id, ipin);
                }
                if (sib_port_id) {
                    sib_net_id = atom_ctx.nlist.port_net(sib_port_id, ipin);
                }

                //The sibling and block must have the same (possibly disconnected)
                //net on this pin
                if (blk_net_id != sib_net_id) {
                    //Nets do not match, not feasible
                    return false;
                }
            }
        }
    }

    return true;
}

/*
 * @brief Check if the given atom is feasible in the given pb.
 */
static bool primitive_feasible(const AtomBlockId blk_id, t_pb* cur_pb) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    const t_pb_type* cur_pb_type = cur_pb->pb_graph_node->pb_type;

    VTR_ASSERT(cur_pb_type->num_modes == 0); /* primitive */

    AtomBlockId cur_pb_blk_id = atom_ctx.lookup.pb_atom(cur_pb);
    if (cur_pb_blk_id && cur_pb_blk_id != blk_id) {
        /* This pb already has a different logical block */
        return false;
    }

    if (cur_pb_type->class_type == MEMORY_CLASS) {
        /* Memory class has additional feasibility requirements:
         *   - all siblings must share all nets, including open nets, with the exception of data nets */

        /* find sibling if one exists */
        AtomBlockId sibling_memory_blk_id = find_memory_sibling(cur_pb);

        if (sibling_memory_blk_id) {
            //There is a sibling, see if the current block is feasible with it
            bool sibling_feasible = primitive_memory_sibling_feasible(blk_id, cur_pb_type, sibling_memory_blk_id);
            if (!sibling_feasible) {
                return false;
            }
        }
    }

    //Generic feasibility check
    return primitive_type_feasible(blk_id, cur_pb_type);
}

/**
 * Try place atom block into current primitive location
 */
static enum e_block_pack_status
try_place_atom_block_rec(const t_pb_graph_node* pb_graph_node,
                         const AtomBlockId blk_id,
                         t_pb* cb,
                         t_pb** parent,
                         const int max_models,
                         const int max_cluster_size,
                         const LegalizationClusterId cluster_id,
                         vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                         const t_cluster_placement_stats* cluster_placement_stats_ptr,
                         const t_pack_molecule* molecule,
                         t_lb_router_data* router_data,
                         int verbosity,
                         const int feasible_block_array_size) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    AtomContext& mutable_atom_ctx = g_vpr_ctx.mutable_atom();

    VTR_ASSERT_SAFE(cb != nullptr);
    e_block_pack_status block_pack_status = e_block_pack_status::BLK_PASSED;

    /* Discover parent */
    t_pb* parent_pb = nullptr;
    if (pb_graph_node->parent_pb_graph_node != cb->pb_graph_node) {
        t_pb* my_parent = nullptr;
        block_pack_status = try_place_atom_block_rec(pb_graph_node->parent_pb_graph_node, blk_id, cb,
                                                     &my_parent, max_models, max_cluster_size, cluster_id,
                                                     atom_cluster,
                                                     cluster_placement_stats_ptr, molecule, router_data,
                                                     verbosity, feasible_block_array_size);
        parent_pb = my_parent;
    } else {
        parent_pb = cb;
    }

    /* Create siblings if siblings are not allocated */
    VTR_ASSERT(parent_pb != nullptr);
    if (parent_pb->child_pbs == nullptr) {
        VTR_ASSERT(parent_pb->name == nullptr);
        parent_pb->name = vtr::strdup(atom_ctx.nlist.block_name(blk_id).c_str());
        parent_pb->mode = pb_graph_node->pb_type->parent_mode->index;
        set_reset_pb_modes(router_data, parent_pb, true);
        const t_mode* mode = &parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode];
        parent_pb->child_pbs = new t_pb*[mode->num_pb_type_children];

        for (int i = 0; i < mode->num_pb_type_children; i++) {
            parent_pb->child_pbs[i] = new t_pb[mode->pb_type_children[i].num_pb];

            for (int j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                parent_pb->child_pbs[i][j].parent_pb = parent_pb;
                parent_pb->child_pbs[i][j].pb_graph_node = &(parent_pb->pb_graph_node->child_pb_graph_nodes[parent_pb->mode][i][j]);
            }
        }
    } else {
        /* if this is not the first child of this parent, must match existing parent mode */
        if (parent_pb->mode != pb_graph_node->pb_type->parent_mode->index) {
            return e_block_pack_status::BLK_FAILED_FEASIBLE;
        }
    }

    const t_mode* mode = &parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode];
    int i;
    for (i = 0; i < mode->num_pb_type_children; i++) {
        if (pb_graph_node->pb_type == &mode->pb_type_children[i]) {
            break;
        }
    }
    VTR_ASSERT(i < mode->num_pb_type_children);
    t_pb* pb = &parent_pb->child_pbs[i][pb_graph_node->placement_index];
    VTR_ASSERT_SAFE(pb != nullptr);
    *parent = pb; /* this pb is parent of it's child that called this function */
    VTR_ASSERT(pb->pb_graph_node == pb_graph_node);
    if (pb->pb_stats == nullptr) {
        alloc_and_load_pb_stats(pb, feasible_block_array_size);
    }
    const t_pb_type* pb_type = pb_graph_node->pb_type;

    /* Any pb_type under an mode, which is disabled for packing, should not be considerd for mapping
     * Early exit to flag failure
     */
    if (true == pb_type->parent_mode->disable_packing) {
        return e_block_pack_status::BLK_FAILED_FEASIBLE;
    }

    bool is_primitive = (pb_type->num_modes == 0);

    if (is_primitive) {
        VTR_ASSERT(!atom_ctx.lookup.pb_atom(pb)
                   && atom_ctx.lookup.atom_pb(blk_id) == nullptr
                   && atom_cluster[blk_id] == LegalizationClusterId::INVALID());
        /* try pack to location */
        VTR_ASSERT(pb->name == nullptr);
        pb->name = vtr::strdup(atom_ctx.nlist.block_name(blk_id).c_str());

        //Update the atom netlist mappings
        atom_cluster[blk_id] = cluster_id;
        // NOTE: This pb is different from the pb of the cluster. It is the pb
        //       of the actual primitive.
        // TODO: It would be a good idea to remove the use of this global
        //       variables to prevent external users from modifying this by
        //       mistake.
        mutable_atom_ctx.lookup.set_atom_pb(blk_id, pb);

        add_atom_as_target(router_data, blk_id);
        if (!primitive_feasible(blk_id, pb)) {
            /* failed location feasibility check, revert pack */
            block_pack_status = e_block_pack_status::BLK_FAILED_FEASIBLE;
        }

        // if this block passed and is part of a chained molecule
        if (block_pack_status == e_block_pack_status::BLK_PASSED && molecule->is_chain()) {
            auto molecule_root_block = molecule->atom_block_ids[molecule->root];
            // if this is the root block of the chain molecule check its placmeent feasibility
            if (blk_id == molecule_root_block) {
                block_pack_status = check_chain_root_placement_feasibility(pb_graph_node, molecule, blk_id);
            }
        }

        VTR_LOGV(verbosity > 4 && block_pack_status == e_block_pack_status::BLK_PASSED,
                 "\t\t\tPlaced atom '%s' (%s) at %s\n",
                 atom_ctx.nlist.block_name(blk_id).c_str(),
                 atom_ctx.nlist.block_model(blk_id)->name,
                 pb->hierarchical_type_name().c_str());
    }

    if (block_pack_status != e_block_pack_status::BLK_PASSED) {
        free(pb->name);
        pb->name = nullptr;
    }
    return block_pack_status;
}

/* Resets nets used at different pin classes for determining pin feasibility */
static void reset_lookahead_pins_used(t_pb* cur_pb) {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;
    if (cur_pb->pb_stats == nullptr) {
        return; /* No pins used, no need to continue */
    }

    if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
        for (int i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
            cur_pb->pb_stats->lookahead_input_pins_used[i].clear();
        }

        for (int i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
            cur_pb->pb_stats->lookahead_output_pins_used[i].clear();
        }

        if (cur_pb->child_pbs != nullptr) {
            for (int i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i] != nullptr) {
                    for (int j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        reset_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
                    }
                }
            }
        }
    }
}

/*
 * @brief Checks if the sinks of the given net are reachable from the driver
 *        pb gpin.
 */
static int net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    //Record the sink pb graph pins we are looking for
    std::unordered_set<const t_pb_graph_pin*> sink_pb_gpins;
    for (const AtomPinId pin_id : atom_ctx.nlist.net_sinks(net_id)) {
        const t_pb_graph_pin* sink_pb_gpin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
        VTR_ASSERT(sink_pb_gpin);

        sink_pb_gpins.insert(sink_pb_gpin);
    }

    //Count how many sink pins are reachable
    size_t num_reachable_sinks = 0;
    for (int i_prim_pin = 0; i_prim_pin < driver_pb_gpin->num_connectable_primitive_input_pins[depth]; ++i_prim_pin) {
        const t_pb_graph_pin* reachable_pb_gpin = driver_pb_gpin->list_of_connectable_input_pin_ptrs[depth][i_prim_pin];

        if (sink_pb_gpins.count(reachable_pb_gpin)) {
            ++num_reachable_sinks;
            if (num_reachable_sinks == atom_ctx.nlist.net_sinks(net_id).size()) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Returns the pb_graph_pin of the atom pin defined by the driver_pin_id in the driver_pb
 */
static t_pb_graph_pin* get_driver_pb_graph_pin(const t_pb* driver_pb, const AtomPinId driver_pin_id) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    const auto driver_pb_type = driver_pb->pb_graph_node->pb_type;
    int output_port = 0;
    // find the port of the pin driving the net as well as the port model
    auto driver_port_id = atom_ctx.nlist.pin_port(driver_pin_id);
    auto driver_model_port = atom_ctx.nlist.port_model(driver_port_id);
    // find the port id of the port containing the driving pin in the driver_pb_type
    for (int i = 0; i < driver_pb_type->num_ports; i++) {
        auto& prim_port = driver_pb_type->ports[i];
        if (prim_port.type == OUT_PORT) {
            if (prim_port.model_port == driver_model_port) {
                // get the output pb_graph_pin driving this input net
                return &(driver_pb->pb_graph_node->output_pins[output_port][atom_ctx.nlist.pin_port_bit(driver_pin_id)]);
            }
            output_port++;
        }
    }
    // the pin should be found
    VTR_ASSERT(false);
    return nullptr;
}

/**
 * Given a pin and its assigned net, mark all pin classes that are affected.
 * Check if connecting this pin to it's driver pin or to all sink pins will
 * require leaving a pb_block starting from the parent pb_block of the
 * primitive till the root block (depth = 0). If leaving a pb_block is
 * required add this net to the pin class (to increment the number of used
 * pins from this class) that should be used to leave the pb_block.
 */
static void compute_and_mark_lookahead_pins_used_for_pin(const t_pb_graph_pin* pb_graph_pin,
                                                         const t_pb* primitive_pb,
                                                         const AtomNetId net_id,
                                                         const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    // starting from the parent pb of the input primitive go up in the hierarchy till the root block
    for (auto cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const auto depth = cur_pb->pb_graph_node->pb_type->depth;
        const auto pin_class = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(pin_class != OPEN);

        const auto driver_blk_id = atom_ctx.nlist.net_driver_block(net_id);

        // if this primitive pin is an input pin
        if (pb_graph_pin->port->type == IN_PORT) {
            /* find location of net driver if exist in clb, NULL otherwise */
            // find the driver of the input net connected to the pin being studied
            const auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);
            // find the id of the atom occupying the input primitive_pb
            const auto prim_blk_id = atom_ctx.lookup.pb_atom(primitive_pb);
            // find the pb block occupied by the driving atom
            const auto driver_pb = atom_ctx.lookup.atom_pb(driver_blk_id);
            // pb_graph_pin driving net_id in the driver pb block
            t_pb_graph_pin* output_pb_graph_pin = nullptr;
            // if the driver block is in the same clb as the input primitive block
            LegalizationClusterId driver_cluster_id = atom_cluster[driver_blk_id];
            LegalizationClusterId prim_cluster_id = atom_cluster[prim_blk_id];
            if (driver_cluster_id == prim_cluster_id) {
                // get pb_graph_pin driving the given net
                output_pb_graph_pin = get_driver_pb_graph_pin(driver_pb, driver_pin_id);
            }

            bool is_reachable = false;

            // if the driver pin is within the cluster
            if (output_pb_graph_pin) {
                // find if the driver pin can reach the input pin of the primitive or not
                const t_pb* check_pb = driver_pb;
                while (check_pb && check_pb != cur_pb) {
                    check_pb = check_pb->parent_pb;
                }
                if (check_pb) {
                    for (int i = 0; i < output_pb_graph_pin->num_connectable_primitive_input_pins[depth]; i++) {
                        if (pb_graph_pin == output_pb_graph_pin->list_of_connectable_input_pin_ptrs[depth][i]) {
                            is_reachable = true;
                            break;
                        }
                    }
                }
            }

            // Must use an input pin to connect the driver to the input pin of the given primitive, either the
            // driver atom is not contained in the cluster or is contained but cannot reach the primitive pin
            if (!is_reachable) {
                // add net to lookahead_input_pins_used if not already added
                auto it = std::find(cur_pb->pb_stats->lookahead_input_pins_used[pin_class].begin(),
                                    cur_pb->pb_stats->lookahead_input_pins_used[pin_class].end(), net_id);
                if (it == cur_pb->pb_stats->lookahead_input_pins_used[pin_class].end()) {
                    cur_pb->pb_stats->lookahead_input_pins_used[pin_class].push_back(net_id);
                }
            }
        } else {
            VTR_ASSERT(pb_graph_pin->port->type == OUT_PORT);
            /*
             * Determine if this net (which is driven from within this cluster) leaves this cluster
             * (and hence uses an output pin).
             */

            bool net_exits_cluster = true;
            int num_net_sinks = static_cast<int>(atom_ctx.nlist.net_sinks(net_id).size());

            if (pb_graph_pin->num_connectable_primitive_input_pins[depth] >= num_net_sinks) {
                //It is possible the net is completely absorbed in the cluster,
                //since this pin could (potentially) drive all the net's sinks

                /* Important: This runtime penalty looks a lot scarier than it really is.
                 * For high fan-out nets, I at most look at the number of pins within the
                 * cluster which limits runtime.
                 *
                 * DO NOT REMOVE THIS INITIAL FILTER WITHOUT CAREFUL ANALYSIS ON RUNTIME!!!
                 *
                 * Key Observation:
                 * For LUT-based designs it is impossible for the average fanout to exceed
                 * the number of LUT inputs so it's usually around 4-5 (pigeon-hole argument,
                 * if the average fanout is greater than the number of LUT inputs, where do
                 * the extra connections go?  Therefore, average fanout must be capped to a
                 * small constant where the constant is equal to the number of LUT inputs).
                 * The real danger to runtime is when the number of sinks of a net gets doubled
                 */

                //Check if all the net sinks are, in fact, inside this cluster
                bool all_sinks_in_cur_cluster = true;
                LegalizationClusterId driver_cluster = atom_cluster[driver_blk_id];
                for (auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {
                    auto sink_blk_id = atom_ctx.nlist.pin_block(pin_id);
                    if (atom_cluster[sink_blk_id] != driver_cluster) {
                        all_sinks_in_cur_cluster = false;
                        break;
                    }
                }

                if (all_sinks_in_cur_cluster) {
                    //All the sinks are part of this cluster, so the net may be fully absorbed.
                    //
                    //Verify this, by counting the number of net sinks reachable from the driver pin.
                    //If the count equals the number of net sinks then the net is fully absorbed and
                    //the net does not exit the cluster
                    /* TODO: I should cache the absorbed outputs, once net is absorbed,
                     *       net is forever absorbed, no point in rechecking every time */
                    if (net_sinks_reachable_in_cluster(pb_graph_pin, depth, net_id)) {
                        //All the sinks are reachable inside the cluster
                        net_exits_cluster = false;
                    }
                }
            }

            if (net_exits_cluster) {
                /* This output must exit this cluster */
                cur_pb->pb_stats->lookahead_output_pins_used[pin_class].push_back(net_id);
            }
        }
    }
}


/* Determine if pins of speculatively packed pb are legal */
static void compute_and_mark_lookahead_pins_used(const AtomBlockId blk_id,
                                                 const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    const t_pb* cur_pb = atom_ctx.lookup.atom_pb(blk_id);
    VTR_ASSERT(cur_pb != nullptr);

    /* Walk through inputs, outputs, and clocks marking pins off of the same class */
    for (auto pin_id : atom_ctx.nlist.block_pins(blk_id)) {
        auto net_id = atom_ctx.nlist.pin_net(pin_id);

        const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
        compute_and_mark_lookahead_pins_used_for_pin(pb_graph_pin, cur_pb, net_id, atom_cluster);
    }
}

/* Determine if speculatively packed cur_pb is pin feasible
 * Runtime is actually not that bad for this.  It's worst case O(k^2) where k is the
 * number of pb_graph pins.  Can use hash tables or make incremental if becomes an issue.
 */
static void try_update_lookahead_pins_used(t_pb* cur_pb,
                                           const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    // run recursively till a leaf (primitive) pb block is reached
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;
    if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
        if (cur_pb->child_pbs != nullptr) {
            for (int i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i] != nullptr) {
                    for (int j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        try_update_lookahead_pins_used(&cur_pb->child_pbs[i][j], atom_cluster);
                    }
                }
            }
        }
    } else {
        // find if this child (primitive) pb block has an atom mapped to it,
        // if yes compute and mark lookahead pins used for that pb block
        AtomBlockId blk_id = atom_ctx.lookup.pb_atom(cur_pb);
        if (pb_type->blif_model != nullptr && blk_id) {
            compute_and_mark_lookahead_pins_used(blk_id, atom_cluster);
        }
    }
}

/* Check if the number of available inputs/outputs for a pin class is sufficient for speculatively packed blocks */
static bool check_lookahead_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    if (pb_type->num_modes > 0 && cur_pb->name) {
        for (int i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
            size_t class_size = cur_pb->pb_graph_node->input_pin_class_size[i];

            if (cur_pb->is_root()) {
                // Scale the class size by the maximum external pin utilization factor
                // Use ceil to avoid classes of size 1 from being scaled to zero
                class_size = std::ceil(max_external_pin_util.input_pin_util * class_size);
                // if the number of pins already used is larger than class size, then the number of
                // cluster inputs already used should be our constraint. Why is this needed? This is
                // needed since when packing the seed block the maximum external pin utilization is
                // used as 1.0 allowing molecules that are using up to all the cluster inputs to be
                // packed legally. Therefore, if the seed block is already using more inputs than
                // the allowed maximum utilization, this should become the new maximum pin utilization.
                class_size = std::max<size_t>(class_size, cur_pb->pb_stats->input_pins_used[i].size());
            }

            if (cur_pb->pb_stats->lookahead_input_pins_used[i].size() > class_size) {
                return false;
            }
        }

        for (int i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
            size_t class_size = cur_pb->pb_graph_node->output_pin_class_size[i];
            if (cur_pb->is_root()) {
                // Scale the class size by the maximum external pin utilization factor
                // Use ceil to avoid classes of size 1 from being scaled to zero
                class_size = std::ceil(max_external_pin_util.output_pin_util * class_size);
                // if the number of pins already used is larger than class size, then the number of
                // cluster outputs already used should be our constraint. Why is this needed? This is
                // needed since when packing the seed block the maximum external pin utilization is
                // used as 1.0 allowing molecules that are using up to all the cluster inputs to be
                // packed legally. Therefore, if the seed block is already using more inputs than
                // the allowed maximum utilization, this should become the new maximum pin utilization.
                class_size = std::max<size_t>(class_size, cur_pb->pb_stats->output_pins_used[i].size());
            }

            if (cur_pb->pb_stats->lookahead_output_pins_used[i].size() > class_size) {
                return false;
            }
        }

        if (cur_pb->child_pbs) {
            for (int i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i]) {
                    for (int j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        if (!check_lookahead_pins_used(&cur_pb->child_pbs[i][j], max_external_pin_util))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

/**
 * This function takes a chain molecule, and the pb_graph_node that is chosen
 * for packing the molecule's root block. Using the given root_primitive, this
 * function will identify which chain id this molecule is being mapped to and
 * will update the chain id value inside the chain info data structure of this
 * molecule
 */
static void update_molecule_chain_info(t_pack_molecule* chain_molecule, const t_pb_graph_node* root_primitive) {
    VTR_ASSERT(chain_molecule->chain_info->chain_id == -1 && chain_molecule->chain_info->is_long_chain);

    auto chain_root_pins = chain_molecule->pack_pattern->chain_root_pins;

    // long chains should only be placed at the beginning of the chain
    // Since for long chains the molecule size is already equal to the
    // total number of adders in the cluster. Therefore, it should
    // always be placed at the very first adder in this cluster.
    for (size_t chainId = 0; chainId < chain_root_pins.size(); chainId++) {
        if (chain_root_pins[chainId][0]->parent_node == root_primitive) {
            chain_molecule->chain_info->chain_id = chainId;
            chain_molecule->chain_info->first_packed_molecule = chain_molecule;
            return;
        }
    }

    VTR_ASSERT(false);
}

/* Revert trial atom block iblock and free up memory space accordingly
 */
static void revert_place_atom_block(const AtomBlockId blk_id,
                                    t_lb_router_data* router_data,
                                    const Prepacker& prepacker,
                                    vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    AtomContext& mutable_atom_ctx = g_vpr_ctx.mutable_atom();

    //We cast away const here since we may free the pb, and it is
    //being removed from the active mapping.
    //
    //In general most code works fine accessing cosnt t_pb*,
    //which is why we store them as such in atom_ctx.lookup
    t_pb* pb = const_cast<t_pb*>(atom_ctx.lookup.atom_pb(blk_id));

    if (pb != nullptr) {
        /* When freeing molecules, the current block might already have been freed by a prior revert
         * When this happens, no need to do anything beyond basic book keeping at the atom block
         */

        t_pb* next = pb->parent_pb;
        revalid_molecules(pb, prepacker);
        free_pb(pb);
        pb = next;

        while (pb != nullptr) {
            /* If this is pb is created only for the purposes of holding new molecule, remove it
             * Must check if cluster is already freed (which can be the case)
             */
            next = pb->parent_pb;

            if (pb->child_pbs != nullptr && pb->pb_stats != nullptr
                && pb->pb_stats->num_child_blocks_in_pb == 0) {
                set_reset_pb_modes(router_data, pb, false);
                if (next != nullptr) {
                    /* If the code gets here, then that means that placing the initial seed molecule
                     * failed, don't free the actual complex block itself as the seed needs to find
                     * another placement */
                    revalid_molecules(pb, prepacker);
                    free_pb(pb);
                }
            }
            pb = next;
        }
    }

    //Update the atom netlist mapping
    atom_cluster[blk_id] = LegalizationClusterId::INVALID();
    mutable_atom_ctx.lookup.set_atom_pb(blk_id, nullptr);
}

/* Speculation successful, commit input/output pins used */
static void commit_lookahead_pins_used(t_pb* cur_pb) {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    if (pb_type->num_modes > 0 && cur_pb->name) {
        for (int i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
            VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->input_pin_class_size[i]);
            for (size_t j = 0; j < cur_pb->pb_stats->lookahead_input_pins_used[i].size(); j++) {
                VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i][j]);
                cur_pb->pb_stats->input_pins_used[i].insert({j, cur_pb->pb_stats->lookahead_input_pins_used[i][j]});
            }
        }

        for (int i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
            VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->output_pin_class_size[i]);
            for (size_t j = 0; j < cur_pb->pb_stats->lookahead_output_pins_used[i].size(); j++) {
                VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i][j]);
                cur_pb->pb_stats->output_pins_used[i].insert({j, cur_pb->pb_stats->lookahead_output_pins_used[i][j]});
            }
        }

        if (cur_pb->child_pbs) {
            for (int i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i]) {
                    for (int j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        commit_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
                    }
                }
            }
        }
    }
}

/**
 * Cleans up a pb after unsuccessful molecule packing
 *
 * Recursively frees pbs from a t_pb tree. The given root pb itself is not
 * deleted.
 *
 * If a pb object has its children allocated then before freeing them the
 * function checks if there is no atom that corresponds to any of them. The
 * check is performed only for leaf (primitive) pbs. The function recurses for
 * non-primitive pbs.
 *
 * The cleaning itself includes deleting all child pbs, resetting mode of the
 * pb and also freeing its name. This prepares the pb for another round of
 * molecule packing tryout.
 */
static bool cleanup_pb(t_pb* pb) {
    bool can_free = true;

    /* Recursively check if there are any children with already assigned atoms */
    if (pb->child_pbs != nullptr) {
        const t_mode* mode = &pb->pb_graph_node->pb_type->modes[pb->mode];
        VTR_ASSERT(mode != nullptr);

        /* Check each mode */
        for (int i = 0; i < mode->num_pb_type_children; ++i) {
            /* Check each child */
            if (pb->child_pbs[i] != nullptr) {
                for (int j = 0; j < mode->pb_type_children[i].num_pb; ++j) {
                    t_pb* pb_child = &pb->child_pbs[i][j];
                    t_pb_type* pb_type = pb_child->pb_graph_node->pb_type;

                    /* Primitive, check occupancy */
                    if (pb_type->num_modes == 0) {
                        if (pb_child->name != nullptr) {
                            can_free = false;
                        }
                    }

                    /* Non-primitive, recurse */
                    else {
                        if (!cleanup_pb(pb_child)) {
                            can_free = false;
                        }
                    }
                }
            }
        }

        /* Free if can */
        if (can_free) {
            for (int i = 0; i < mode->num_pb_type_children; ++i) {
                if (pb->child_pbs[i] != nullptr) {
                    delete[] pb->child_pbs[i];
                }
            }

            delete[] pb->child_pbs;
            pb->child_pbs = nullptr;
            pb->mode = 0;

            if (pb->name) {
                free(pb->name);
                pb->name = nullptr;
            }
        }
    }

    return can_free;
}

e_block_pack_status ClusterLegalizer::try_pack_molecule(t_pack_molecule* molecule,
                                                        LegalizationCluster& cluster,
                                                        LegalizationClusterId cluster_id,
                                                        const t_ext_pin_util& max_external_pin_util) {
    // Try to pack the molecule into a cluster with this pb type.

    // Safety debugs.
    VTR_ASSERT_DEBUG(molecule != nullptr);
    VTR_ASSERT_DEBUG(cluster.pb != nullptr);
    VTR_ASSERT_DEBUG(cluster.type != nullptr);

    // TODO: Remove these global accesses.
    // AtomContext used for:
    //  - printing verbose statements
    //  - Looking up the primitive pb
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    // FloorplanningContext used for:
    //  - Checking if the atom can be placed in the cluster for floorplanning
    //    constraints.
    const FloorplanningContext& floorplanning_ctx = g_vpr_ctx.floorplanning();
    if (log_verbosity_ > 3) {
        AtomBlockId root_atom = molecule->atom_block_ids[molecule->root];
        VTR_LOG("\t\tTry pack molecule: '%s' (%s)",
                atom_ctx.nlist.block_name(root_atom).c_str(),
                atom_ctx.nlist.block_model(root_atom)->name);
        VTR_LOGV(molecule->pack_pattern,
                 " molecule_type %s molecule_size %zu",
                 molecule->pack_pattern->name,
                 molecule->atom_block_ids.size());
        VTR_LOG("\n");
    }

    // if this cluster has a molecule placed in it that is part of a long chain
    // (a chain that consists of more than one molecule), don't allow more long chain
    // molecules to be placed in this cluster. To avoid possibly creating cluster level
    // blocks that have incompatible placement constraints or form very long placement
    // macros that limit placement flexibility.
    t_cluster_placement_stats* cluster_placement_stats_ptr = &(cluster_placement_stats_[cluster.type->index]);
    if (cluster_placement_stats_ptr->has_long_chain && molecule->is_chain() && molecule->chain_info->is_long_chain) {
        VTR_LOGV(log_verbosity_ > 4, "\t\t\tFAILED Placement Feasibility Filter: Only one long chain per cluster is allowed\n");
        //Record the failure of this molecule in the current pb stats
        record_molecule_failure(molecule, cluster.pb);
        // Free the allocated data.
        return e_block_pack_status::BLK_FAILED_FEASIBLE;
    }

    // Check if every atom in the molecule is legal in the cluster from a
    // floorplanning perspective
    bool cluster_pr_update_check = false;
    PartitionRegion new_cluster_pr = cluster.pr;
    // TODO: This can be made more efficient by pre-computing the intersection
    //       of all the atoms' PRs in the molecule.
    int molecule_size = get_array_size_of_molecule(molecule);
    for (int i_mol = 0; i_mol < molecule_size; i_mol++) {
        // Try to intersect with atom PartitionRegion if atom exists
        AtomBlockId atom_blk_id = molecule->atom_block_ids[i_mol];
        if (atom_blk_id) {
            bool cluster_pr_needs_update = false;
            bool block_pack_floorplan_status = check_cluster_floorplanning(atom_blk_id,
                                                                           new_cluster_pr,
                                                                           floorplanning_ctx.constraints,
                                                                           log_verbosity_,
                                                                           cluster_pr_needs_update);
            if (!block_pack_floorplan_status) {
                // Record the failure of this molecule in the current pb stats
                record_molecule_failure(molecule, cluster.pb);
                return e_block_pack_status::BLK_FAILED_FLOORPLANNING;
            }

            if (cluster_pr_needs_update) {
                cluster_pr_update_check = true;
            }
        }
    }

    // Check if all atoms in the molecule can be added to the cluster without
    // NoC group conflicts
    NocGroupId new_cluster_noc_grp_id = cluster.noc_grp_id;
    for (int i_mol = 0; i_mol < molecule_size; i_mol++) {
        AtomBlockId atom_blk_id = molecule->atom_block_ids[i_mol];
        if (atom_blk_id) {
            bool block_pack_noc_grp_status = check_cluster_noc_group(atom_blk_id,
                                                                     new_cluster_noc_grp_id,
                                                                     atom_noc_grp_id_,
                                                                     log_verbosity_);
            if (!block_pack_noc_grp_status) {
                // Record the failure of this molecule in the current pb stats
                record_molecule_failure(molecule, cluster.pb);
                return e_block_pack_status::BLK_FAILED_NOC_GROUP;
            }
        }
    }

    std::vector<t_pb_graph_node*> primitives_list(max_molecule_size_, nullptr);
    e_block_pack_status block_pack_status = e_block_pack_status::BLK_STATUS_UNDEFINED;
    while (block_pack_status != e_block_pack_status::BLK_PASSED) {
        if (!get_next_primitive_list(cluster_placement_stats_ptr,
                                     molecule,
                                     primitives_list.data())) {
            VTR_LOGV(log_verbosity_ > 3, "\t\tFAILED No candidate primitives available\n");
            block_pack_status = e_block_pack_status::BLK_FAILED_FEASIBLE;
            break; /* no more candidate primitives available, this molecule will not pack, return fail */
        }

        block_pack_status = e_block_pack_status::BLK_PASSED;
        int failed_location = 0;
        for (int i_mol = 0; i_mol < molecule_size && block_pack_status == e_block_pack_status::BLK_PASSED; i_mol++) {
            VTR_ASSERT((primitives_list[i_mol] == nullptr) == (!molecule->atom_block_ids[i_mol]));
            failed_location = i_mol + 1;
            AtomBlockId atom_blk_id = molecule->atom_block_ids[i_mol];
            if (!atom_blk_id.is_valid())
                continue;
            // NOTE: This parent variable is only used in the recursion of this
            //       function.
            t_pb* parent = nullptr;
            block_pack_status = try_place_atom_block_rec(primitives_list[i_mol],
                                                         atom_blk_id,
                                                         cluster.pb,
                                                         &parent,
                                                         num_models_,
                                                         max_cluster_size_,
                                                         cluster_id,
                                                         atom_cluster_,
                                                         cluster_placement_stats_ptr,
                                                         molecule,
                                                         cluster.router_data,
                                                         log_verbosity_,
                                                         feasible_block_array_size_);
        }

        if (enable_pin_feasibility_filter_ && block_pack_status == e_block_pack_status::BLK_PASSED) {
            // Check if pin usage is feasible for the current packing assignment
            reset_lookahead_pins_used(cluster.pb);
            try_update_lookahead_pins_used(cluster.pb, atom_cluster_);
            if (!check_lookahead_pins_used(cluster.pb, max_external_pin_util)) {
                VTR_LOGV(log_verbosity_ > 4, "\t\t\tFAILED Pin Feasibility Filter\n");
                block_pack_status = e_block_pack_status::BLK_FAILED_FEASIBLE;
            } else {
                VTR_LOGV(log_verbosity_ > 3, "\t\t\tPin Feasibility: Passed pin feasibility filter\n");
            }
        }

        if (block_pack_status == e_block_pack_status::BLK_PASSED) {
            /*
             * during the clustering step of `do_clustering`, `detailed_routing_stage` is incremented at each iteration until it a cluster
             * is correctly generated or `detailed_routing_stage` assumes an invalid value (E_DETAILED_ROUTE_INVALID).
             * depending on its value we have different behaviors:
             *  - E_DETAILED_ROUTE_AT_END_ONLY: Skip routing if heuristic is to route at the end of packing complex block.
             *  - E_DETAILED_ROUTE_FOR_EACH_ATOM: Try to route if heuristic is to route for every atom. If the clusterer arrives at this stage,
             *                                    it means that more checks have to be performed as the previous stage failed to generate a new cluster.
             *
             * mode_status is a data structure containing the status of the mode selection. Its members are:
             *  - bool is_mode_conflict
             *  - bool try_expand_all_modes
             *  - bool expand_all_modes
             *
             * is_mode_conflict affects this stage. Its value determines whether the cluster failed to pack after a mode conflict issue.
             * It holds a flag that is used to verify whether try_intra_lb_route ended in a mode conflict issue.
             *
             * Until is_mode_conflict is set to FALSE by try_intra_lb_route, the loop re-iterates. If all the available modes are exhausted
             * an error will be thrown during mode conflicts checks (this to prevent infinite loops).
             *
             * If the value is TRUE the cluster has to be re-routed, and its internal pb_graph_nodes will have more restrict choices
             * for what regards the mode that has to be selected.
             *
             * is_mode_conflict is initially set to TRUE, and, unless a mode conflict is found, it is set to false in `try_intra_lb_route`.
             *
             * try_expand_all_modes is set if the node expansion failed to find a valid routing path. The clusterer tries to find another route
             * by using all the modes during node expansion.
             *
             * expand_all_modes is used to enable the expansion of all the nodes using all the possible modes.
             */
            t_mode_selection_status mode_status;
            bool is_routed = false;
            bool do_detailed_routing_stage = (cluster_legalization_strategy_ == ClusterLegalizationStrategy::FULL);
            if (do_detailed_routing_stage) {
                do {
                    reset_intra_lb_route(cluster.router_data);
                    is_routed = try_intra_lb_route(cluster.router_data, log_verbosity_, &mode_status);
                } while (do_detailed_routing_stage && mode_status.is_mode_issue());
            }

            if (do_detailed_routing_stage && !is_routed) {
                /* Cannot pack */
                VTR_LOGV(log_verbosity_ > 4, "\t\t\tFAILED Detailed Routing Legality\n");
                block_pack_status = e_block_pack_status::BLK_FAILED_ROUTE;
            } else {
                /* Pack successful, commit
                 * TODO: SW Engineering note - may want to update cluster stats here too instead of doing it outside
                 */
                VTR_ASSERT(block_pack_status == e_block_pack_status::BLK_PASSED);
                if (molecule->is_chain()) {
                    /* Chained molecules often take up lots of area and are important,
                     * if a chain is packed in, want to rename logic block to match chain name */
                    AtomBlockId chain_root_blk_id = molecule->atom_block_ids[molecule->pack_pattern->root_block->block_id];
                    t_pb* cur_pb = atom_ctx.lookup.atom_pb(chain_root_blk_id)->parent_pb;
                    while (cur_pb != nullptr) {
                        free(cur_pb->name);
                        cur_pb->name = vtr::strdup(atom_ctx.nlist.block_name(chain_root_blk_id).c_str());
                        cur_pb = cur_pb->parent_pb;
                    }
                    // if this molecule is part of a chain, mark the cluster as having a long chain
                    // molecule. Also check if it's the first molecule in the chain to be packed.
                    // If so, update the chain id for this chain of molecules to make sure all
                    // molecules will be packed to the same chain id and can reach each other using
                    // the chain direct links between clusters
                    if (molecule->chain_info->is_long_chain) {
                        cluster_placement_stats_ptr->has_long_chain = true;
                        if (molecule->chain_info->chain_id == -1) {
                            update_molecule_chain_info(molecule, primitives_list[molecule->root]);
                        }
                    }
                }

                //update cluster PartitionRegion if atom with floorplanning constraints was added
                if (cluster_pr_update_check) {
                    cluster.pr = new_cluster_pr;
                    VTR_LOGV(log_verbosity_ > 2, "\nUpdated PartitionRegion of cluster\n");
                }

                // Update the cluster's NoC group ID. This is cheap so it does
                // not need the check like the what the PR did above.
                cluster.noc_grp_id = new_cluster_noc_grp_id;

                // Insert the molecule into the cluster for bookkeeping.
                cluster.molecules.insert(molecule);

                for (int i = 0; i < molecule_size; i++) {
                    AtomBlockId atom_blk_id = molecule->atom_block_ids[i];
                    if (!atom_blk_id.is_valid())
                        continue;

                    /* invalidate all molecules that share atom block with current molecule */
                    t_pack_molecule* cur_molecule = prepacker_.get_atom_molecule(atom_blk_id);
                    // TODO: This should really be named better. Something like
                    //       "is_clustered". and then it should be set to true.
                    //       Right now, valid implies "not clustered" which is
                    //       confusing.
                    cur_molecule->valid = false;

                    commit_primitive(cluster_placement_stats_ptr, primitives_list[i]);

                    atom_cluster_[atom_blk_id] = cluster_id;

                    // Update the num child blocks in pb
                    const t_pb* atom_pb = atom_ctx.lookup.atom_pb(atom_blk_id);
                    VTR_ASSERT_SAFE(atom_pb != nullptr);
                    t_pb* cur_pb = atom_pb->parent_pb;
                    while (cur_pb != nullptr) {
                        cur_pb->pb_stats->num_child_blocks_in_pb++;
                        cur_pb = cur_pb->parent_pb;
                    }
                }

                // Update the lookahead pins used.
                commit_lookahead_pins_used(cluster.pb);
            }
        }

        if (block_pack_status != e_block_pack_status::BLK_PASSED) {
            /* Pack unsuccessful, undo inserting molecule into cluster */
            for (int i = 0; i < failed_location; i++) {
                AtomBlockId atom_blk_id = molecule->atom_block_ids[i];
                if (atom_blk_id) {
                    remove_atom_from_target(cluster.router_data, atom_blk_id);
                }
            }
            for (int i = 0; i < failed_location; i++) {
                AtomBlockId atom_blk_id = molecule->atom_block_ids[i];
                if (atom_blk_id) {
                    revert_place_atom_block(atom_blk_id, cluster.router_data, prepacker_, atom_cluster_);
                }
            }

            // Record the failure of this molecule in the current pb stats
            record_molecule_failure(molecule, cluster.pb);

            /* Packing failed, but a part of the pb tree is still allocated and pbs have their modes set.
             * Before trying to pack next molecule the unused pbs need to be freed and, the most important,
             * their modes reset. This task is performed by the cleanup_pb() function below. */
            cleanup_pb(cluster.pb);
        } else {
            VTR_LOGV(log_verbosity_ > 3, "\t\tPASSED pack molecule\n");
        }
    }
    return block_pack_status;
}

std::tuple<e_block_pack_status, LegalizationClusterId>
ClusterLegalizer::start_new_cluster(t_pack_molecule* molecule,
                                    t_logical_block_type_ptr cluster_type,
                                    int cluster_mode) {
    // Safety asserts to ensure the API is being called with valid arguments.
    VTR_ASSERT_DEBUG(molecule != nullptr);
    VTR_ASSERT_DEBUG(cluster_type != nullptr);
    VTR_ASSERT_DEBUG(cluster_mode < cluster_type->pb_graph_head->pb_type->num_modes);
    // Ensure that the molecule has not already been placed.
    VTR_ASSERT_SAFE(molecule_cluster_.find(molecule) == molecule_cluster_.end() ||
                    !molecule_cluster_[molecule].is_valid());
    // Safety asserts to ensure that the API was initialized properly.
    VTR_ASSERT_DEBUG(cluster_placement_stats_ != nullptr &&
                     lb_type_rr_graphs_ != nullptr);

    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().nlist;

    // Create the physical block for this cluster based on the type.
    t_pb* cluster_pb = new t_pb;
    cluster_pb->pb_graph_node = cluster_type->pb_graph_head;
    alloc_and_load_pb_stats(cluster_pb, feasible_block_array_size_);
    cluster_pb->parent_pb = nullptr;
    cluster_pb->mode = cluster_mode;

    // Allocate and load the LB router data
    t_lb_router_data* router_data = alloc_and_load_router_data(&lb_type_rr_graphs_[cluster_type->index],
                                                               cluster_type);

    // Reset the cluster placement stats
    t_cluster_placement_stats* cluster_placement_stats_ptr = &(cluster_placement_stats_[cluster_type->index]);
    reset_cluster_placement_stats(cluster_placement_stats_ptr);
    set_mode_cluster_placement_stats(cluster_pb->pb_graph_node, cluster_pb->mode);

    // Create the new cluster
    LegalizationCluster new_cluster;
    new_cluster.pb = cluster_pb;
    new_cluster.router_data = router_data;
    new_cluster.pr = PartitionRegion();
    new_cluster.noc_grp_id = NocGroupId::INVALID();
    new_cluster.type = cluster_type;

    // Try to pack the molecule into the new_cluster.
    // When starting a new cluster, we set the external pin utilization to full
    // (meaning all cluster pins are allowed to be used).
    const t_ext_pin_util FULL_EXTERNAL_PIN_UTIL(1., 1.);
    LegalizationClusterId new_cluster_id = LegalizationClusterId(legalization_cluster_ids_.size());
    e_block_pack_status pack_status = try_pack_molecule(molecule,
                                                        new_cluster,
                                                        new_cluster_id,
                                                        FULL_EXTERNAL_PIN_UTIL);

    if (pack_status == e_block_pack_status::BLK_PASSED) {
        // Give the new cluster pb a name. The current convention is to name the
        // cluster after the root atom of the first molecule packed into it.
        AtomBlockId root_atom = molecule->atom_block_ids[molecule->root];
        const std::string& root_atom_name = atom_nlist.block_name(root_atom);
        if (new_cluster.pb->name != nullptr)
            free(new_cluster.pb->name);
        new_cluster.pb->name = vtr::strdup(root_atom_name.c_str());
        // Move the cluster into the vector of clusters and ids.
        legalization_cluster_ids_.push_back(new_cluster_id);
        legalization_clusters_.push_back(std::move(new_cluster));
        // Update the molecule to cluster map.
        molecule_cluster_[molecule] = new_cluster_id;
    } else {
        // Delete the new_cluster.
        free_pb(new_cluster.pb);
        delete new_cluster.pb;
        free_router_data(new_cluster.router_data);
        new_cluster_id = LegalizationClusterId::INVALID();
    }

    return {pack_status, new_cluster_id};
}

e_block_pack_status ClusterLegalizer::add_mol_to_cluster(t_pack_molecule* molecule,
                                                         LegalizationClusterId cluster_id) {
    // Safety asserts to make sure the inputs are valid.
    VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
    VTR_ASSERT(legalization_cluster_ids_[cluster_id].is_valid() && "Cannot add to a destroyed cluster");
    // Ensure that the molecule has not already been placed.
    VTR_ASSERT(molecule_cluster_.find(molecule) == molecule_cluster_.end() ||
               !molecule_cluster_[molecule].is_valid());
    // Safety asserts to ensure that the API was initialized properly.
    VTR_ASSERT_DEBUG(cluster_placement_stats_ != nullptr &&
                     lb_type_rr_graphs_ != nullptr);

    // Get the cluster.
    LegalizationCluster& cluster = legalization_clusters_[cluster_id];
    VTR_ASSERT(cluster.router_data != nullptr && "Cannot add molecule to cleaned cluster!");
    // Set the target_external_pin_util.
    t_ext_pin_util target_ext_pin_util = target_external_pin_util_.get_pin_util(cluster.type->name);
    // Try to pack the molecule into the cluster.
    e_block_pack_status pack_status = try_pack_molecule(molecule,
                                                        cluster,
                                                        cluster_id,
                                                        target_ext_pin_util);

    // If the packing was successful, set the molecules' cluster to this one.
    if (pack_status == e_block_pack_status::BLK_PASSED)
        molecule_cluster_[molecule] = cluster_id;

    return pack_status;
}

void ClusterLegalizer::destroy_cluster(LegalizationClusterId cluster_id) {
    // Safety asserts to make sure the inputs are valid.
    VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
    VTR_ASSERT(legalization_cluster_ids_[cluster_id].is_valid() && "Cannot destroy an already destroyed cluster");
    // Get the cluster.
    LegalizationCluster& cluster = legalization_clusters_[cluster_id];
    // Remove all molecules from the cluster.
    for (t_pack_molecule* mol : cluster.molecules) {
        VTR_ASSERT_SAFE(molecule_cluster_.find(mol) != molecule_cluster_.end() &&
                        molecule_cluster_[mol] == cluster_id);
        molecule_cluster_[mol] = LegalizationClusterId::INVALID();
        // The overall clustering algorithm uses this valid flag to indicate
        // that a molecule has not been packed (clustered) yet. Since we are
        // destroying a cluster, all of its molecules are now no longer clustered
        // so they are all validated.
        mol->valid = true;
        // Revert the placement of all blocks in the molecule.
        int molecule_size = get_array_size_of_molecule(mol);
        for (int i = 0; i < molecule_size; i++) {
            AtomBlockId atom_blk_id = mol->atom_block_ids[i];
            if (atom_blk_id) {
                revert_place_atom_block(atom_blk_id, cluster.router_data, prepacker_, atom_cluster_);
            }
        }
    }
    cluster.molecules.clear();
    // Free the rest of the cluster data.
    //  Casting things to nullptr for safety just in case someone is trying to use it.
    free_pb(cluster.pb);
    delete cluster.pb;
    cluster.pb = nullptr;
    free_router_data(cluster.router_data);
    cluster.router_data = nullptr;
    cluster.pr = PartitionRegion();

    // Mark the cluster as invalid.
    legalization_cluster_ids_[cluster_id] = LegalizationClusterId::INVALID();
}

void ClusterLegalizer::compress() {
    // Create a map from the old ids to the new (compressed) one.
    vtr::vector_map<LegalizationClusterId, LegalizationClusterId> cluster_id_map;
    cluster_id_map = compress_ids(legalization_cluster_ids_);
    // Update all cluster values.
    legalization_cluster_ids_ = clean_and_reorder_ids(cluster_id_map);
    legalization_clusters_ = clean_and_reorder_values(legalization_clusters_, cluster_id_map);
    // Update the reverse lookups.
    for (auto& it : molecule_cluster_) {
        if (!it.second.is_valid())
            continue;
        molecule_cluster_[it.first] = cluster_id_map[it.second];
    }
    for (size_t i = 0; i < atom_cluster_.size(); i++) {
        AtomBlockId atom_blk_id = AtomBlockId(i);
        LegalizationClusterId old_cluster_id = atom_cluster_[atom_blk_id];
        if (!old_cluster_id.is_valid())
            continue;
        atom_cluster_[atom_blk_id] = cluster_id_map[old_cluster_id];
    }
    // Shrink everything to fit
    legalization_cluster_ids_.shrink_to_fit();
    legalization_clusters_.shrink_to_fit();
    atom_cluster_.shrink_to_fit();
}

void ClusterLegalizer::clean_cluster(LegalizationClusterId cluster_id) {
    // Safety asserts to make sure the inputs are valid.
    VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
    // Get the cluster.
    LegalizationCluster& cluster = legalization_clusters_[cluster_id];
    VTR_ASSERT(cluster.router_data != nullptr && "Should not clean an already cleaned cluster!");
    // Free the pb stats.
    free_pb_stats_recursive(cluster.pb);
    // Load the pb_route so we can free the cluster router data.
    // The pb_route is used when creating a netlist from the legalized clusters.
    std::vector<t_intra_lb_net>* saved_lb_nets = cluster.router_data->saved_lb_nets;
    t_pb_graph_node* pb_graph_node = cluster.pb->pb_graph_node;
    cluster.pb->pb_route = alloc_and_load_pb_route(saved_lb_nets, pb_graph_node);
    // Free the router data.
    free_router_data(cluster.router_data);
    cluster.router_data = nullptr;
}

// TODO: This is fine for the current implementation of the legalizer. But if
//       more complex strategies are added, this will need to be updated to
//       check more than just routing (such as PR and NoC groups).
bool ClusterLegalizer::check_cluster_legality(LegalizationClusterId cluster_id) {
    // Safety asserts to make sure the inputs are valid.
    VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
    // To check if a cluster is fully legal, try to perform an intra logic block
    // route on the cluster. If it succeeds, the cluster is fully legal.
    t_mode_selection_status mode_status;
    LegalizationCluster& cluster = legalization_clusters_[cluster_id];
    return try_intra_lb_route(cluster.router_data, log_verbosity_, &mode_status);
}

ClusterLegalizer::ClusterLegalizer(const AtomNetlist& atom_netlist,
                                   const Prepacker& prepacker,
                                   const std::vector<t_logical_block_type>& logical_block_types,
                                   std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                   size_t num_models,
                                   const std::vector<std::string>& target_external_pin_util_str,
                                   const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                                   ClusterLegalizationStrategy cluster_legalization_strategy,
                                   bool enable_pin_feasibility_filter,
                                   int feasible_block_array_size,
                                   int log_verbosity) : prepacker_(prepacker) {
    // Verify that the inputs are valid.
    VTR_ASSERT_SAFE(lb_type_rr_graphs != nullptr);

    // Resize the atom_cluster lookup to make the accesses much cheaper.
    atom_cluster_.resize(atom_netlist.blocks().size(), LegalizationClusterId::INVALID());
    // Allocate the cluster_placement_stats
    cluster_placement_stats_ = alloc_and_load_cluster_placement_stats();
    // Pre-compute the max size of any molecule.
    max_molecule_size_ = prepacker.get_max_molecule_size();
    // Calculate the max cluster size
    //  - Limit maximum number of elements for each cluster to MAX_SHORT
    max_cluster_size_ = calc_max_cluster_size(logical_block_types);
    VTR_ASSERT(max_cluster_size_ < MAX_SHORT);
    // Get a reference to the rr graphs.
    lb_type_rr_graphs_ = lb_type_rr_graphs;
    // Get the number of models in the architecture.
    num_models_ = num_models;
    // Find all NoC router atoms.
    std::vector<AtomBlockId> noc_atoms = find_noc_router_atoms(atom_netlist);
    update_noc_reachability_partitions(noc_atoms,
                                       atom_netlist,
                                       high_fanout_thresholds,
                                       atom_noc_grp_id_);
    // Copy the options passed by the user
    cluster_legalization_strategy_ = cluster_legalization_strategy;
    enable_pin_feasibility_filter_ = enable_pin_feasibility_filter;
    feasible_block_array_size_ = feasible_block_array_size;
    log_verbosity_ = log_verbosity;
    // Get the target external pin utilization
    // NOTE: This is really silly, but this can potentially fail. If it does
    //       it is important that everything is allocated. If not, when it fails
    //       it will call the reset method when only parts of the class are
    //       allocated which may cause havoc...
    target_external_pin_util_ = t_ext_pin_util_targets(target_external_pin_util_str);
}

void ClusterLegalizer::reset() {
    // Destroy all of the clusters and compress.
    for (LegalizationClusterId cluster_id : legalization_cluster_ids_) {
        if (!cluster_id.is_valid())
            continue;
        destroy_cluster(cluster_id);
    }
    compress();
    // Reset the molecule_cluster map
    molecule_cluster_.clear();
    // Reset the cluster placement stats.
    free_cluster_placement_stats(cluster_placement_stats_);
    cluster_placement_stats_ = alloc_and_load_cluster_placement_stats();
}

void ClusterLegalizer::verify() {
    std::unordered_set<AtomBlockId> atoms_checked;
    auto& atom_ctx = g_vpr_ctx.atom();

    if (clusters().size() == 0) {
        VTR_LOG_WARN("Packing produced no clustered blocks");
    }

    /*
     * Check that each atom block connects to one physical primitive and that the primitive links up to the parent clb
     */
    for (auto blk_id : atom_ctx.nlist.blocks()) {
        //Each atom should be part of a pb
        const t_pb* atom_pb = atom_ctx.lookup.atom_pb(blk_id);
        if (!atom_pb) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Atom block %s is not mapped to a pb\n",
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }

        //Check the reverse mapping is consistent
        if (atom_ctx.lookup.pb_atom(atom_pb) != blk_id) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "pb %s does not contain atom block %s but atom block %s maps to pb.\n",
                            atom_pb->name,
                            atom_ctx.nlist.block_name(blk_id).c_str(),
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }

        VTR_ASSERT(atom_ctx.nlist.block_name(blk_id) == atom_pb->name);

        const t_pb* cur_pb = atom_pb;
        while (cur_pb->parent_pb) {
            cur_pb = cur_pb->parent_pb;
            VTR_ASSERT(cur_pb->name);
        }

        LegalizationClusterId cluster_id = get_atom_cluster(blk_id);
        if (cluster_id == LegalizationClusterId::INVALID()) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Atom %s is not mapped to a CLB\n",
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }

        if (cur_pb != get_cluster_pb(cluster_id)) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "CLB %s does not match CLB contained by pb %s.\n",
                            cur_pb->name, atom_pb->name);
        }
    }

    /* Check that I do not have spurious links in children pbs */
    for (LegalizationClusterId cluster_id : clusters()) {
        if (!cluster_id.is_valid())
            continue;
        check_cluster_atom_blocks(get_cluster_pb(cluster_id),
                                  atoms_checked);
    }

    for (auto blk_id : atom_ctx.nlist.blocks()) {
        if (!atoms_checked.count(blk_id)) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Atom block %s not found in any cluster.\n",
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }
    }
}

void ClusterLegalizer::finalize() {
    for (LegalizationClusterId cluster_id : legalization_cluster_ids_) {
        if (!cluster_id.is_valid())
            continue;
        // If the cluster has not already been cleaned, clean it. This will
        // generate the pb_route necessary for generating a clustered netlist.
        const LegalizationCluster& cluster = legalization_clusters_[cluster_id];
        if (cluster.router_data != nullptr)
            clean_cluster(cluster_id);
    }
}

ClusterLegalizer::~ClusterLegalizer() {
    // Destroy all clusters (no need to compress).
    for (LegalizationClusterId cluster_id : legalization_cluster_ids_) {
        if (!cluster_id.is_valid())
            continue;
        destroy_cluster(cluster_id);
    }
    // Free the cluster_placement_stats
    free_cluster_placement_stats(cluster_placement_stats_);
}

