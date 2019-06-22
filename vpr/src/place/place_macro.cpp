#include <cstdio>
#include <ctime>
#include <cmath>
#include <sstream>
#include <map>
using namespace std;

#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_util.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "physical_types.h"
#include "globals.h"
#include "place.h"
#include "read_xml_arch_file.h"
#include "place_macro.h"
#include "vpr_utils.h"
#include "echo_files.h"

/******************** File-scope variables declarations **********************/

/* f_idirect_from_blk_pin array allow us to quickly find pins that could be in a    *
 * direct connection. Values stored is the index of the possible direct connection  *
 * as specified in the arch file, OPEN (-1) is stored for pins that could not be    *
 * part of a direct chain conneciton.                                               *
 * [0...device_ctx.num_block_types-1][0...num_pins-1]                               */
static int** f_idirect_from_blk_pin = nullptr;

/* f_direct_type_from_blk_pin array stores the value SOURCE if the pin is the       *
 * from_pin, SINK if the pin is the to_pin in the direct connection as specified in *
 * the arch file, OPEN (-1) is stored for pins that could not be part of a direct   *
 * chain conneciton.                                                                *
 * [0...device_ctx.num_block_types-1][0...num_pins-1]                               */
static int** f_direct_type_from_blk_pin = nullptr;

/* f_imacro_from_blk_pin maps a blk_num to the corresponding macro index.           *
 * If the block is not part of a macro, the value OPEN (-1) is stored.              *
 * [0...cluster_ctx.clb_nlist.blocks().size()-1]                                    */
static vtr::vector_map<ClusterBlockId, int> f_imacro_from_iblk;

/******************** Subroutine declarations ********************************/

static void find_all_the_macro(int* num_of_macro, std::vector<ClusterBlockId>& pl_macro_member_blk_num_of_this_blk, std::vector<int>& pl_macro_idirect, std::vector<int>& pl_macro_num_members, std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num);

static void alloc_and_load_imacro_from_iblk(const std::vector<t_pl_macro>& macros);

static void write_place_macros(std::string filename, const std::vector<t_pl_macro>& macros);

static bool is_constant_clb_net(ClusterNetId clb_net);

static bool net_is_driven_by_direct(ClusterNetId clb_net);

static void validate_macros(const std::vector<t_pl_macro>& macros);

static bool try_combine_macros(std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num, int matching_macro, int latest_macro);
/******************** Subroutine definitions *********************************/

