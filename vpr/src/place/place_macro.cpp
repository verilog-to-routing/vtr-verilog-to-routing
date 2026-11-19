
#include "place_macro.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <sstream>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "atom_lookup.h"
#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "clustered_netlist.h"
#include "physical_types_util.h"
#include "user_relative_macros.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "vpr_utils.h"
#include "vpr_types.h"
#include "vpr_error.h"
#include "physical_types.h"
#include "echo_files.h"

/**
 * @brief Determines whether a cluster net is driven by a constant generator (e.g. gnd/vcc).
 *
 * Nets which were only inferred to be constant are not considered, since they are
 * routed like any other net (see process_constant_nets).
 *
 * @param clb_net The unique id of a cluster net.
 * @return True if the net is driven by a constant generator; otherwise false.
 */
static bool is_constant_clb_net(ClusterNetId clb_net,
                                const AtomLookup& atom_lookup,
                                const AtomNetlist& atom_nlist);

/**
 * @brief Performs a sanity check on macros by making sure that
 * each block appears in at most one macro.
 * @param macros All placement macros in the netlist.
 */
static void validate_macros(const std::vector<t_pl_macro>& macros,
                            const ClusteredNetlist& clb_nlist);

/**
 * @brief   Tries to combine two placement macros.
 * @details This function takes two placement macro ids which have a common cluster block
 * or more in between. The function then tries to find if the two macros could be combined
 * to form a larger macro. If it's impossible to combine the two macros together then
 * this design will never place and route.
 *
 * @param pl_macro_member_blk_num   [0..num_macros-1][0..num_cluster_blocks-1]
 *                                  2D array of macros created so far.
 * @param matching_macro first macro id, which is a previous macro that is found to have the same block
 * @param latest_macro second macro id, which is the macro being created at this iteration
 * @return True if combining two macros was successful; otherwise false.
 */
static bool try_combine_macros(std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num,
                               int matching_macro,
                               int latest_macro);

/**
 * @brief Records which pins belong to one end of a direct connection.
 *
 * Finds the tile type and port named in pin_spec, checking each subtile for a match,
 * and passes each matching port to mark_direct_of_pins(). Reports an error if the
 * range extends past the port and warns about pins shared with another direct.
 *
 * @param idirect Index of this direct in the architecture's direct list.
 * @param direct_type DRIVER for from_pin, RECEIVER for to_pin.
 * @param pin_spec Parsed tile name, port name, and optional pin range.
 * @param src_string Original pin specification, used in error and warning messages.
 * @param line Line number of the direct in the architecture file.
 * @param idirect_from_blk_pin Table of direct indices, indexed by tile type and block pin.
 *                            UNDEFINED means the pin has no assigned direct.
 * @param direct_type_from_blk_pin Table of DRIVER/RECEIVER roles, indexed by tile type and
 *                                block pin. OPEN means the pin has no assigned direct.
 * @param shared_pins Receives shared pins as (tile type index, block pin index) pairs.
 * @param physical_tile_types Physical tile types in the device.
 * @param port_pin_to_block_pin Converts port pin indices to block pin indices.
 */
static void mark_direct_of_ports(int idirect,
                                 e_pin_type direct_type,
                                 const t_direct_pin_spec& pin_spec,
                                 std::string_view src_string,
                                 int line,
                                 std::vector<std::vector<int>>& idirect_from_blk_pin,
                                 std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                                 std::vector<std::pair<int, int>>& shared_pins,
                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                 const PortPinToBlockPinConverter& port_pin_to_block_pin);

/**
 * @brief Records a direct connection for the pins of one port named in pin_spec.
 *
 * Uses the pin range given in pin_spec, or the whole port if no range is given.
 * Records each pin's direct index and DRIVER/RECEIVER role. Only includes pins with
 * Fc = 0 for every routing segment, since pins with access to general routing do not
 * require a placement macro.
 *
 * If a pin already belongs to another direct, leaves its entries unchanged and adds
 * it to shared_pins. The caller clears these pins from the tables after processing
 * all directs, so they cannot be used to form placement macros.
 *
 * @param pin_spec Parsed tile name, port name, and optional pin range. The range must
 *                 fit within the port.
 * @param itype Index of the physical tile type.
 * @param isub_tile Index of the subtile within the tile type.
 * @param iport Index of the port within the subtile.
 * @param idirect_from_blk_pin Table of direct indices, indexed by tile type and block pin.
 *                            UNDEFINED means the pin has no assigned direct.
 * @param idirect Index of this direct in the architecture's direct list.
 * @param direct_type_from_blk_pin Table of DRIVER/RECEIVER roles, indexed by tile type and
 *                                block pin. OPEN means the pin has no assigned direct.
 * @param direct_type DRIVER for from_pin, RECEIVER for to_pin.
 * @param shared_pins Receives shared pins as (tile type index, block pin index) pairs.
 * @param physical_tile_types Physical tile types in the device.
 * @param port_pin_to_block_pin Converts port pin indices to block pin indices.
 * @return Number of eligible pins that already belonged to another direct.
 */
static int mark_direct_of_pins(const t_direct_pin_spec& pin_spec,
                               int itype,
                               int isub_tile,
                               int iport,
                               std::vector<std::vector<int>>& idirect_from_blk_pin,
                               int idirect,
                               std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                               e_pin_type direct_type,
                               std::vector<std::pair<int, int>>& shared_pins,
                               const std::vector<t_physical_tile_type>& physical_tile_types,
                               const PortPinToBlockPinConverter& port_pin_to_block_pin);

const std::vector<t_pl_macro>& PlaceMacros::macros() const {
    return pl_macros_;
}

