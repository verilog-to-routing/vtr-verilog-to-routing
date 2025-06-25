#pragma once

#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"
#include "arch_util.h"

pugiutil::ReqOpt BoolToReqOpt(bool b);

void bad_tag(const pugi::xml_node node,
             const pugiutil::loc_data& loc_data,
             const pugi::xml_node parent_node = pugi::xml_node(),
             const std::vector<std::string>& expected_tags = std::vector<std::string>());

void bad_attribute(const pugi::xml_attribute attr,
                   const pugi::xml_node node,
                   const pugiutil::loc_data& loc_data,
                   const std::vector<std::string>& expected_attributes = std::vector<std::string>());
void bad_attribute_value(const pugi::xml_attribute attr,
                         const pugi::xml_node node,
                         const pugiutil::loc_data& loc_data,
                         const std::vector<std::string>& expected_attributes = std::vector<std::string>());

InstPort make_inst_port(std::string str, pugi::xml_node node, const pugiutil::loc_data& loc_data);
InstPort make_inst_port(pugi::xml_attribute attr, pugi::xml_node node, const pugiutil::loc_data& loc_data);

int get_number_of_layers(pugi::xml_node layout_type_tag, const pugiutil::loc_data& loc_data);

/**
 * @brief Processes <metadata> tags.
 *
 * @param strings String internment storage used to store strings used
 * as keys and values in <metadata> tags.
 * @param Parent An XML node pointing to the parent tag whose <metadata> children
 * are to be parsed.
 * @param loc_data Points to the location in the architecture file where the parser is reading.
 * @return A t_metadata_dict that stored parsed (key, value) pairs.
 */
t_metadata_dict process_meta_data(vtr::string_internment& strings,
                                  pugi::xml_node Parent,
                                  const pugiutil::loc_data& loc_data);
