
#include "place_macro.h"

#include <cstdio>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>
#include <string_view>

#include "atom_lookup.h"
#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "physical_types_util.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "vpr_utils.h"
#include "vpr_types.h"
#include "vpr_error.h"
#include "physical_types.h"
#include "echo_files.h"

/**
 * @brief Determines whether a cluster net is constant.
 * @param clb_net The unique id of a cluster net.
 * @return True if the net is constant; otherwise false.
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

/* Go through all the ports in all the blocks to find the port that has the same   *
 * name as port_name and belongs to the block type that has the name pb_type_name. *
 * Then, check that whether start_pin_index and end_pin_index are specified. If    *
 * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
 * Otherwise, mark down all the pins in that port.                                 */
static void mark_direct_of_ports(int idirect,
                                 e_pin_type direct_type,
                                 std::string_view pb_type_name,
                                 std::string_view port_name,
                                 int end_pin_index,
                                 int start_pin_index,
                                 std::string_view src_string,
                                 int line,
                                 std::vector<std::vector<int>>& idirect_from_blk_pin,
                                 std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                 int absolute_sub_tile_z);

/**
 * @brief Mark the idirect_from_blk_pin and direct_type_from_blk_pin entries for pins
 * start_pin_index..end_pin_index of port iport on sub_tile isub_tile of tile itype.
 *
 * @param absolute_sub_tile_z  The absolute tile-level sub_tile z-position to mark
 *   (i.e., a value in the sub_tile's capacity range). If this value falls inside the
 *   sub_tile's capacity range only that one capacity instance is marked; otherwise
 *   ALL capacity instances are marked. Pass -1 (or any out-of-range value) to
 *   unconditionally mark every instance.
 */
static void mark_direct_of_pins(int start_pin_index,
                                int end_pin_index,
                                int itype,
                                int isub_tile,
                                int iport,
                                std::vector<std::vector<int>>& idirect_from_blk_pin,
                                int idirect,
                                std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                                e_pin_type direct_type,
                                int line,
                                std::string_view src_string,
                                const std::vector<t_physical_tile_type>& physical_tile_types,
                                int absolute_sub_tile_z);

const std::vector<t_pl_macro>& PlaceMacros::macros() const {
    return pl_macros_;
}