PlaceMacros::PlaceMacros(const std::vector<t_direct_inf>& directs,
                         const std::vector<t_physical_tile_type>& physical_tile_types,
                         const ClusteredNetlist& clb_nlist,
                         const AtomNetlist& atom_nlist,
                         const AtomLookup& atom_lookup,
                         const UserRelativeMacros& relative_macros) {
    /* Allocates allocates and loads placement macros and returns
     * the total number of macros in 2 steps.
     *   1) Allocate temporary data structure for maximum possible
     *      size and loops through all the blocks storing the data
     *      relevant to the carry chains. At the same time, also count
     *      the amount of memory required for the actual variables.
     *   2) Allocate the actual variables with the exact amount of
     *      memory. Then loads the data from the temporary data
     *      structures before freeing them.
     */

    size_t num_clusters = clb_nlist.blocks().size();

    // Allocate maximum memory for temporary variables.
    std::vector<int> pl_macro_idirect(num_clusters);
    std::vector<int> pl_macro_num_members(num_clusters);
    /* For pl_macro_member_blk_num, Allocate for the first dimension only at first. Allocate for the second dimension
     * when I know the size. Otherwise, the array is going to be of size cluster_ctx.clb_nlist.blocks().size()^2 */
    std::vector<std::vector<ClusterBlockId>> pl_macro_member_blk_num(num_clusters);

    alloc_and_load_idirect_from_blk_pin_(directs, physical_tile_types);

    /* Compute required size:
     * Go through all the pins with possible direct connections in
     * idirect_from_blk_pin_. Count the number of heads (which is the same
     * as the number macros) and also the length of each macro
     * Head - blocks with to_pin OPEN and from_pin connected
     * Tail - blocks with to_pin connected and from_pin OPEN
     */
    const int num_macro = find_all_the_macro_(clb_nlist, atom_nlist, atom_lookup,
                                              pl_macro_idirect, pl_macro_num_members,
                                              pl_macro_member_blk_num);

    // Allocate the memories for the macro.
    pl_macros_.resize(num_macro);

    /* Allocate the memories for the chain members.
     * Load the values from the temporary data structures.
     */
    for (int imacro = 0; imacro < num_macro; imacro++) {
        pl_macros_[imacro].members = std::vector<t_pl_macro_member>(pl_macro_num_members[imacro]);

        // Load the values for each member of the macro
        for (size_t imember = 0; imember < pl_macros_[imacro].members.size(); imember++) {
            pl_macros_[imacro].members[imember].offset.x = imember * directs[pl_macro_idirect[imacro]].x_offset;
            pl_macros_[imacro].members[imember].offset.y = imember * directs[pl_macro_idirect[imacro]].y_offset;
            pl_macros_[imacro].members[imember].offset.sub_tile = directs[pl_macro_idirect[imacro]].sub_tile_offset;
            pl_macros_[imacro].members[imember].blk_index = pl_macro_member_blk_num[imacro][imember];
        }
    }

    append_relative_placement_macros_(relative_macros, clb_nlist, atom_lookup);

    if (isEchoFileEnabled(E_ECHO_PLACE_MACROS)) {
        write_place_macros_(getEchoFileName(E_ECHO_PLACE_MACROS),
                            pl_macros_,
                            physical_tile_types,
                            clb_nlist);
    }

    validate_macros(pl_macros_, clb_nlist);

    alloc_and_load_imacro_from_iblk_(pl_macros_, clb_nlist);
}

/**
 * @brief Finds the cluster that the atoms of each relative placement group
 *        were packed into.
 *
 * All atoms in a group must belong to the same cluster
 * including for loaded .net files. Therefore, any
 * atom in the group can be used to identify its cluster.
 *
 * @return A vector indexed by group ID, holding the cluster of that group.
 */
static vtr::vector<UserRelativeGroupId, ClusterBlockId> find_group_clusters(const UserRelativeMacros& relative_macros,
                                                                            const AtomLookup& atom_lookup) {
    vtr::vector<UserRelativeGroupId, ClusterBlockId> group_cluster(relative_macros.get_num_groups());
    for (UserRelativeGroupId group_id : relative_macros.group_ids()) {
        const t_user_relative_group& group = relative_macros.get_group(group_id);
        VTR_ASSERT(!group.atoms.empty());

        ClusterBlockId clb_id = atom_lookup.atom_clb(group.atoms[0]);
        VTR_ASSERT_SAFE_MSG(clb_id.is_valid(), "Every atom of a relative placement group is clustered");
        group_cluster[group_id] = clb_id;
    }
    return group_cluster;
}

/// @brief A cluster that two macros being merged place at different offsets.
struct t_macro_offset_conflict {
    ClusterBlockId clb_id;   ///< The cluster both macros contain.
    t_pl_offset into_offset; ///< The cluster's offset in the macro being merged into.
    t_pl_offset from_offset; ///< The cluster's offset in the merged macro, rebased onto the head of the macro being merged into.
};

/**
 * @brief Merges the macro `from` into the macro `into`, which share the cluster shared_clb_id.
 *
 * The members of `from` that `into` does not already contain are appended to `into`,
 * with their offsets rebased onto the head of `into` through the shared cluster.
 *
 * @return The first cluster that both macros contain but place at different offsets, if
 *         any. `into` is left unchanged in that case.
 */
static std::optional<t_macro_offset_conflict> merge_macros(t_pl_macro& into,
                                                           const t_pl_macro& from,
                                                           ClusterBlockId shared_clb_id) {
    std::unordered_map<ClusterBlockId, t_pl_offset> into_offset_of_cluster;
    for (const t_pl_macro_member& member : into.members) {
        into_offset_of_cluster.emplace(member.blk_index, member.offset);
    }

    // Rebase the offsets of `from` onto the head of `into` through the shared cluster
    auto into_shared_it = into_offset_of_cluster.find(shared_clb_id);
    auto from_shared_it = std::ranges::find(from.members, shared_clb_id, &t_pl_macro_member::blk_index);
    VTR_ASSERT(into_shared_it != into_offset_of_cluster.end() && from_shared_it != from.members.end());
    t_pl_offset base = into_shared_it->second - from_shared_it->offset;

    std::vector<t_pl_macro_member> new_members;
    for (const t_pl_macro_member& from_member : from.members) {
        t_pl_offset offset = base + from_member.offset;
        auto into_it = into_offset_of_cluster.find(from_member.blk_index);
        if (into_it == into_offset_of_cluster.end()) {
            new_members.push_back({from_member.blk_index, offset});
        } else if (into_it->second != offset) {
            return t_macro_offset_conflict{from_member.blk_index, into_it->second, offset};
        }
    }

    into.members.insert(into.members.end(), new_members.begin(), new_members.end());
    return std::nullopt;
}

