
#include "place_macro.h"

#include <cstdio>
#include <cmath>
#include <sstream>
#include <map>
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
                                 int direct_type,
                                 std::string_view pb_type_name,
                                 std::string_view port_name,
                                 int end_pin_index,
                                 int start_pin_index,
                                 std::string_view src_string,
                                 int line,
                                 std::vector<std::vector<int>>& idirect_from_blk_pin,
                                 std::vector<std::vector<int>>& direct_type_from_blk_pin,
                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                 const PortPinToBlockPinConverter& port_pin_to_block_pin);

/**
 * @brief Mark the pin entry in idirect_from_blk_pin with idirect and the pin entry in
 * direct_type_from_blk_pin with direct_type from start_pin_index to end_pin_index.
 */
static void mark_direct_of_pins(int start_pin_index,
                                int end_pin_index,
                                int itype,
                                int isub_tile,
                                int iport,
                                std::vector<std::vector<int>>& idirect_from_blk_pin,
                                int idirect,
                                std::vector<std::vector<int>>& direct_type_from_blk_pin,
                                int direct_type,
                                int line,
                                std::string_view src_string,
                                const std::vector<t_physical_tile_type>& physical_tile_types,
                                const PortPinToBlockPinConverter& port_pin_to_block_pin);

const std::vector<t_pl_macro>& PlaceMacros::macros() const {
    return pl_macros_;
}

PlaceMacros::PlaceMacros(const std::vector<t_direct_inf>& directs,
                         const std::vector<t_physical_tile_type>& physical_tile_types,
                         const ClusteredNetlist& clb_nlist,
                         const AtomNetlist& atom_nlist,
                         const AtomLookup& atom_lookup) {
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
    if (macro_index == OPEN) {
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
            int to_src_or_sink = direct_type_from_blk_pin_[physical_tile->index][to_physical_pin];

            // Identify potential macro head blocks (i.e. start of a macro)
            //
            // The input SINK (to_pin) of a potential HEAD macro will have either:
            //  * no connection to any net (OPEN), or
            //  * a connection to a constant net (e.g. gnd/vcc) which is not driven by a direct
            //
            // Note that the restriction that constant nets are not driven from another direct ensures that
            // blocks in the middle of a chain with internal constant signals are not detected as potential
            // head blocks.
            if (to_src_or_sink == SINK && to_idirect != OPEN &&
                (to_net_id == ClusterNetId::INVALID() || (is_constant_clb_net(to_net_id, atom_lookup, atom_nlist) && !net_is_driven_by_direct_(to_net_id, clb_nlist)))) {
                for (int from_iblk_pin = 0; from_iblk_pin < num_blk_pins; from_iblk_pin++) {
                    int from_physical_pin = get_physical_pin(physical_tile, logical_block, from_iblk_pin);

                    ClusterNetId from_net_id = clb_nlist.block_net(blk_id, from_iblk_pin);
                    int from_idirect = idirect_from_blk_pin_[physical_tile->index][from_physical_pin];
                    int from_src_or_sink = direct_type_from_blk_pin_[physical_tile->index][from_physical_pin];

                    // Confirm whether this is a head macro
                    //
                    // The output SOURCE (from_pin) of a true head macro will:
                    //  * drive another block with the same direct connection
                    if (from_src_or_sink == SOURCE && to_idirect == from_idirect && from_net_id != ClusterNetId::INVALID()) {
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

                            // Assume that carry chains only has 1 sink - direct connection
                            VTR_ASSERT(clb_nlist.net_sinks(curr_net_id).size() == 1);
                            next_blk_id = clb_nlist.net_pin_block(curr_net_id, 1);

                            // Assume that the from_iblk_pin index is the same for the next block
                            VTR_ASSERT(idirect_from_blk_pin_[physical_tile->index][from_physical_pin] == from_idirect
                                       && direct_type_from_blk_pin_[physical_tile->index][from_physical_pin] == SOURCE);
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
                }     // Finish going through all the pins for from_pins.
            }         // Do nothing if the to_pins does not have same possible direct connection.
        }             // Finish going through all the pins for to_pins.
    }                 // Finish going through all blocks.

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
    auto old_macro_it = std::find(old_macro_blocks.begin(), old_macro_blocks.end(), *new_macro_it);
    if (old_macro_it == old_macro_blocks.end()) {
        old_macro_it = old_macro_blocks.begin();
        new_macro_it = std::find(new_macro_blocks.begin(), new_macro_blocks.end(), *old_macro_it);
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
        imacro = OPEN; //No valid block, so no valid macro
    }

    return imacro;
}

