#ifndef READ_XML_UTIL_H
#define READ_XML_UTIL_H

#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"
#include "arch_util.h"

pugiutil::ReqOpt BoolToReqOpt(bool b);

void bad_tag(const pugi::xml_node node,
             const pugiutil::loc_data& loc_data,
             const pugi::xml_node parent_node = pugi::xml_node(),
             const std::vector<std::string> expected_tags = std::vector<std::string>());

void bad_attribute(const pugi::xml_attribute attr,
                   const pugi::xml_node node,
                   const pugiutil::loc_data& loc_data,
                   const std::vector<std::string> expected_attributes = std::vector<std::string>());
void bad_attribute_value(const pugi::xml_attribute attr,
                         const pugi::xml_node node,
                         const pugiutil::loc_data& loc_data,
                         const std::vector<std::string> expected_attributes = std::vector<std::string>());

InstPort make_inst_port(std::string str, pugi::xml_node node, const pugiutil::loc_data& loc_data);
InstPort make_inst_port(pugi::xml_attribute attr, pugi::xml_node node, const pugiutil::loc_data& loc_data);

#endif