void PlaceMacros::append_relative_placement_macros_(const UserRelativeMacros& relative_macros,
                                                    const ClusteredNetlist& clb_nlist,
                                                    const AtomLookup& atom_lookup) {
    const size_t num_relative_placement_macros = relative_macros.get_num_macros();
    if (num_relative_placement_macros == 0)
        return;

    // pl_macros_ holds only the architecture-derived macros here. The relative placement macros
    // are collected in new_macros and appended once the merges are known.
    const size_t num_arch_macros = pl_macros_.size();

    // The index in pl_macros_ of the architecture-derived macro each cluster is a member of, if any
    std::unordered_map<ClusterBlockId, size_t> arch_macro_idx_of_cluster;
    for (size_t iarch = 0; iarch < num_arch_macros; iarch++) {
        for (const t_pl_macro_member& member : pl_macros_[iarch].members) {
            arch_macro_idx_of_cluster.emplace(member.blk_index, iarch);
        }
    }

    vtr::vector<UserRelativeGroupId, ClusterBlockId> group_cluster = find_group_clusters(relative_macros, atom_lookup);

    // The relative placement macro each group's cluster belongs to. The packer puts at most one
    // group into a cluster, which verify_clustering() also checks.
    std::unordered_map<ClusterBlockId, UserRelativeMacroId> relative_placement_macro_of_cluster;
    for (UserRelativeGroupId group_id : relative_macros.group_ids()) {
        bool cluster_unclaimed = relative_placement_macro_of_cluster.emplace(group_cluster[group_id], relative_macros.get_group(group_id).macro_id).second;
        VTR_ASSERT_MSG(cluster_unclaimed, "A cluster holds at most one relative placement group");
    }

    // The relative placement macro each architecture-derived macro has been absorbed into, if any
    std::vector<UserRelativeMacroId> arch_macro_absorbed_by(num_arch_macros, UserRelativeMacroId::INVALID());
    size_t num_absorbed_arch_macros = 0;

    std::vector<t_pl_macro> new_macros;
    for (UserRelativeMacroId macro_id : relative_macros.macro_ids()) {
        const t_user_relative_macro& relative_placement_macro = relative_macros.get_macro(macro_id);

        // The macro's members: the reference group's cluster (the head, at offset zero)
        // followed by the relative groups' clusters
        t_pl_macro pl_macro;
        pl_macro.relative_placement_macro_id = macro_id;
        std::vector<t_pl_macro_member>& members = pl_macro.members;
        for (UserRelativeGroupId group_id : relative_placement_macro.groups) {
            members.push_back({group_cluster[group_id], relative_macros.get_group(group_id).offset});
        }
        // The loader requires a reference group and at least one relative group
        VTR_ASSERT(members.size() >= 2);
        VTR_ASSERT(members[0].offset == t_pl_offset(0, 0, 0, 0));

        // Absorb the architecture-derived macro, if any, that each group's cluster is part of
        for (UserRelativeGroupId group_id : relative_placement_macro.groups) {
            ClusterBlockId shared_clb_id = group_cluster[group_id];
            auto arch_it = arch_macro_idx_of_cluster.find(shared_clb_id);
            if (arch_it == arch_macro_idx_of_cluster.end())
                continue;
            size_t iarch = arch_it->second;

            // A chain can hold several groups of this macro; it is absorbed once
            if (arch_macro_absorbed_by[iarch] == macro_id)
                continue;
            // A chain holding groups of two macros is a fatal error below when the
            // first of them absorbs it, so it is never reached by the second
            VTR_ASSERT(!arch_macro_absorbed_by[iarch].is_valid());
            arch_macro_absorbed_by[iarch] = macro_id;
            num_absorbed_arch_macros++;

            const t_pl_macro& arch_macro = pl_macros_[iarch];
            const std::string& chain_head_name = clb_nlist.block_name(arch_macro.members[0].blk_index);

            // The chain is placed as one unit with this macro, so it cannot hold a group of another macro
            for (const t_pl_macro_member& arch_member : arch_macro.members) {
                auto other_macro_it = relative_placement_macro_of_cluster.find(arch_member.blk_index);
                if (other_macro_it != relative_placement_macro_of_cluster.end() && other_macro_it->second != macro_id) {
                    VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                                    "Relative placement macros '%s' and '%s' both have a group in the chain of directly "
                                    "connected clusters starting at cluster '%s' (e.g. a carry chain), which must be placed as "
                                    "one unit. Put these groups in one macro, or re-run packing if the packed netlist was "
                                    "produced with other constraints.\n",
                                    relative_placement_macro.name.c_str(),
                                    relative_macros.get_macro(other_macro_it->second).name.c_str(),
                                    chain_head_name.c_str());
                }
            }

            std::optional<t_macro_offset_conflict> conflict = merge_macros(pl_macro, arch_macro, shared_clb_id);
            if (conflict.has_value()) {
                // The cluster holds a group of this macro, and the chain and the constraints
                // disagree on its offset
                VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                                "Relative placement macro '%s' places cluster '%s' at offset (%d, %d, sub_tile %d) from "
                                "cluster '%s', but the chain of directly connected clusters starting at cluster '%s' (e.g. "
                                "a carry chain) requires (%d, %d, sub_tile %d). Change the group's offset to match the "
                                "chain, or re-run packing if the packed netlist was produced with other constraints.\n",
                                relative_placement_macro.name.c_str(), clb_nlist.block_name(conflict->clb_id).c_str(),
                                conflict->into_offset.x, conflict->into_offset.y, conflict->into_offset.sub_tile,
                                clb_nlist.block_name(members[0].blk_index).c_str(), chain_head_name.c_str(),
                                conflict->from_offset.x, conflict->from_offset.y, conflict->from_offset.sub_tile);
            }
        }

        // Two clusters at one offset would occupy the same location. The constraints
        // loader rejects this within a macro; an absorbed chain can reintroduce it.
        std::unordered_map<t_pl_offset, ClusterBlockId> cluster_at_offset;
        for (const t_pl_macro_member& member : members) {
            auto [existing_it, inserted] = cluster_at_offset.emplace(member.offset, member.blk_index);
            if (!inserted) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                                "Relative placement macro '%s' puts clusters '%s' and '%s' at the same offset "
                                "(%d, %d, sub_tile %d) from cluster '%s', since one of its groups is in a chain of "
                                "directly connected clusters (e.g. a carry chain) that is placed with it. Change the "
                                "group offsets so they do not collide with the chain.\n",
                                relative_placement_macro.name.c_str(),
                                clb_nlist.block_name(existing_it->second).c_str(),
                                clb_nlist.block_name(member.blk_index).c_str(),
                                member.offset.x, member.offset.y, member.offset.sub_tile,
                                clb_nlist.block_name(members[0].blk_index).c_str());
            }
        }

        new_macros.push_back(std::move(pl_macro));
    }

    // Drop the absorbed architecture-derived macros and append the relative placement macros
    if (num_absorbed_arch_macros > 0) {
        std::vector<t_pl_macro> kept_macros;
        kept_macros.reserve(num_arch_macros - num_absorbed_arch_macros);
        for (size_t iarch = 0; iarch < num_arch_macros; iarch++) {
            if (!arch_macro_absorbed_by[iarch].is_valid())
                kept_macros.push_back(std::move(pl_macros_[iarch]));
        }
        pl_macros_ = std::move(kept_macros);
    }
    for (t_pl_macro& new_macro : new_macros) {
        pl_macros_.push_back(std::move(new_macro));
    }

    VTR_LOG("Created %zu placement macro(s) from the relative placement constraints", num_relative_placement_macros);
    if (num_absorbed_arch_macros > 0) {
        VTR_LOG(", merging in %zu chain(s) of directly connected clusters (e.g. carry chains) they share clusters with", num_absorbed_arch_macros);
    }
    VTR_LOG(".\n");
}

