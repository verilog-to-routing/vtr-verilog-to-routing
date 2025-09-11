#pragma once

/**
 * @file read_xml_arch_file_interposer.h
 * @brief This file contains functions related to parsing and processing interposer tags in the architecture file
 * 
 */

#include "interposer_types.h"
#include "read_xml_util.h"

/**
 * @brief Parse an <interposer_cut> tag and its children
 * 
 * @param interposer_cut_tag xml_node pointing to the <interposer_cut> tag
 * @param loc_data Points to the location in the architecture file where the parser is reading. Used for priting error messages.
 * @return t_interposer_cut_inf with parsed information of the <interposer_cut> tag
 */
t_interposer_cut_inf parse_interposer_cut_tag(pugi::xml_node interposer_cut_tag, const pugiutil::loc_data& loc_data);