PlaceMacros::PlaceMacros(const std::vector<t_direct_inf>& directs,
                         const std::vector<t_physical_tile_type>& physical_tile_types,
                         const ClusteredNetlist& clb_nlist,
                         const AtomNetlist& atom_nlist,
                         const AtomLookup& atom_lookup) {
    /* Allocates and loads placement macros in two steps:
     *   1) Allocate temporary data structures sized for the worst case and
     *      call find_all_the_macro_ to identify all macros. Two kinds are detected:
     *      - Carry-chain macros: blocks that both drive and receive the same direct
     *        connection (e.g. a carry chain). pl_macro_idirect is set to the direct index.
     *      - Pure-source macros: blocks that only DRIVE a direct with no corresponding
     *        receiver pin of their own (e.g. a multiplier driving a memory block via a
     *        sub-tile direct). pl_macro_idirect is set to UNDEFINED for these.
     *   2) Copy the data into the final pl_macros_ vector. Carry-chain offsets are
     *      derived from the direct's step vector (offset = member_index * direct_offset).
     *      Pure-source offsets are taken verbatim from pl_macro_member_offsets, which
     *      find_all_the_macro_ pre-computes for each member.
     */

    size_t num_clusters = clb_nlist.blocks().size();

    // Allocate maximum memory for temporary variables.
    std::vector<int> pl_macro_idirect(num_clusters);
    std::vector<int> pl_macro_num_members(num_clusters);
    // First dimension allocated up-front; second dimension allocated per macro once the size is known.
    std::vector<std::vector<ClusterBlockId>> pl_macro_member_blk_num(num_clusters);

    alloc_and_load_idirect_from_blk_pin_(directs, physical_tile_types);

    // Per-member offsets pre-computed by find_all_the_macro_ for pure-source macros.
    // Carry-chain macro offsets are derived later from the direct's step vector.
    std::vector<std::vector<t_pl_offset>> pl_macro_member_offsets(num_clusters);

    const int num_macro = find_all_the_macro_(clb_nlist, atom_nlist, atom_lookup,
                                              directs,
                                              pl_macro_idirect, pl_macro_num_members,
                                              pl_macro_member_blk_num,
                                              pl_macro_member_offsets);

    // Allocate the memories for the macro.
    pl_macros_.resize(num_macro);

    /* Allocate the memories for the chain members.
     * Load the values from the temporary data structures.
     */
    for (int imacro = 0; imacro < num_macro; imacro++) {
        pl_macros_[imacro].members = std::vector<t_pl_macro_member>(pl_macro_num_members[imacro]);

        // Load the values for each member of the macro
        for (size_t imember = 0; imember < pl_macros_[imacro].members.size(); imember++) {
            if (pl_macro_idirect[imacro] != UNDEFINED) {
                // Carry-chain macro: offset for member i is i times the direct's step vector
                const auto& direct = directs[pl_macro_idirect[imacro]];
                pl_macros_[imacro].members[imember].offset.x = imember * direct.x_offset;
                pl_macros_[imacro].members[imember].offset.y = imember * direct.y_offset;
                pl_macros_[imacro].members[imember].offset.sub_tile = imember * direct.sub_tile_offset;
            } else {
                // Pure-source macro: offsets were pre-computed in find_all_the_macro_
                pl_macros_[imacro].members[imember].offset = pl_macro_member_offsets[imacro][imember];
            }
            pl_macros_[imacro].members[imember].blk_index = pl_macro_member_blk_num[imacro][imember];
        }
    }

    if (isEchoFileEnabled(E_ECHO_PLACE_MACROS)) {
        write_place_macros_(getEchoFileName(E_ECHO_PLACE_MACROS),
                            pl_macros_,
                            physical_tile_types,
                            clb_nlist);
    }

    validate_macros(pl_macros_, clb_nlist);

    alloc_and_load_imacro_from_iblk_(pl_macros_, clb_nlist);
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
                                     const std::vector<t_direct_inf>& directs,
                                     std::vector<int>& pl_macro_idirect,
                                     std::vector<int>& pl_macro_num_members,
                                     std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num,
                                     std::vector<std::vector<t_pl_offset>>& pl_macro_member_offsets) {
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

    // Pure-source macro detection:
    // Handles blocks that have DRIVER pins for a direct but no RECEIVER pins for that
    // same direct (e.g. a multiplier driving memory blocks via sub-tile direct connections).
    for (ClusterBlockId blk_id : clb_nlist.blocks()) {
        if (clusters_macro.count(blk_id)) continue;

        auto logical_block = clb_nlist.block_type(blk_id);
        auto physical_tile = pick_physical_type(logical_block);
        int num_blk_pins = logical_block->pb_type->num_pins;

        // Collect per-direct info: which directs have receiver pins on this block,
        // and which receiver blocks are reached by driver nets.
        std::set<int> directs_with_receiver_pin;
        std::map<int, std::set<ClusterBlockId>> direct_to_receivers;

        for (int iblk_pin = 0; iblk_pin < num_blk_pins; iblk_pin++) {
            int physical_pin = get_physical_pin(physical_tile, logical_block, iblk_pin);
            int pin_idirect = idirect_from_blk_pin_[physical_tile->index][physical_pin];
            if (pin_idirect == UNDEFINED) continue;

            e_pin_type pin_type = direct_type_from_blk_pin_[physical_tile->index][physical_pin];

            if (pin_type == e_pin_type::RECEIVER) {
                directs_with_receiver_pin.insert(pin_idirect);
            } else if (pin_type == e_pin_type::DRIVER) {
                ClusterNetId net_id = clb_nlist.block_net(blk_id, iblk_pin);
                if (net_id == ClusterNetId::INVALID()) continue;
                if (clb_nlist.net_sinks(net_id).size() != 1) continue;
                ClusterBlockId sink_blk = clb_nlist.net_pin_block(net_id, 1);
                if (sink_blk == blk_id) continue;
                // Default: use the driver's own direct marking. get_physical_pin always
                // returns cap_idx=0 pins, so pin_idirect is accurate when the driver
                // sub_tile has capacity=1 (no aliasing). When the driver sub_tile has
                // capacity > 1, cap_idx=0 aliases all instances so we instead look at
                // the receiver's physical pin to determine the actual direct.
                int effective_idirect = pin_idirect;
                int sink_logical_pin = clb_nlist.net_pin_logical_index(net_id, 1);
                if (sink_logical_pin != UNDEFINED) {
                    auto sink_logical_block = clb_nlist.block_type(sink_blk);
                    auto sink_physical_tile = pick_physical_type(sink_logical_block);
                    int sink_physical_pin = get_physical_pin(sink_physical_tile, sink_logical_block, sink_logical_pin);
                    int actual_idirect = idirect_from_blk_pin_[sink_physical_tile->index][sink_physical_pin];
                    if (actual_idirect != UNDEFINED && actual_idirect != pin_idirect) {
                        // The two sides disagree. Use the receiver's direct when the
                        // driver sub_tile has capacity > 1, since then pin_idirect
                        // only reflects the cap_idx=0 instance (aliasing).
                        int driver_cap = 1;
                        for (const auto& st : physical_tile->sub_tiles) {
                            int cap_total = st.capacity.total();
                            int pins_per_instance = (int)st.sub_tile_to_tile_pin_indices.size() / cap_total;
                            for (int p = 0; p < pins_per_instance; p++) {
                                if (st.sub_tile_to_tile_pin_indices[p] == physical_pin) {
                                    driver_cap = cap_total;
                                    break;
                                }
                            }
                            if (driver_cap > 1) break;
                        }
                        if (driver_cap > 1) {
                            effective_idirect = actual_idirect;
                        }
                    }
                }
                direct_to_receivers[effective_idirect].insert(sink_blk);
            }
        }

        // Build macro members from pure-source directs (driver but no receiver pin).
        // Member 0 is the source block at offset (0,0,0,0).
        std::vector<std::pair<t_pl_offset, ClusterBlockId>> macro_members;
        macro_members.push_back({t_pl_offset(0, 0, 0, 0), blk_id});

        // If a receiver is already in an existing macro, extend that macro rather than
        // creating a duplicate. This handles tiles with capacity > 1 driver sub-tiles
        // where multiple driver instances connect to the same receiver block.
        int existing_macro = -1;
        t_pl_offset driver_offset_in_existing{};

        for (auto& [idir, receivers] : direct_to_receivers) {
            if (directs_with_receiver_pin.count(idir)) continue; // carry-chain, not pure-source
            // If multiple different receiver blocks are reachable via the same direct it
            // is ambiguous which one to use — skip and leave unconstrained.
            if (receivers.size() != 1) continue;
            ClusterBlockId receiver_blk = *receivers.begin();

            if (clusters_macro.count(receiver_blk)) {
                // Receiver is already in an existing macro — compute this driver's offset
                // relative to that macro's head.
                // driver_offset = receiver_offset - direct_offset, because:
                //   driver_z + direct.sub_tile_offset = receiver_z
                //   => driver_z = receiver_z - direct.sub_tile_offset
                int recv_macro = clusters_macro[receiver_blk];

                // A single driver block should not have receivers in two different
                // existing macros; that would require the two macros to be merged,
                // which is not supported here.
                VTR_ASSERT_MSG(existing_macro == -1 || existing_macro == recv_macro,
                               "Pure-source driver block connects to receivers that "
                               "already belong to two different placement macros.");

                for (size_t i = 0; i < pl_macro_member_blk_num[recv_macro].size(); i++) {
                    if (pl_macro_member_blk_num[recv_macro][i] == receiver_blk) {
                        const t_pl_offset& recv_offset = pl_macro_member_offsets[recv_macro][i];
                        driver_offset_in_existing = recv_offset
                                                    - t_pl_offset(directs[idir].x_offset,
                                                                  directs[idir].y_offset,
                                                                  directs[idir].sub_tile_offset,
                                                                  0);
                        existing_macro = recv_macro;
                        break;
                    }
                }
            } else {
                macro_members.push_back({t_pl_offset(directs[idir].x_offset,
                                                     directs[idir].y_offset,
                                                     directs[idir].sub_tile_offset,
                                                     0),
                                         receiver_blk});
            }
        }

        if (existing_macro != -1) {
            // Merge this driver into the existing macro
            pl_macro_member_blk_num[existing_macro].push_back(blk_id);
            pl_macro_member_offsets[existing_macro].push_back(driver_offset_in_existing);
            pl_macro_num_members[existing_macro] = pl_macro_member_blk_num[existing_macro].size();
            clusters_macro[blk_id] = existing_macro;
            // Add any new receivers (from macro_members[1:]) not yet in a macro
            for (size_t i = 1; i < macro_members.size(); i++) {
                ClusterBlockId new_recv = macro_members[i].second;
                t_pl_offset new_recv_offset = driver_offset_in_existing + macro_members[i].first;
                pl_macro_member_blk_num[existing_macro].push_back(new_recv);
                pl_macro_member_offsets[existing_macro].push_back(new_recv_offset);
                pl_macro_num_members[existing_macro] = pl_macro_member_blk_num[existing_macro].size();
                clusters_macro[new_recv] = existing_macro;
            }
            continue;
        }

        if (macro_members.size() < 2) continue;

        // Sort non-head members by sub_tile offset for a deterministic ordering
        std::sort(macro_members.begin() + 1, macro_members.end(),
                  [](const auto& a, const auto& b) {
                      return a.first.sub_tile < b.first.sub_tile;
                  });

        // Register this macro (pl_macro_idirect == UNDEFINED signals pure-source to constructor)
        pl_macro_idirect[num_macro] = UNDEFINED;
        pl_macro_num_members[num_macro] = (int)macro_members.size();
        pl_macro_member_blk_num[num_macro].resize(macro_members.size());
        pl_macro_member_offsets[num_macro].resize(macro_members.size());

        for (size_t i = 0; i < macro_members.size(); i++) {
            pl_macro_member_blk_num[num_macro][i] = macro_members[i].second;
            pl_macro_member_offsets[num_macro][i] = macro_members[i].first;
            clusters_macro[macro_members[i].second] = num_macro;
        }

        num_macro++;
    }

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

    /* Load the values */
    // Go through directs and find pins with possible direct connections
    for (size_t idirect = 0; idirect < directs.size(); idirect++) {
        // Parse out the pb_type and port name, possibly pin_indices from from_pin
        auto [from_end_pin_index, from_start_pin_index, from_pb_type_name, from_port_name] = parse_direct_pin_name(directs[idirect].from_pin,
                                                                                                                   directs[idirect].line);

        // Parse out the pb_type and port name, possibly pin_indices from to_pin
        auto [to_end_pin_index, to_start_pin_index, to_pb_type_name, to_port_name] = parse_direct_pin_name(directs[idirect].to_pin,
                                                                                                           directs[idirect].line);

        // Returns the first sub_tile of a tile whose name matches tile_name and
        // that contains a port whose name matches port_name. Returns nullptr if
        // no match is found.
        auto find_sub_tile = [&](std::string_view tile_name,
                                 std::string_view port_name) -> const t_sub_tile* {
            for (const auto& tile : physical_tile_types) {
                if (tile_name != tile.name) continue;
                for (const auto& st : tile.sub_tiles) {
                    bool found = std::any_of(st.ports.begin(), st.ports.end(),
                                             [&](const t_physical_tile_port& p) { return p.name == port_name; });
                    if (found) return &st;
                }
            }
            return nullptr;
        };

        const t_sub_tile* from_sub_tile = find_sub_tile(from_pb_type_name, from_port_name);
        const t_sub_tile* to_sub_tile = find_sub_tile(to_pb_type_name, to_port_name);

        // Compute the absolute sub_tile positions of the driver and receiver.
        // When driver and receiver live in the same sub_tile (e.g. carry chains), every
        // capacity instance can act as either end, so pass -1 to mark all instances.
        // When they are in different sub_tiles (e.g. CIM directs), walk every driver
        // instance and find the one whose z + z_offset lands inside the receiver's
        // capacity range.  If the search is ambiguous (multiple matches), fall back to
        // marking all instances.
        int driver_abs_z = -1;   // -1 means "mark all driver instances"
        int receiver_abs_z = -1; // -1 means "mark all receiver instances"

        if (from_sub_tile && to_sub_tile && from_sub_tile != to_sub_tile) {
            int found_driver_z = -1;
            int found_receiver_z = -1;
            for (int z = from_sub_tile->capacity.low; z <= from_sub_tile->capacity.high; z++) {
                int recv_z = z + directs[idirect].sub_tile_offset;
                if (to_sub_tile->capacity.is_in_range(recv_z)) {
                    if (found_driver_z != -1) {
                        // Multiple driver instances produce a valid receiver → mark all
                        found_driver_z = -1;
                        found_receiver_z = -1;
                        break;
                    }
                    found_driver_z = z;
                    found_receiver_z = recv_z;
                }
            }
            driver_abs_z = found_driver_z;
            receiver_abs_z = found_receiver_z;
        }

        mark_direct_of_ports(idirect, e_pin_type::DRIVER, from_pb_type_name, from_port_name,
                             from_end_pin_index, from_start_pin_index, directs[idirect].from_pin,
                             directs[idirect].line,
                             idirect_from_blk_pin_, direct_type_from_blk_pin_,
                             physical_tile_types,
                             driver_abs_z);

        mark_direct_of_ports(idirect, e_pin_type::RECEIVER, to_pb_type_name, to_port_name,
                             to_end_pin_index, to_start_pin_index, directs[idirect].to_pin,
                             directs[idirect].line,
                             idirect_from_blk_pin_, direct_type_from_blk_pin_,
                             physical_tile_types,
                             receiver_abs_z);

    } // Finish going through all the directs
}