ClusterBlockId PlaceMacros::macro_head(ClusterBlockId blk) const {
    int macro_index = get_imacro_from_iblk(blk);
    if (macro_index == UNDEFINED) {
        return ClusterBlockId::INVALID();
    } else {
        return pl_macros_[macro_index].members[0].blk_index;
    }
}

int PlaceMacros::find_all_the_macro_(const ClusteredNetlist& clb_nlist,
                                     const AtomNetlist& atom_nlist,
                                     const AtomLookup& atom_lookup,
                                     std::vector<int>& pl_macro_idirect,
                                     std::vector<int>& pl_macro_num_members,
                                     std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num) {
    /* Compute required size:                                                *
     * Go through all the pins with possible direct connections in           *
     * idirect_from_blk_pin_. Count the number of heads (which is the same  *
     * as the number macros) and also the length of each macro               *
     * Head - blocks with to_pin OPEN and from_pin connected                 *
     * Tail - blocks with to_pin connected and from_pin OPEN                 */
    std::vector<ClusterBlockId> pl_macro_member_blk_num_of_this_blk(clb_nlist.blocks().size());

    // Hash table holding the unique cluster ids and the macro id it belongs to
    std::unordered_map<ClusterBlockId, int> clusters_macro;

    // counts the total number of macros
    int num_macro = 0;

    for (ClusterBlockId blk_id : clb_nlist.blocks()) {
        t_logical_block_type_ptr logical_block = clb_nlist.block_type(blk_id);
        t_physical_tile_type_ptr physical_tile = pick_physical_type(logical_block);

        int num_blk_pins = clb_nlist.block_type(blk_id)->pb_type->num_pins;
        for (int to_iblk_pin = 0; to_iblk_pin < num_blk_pins; to_iblk_pin++) {
            int to_physical_pin = get_physical_pin(physical_tile, logical_block, to_iblk_pin);

            ClusterNetId to_net_id = clb_nlist.block_net(blk_id, to_iblk_pin);
            int to_idirect = idirect_from_blk_pin_[physical_tile->index][to_physical_pin];
            e_pin_type to_src_or_sink = direct_type_from_blk_pin_[physical_tile->index][to_physical_pin];

            // Identify potential macro head blocks (i.e. start of a macro)
            //
            // The input SINK (to_pin) of a potential HEAD macro will have either:
            //  * no connection to any net (OPEN), or
            //  * a connection to a constant net (e.g. gnd/vcc) which is not driven by a direct
            //
            // Note that the restriction that constant nets are not driven from another direct ensures that
            // blocks in the middle of a chain with internal constant signals are not detected as potential
            // head blocks.
            if (to_src_or_sink == e_pin_type::RECEIVER && to_idirect != UNDEFINED
                && (to_net_id == ClusterNetId::INVALID() || (is_constant_clb_net(to_net_id, atom_lookup, atom_nlist) && !is_net_direct_connection(to_net_id, to_idirect, clb_nlist)))) {
                for (int from_iblk_pin = 0; from_iblk_pin < num_blk_pins; from_iblk_pin++) {
                    int from_physical_pin = get_physical_pin(physical_tile, logical_block, from_iblk_pin);

                    ClusterNetId from_net_id = clb_nlist.block_net(blk_id, from_iblk_pin);
                    int from_idirect = idirect_from_blk_pin_[physical_tile->index][from_physical_pin];
                    e_pin_type from_src_or_sink = direct_type_from_blk_pin_[physical_tile->index][from_physical_pin];

                    // Confirm whether this is a head macro
                    //
                    // The output SOURCE (from_pin) of a true head macro will:
                    //  * drive another block with the same direct connection
                    if (from_src_or_sink == e_pin_type::DRIVER && to_idirect == from_idirect && from_net_id != ClusterNetId::INVALID() && is_net_direct_connection(from_net_id, to_idirect, clb_nlist)) {
                        // Mark down that this is the first block in the macro
                        pl_macro_member_blk_num_of_this_blk[0] = blk_id;
                        pl_macro_idirect[num_macro] = to_idirect;

                        // Increment the num_member count.
                        pl_macro_num_members[num_macro]++;

                        // Also find out how many members are in the macros,
                        // there are at least 2 members - 1 head and 1 tail.

                        // Initialize the variables
                        ClusterNetId next_net_id = from_net_id;
                        ClusterBlockId next_blk_id = blk_id;

                        // Start finding the other members
                        while (next_net_id != ClusterNetId::INVALID()) {
                            ClusterNetId curr_net_id = next_net_id;

                            // Check that the next net is driven by a direct with the same idirect and also have the same to_pin direct connection
                            // If not, this is not a valid macro and we stop here.
                            if (!is_net_direct_connection(curr_net_id, to_idirect, clb_nlist)) {
                                break;
                            }

                            // Assume that carry chains only has 1 sink - direct connection
                            VTR_ASSERT(clb_nlist.net_sinks(curr_net_id).size() == 1);
                            next_blk_id = clb_nlist.net_pin_block(curr_net_id, 1);

                            // Assume that the from_iblk_pin index is the same for the next block
                            VTR_ASSERT(idirect_from_blk_pin_[physical_tile->index][from_physical_pin] == from_idirect
                                       && direct_type_from_blk_pin_[physical_tile->index][from_physical_pin] == e_pin_type::DRIVER);
                            next_net_id = clb_nlist.block_net(next_blk_id, from_iblk_pin);

                            // Mark down this block as a member of the macro
                            int imember = pl_macro_num_members[num_macro];
                            pl_macro_member_blk_num_of_this_blk[imember] = next_blk_id;

                            // Increment the num_member count.
                            pl_macro_num_members[num_macro]++;

                        } // Found all the members of this macro at this point

                        // Allocate the second dimension of the blk_num array since I now know the size
                        pl_macro_member_blk_num[num_macro].resize(pl_macro_num_members[num_macro]);
                        int matching_macro = -1;
                        // Copy the data from the temporary array to the newly allocated array.
                        for (int imember = 0; imember < pl_macro_num_members[num_macro]; imember++) {
                            auto cluster_id = pl_macro_member_blk_num_of_this_blk[imember];
                            pl_macro_member_blk_num[num_macro][imember] = cluster_id;
                            // check if this cluster block was in a previous macro
                            auto cluster_macro_pair = std::pair<ClusterBlockId, int>(cluster_id, num_macro);
                            if (!clusters_macro.insert(cluster_macro_pair).second) {
                                matching_macro = clusters_macro[cluster_id];
                            }
                        }

                        // one cluster from this macro is found in a previous macro try to combine both
                        // macros, since otherwise the program will fail when validating the macros.
                        if (matching_macro != -1) {
                            // try to combine the newly created macro with the found match
                            if (try_combine_macros(pl_macro_member_blk_num, matching_macro, num_macro)) {
                                // the newly created macro is combined with a previous macro
                                // reset the number of members of the newly created macro since it's now removed
                                pl_macro_num_members[num_macro] = 0;
                                // update the number of blocks of the matching macro after combining it with the new macro
                                pl_macro_num_members[matching_macro] = pl_macro_member_blk_num[matching_macro].size();
                                // decrement the number of found macros since the latest one is removed
                                num_macro--;
                            }
                        }

                        // Increment the macro count
                        num_macro++;

                    } // Do nothing if the from_pins does not have same possible direct connection.
                } // Finish going through all the pins for from_pins.
            } // Do nothing if the to_pins does not have same possible direct connection.
        } // Finish going through all the pins for to_pins.
    } // Finish going through all blocks.

    // Now, all the data is readily stored in the temporary data structures.
    return num_macro;
}

