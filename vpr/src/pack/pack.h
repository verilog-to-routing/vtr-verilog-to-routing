#ifndef PACK_H
#define PACK_H
#include <vector>
#include <unordered_set>
#include "vpr_types.h"
#include "atom_netlist_fwd.h"

bool try_pack(t_packer_opts* packer_opts,
              const t_analysis_opts* analysis_opts,
              const t_arch* arch,
              const t_model* user_models,
              const t_model* library_models,
              float interc_delay,
              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs);

float get_arch_switch_info(short switch_index, int switch_fanin, float& Tdel_switch, float& R_switch, float& Cout_switch);

std::unordered_set<AtomNetId> alloc_and_load_is_clock(bool global_clocks);

#endif