static void mark_direct_of_ports(int idirect,
                                 e_pin_type direct_type,
                                 std::string_view pb_type_name,
                                 std::string_view port_name,
                                 int end_pin_index,
                                 int start_pin_index,
                                 std::string_view src_string,
                                 int line,
                                 std::vector<std::vector<int>>& idirect_from_blk_pin,
                                 std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                 int absolute_sub_tile_z) {
    for (int itype = 1; itype < (int)physical_tile_types.size(); itype++) {
        auto& physical_tile = physical_tile_types[itype];
        if (pb_type_name != physical_tile.name) continue;
        int num_sub_tiles = physical_tile.sub_tiles.size();
        for (int isub_tile = 0; isub_tile < num_sub_tiles; isub_tile++) {
            auto& ports = physical_tile.sub_tiles[isub_tile].ports;
            int num_ports = ports.size();
            for (int iport = 0; iport < num_ports; iport++) {
                if (port_name != ports[iport].name) continue;
                int num_port_pins = ports[iport].num_pins;

                if (end_pin_index > num_port_pins) {
                    VTR_LOG_ERROR(
                        "[LINE %d] Invalid pin - %s, the end_pin_index in "
                        "[end_pin_index:start_pin_index] should "
                        "be less than the num_port_pins %d.\n",
                        line, src_string, num_port_pins);
                    exit(1);
                }

                int pin_lo = (start_pin_index >= 0 || end_pin_index >= 0) ? start_pin_index : 0;
                int pin_hi = (start_pin_index >= 0 || end_pin_index >= 0) ? end_pin_index : num_port_pins - 1;
                mark_direct_of_pins(pin_lo, pin_hi, itype,
                                    isub_tile, iport, idirect_from_blk_pin, idirect,
                                    direct_type_from_blk_pin, direct_type, line, src_string,
                                    physical_tile_types,
                                    absolute_sub_tile_z);
            }
        }
    }
}

