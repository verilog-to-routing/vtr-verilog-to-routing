#pragma once
#include "place_delay_model.h"
#include "timing_place.h"
#include "move_transactions.h"
#include "place_util.h"

enum e_cost_methods {
    NORMAL,
    CHECK
};

/**
 * @brief Update the wire length and timing cost of the blocks (ts and proposed_* data structures) and set
 * the delta costs in bb_delta_c and timing_delta_c. This functions is used when the moving bocks are atoms
 * @param place_algorithm
 * @param delay_model
 * @param criticalities
 * @param blocks_affected
 * @param bb_delta_c
 * @param timing_delta_c
 * @return
 */
int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_atom_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c);

/**
 * @brief Update the wire length and timing cost of the blocks (ts and proposed_* data structures) and set
 * the delta costs in bb_delta_c and timing_delta_c. This functions is used when the moving bocks are clusters
 * @param place_algorithm
 * @param delay_model
 * @param criticalities
 * @param blocks_affected
 * @param bb_delta_c
 * @param timing_delta_c
 * @return
 */
int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c);

/**
 * @brief Finds the bb cost from scratch (based on 3D BB).  Done only when the placement   *
* has been radically changed (i.e. after initial placement).   *
* Otherwise find the cost change incrementally.  If method     *
* check is NORMAL, we find bounding boxes that are updatable  *
* for the larger nets.  If method is CHECK, all bounding boxes *
* are found via the non_updateable_bb routine, to provide a    *
* cost which can be used to check the correctness of the       *
* other routine.                                               *
 * @param method
 * @return
 */
double comp_bb_cost(e_cost_methods method);

/**
 * @brief Finds the bb cost from scratch (based on per-layer BB).  Done only when the placement   *
* has been radically changed (i.e. after initial placement).   *
* Otherwise find the cost change incrementally.  If method     *
* check is NORMAL, we find bounding boxes that are updateable  *
* for the larger nets.  If method is CHECK, all bounding boxes *
* are found via the non_updateable_bb routine, to provide a    *
* cost which can be used to check the correctness of the       *
* other routine.                                               *
 * @param method
 * @return
 */
double comp_layer_bb_cost(e_cost_methods method);

/**
 * @brief update net cost data structures (in placer context and net_cost in .cpp file) and reset flags (proposed_net_cost and bb_updated_before).
 * @param num_nets_affected
 * @param cube_bb
 */
void update_move_nets(int num_nets_affected,
                      const bool cube_bb);

/**
 * @brief Reset the net cost function flags (proposed_net_cost and bb_updated_before)
 * @param num_nets_affected
 */
void reset_move_nets(int num_nets_affected);

/**
 * @brief re-calculates different terms of the cost function (wire-length, timing, NoC) and update "costs" accordingly. It is important to note that
 * in this function bounding box and connection delays are not calculated from scratch. However, it iterated over nets and add their costs from beginning.
 * @param placer_opts
 * @param noc_opts
 * @param delay_model
 * @param criticalities
 * @param costs
 */
void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                  const t_noc_opts& noc_opts,
                                  const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities* criticalities,
                                  t_placer_costs* costs);

/**
 * @brief Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac
 * arrays with the inverse of the average number of tracks per channel
 * between [subhigh] and [sublow].
 * @param place_cost_exp
 */
void alloc_and_load_for_fast_cost_update(float place_cost_exp);

/**
 * @brief Frees the chanx_place_cost_fac and chany_place_cost_fac arrays.
 */
void free_fast_cost_update();

/**
 * @brief Resize net_cost, proposed_net_cost, and  bb_updated_before data structures to accommodate all nets.
 * @param num_nets
 */
void init_net_cost_structs(size_t num_nets);

/**
 * @brief Free net_cost, proposed_net_cost, and  bb_updated_before data structures.
 */
void free_net_cost_structs();

/**
 * @brief Resize (layer_)ts_bb_edge_new, (layer_)ts_bb_coord_new, ts_layer_sink_pin_count, and ts_nets_to_update to accommodate all nets.
 * @param num_nets
 * @param cube_bb
 */
void init_try_swap_net_cost_structs(size_t num_nets, bool cube_bb);

/**
 * @brief Free (layer_)ts_bb_edge_new, (layer_)ts_bb_coord_new, ts_layer_sink_pin_count, and ts_nets_to_update data structures.
 */
void free_try_swap_net_cost_structs();
