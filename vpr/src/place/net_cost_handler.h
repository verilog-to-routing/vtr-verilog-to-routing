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

void update_move_nets(int num_nets_affected);

void reset_move_nets(int num_nets_affected);

void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                  const t_noc_opts& noc_opts,
                                  const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities* criticalities,
                                  t_placer_costs* costs);

void init_net_cost_structs(size_t num_nets);