static bool try_combine_macros(std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num,
                               int matching_macro,
                               int latest_macro) {
    auto& old_macro_blocks = pl_macro_member_blk_num[matching_macro];
    auto& new_macro_blocks = pl_macro_member_blk_num[latest_macro];

    // Algorithm:
    // 1) Combining two macros is valid when the first block of one of the two macros
    //    matches one of the blocks in the other macro. Examples for valid cases:
    //
    // Case 1: Macro 2 is a subset of Macro 1
    //
    //        Macro 1 (and Combined Macro)
    //          ---
    //          |0|<--- First      Macro 2
    //          ---                 ---
    //          |1|<---- Match ---->|1|<--- First
    //          ---                 ---
    //          |2|                 |2|<---- ClusterBlockId
    //          ---                 ---
    //          |3|
    //          ---
    //
    // Case 2: Macro 2 is an extension of Macro 1
    //
    //        Macro 1             Macro 2            Combined Macro
    //          ---                 ---                  ---
    //First --->|0|      ---------->|2|<--- First        |0|
    //          ---      |          ---                  ---
    //          |1|    Match        |3|                  |1|
    //          ---      |          ---   ========>      ---
    //          |2|<------          |4|                  |2|
    //          ---                 ---                  ---
    //          |3|                 |5|                  |3|
    //          ---                 ---                  ---
    //                                                   |4|
    //                                                   ---
    //                                                   |5|
    //                                                   ---
    //
    // 2) Starting from this match and going forward in both macros all the blocks
    //    should match till we reach the end of one of the macros or both of them.
    // 3) If combining the macros is valid, create a new macro that is the union
    //    of both macros.
    // 4) Replace the old macro with this new combined macro.

    // Step 1) find the staring point of the matching
    auto new_macro_it = new_macro_blocks.begin();
    auto old_macro_it = std::ranges::find(old_macro_blocks, *new_macro_it);
    if (old_macro_it == old_macro_blocks.end()) {
        old_macro_it = old_macro_blocks.begin();
        new_macro_it = std::ranges::find(new_macro_blocks, *old_macro_it);
        // if matching is from the middle of the two macros, then combining macros is not possible
        if (new_macro_it == new_macro_blocks.end()) {
            return false;
        }
    }

    // Store the first part of the combined macro. Similar to blocks 0 -> 1 in case 2
    std::vector<ClusterBlockId> combined_macro;
    // old_macro is similar to Macro 1 in case 2
    if (old_macro_it != old_macro_blocks.begin()) {
        combined_macro.insert(combined_macro.begin(), old_macro_blocks.begin(), old_macro_it);
        // new_macro is similar to Macro 1 in case 2
    } else {
        combined_macro.insert(combined_macro.begin(), new_macro_blocks.begin(), new_macro_it);
    }

    // Step 2) The matching block between the two macros is found, move forward
    // from the matching block to find if combining both macros is valid or not
    while (old_macro_it != old_macro_blocks.end() && new_macro_it != new_macro_blocks.end()) {
        // block ids should match till the end of one
        // of the macros or both of them is reached
        if (*old_macro_it != *new_macro_it) {
            return false;
        }
        // add the block id to the combined macro
        combined_macro.push_back(*old_macro_it);
        // go to the next block in both macros
        old_macro_it++;
        new_macro_it++;
    }

    // Store the last part of the combined macro. Similar to blocks 4 -> 5 in case 2.
    if (old_macro_it != old_macro_blocks.end()) {
        // old_macro is similar to Macro 2 in case 2
        combined_macro.insert(combined_macro.end(), old_macro_it, old_macro_blocks.end());
    } else if (new_macro_it != new_macro_blocks.end()) {
        // new_macro is similar to Macro 2 in case 2
        combined_macro.insert(combined_macro.end(), new_macro_it, new_macro_blocks.end());
    }

    // updated the old macro in the 2D array of macros with the new combined macro
    pl_macro_member_blk_num[matching_macro] = combined_macro;
    // remove the newly created macro which is now included in another macro
    pl_macro_member_blk_num[latest_macro].clear();

    return true;
}

