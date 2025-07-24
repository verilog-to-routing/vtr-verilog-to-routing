#pragma once

#include "read_xml_util.h"

/**
 * @brief Parses a <scatter_gather_list> tag and all child tags and fills arch->scatter_gather_patterns.
 * This function should be called after read_xml_arch_file.cpp:process_switches().
 * 
 * @param sg_list_tag XML node pointing to the <scatter_gather_patterns> tag.
 * @param arch High-level architecture information. This function fills
 * arch->scatter_gather_patterns with scatter-gather-related information.
 * @param loc_data Points to the location in the architecture file where the parser is reading. Used for priting error messages.
 * @param switches Contains all the architecture switches. Usually same as arch->switches.
 */
void process_sg_tag(pugi::xml_node sg_list_tag,
                    t_arch* arch,
                    const pugiutil::loc_data& loc_data,
                    const std::vector<t_arch_switch_inf>& switches);
