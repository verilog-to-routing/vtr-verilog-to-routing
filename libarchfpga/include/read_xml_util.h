#ifndef READ_XML_UTIL_H
#define READ_XML_UTIL_H

#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "pugixml_util.hpp"

pugiutil::ReqOpt BoolToReqOpt(bool b);

void archfpga_throw(const char* filename, int line, const char* fmt, ...);

void bad_tag(const pugi::xml_node node,
             const pugiutil::loc_data& loc_data,
             const pugi::xml_node parent_node=pugi::xml_node(),
             const std::vector<std::string> expected_tags=std::vector<std::string>());

void bad_attribute(const pugi::xml_attribute attr,
                   const pugi::xml_node node,
                   const pugiutil::loc_data& loc_data,
                   const std::vector<std::string> expected_attributes=std::vector<std::string>());
void bad_attribute_value(const pugi::xml_attribute attr,
                         const pugi::xml_node node,
                         const pugiutil::loc_data& loc_data,
                         const std::vector<std::string> expected_attributes=std::vector<std::string>());

class InstPort {
    public:
        InstPort(std::string str);
        InstPort(pugi::xml_attribute attr, pugi::xml_node node, const pugiutil::loc_data& loc_data);
        std::string instance_name() { return instance_.name; }
        std::string port_name() { return port_.name; }

        size_t instance_low_index() { return instance_.low_idx; }
        size_t instance_high_index() { return instance_.high_idx; }
        size_t port_low_index() { return port_.low_idx; }
        size_t port_high_index() { return port_.high_idx; }

    private:
        struct name_index {
            std::string name;
            size_t low_idx;
            size_t high_idx;
        };

        name_index parse_name_index(std::string str);

        name_index instance_;
        name_index port_;
};
#endif