int PlaceMacros::get_imacro_from_iblk(ClusterBlockId iblk) const {
    int imacro;
    if (iblk != ClusterBlockId::INVALID()) {
        // Return the imacro for the block.
        imacro = imacro_from_iblk_[iblk];
    } else {
        imacro = UNDEFINED; //No valid block, so no valid macro
    }

    return imacro;
}

void PlaceMacros::alloc_and_load_idirect_from_blk_pin_(const std::vector<t_direct_inf>& directs,
                                                       const std::vector<t_physical_tile_type>& physical_tile_types) {
    // Allocate and initialize the values to UNDEFINED (-1).
    idirect_from_blk_pin_.resize(physical_tile_types.size());
    direct_type_from_blk_pin_.resize(physical_tile_types.size());
    for (const t_physical_tile_type& type : physical_tile_types) {
        if (is_empty_type(&type)) {
            continue;
        }

        idirect_from_blk_pin_[type.index].resize(type.num_pins, UNDEFINED);
        direct_type_from_blk_pin_[type.index].resize(type.num_pins, e_pin_type::OPEN);
    }

    const PortPinToBlockPinConverter port_pin_to_block_pin;

    // (type index, block pin index) of pins that are part of more than one direct
    std::vector<std::pair<int, int>> shared_pins;

    /* Load the values */
    // Go through directs and find pins with possible direct connections
    for (size_t idirect = 0; idirect < directs.size(); idirect++) {
        // Parse out the pb_type and port name, possibly pin_indices from from_pin and to_pin
        t_direct_pin_spec from_pin = parse_direct_pin_name(directs[idirect].from_pin, directs[idirect].line);
        t_direct_pin_spec to_pin = parse_direct_pin_name(directs[idirect].to_pin, directs[idirect].line);

        /* Now I have all the data that I need, I could go through all the block pins
         * in all the blocks to find all the pins that could have possible direct
         * connections. Mark all down all those pins with the idirect the pins belong
         * to and whether it is a source or a sink of the direct connection. */

        // Find blocks with the same name as from_pin's pb_type and port
        mark_direct_of_ports(idirect, e_pin_type::DRIVER, from_pin, directs[idirect].from_pin,
                             directs[idirect].line,
                             idirect_from_blk_pin_, direct_type_from_blk_pin_, shared_pins,
                             physical_tile_types,
                             port_pin_to_block_pin);

        // Then, find blocks with the same name as to_pin's pb_type and port
        mark_direct_of_ports(idirect, e_pin_type::RECEIVER, to_pin, directs[idirect].to_pin,
                             directs[idirect].line,
                             idirect_from_blk_pin_, direct_type_from_blk_pin_, shared_pins,
                             physical_tile_types,
                             port_pin_to_block_pin);

    } // Finish going through all the directs

    // A placement macro needs a single direct per pin, so pins that are part of more
    // than one direct are not used for macros. They were left marked above so that
    // every direct naming them was detected; drop them now.
    for (auto [itype, iblk_pin] : shared_pins) {
        idirect_from_blk_pin_[itype][iblk_pin] = UNDEFINED;
        direct_type_from_blk_pin_[itype][iblk_pin] = e_pin_type::OPEN;
    }
}

