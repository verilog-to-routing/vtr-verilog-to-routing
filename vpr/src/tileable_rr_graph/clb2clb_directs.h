#ifndef CLB2CLB_DIRECTS_H
#define CLB2CLB_DIRECTS_H

#include "physical_types.h"

struct t_clb_to_clb_directs {
    t_physical_tile_type_ptr from_clb_type;
    int from_clb_pin_start_index;
    int from_clb_pin_end_index;
    t_physical_tile_type_ptr to_clb_type;
    int to_clb_pin_start_index;
    int to_clb_pin_end_index;
    int switch_index; //The switch type used by this direct connection
};

#endif
