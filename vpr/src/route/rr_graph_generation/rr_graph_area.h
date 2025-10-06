#pragma once

#include <vector>
#include "physical_types.h"

void count_routing_transistors(enum e_directionality directionality,
                               int num_switch,
                               RRSwitchId wire_to_ipin_switch,
                               std::vector<t_segment_inf>& segment_inf,
                               float R_minW_nmos,
                               float R_minW_pmos,
                               bool is_flat);

float trans_per_buf(float Rbuf, float R_minW_nmos, float R_minW_pmos);
