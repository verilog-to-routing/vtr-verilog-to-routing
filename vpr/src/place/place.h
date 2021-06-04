#ifndef VPR_PLACE_H
#define VPR_PLACE_H

#include "vpr_types.h"
#include "manual_moves.h"
void try_place(const t_placer_opts& placer_opts,
               t_annealing_sched annealing_sched,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               t_direct_inf* directs,
               int num_directs);

bool placer_needs_lookahead(const t_vpr_setup& vpr_setup);
void manual_move_info_from_user_and_open_window(ManualMovesInfo* manual_move_info);
//void update_manual_move_cost_and_open_window(ManualMovesInfo* manual_move_info, e_move_result& move_outcome, double delta_c, double delta_bb, double delta_t);

#endif
