#pragma once

#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "vtr_expr_eval.h"

/**
 * @brief Loads permutation functions from an XML node into a switchblock structure.
 *
 * This function parses an XML `<switchfuncs>` node, extracts all `<func>` child elements,
 * and populates the corresponding entries in the `t_switchblock_inf::permutation_map`.
 * Each `<func>` element must have a `type` attribute indicating the connection type
 * (e.g., side-to-side) and a `formula` attribute specifying the permutation function.
 *
 * @param node      XML node containing the `<switchfuncs>` specification.
 * @param sb        The switchblock structure where permutation functions will be stored.
 * @param loc_data  Location data used for error reporting during XML parsing.
 *
 * The function expects the following XML structure:
 * @code{.xml}
 * <switchfuncs>
 *     <func type="..." formula="..."/>
 *     ...
 * </switchfuncs>
 * @endcode
 */
void read_sb_switchfuncs(pugi::xml_node node, t_switchblock_inf& sb, const pugiutil::loc_data& loc_data);

/* Reads-in the wire connections specified for the switchblock in the xml arch file */
void read_sb_wireconns(const std::vector<t_arch_switch_inf>& switches, pugi::xml_node Node, t_switchblock_inf* sb, const pugiutil::loc_data& loc_data);

/* checks for correctness of switch block read-in from the XML architecture file */
void check_switchblock(const t_switchblock_inf* sb, const t_arch* arch);

/* returns integer result according to the specified formula and data */
int get_sb_formula_raw_result(vtr::FormulaParser& formula_parser, const char* formula, const vtr::t_formula_data& mydata);
