#pragma once
#include "place_delay_model.h"
#include "timing_place.h"
#include "move_transactions.h"
#include "place_util.h"

enum e_cost_methods {
    NORMAL,
    CHECK
};

int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_atom_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c);

int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c);

double comp_bb_cost(e_cost_methods method);

double comp_layer_bb_cost(e_cost_methods method);

void update_move_nets(int num_nets_affected,
                      const bool cube_bb);

void reset_move_nets(int num_nets_affected);

void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                  const t_noc_opts& noc_opts,
                                  const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities* criticalities,
                                  t_placer_costs* costs);

void alloc_and_load_for_fast_cost_update(float place_cost_exp);

void free_fast_cost_update();

void init_net_cost_structs(size_t num_nets);

void free_net_cost_structs();

void init_try_swap_net_cost_structs(size_t num_nets, bool cube_bb);

void free_try_swap_net_cost_structs();
