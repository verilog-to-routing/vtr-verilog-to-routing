#pragma once
#include "place_delay_model.h"
#include "timing_place.h"
#include "move_transactions.h"

static int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_atom_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c);

static int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c);