static void find_all_the_macro(int* num_of_macro, std::vector<ClusterBlockId>& pl_macro_member_blk_num_of_this_blk, std::vector<int>& pl_macro_idirect, std::vector<int>& pl_macro_num_members, std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num) {
    /* Compute required size:                                                *
     * Go through all the pins with possible direct connections in           *
     * f_idirect_from_blk_pin. Count the number of heads (which is the same  *
     * as the number macros) and also the length of each macro               *
     * Head - blocks with to_pin OPEN and from_pin connected                 *
     * Tail - blocks with to_pin connected and from_pin OPEN                 */

    int from_iblk_pin, to_iblk_pin, from_idirect, to_idirect,
        from_src_or_sink, to_src_or_sink;
    ClusterNetId to_net_id, from_net_id, next_net_id, curr_net_id;
    ClusterBlockId next_blk_id;
    int num_blk_pins, num_macro;
    int imember;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    // Hash table holding the unique cluster ids and the macro id it belongs to
    std::unordered_map<ClusterBlockId, int> clusters_macro;

    num_macro = 0;
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        num_blk_pins = cluster_ctx.clb_nlist.block_type(blk_id)->num_pins;
        for (to_iblk_pin = 0; to_iblk_pin < num_blk_pins; to_iblk_pin++) {
            to_net_id = cluster_ctx.clb_nlist.block_net(blk_id, to_iblk_pin);
            to_idirect = f_idirect_from_blk_pin[cluster_ctx.clb_nlist.block_type(blk_id)->index][to_iblk_pin];
            to_src_or_sink = f_direct_type_from_blk_pin[cluster_ctx.clb_nlist.block_type(blk_id)->index][to_iblk_pin];

            // Identify potential macro head blocks (i.e. start of a macro)
            //
            // The input SINK (to_pin) of a potential HEAD macro will have either:
            //  * no connection to any net (OPEN), or
            //  * a connection to a constant net (e.g. gnd/vcc) which is not driven by a direct
            //
            // Note that the restriction that constant nets are not driven from another direct ensures that
            // blocks in the middle of a chain with internal constant signals are not detected as potential
            // head blocks.
            if (to_src_or_sink == SINK && to_idirect != OPEN
                && (to_net_id == ClusterNetId::INVALID()
                    || (is_constant_clb_net(to_net_id)
                        && !net_is_driven_by_direct(to_net_id)))) {
                for (from_iblk_pin = 0; from_iblk_pin < num_blk_pins; from_iblk_pin++) {
                    from_net_id = cluster_ctx.clb_nlist.block_net(blk_id, from_iblk_pin);
                    from_idirect = f_idirect_from_blk_pin[cluster_ctx.clb_nlist.block_type(blk_id)->index][from_iblk_pin];
                    from_src_or_sink = f_direct_type_from_blk_pin[cluster_ctx.clb_nlist.block_type(blk_id)->index][from_iblk_pin];

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
                        next_net_id = from_net_id;
                        next_blk_id = blk_id;

                        // Start finding the other members
                        while (next_net_id != ClusterNetId::INVALID()) {
                            curr_net_id = next_net_id;

                            // Assume that carry chains only has 1 sink - direct connection
                            VTR_ASSERT(cluster_ctx.clb_nlist.net_sinks(curr_net_id).size() == 1);
                            next_blk_id = cluster_ctx.clb_nlist.net_pin_block(curr_net_id, 1);

                            // Assume that the from_iblk_pin index is the same for the next block
                            VTR_ASSERT(f_idirect_from_blk_pin[cluster_ctx.clb_nlist.block_type(next_blk_id)->index][from_iblk_pin] == from_idirect
                                       && f_direct_type_from_blk_pin[cluster_ctx.clb_nlist.block_type(next_blk_id)->index][from_iblk_pin] == SOURCE);
                            next_net_id = cluster_ctx.clb_nlist.block_net(next_blk_id, from_iblk_pin);

                            // Mark down this block as a member of the macro
                            imember = pl_macro_num_members[num_macro];
                            pl_macro_member_blk_num_of_this_blk[imember] = next_blk_id;

                            // Increment the num_member count.
                            pl_macro_num_members[num_macro]++;

                        } // Found all the members of this macro at this point

                        // Allocate the second dimension of the blk_num array since I now know the size
                        pl_macro_member_blk_num[num_macro].resize(pl_macro_num_members[num_macro]);
                        int matching_macro = -1;
                        // Copy the data from the temporary array to the newly allocated array.
                        for (imember = 0; imember < pl_macro_num_members[num_macro]; imember++) {
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
    *num_of_macro = num_macro;
}

static bool try_combine_macros(std::vector<std::vector<ClusterBlockId>>& pl_macro_member_blk_num, int matching_macro, int latest_macro) {
    /* This function takes two placement macro ids which have a common cluster block
     * or more in between. The function then tries to find if the two macros could
     * be combined together to form a larger macro. If it's impossible to combine
     * the two macros together then this design will never place and route.
     * Arguments:
     *  pl_macro_member_blk_num : [0..num_macros-1][0..num_cluster_blocks-1] 2D array
     *                            of macros created so far.
     *  matching_macro          : first macro id, which is a previous macro that is found to have the same block
     *  latest_macro            : second macro id, which is the macro being created at this iteration */

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
        if (new_macro_it == old_macro_blocks.end()) {
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

std::vector<t_pl_macro> alloc_and_load_placement_macros(t_direct_inf* directs, int num_directs) {
    /* This function allocates and loads the macros placement macros   *
     * and returns the total number of macros in 2 steps.              *
     *   1) Allocate temporary data structure for maximum possible     *
     *      size and loops through all the blocks storing the data     *
     *      relevant to the carry chains. At the same time, also count *
     *      the amount of memory required for the actual variables.    *
     *   2) Allocate the actual variables with the exact amount of     *
     *      memory. Then loads the data from the temporary data        *
     *       structures before freeing them.                           *
     *                                                                 *
     * For pl_macro_member_blk_num, allocate for the first dimension   *
     * only at first. Allocate for the second dimension when I know    *
     * the size. Otherwise, the array is going to be of size           *
     * cluster_ctx.clb_nlist.blocks().size()^2 (There are big		   *
     * benckmarks VPR that have cluster_ctx.clb_nlist.blocks().size()  *
     * in the 100k's range).										   *
     *																   *
     * The placement macro array is freed by the caller(s).            */

    /* Declaration of local variables */
    int num_macro;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Allocate maximum memory for temporary variables. */
    std::vector<int> pl_macro_idirect(cluster_ctx.clb_nlist.blocks().size());
    std::vector<int> pl_macro_num_members(cluster_ctx.clb_nlist.blocks().size());
    std::vector<std::vector<ClusterBlockId>> pl_macro_member_blk_num(cluster_ctx.clb_nlist.blocks().size());
    std::vector<ClusterBlockId> pl_macro_member_blk_num_of_this_blk(cluster_ctx.clb_nlist.blocks().size());

    /* Sets up the required variables. */
    alloc_and_load_idirect_from_blk_pin(directs, num_directs,
                                        &f_idirect_from_blk_pin, &f_direct_type_from_blk_pin);

    /* Compute required size:                                                *
     * Go through all the pins with possible direct connections in           *
     * f_idirect_from_blk_pin. Count the number of heads (which is the same  *
     * as the number macros) and also the length of each macro               *
     * Head - blocks with to_pin OPEN and from_pin connected                 *
     * Tail - blocks with to_pin connected and from_pin OPEN                 */
    num_macro = 0;
    find_all_the_macro(&num_macro, pl_macro_member_blk_num_of_this_blk,
                       pl_macro_idirect, pl_macro_num_members, pl_macro_member_blk_num);

    /* Allocate the memories for the macro. */
    std::vector<t_pl_macro> macros(num_macro);

    /* Allocate the memories for the chain members.             *
     * Load the values from the temporary data structures.      */
    for (int imacro = 0; imacro < num_macro; imacro++) {
        macros[imacro].members = std::vector<t_pl_macro_member>(pl_macro_num_members[imacro]);

        /* Load the values for each member of the macro */
        for (size_t imember = 0; imember < macros[imacro].members.size(); imember++) {
            macros[imacro].members[imember].offset.x = imember * directs[pl_macro_idirect[imacro]].x_offset;
            macros[imacro].members[imember].offset.y = imember * directs[pl_macro_idirect[imacro]].y_offset;
            macros[imacro].members[imember].offset.z = directs[pl_macro_idirect[imacro]].z_offset;
            macros[imacro].members[imember].blk_index = pl_macro_member_blk_num[imacro][imember];
        }
    }

    if (isEchoFileEnabled(E_ECHO_PLACE_MACROS)) {
        write_place_macros(getEchoFileName(E_ECHO_PLACE_MACROS), macros);
    }

    validate_macros(macros);

    return macros;
}

void get_imacro_from_iblk(int* imacro, ClusterBlockId iblk, const std::vector<t_pl_macro>& macros) {
    /* This mapping is needed for fast lookup's whether the block with index *
     * iblk belongs to a placement macro or not.                             *
     *                                                                       *
     * The array f_imacro_from_iblk is used for the mapping for speed reason *
     * [0...cluster_ctx.clb_nlist.blocks().size()-1]                                                    */

    /* If the array is not allocated and loaded, allocate it.                */
    if (f_imacro_from_iblk.size() == 0) {
        alloc_and_load_imacro_from_iblk(macros);
    }

    if (iblk) {
        /* Return the imacro for the block. */
        *imacro = f_imacro_from_iblk[iblk];
    } else {
        *imacro = OPEN; //No valid block, so no valid macro
    }
}

/* Allocates and loads imacro_from_iblk array. */
static void alloc_and_load_imacro_from_iblk(const std::vector<t_pl_macro>& macros) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    f_imacro_from_iblk.resize(cluster_ctx.clb_nlist.blocks().size());

    /* Allocate and initialize the values to OPEN (-1). */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        f_imacro_from_iblk.insert(blk_id, OPEN);
    }

    /* Load the values */
    for (size_t imacro = 0; imacro < macros.size(); imacro++) {
        for (size_t imember = 0; imember < macros[imacro].members.size(); imember++) {
            ClusterBlockId blk_id = macros[imacro].members[imember].blk_index;
            f_imacro_from_iblk.insert(blk_id, imacro);
        }
    }
}

void free_placement_macros_structs() {
    /* This function frees up all the static data structures used. */

    // This frees up the two arrays and set the pointers to NULL
    auto& device_ctx = g_vpr_ctx.device();
    int itype;
    if (f_idirect_from_blk_pin != nullptr) {
        for (itype = 1; itype < device_ctx.num_block_types; itype++) {
            free(f_idirect_from_blk_pin[itype]);
        }
        free(f_idirect_from_blk_pin);
        f_idirect_from_blk_pin = nullptr;
    }

    if (f_direct_type_from_blk_pin != nullptr) {
        for (itype = 1; itype < device_ctx.num_block_types; itype++) {
            free(f_direct_type_from_blk_pin[itype]);
        }
        free(f_direct_type_from_blk_pin);
        f_direct_type_from_blk_pin = nullptr;
    }
}

static void write_place_macros(std::string filename, const std::vector<t_pl_macro>& macros) {
    FILE* f = vtr::fopen(filename.c_str(), "w");

    auto& cluster_ctx = g_vpr_ctx.clustering();

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
                    cluster_ctx.clb_nlist.block_name(macro_memb->blk_index).c_str(),
                    macro_memb->offset.x,
                    macro_memb->offset.y,
                    macro_memb->offset.z);
        }
        fprintf(f, "\n");
    }

    fprintf(f, "\n");

    fprintf(f, "#Macro-related direct connections\n");
    fprintf(f, "type      type_pin  is_direct direct_type\n");
    fprintf(f, "------------------------------------------\n");
    auto& device_ctx = g_vpr_ctx.device();
    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        t_type_descriptor* type = &device_ctx.block_types[itype];

        for (int ipin = 0; ipin < type->num_pins; ++ipin) {
            if (f_idirect_from_blk_pin[itype][ipin] != OPEN) {
                if (f_direct_type_from_blk_pin[itype][ipin] == SOURCE) {
                    fprintf(f, "%-9s %-9d true      SOURCE    \n", type->name, ipin);
                } else {
                    VTR_ASSERT(f_direct_type_from_blk_pin[itype][ipin] == SINK);
                    fprintf(f, "%-9s %-9d true      SINK      \n", type->name, ipin);
                }
            } else {
                VTR_ASSERT(f_direct_type_from_blk_pin[itype][ipin] == OPEN);
            }
        }
    }

    fclose(f);
}

