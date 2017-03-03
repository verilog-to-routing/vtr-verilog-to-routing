#ifndef LUT_STATS_H
#define LUT_STATS_H

#include <utility> //for pair<>
#include "vqm_dll.h"
void print_lut_stats(t_module* module);
void print_lut_carry_stats(t_module* module);
std::pair<bool,bool> is_carry_chain_lut(t_node* node);

#endif