void PlaceMacros::alloc_and_load_idirect_from_blk_pin_(const std::vector<t_direct_inf>& directs,
                                                       const std::vector<t_physical_tile_type>& physical_tile_types) {
    // Allocate and initialize the values to OPEN (-1).
    idirect_from_blk_pin_.resize(physical_tile_types.size());
    direct_type_from_blk_pin_.resize(physical_tile_types.size());
    for (const t_physical_tile_type& type : physical_tile_types) {
        if (is_empty_type(&type)) {
            continue;
        }

        idirect_from_blk_pin_[type.index].resize(type.num_pins, OPEN);
        direct_type_from_blk_pin_[type.index].resize(type.num_pins, OPEN);
    }

    const PortPinToBlockPinConverter port_pin_to_block_pin;

    /* Load the values */
    // Go through directs and find pins with possible direct connections
    for (size_t idirect = 0; idirect < directs.size(); idirect++) {
        // Parse out the pb_type and port name, possibly pin_indices from from_pin
        auto [from_end_pin_index, from_start_pin_index, from_pb_type_name, from_port_name] = parse_direct_pin_name(directs[idirect].from_pin,
                                                                                                                   directs[idirect].line);

        // Parse out the pb_type and port name, possibly pin_indices from to_pin
        auto [to_end_pin_index, to_start_pin_index, to_pb_type_name, to_port_name] = parse_direct_pin_name(directs[idirect].to_pin,
                                                                                                           directs[idirect].line);

        /* Now I have all the data that I need, I could go through all the block pins
         * in all the blocks to find all the pins that could have possible direct
         * connections. Mark all down all those pins with the idirect the pins belong
         * to and whether it is a source or a sink of the direct connection. */

        // Find blocks with the same name as from_pb_type_name and from_port_name
        mark_direct_of_ports(idirect, SOURCE, from_pb_type_name, from_port_name,
                             from_end_pin_index, from_start_pin_index, directs[idirect].from_pin,
                             directs[idirect].line,
                             idirect_from_blk_pin_, direct_type_from_blk_pin_,
                             physical_tile_types,
                             port_pin_to_block_pin);

        // Then, find blocks with the same name as to_pb_type_name and from_port_name
        mark_direct_of_ports(idirect, SINK, to_pb_type_name, to_port_name,
                             to_end_pin_index, to_start_pin_index, directs[idirect].to_pin,
                             directs[idirect].line,
                             idirect_from_blk_pin_, direct_type_from_blk_pin_,
                             physical_tile_types,
                             port_pin_to_block_pin);

    } // Finish going through all the directs
}

static void mark_direct_of_ports(int idirect,
                                 int direct_type,
                                 std::string_view pb_type_name,
                                 std::string_view port_name,
                                 int end_pin_index,
                                 int start_pin_index,
                                 std::string_view src_string,
                                 int line,
                                 std::vector<std::vector<int>>& idirect_from_blk_pin,
                                 std::vector<std::vector<int>>& direct_type_from_blk_pin,
                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                 const PortPinToBlockPinConverter& port_pin_to_block_pin) {
    /* Go through all the ports in all the blocks to find the port that has the same   *
     * name as port_name and belongs to the block type that has the name pb_type_name. *
     * Then, check that whether start_pin_index and end_pin_index are specified. If    *
     * they are, mark down the pins from start_pin_index to end_pin_index, inclusive.  *
     * Otherwise, mark down all the pins in that port.                                 */

    // Go through all the block types
    for (int itype = 1; itype < (int)physical_tile_types.size(); itype++) {
        auto& physical_tile = physical_tile_types[itype];
        // Find blocks with the same pb_type_name
        if (pb_type_name == physical_tile.name) {
            int num_sub_tiles = physical_tile.sub_tiles.size();
            for (int isub_tile = 0; isub_tile < num_sub_tiles; isub_tile++) {
                auto& ports = physical_tile.sub_tiles[isub_tile].ports;
                int num_ports = ports.size();
                for (int iport = 0; iport < num_ports; iport++) {
                    // Find ports with the same port_name
                    if (port_name == ports[iport].name) {
                        int num_port_pins = ports[iport].num_pins;

                        // Check whether the end_pin_index is valid
                        if (end_pin_index > num_port_pins) {
                            VTR_LOG_ERROR(
                                "[LINE %d] Invalid pin - %s, the end_pin_index in "
                                "[end_pin_index:start_pin_index] should "
                                "be less than the num_port_pins %d.\n",
                                line, src_string, num_port_pins);
                            exit(1);
                        }

                        // Check whether the pin indices are specified
                        if (start_pin_index >= 0 || end_pin_index >= 0) {
                            mark_direct_of_pins(start_pin_index, end_pin_index, itype,
                                                isub_tile, iport, idirect_from_blk_pin, idirect,
                                                direct_type_from_blk_pin, direct_type, line, src_string,
                                                physical_tile_types,
                                                port_pin_to_block_pin);
                        } else {
                            mark_direct_of_pins(0, num_port_pins - 1, itype,
                                                isub_tile, iport, idirect_from_blk_pin, idirect,
                                                direct_type_from_blk_pin, direct_type, line, src_string,
                                                physical_tile_types,
                                                port_pin_to_block_pin);
                        }
                    } // Do nothing if port_name does not match
                }     // Finish going through all the ports
            }         // Finish going through all the subtiles
        }             // Do nothing if pb_type_name does not match
    }                 // Finish going through all the blocks
}