static void mark_direct_of_ports(int idirect,
                                 e_pin_type direct_type,
                                 const t_direct_pin_spec& pin_spec,
                                 std::string_view src_string,
                                 int line,
                                 std::vector<std::vector<int>>& idirect_from_blk_pin,
                                 std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                                 std::vector<std::pair<int, int>>& shared_pins,
                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                 const PortPinToBlockPinConverter& port_pin_to_block_pin) {
    // Go through all the block types
    for (int itype = 1; itype < (int)physical_tile_types.size(); itype++) {
        auto& physical_tile = physical_tile_types[itype];
        // Find blocks with the same pb_type_name
        if (pin_spec.pb_type_name == physical_tile.name) {
            int num_sub_tiles = physical_tile.sub_tiles.size();
            for (int isub_tile = 0; isub_tile < num_sub_tiles; isub_tile++) {
                auto& ports = physical_tile.sub_tiles[isub_tile].ports;
                int num_ports = ports.size();
                for (int iport = 0; iport < num_ports; iport++) {
                    // Find ports with the same port_name
                    if (pin_spec.port_name == ports[iport].name) {
                        int num_port_pins = ports[iport].num_pins;

                        // Check whether the pin range lies within the port
                        if (!pin_spec.fits_in_port(num_port_pins)) {
                            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line,
                                      "Invalid pin - %.*s, the pin range [end_pin_index:start_pin_index] "
                                      "must lie within the %d pins of the port.\n",
                                      static_cast<int>(src_string.length()), src_string.data(), num_port_pins);
                        }

                        int num_shared_pins = mark_direct_of_pins(pin_spec, itype,
                                                                  isub_tile, iport, idirect_from_blk_pin, idirect,
                                                                  direct_type_from_blk_pin, direct_type, shared_pins,
                                                                  physical_tile_types,
                                                                  port_pin_to_block_pin);
                        if (num_shared_pins > 0) {
                            VTR_LOG_WARN("[LINE %d] %d pin(s) of %.*s are part of more than one direct connection; "
                                         "placement macros will not be formed through them.\n",
                                         line, num_shared_pins, static_cast<int>(src_string.length()), src_string.data());
                        }
                    } // Do nothing if port_name does not match
                } // Finish going through all the ports
            } // Finish going through all the subtiles
        } // Do nothing if pb_type_name does not match
    } // Finish going through all the blocks
}

static int mark_direct_of_pins(const t_direct_pin_spec& pin_spec,
                               int itype,
                               int isub_tile,
                               int iport,
                               std::vector<std::vector<int>>& idirect_from_blk_pin,
                               int idirect,
                               std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                               e_pin_type direct_type,
                               std::vector<std::pair<int, int>>& shared_pins,
                               const std::vector<t_physical_tile_type>& physical_tile_types,
                               const PortPinToBlockPinConverter& port_pin_to_block_pin) {
    int num_shared_pins = 0;

    // Mark the specified pin range (inclusive), or the whole port if no range is given
    int num_port_pins = physical_tile_types[itype].sub_tiles[isub_tile].ports[iport].num_pins;
    int start_pin_index = pin_spec.has_pin_range() ? pin_spec.start_pin_index : 0;
    int end_pin_index = pin_spec.has_pin_range() ? pin_spec.end_pin_index : num_port_pins - 1;
    for (int iport_pin = start_pin_index; iport_pin <= end_pin_index; iport_pin++) {
        int iblk_pin = port_pin_to_block_pin.get_blk_pin_from_port_pin(itype, isub_tile, iport, iport_pin);

        // iterate through all segment connections and check if all Fc's are 0
        bool all_fcs_0 = true;
        for (const auto& fc_spec : physical_tile_types[itype].fc_specs) {
            for (int ipin : fc_spec.pins) {
                if (iblk_pin == ipin && fc_spec.fc_value > 0) {
                    all_fcs_0 = false;
                    break;
                }
            }
            if (!all_fcs_0) break;
        }

        // Check the fc for the pin, direct chain link only if fc == 0
        if (all_fcs_0) {
            if (direct_type_from_blk_pin[itype][iblk_pin] != e_pin_type::OPEN) {
                // Already part of another direct; the caller drops it once all directs are loaded
                shared_pins.emplace_back(itype, iblk_pin);
                num_shared_pins++;
            } else {
                idirect_from_blk_pin[itype][iblk_pin] = idirect;
                direct_type_from_blk_pin[itype][iblk_pin] = direct_type;
            }
        }
    } // Finish marking all the pins

    return num_shared_pins;
}

/* Allocates and loads imacro_from_iblk array. */
void PlaceMacros::alloc_and_load_imacro_from_iblk_(const std::vector<t_pl_macro>& macros,
                                                   const ClusteredNetlist& clb_nlist) {
    imacro_from_iblk_.resize(clb_nlist.blocks().size());

    /* Allocate and initialize the values to OPEN (-1). */
    for (auto blk_id : clb_nlist.blocks()) {
        imacro_from_iblk_.insert(blk_id, UNDEFINED);
    }

    /* Load the values */
    for (size_t imacro = 0; imacro < macros.size(); imacro++) {
        for (size_t imember = 0; imember < macros[imacro].members.size(); imember++) {
            ClusterBlockId blk_id = macros[imacro].members[imember].blk_index;
            imacro_from_iblk_.insert(blk_id, imacro);
        }
    }
}

