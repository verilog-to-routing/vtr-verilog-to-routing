#ifndef PARSE_SWITCHBLOCKS_H
#define PARSE_SWITCHBLOCKS_H

#include <vector>
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "expr_eval.h"

/**** Function Declarations ****/
/* Loads permutation funcs specified under Node into t_switchblock_inf */
void read_sb_switchfuncs(pugi::xml_node Node, t_switchblock_inf* sb, const pugiutil::loc_data& loc_data);

/* Reads-in the wire connections specified for the switchblock in the xml arch file */
void read_sb_wireconns(const t_arch_switch_inf* switches, int num_switches, pugi::xml_node Node, t_switchblock_inf* sb, const pugiutil::loc_data& loc_data);

/* checks for correctness of switch block read-in from the XML architecture file */
void check_switchblock(const t_switchblock_inf* sb, const t_arch* arch);

/* returns integer result according to the specified formula and data */
int get_sb_formula_raw_result(const char* formula, const t_formula_data& mydata);

#endif /* PARSE_SWITCHBLOCKS_H */