static bool is_constant_clb_net(ClusterNetId clb_net) {
    auto& atom_ctx = g_vpr_ctx.atom();
    AtomNetId atom_net = atom_ctx.lookup.atom_net(clb_net);

    return atom_ctx.nlist.net_is_constant(atom_net);
}

static bool net_is_driven_by_direct(ClusterNetId clb_net) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    ClusterBlockId block_id = cluster_ctx.clb_nlist.net_driver_block(clb_net);
    int pin_index = cluster_ctx.clb_nlist.net_pin_physical_index(clb_net, 0);

    auto direct = f_idirect_from_blk_pin[cluster_ctx.clb_nlist.block_type(block_id)->index][pin_index];

    return direct != OPEN;
}

static void validate_macros(const std::vector<t_pl_macro>& macros) {
    //Perform sanity checks on macros
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Verify that blocks only appear in a single macro
    std::multimap<ClusterBlockId, int> block_to_macro;
    for (size_t imacro = 0; imacro < macros.size(); ++imacro) {
        for (size_t imember = 0; imember < macros[imacro].members.size(); ++imember) {
            ClusterBlockId iblk = macros[imacro].members[imember].blk_index;

            block_to_macro.emplace(iblk, imacro);
        }
    }

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto range = block_to_macro.equal_range(blk_id);

        int blk_macro_cnt = std::distance(range.first, range.second);
        if (blk_macro_cnt > 1) {
            std::stringstream msg;
            msg << "Block #" << size_t(blk_id) << " '" << cluster_ctx.clb_nlist.block_name(blk_id) << "'"
                << " appears in " << blk_macro_cnt << " placement macros (should appear in at most one). Related Macros:\n";

            for (auto iter = range.first; iter != range.second; ++iter) {
                int imacro = iter->second;
                msg << "  Macro #: " << imacro << "\n";
            }

            VPR_THROW(VPR_ERROR_PLACE, msg.str().c_str());
        }
    }
}