static void mark_direct_of_pins(int start_pin_index,
                                int end_pin_index,
                                int itype,
                                int isub_tile,
                                int iport,
                                std::vector<std::vector<int>>& idirect_from_blk_pin,
                                int idirect,
                                std::vector<std::vector<int>>& direct_type_from_blk_pin,
                                int direct_type,
                                int line,
                                std::string_view src_string,
                                const std::vector<t_physical_tile_type>& physical_tile_types,
                                const PortPinToBlockPinConverter& port_pin_to_block_pin) {
    // Mark pins with indices from start_pin_index to end_pin_index, inclusive
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
            idirect_from_blk_pin[itype][iblk_pin] = idirect;

            // Check whether the pins are marked, errors out if so
            if (direct_type_from_blk_pin[itype][iblk_pin] != OPEN) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "[LINE %d] Invalid pin - %s, this pin is in more than one direct connection.\n",
                                line, src_string);
            } else {
                direct_type_from_blk_pin[itype][iblk_pin] = direct_type;
            }
        }
    } // Finish marking all the pins
}

/* Allocates and loads imacro_from_iblk array. */
void PlaceMacros::alloc_and_load_imacro_from_iblk_(const std::vector<t_pl_macro>& macros,
                                                   const ClusteredNetlist& clb_nlist) {
    imacro_from_iblk_.resize(clb_nlist.blocks().size());

    /* Allocate and initialize the values to OPEN (-1). */
    for (auto blk_id : clb_nlist.blocks()) {
        imacro_from_iblk_.insert(blk_id, OPEN);
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
                                      const ClusteredNetlist& clb_nlist) {
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
            if (idirect_from_blk_pin_[itype][ipin] != OPEN) {
                if (direct_type_from_blk_pin_[itype][ipin] == SOURCE) {
                    fprintf(f, "%-9s %-9d true      SOURCE    \n", type.name.c_str(), ipin);
                } else {
                    VTR_ASSERT(direct_type_from_blk_pin_[itype][ipin] == SINK);
                    fprintf(f, "%-9s %-9d true      SINK      \n", type.name.c_str(), ipin);
                }
            } else {
                VTR_ASSERT(direct_type_from_blk_pin_[itype][ipin] == OPEN);
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

bool PlaceMacros::net_is_driven_by_direct_(ClusterNetId clb_net,
                                           const ClusteredNetlist& clb_nlist) {
    ClusterBlockId block_id = clb_nlist.net_driver_block(clb_net);
    int pin_index = clb_nlist.net_pin_logical_index(clb_net, 0);

    auto logical_block = clb_nlist.block_type(block_id);
    auto physical_tile = pick_physical_type(logical_block);
    auto physical_pin = get_physical_pin(physical_tile, logical_block, pin_index);

    auto direct = idirect_from_blk_pin_[physical_tile->index][physical_pin];

    return direct != OPEN;
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

