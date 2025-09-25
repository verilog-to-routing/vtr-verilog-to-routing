
#pragma once

#include <map>
#include <vector>

struct t_arch_switch_inf;
struct t_rr_switch_inf;
class RRGraphBuilder;

// Sets the spec for the rr_switch based on the arch switch
void load_rr_switch_from_arch_switch(RRGraphBuilder& rr_graph_builder,
                                     const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                     int arch_switch_idx,
                                     int rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos);

t_rr_switch_inf create_rr_switch_from_arch_switch(const t_arch_switch_inf& arch_sw_inf,
                                                  const float R_minW_nmos,
                                                  const float R_minW_pmos);

void alloc_and_load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                  std::vector<std::map<int, int>>& switch_fanin_remap,
                                  const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                  const float R_minW_nmos,
                                  const float R_minW_pmos,
                                  const int wire_to_arch_ipin_switch,
                                  int* wire_to_rr_ipin_switch);