void PlaceMacros::write_place_macros_(std::string filename,
                                      const std::vector<t_pl_macro>& macros,
                                      const std::vector<t_physical_tile_type>& physical_tile_types,
                                      const ClusteredNetlist& clb_nlist) const {
    FILE* f = vtr::fopen(filename.c_str(), "w");

    fprintf(f, "#Identified Placement macros\n");
    fprintf(f, "Num_Macros: %zu\n", macros.size());
    for (size_t imacro = 0; imacro < macros.size(); ++imacro) {
        const t_pl_macro* macro = &macros[imacro];
        fprintf(f, "Macro_Id: %zu, Num_Blocks: %zu%s\n", imacro, macro->members.size(), macro->is_relative_placement_macro() ? ", relative_placement_macro" : "");
        fprintf(f, "------------------------------------------------------\n");
        for (size_t imember = 0; imember < macro->members.size(); ++imember) {
            const t_pl_macro_member* macro_memb = &macro->members[imember];
            fprintf(f, "Block_Id: %zu (%s), x_offset: %d, y_offset: %d, z_offset: %d\n",
                    size_t(macro_memb->blk_index),
                    clb_nlist.block_name(macro_memb->blk_index).c_str(),
                    macro_memb->offset.x,
                    macro_memb->offset.y,
                    macro_memb->offset.sub_tile);
        }
        fprintf(f, "\n");
    }

    fprintf(f, "\n");

    fprintf(f, "#Macro-related direct connections\n");
    fprintf(f, "type      type_pin  is_direct direct_type\n");
    fprintf(f, "------------------------------------------\n");
    for (const auto& type : physical_tile_types) {
        if (is_empty_type(&type)) {
            continue;
        }

        int itype = type.index;
        for (int ipin = 0; ipin < type.num_pins; ++ipin) {
            if (idirect_from_blk_pin_[itype][ipin] != UNDEFINED) {
                if (direct_type_from_blk_pin_[itype][ipin] == e_pin_type::DRIVER) {
                    fprintf(f, "%-9s %-9d true      SOURCE    \n", type.name.c_str(), ipin);
                } else {
                    VTR_ASSERT(direct_type_from_blk_pin_[itype][ipin] == e_pin_type::RECEIVER);
                    fprintf(f, "%-9s %-9d true      SINK      \n", type.name.c_str(), ipin);
                }
            } else {
                VTR_ASSERT(direct_type_from_blk_pin_[itype][ipin] == e_pin_type::OPEN);
            }
        }
    }

    fclose(f);
}

static bool is_constant_clb_net(ClusterNetId clb_net,
                                const AtomLookup& atom_lookup,
                                const AtomNetlist& atom_nlist) {
    AtomNetId atom_net = atom_lookup.atom_net(clb_net);

    return is_constant_generator_net(atom_nlist, atom_net);
}

bool PlaceMacros::is_net_direct_connection(ClusterNetId clb_net, int idirect, const ClusteredNetlist& clb_nlist) {
    ClusterBlockId block_id = clb_nlist.net_driver_block(clb_net);
    int pin_index = clb_nlist.net_pin_logical_index(clb_net, 0);

    auto logical_block = clb_nlist.block_type(block_id);
    auto physical_tile = pick_physical_type(logical_block);
    auto physical_pin = get_physical_pin(physical_tile, logical_block, pin_index);

    auto driver_direct = idirect_from_blk_pin_[physical_tile->index][physical_pin];

    int receiver_direct = UNDEFINED;

    //if there is more than one sink, it should not be counted as a direct connection
    if (clb_nlist.net_sinks(clb_net).size() > 1) {
        return false;
    }

    ClusterPinId net_sink = clb_nlist.net_pin(clb_net, 1);
    ClusterBlockId sink_block_id = clb_nlist.pin_block(net_sink);

    if (sink_block_id == block_id) {
        //net is connected back to the same block, should not be counted as a direct connection
        VTR_LOG_WARN(
            "Block '%s'have a direct connection to itself, this connection will not be counted as a direct connection to form a macro.\n",
            clb_nlist.block_name(block_id).c_str());
        return false;
    }

    int sink_pin_index = clb_nlist.net_pin_logical_index(clb_net, 1);

    auto sink_logical_block = clb_nlist.block_type(sink_block_id);
    auto sink_physical_tile = pick_physical_type(sink_logical_block);
    auto sink_physical_pin = get_physical_pin(sink_physical_tile, sink_logical_block, sink_pin_index);

    receiver_direct = idirect_from_blk_pin_[sink_physical_tile->index][sink_physical_pin];

    if (driver_direct == UNDEFINED || receiver_direct == UNDEFINED) {
        return false;
    }

    return driver_direct == idirect && receiver_direct == idirect;
}

const t_pl_macro& PlaceMacros::operator[](int idx) const {
    return pl_macros_[idx];
}

static void validate_macros(const std::vector<t_pl_macro>& macros,
                            const ClusteredNetlist& clb_nlist) {
    //Perform sanity checks on macros

    //Verify that blocks only appear in a single macro
    std::multimap<ClusterBlockId, int> block_to_macro;
    for (size_t imacro = 0; imacro < macros.size(); ++imacro) {
        for (size_t imember = 0; imember < macros[imacro].members.size(); ++imember) {
            ClusterBlockId iblk = macros[imacro].members[imember].blk_index;

            block_to_macro.emplace(iblk, imacro);
        }
    }

    for (ClusterBlockId blk_id : clb_nlist.blocks()) {
        auto range = block_to_macro.equal_range(blk_id);

        int blk_macro_cnt = std::distance(range.first, range.second);
        if (blk_macro_cnt > 1) {
            std::stringstream msg;
            msg << "Block #" << size_t(blk_id) << " '" << clb_nlist.block_name(blk_id) << "'"
                << " appears in " << blk_macro_cnt << " placement macros (should appear in at most one). Related Macros:\n";

            for (auto iter = range.first; iter != range.second; ++iter) {
                int imacro = iter->second;
                msg << "  Macro #: " << imacro << "\n";
            }

            VPR_FATAL_ERROR(VPR_ERROR_PLACE, msg.str().c_str());
        }
    }
}
