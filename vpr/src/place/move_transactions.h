#ifndef VPR_MOVE_TRANSACTIONS_H
#define VPR_MOVE_TRANSACTIONS_H
#include "vpr_types.h"
#include "clustered_netlist_utils.h"

enum class e_block_move_result {
    VALID,       //Move successful
    ABORT,       //Unable to perform move
    INVERT,      //Try move again but with from/to inverted
    INVERT_VALID //Completed inverted move
};

/* Stores the information of the move for a block that is       *
 * moved during placement                                       *
 * block_num: the index of the moved block                      *
 * old_loc: the location the block is moved from                *
 * new_loc: the location the block is moved to                  */
struct t_pl_moved_block {
    t_pl_moved_block() = default;
    t_pl_moved_block(ClusterBlockId block_num_, const t_pl_loc& old_loc_, const t_pl_loc& new_loc_)
        : block_num(block_num_)
        , old_loc(old_loc_)
        , new_loc(new_loc_) {}
    ClusterBlockId block_num;
    t_pl_loc old_loc;
    t_pl_loc new_loc;
};

/* Stores the list of cluster blocks to be moved in a swap during       *
 * placement.                                                   *
 * Store the information on the blocks to be moved in a swap during     *
 * placement, in the form of array of structs instead of struct with    *
 * arrays for cache effifiency                                          *
 *
 * moved blocks: a list of moved blocks data structure with     *
 *               information on the move.                       *
 *               [0...max_blocks-1]                       *
 * affected_pins: pins affected by this move (used to           *
 *                incrementally invalidate parts of the timing  *
 *                graph.                                        */
struct t_pl_blocks_to_be_moved {
    explicit t_pl_blocks_to_be_moved(size_t max_blocks);
    t_pl_blocks_to_be_moved() = delete;
    t_pl_blocks_to_be_moved(const t_pl_blocks_to_be_moved&) = delete;
    t_pl_blocks_to_be_moved(t_pl_blocks_to_be_moved&&) = delete;

/**
 * @brief This function increments the size of the moved_blocks vector and return the index
 * of the newly added last elements. 
 */
    size_t get_size_and_increment();

/**
 * @brief This function clears all data structures of this struct.
 */
    void clear_move_blocks();

    e_block_move_result record_block_move(ClusterBlockId blk, t_pl_loc to);
    
    std::set<t_pl_loc> determine_locations_emptied_by_move();

    std::vector<t_pl_moved_block> moved_blocks;
    std::unordered_set<t_pl_loc> moved_from;
    std::unordered_set<t_pl_loc> moved_to;

    std::vector<ClusterPinId> affected_pins;
};


void apply_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected);

void commit_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected);

void revert_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected);

#endif
