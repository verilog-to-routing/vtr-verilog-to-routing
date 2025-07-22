#pragma once

#include "read_xml_util.h"

/**
 * @brief Parses Scatter-Gather related information under <scatter_gather_list> tag.
 * @param sg_list_tag An XML node pointing to <scatter_gather_list> tag.
 * @param arch High-level architecture information. This function fills
 * arch->scatter_gather_patterns with NoC-related information.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 */
void process_sg_tag(pugi::xml_node sg_list_tag,
                    t_arch* arch,
                    const pugiutil::loc_data& loc_data,
                    const std::vector<t_arch_switch_inf>& switches);