static void mark_direct_of_pins(int start_pin_index,
                                int end_pin_index,
                                int itype,
                                int isub_tile,
                                int iport,
                                std::vector<std::vector<int>>& idirect_from_blk_pin,
                                int idirect,
                                std::vector<std::vector<e_pin_type>>& direct_type_from_blk_pin,
                                e_pin_type direct_type,
                                int line,
                                std::string_view src_string,
                                const std::vector<t_physical_tile_type>& physical_tile_types,
                                int absolute_sub_tile_z) {
    const auto& sub_tile = physical_tile_types[itype].sub_tiles[isub_tile];
    int cap_total = sub_tile.capacity.total();
    // Number of tile-level pin indices belonging to one capacity instance of this sub_tile.
    int pins_per_instance = (int)sub_tile.sub_tile_to_tile_pin_indices.size() / cap_total;

    // If absolute_sub_tile_z falls inside this sub_tile's capacity range, mark only that
    // one capacity instance. Otherwise mark all instances. The caller passes -1 (or any
    // out-of-range value) to unconditionally mark every instance.
    int start_cap_idx = 0;
    int end_cap_idx = cap_total - 1;
    if (sub_tile.capacity.is_in_range(absolute_sub_tile_z)) {
        int cap_idx = absolute_sub_tile_z - sub_tile.capacity.low;
        start_cap_idx = cap_idx;
        end_cap_idx = cap_idx;
    }

    // Normalise pin range in case from/to indices are given in reverse order.
    int pin_lo = std::min(start_pin_index, end_pin_index);
    int pin_hi = std::max(start_pin_index, end_pin_index);

    // Accumulated pin offset within one capacity instance up to (but not including) iport.
    // Using sub_tile.ports instead of PortPinToBlockPinConverter gives correct tile-level
    // indices even when preceding sub_tiles have capacity > 1.
    int port_start_in_instance = 0;
    for (int p = 0; p < iport; p++) {
        port_start_in_instance += sub_tile.ports[p].num_pins;
    }

    for (int cap_idx = start_cap_idx; cap_idx <= end_cap_idx; cap_idx++) {
        for (int iport_pin = pin_lo; iport_pin <= pin_hi; iport_pin++) {
            // Use sub_tile_to_tile_pin_indices for the correct tile-level pin index.
            // This works correctly regardless of the capacity of preceding sub_tiles.
            int pin_within_instance = port_start_in_instance + iport_pin;
            int iblk_pin = sub_tile.sub_tile_to_tile_pin_indices[cap_idx * pins_per_instance + pin_within_instance];

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

            // Direct chain link only if fc == 0
            if (all_fcs_0) {
                idirect_from_blk_pin[itype][iblk_pin] = idirect;

                // Check whether the pin is already marked; error if so
                if (direct_type_from_blk_pin[itype][iblk_pin] != e_pin_type::OPEN) {
                    VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                    "[LINE %d] Invalid pin - %s, this pin is in more than one direct connection.\n",
                                    line, src_string.data());
                } else {
                    direct_type_from_blk_pin[itype][iblk_pin] = direct_type;
                }
            }
        }
    }
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
        fprintf(f, "Macro_Id: %zu, Num_Blocks: %zu\n", imacro, macro->members.size());
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

    return atom_nlist.net_is_constant(atom_net);
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
