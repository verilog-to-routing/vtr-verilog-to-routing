#include "read_xml_arch_file_sg.h"
#include <limits>
#include <string_view>
#include "read_xml_util.h"
#include "parse_switchblocks.h"
#include "pugixml_util.hpp"
#include "arch_error.h"
#include "arch_types.h"
#include "switchblock_types.h"
#include "vtr_util.h"

/**
 * @brief Parses all <sg_link> tags under a <sg_link_list> tag.
 * 
 * @param sg_link_list_tag XML node pointing to the <sg_link_list> tag.
 * @param loc_data Points to the location in the architecture file where the parser is reading. Used for printing error messages.
 * @return std::vector<t_sg_link> the information for the sg_links in the sg_link_list.
 */
static std::vector<t_sg_link> parse_sg_link_tags(pugi::xml_node sg_link_list_tag,
                                                 const std::vector<t_arch_switch_inf>& switches,
                                                 const pugiutil::loc_data& loc_data) {
    std::vector<t_sg_link> sg_link_list;
    pugiutil::expect_only_children(sg_link_list_tag, {"sg_link"}, loc_data);
    for (pugi::xml_node node : sg_link_list_tag.children()) {
        const std::vector<std::string> expected_attributes = {"name", "x_offset", "y_offset", "z_offset", "mux", "seg_type"};
        pugiutil::expect_only_attributes(node, expected_attributes, loc_data);

        t_sg_link sg_link;
        sg_link.name = pugiutil::get_attribute(node, "name", loc_data).as_string();

        std::string_view mux_name = pugiutil::get_attribute(node, "mux", loc_data).as_string();
        if (!mux_name.empty()) {
            int mux_index = find_switch_by_name(switches, mux_name);
            if (mux_index < 0) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
                               vtr::string_fmt("'%s' is not a valid mux name.\n", mux_name.data()).c_str());
            } else {
                sg_link.mux_index = mux_index;
            }
        }

        sg_link.seg_type = pugiutil::get_attribute(node, "seg_type", loc_data).as_string();

        // Since the offset attributes are optional and might not exist, the as_int method will return a value of zero if the attribute is empty
        sg_link.x_offset = pugiutil::get_attribute(node, "x_offset", loc_data, pugiutil::OPTIONAL).as_int(0);
        sg_link.y_offset = pugiutil::get_attribute(node, "y_offset", loc_data, pugiutil::OPTIONAL).as_int(0);
        sg_link.z_offset = pugiutil::get_attribute(node, "z_offset", loc_data, pugiutil::OPTIONAL).as_int(0);

        if (!vtr::exactly_k_conditions(1, sg_link.x_offset != 0, sg_link.y_offset != 0, sg_link.z_offset != 0)) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "One and only one of the offset fields in the <sg_link> should be non-zero.");
        }

        sg_link_list.push_back(sg_link);
    }

    return sg_link_list;
}

/**
 * @brief Parses all <sg_location> tags under a <sg_pattern> tag.
 * 
 * @param sg_pattern_tag XML node pointing to the <sg_pattern> tag.
 * @param loc_data Points to the location in the architecture file where the parser is reading. Used for printing error messages.
 * @return std::vector<t_sg_location> contains information of scatter gather locations.
 */
static std::vector<t_sg_location> parse_sg_location_tags(pugi::xml_node sg_pattern_tag,
                                                         const pugiutil::loc_data& loc_data) {
    std::vector<t_sg_location> sg_location_list;
    for (pugi::xml_node node : sg_pattern_tag.children()) {
        if (strcmp(node.name(), "sg_location") != 0) continue;

        t_sg_location sg_location;

        sg_location.num = pugiutil::get_attribute(node, "num", loc_data).as_string();
        sg_location.sg_link_name = pugiutil::get_attribute(node, "sg_link_name", loc_data).as_string();

        auto sg_location_type_iter = SB_LOCATION_STRING_MAP.find(pugiutil::get_attribute(node, "type", loc_data).as_string());
        if (sg_location_type_iter == SB_LOCATION_STRING_MAP.end()) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
                           vtr::string_fmt("unrecognized switchblock location: %s\n", pugiutil::get_attribute(node, "type", loc_data).as_string()).c_str());
        } else {
            sg_location.type = sg_location_type_iter->second;
        }

        if (sg_location.type == e_sb_location::E_XY_SPECIFIED) {
            pugiutil::expect_only_attributes(node,
                                             {"num", "sg_link_name", "type",
                                              "x", "y",
                                              "startx", "endx", "repeatx", "incrx",
                                              "starty", "endy", "repeaty", "incry"},
                                             loc_data);

            sg_location.region.x = pugiutil::get_attribute(node, "x", loc_data, pugiutil::OPTIONAL).as_int(ARCH_FPGA_UNDEFINED_VAL);
            sg_location.region.y = pugiutil::get_attribute(node, "y", loc_data, pugiutil::OPTIONAL).as_int(ARCH_FPGA_UNDEFINED_VAL);

            sg_location.region.reg_x.start = pugiutil::get_attribute(node, "startx", loc_data, pugiutil::OPTIONAL).as_int(0);
            // No explicit endx: use INT_MAX so the region extends to the device edge.
            sg_location.region.reg_x.end = pugiutil::get_attribute(node, "endx", loc_data, pugiutil::OPTIONAL).as_int(std::numeric_limits<int>::max());

            sg_location.region.reg_y.start = pugiutil::get_attribute(node, "starty", loc_data, pugiutil::OPTIONAL).as_int(0);
            // Same as endx: default means extend to the device edge.
            sg_location.region.reg_y.end = pugiutil::get_attribute(node, "endy", loc_data, pugiutil::OPTIONAL).as_int(std::numeric_limits<int>::max());

            sg_location.region.reg_x.repeat = pugiutil::get_attribute(node, "repeatx", loc_data, pugiutil::OPTIONAL).as_int(std::numeric_limits<int>::max());
            sg_location.region.reg_y.repeat = pugiutil::get_attribute(node, "repeaty", loc_data, pugiutil::OPTIONAL).as_int(std::numeric_limits<int>::max());

            sg_location.region.reg_x.incr = pugiutil::get_attribute(node, "incrx", loc_data, pugiutil::OPTIONAL).as_int(1);
            sg_location.region.reg_y.incr = pugiutil::get_attribute(node, "incry", loc_data, pugiutil::OPTIONAL).as_int(1);
        }

        sg_location_list.push_back(sg_location);
    }
    return sg_location_list;
}

void process_sg_tag(pugi::xml_node sg_list_tag,
                    t_arch* arch,
                    const pugiutil::loc_data& loc_data,
                    const std::vector<t_arch_switch_inf>& switches) {
    std::vector<t_scatter_gather_pattern> sg_patterns;
    pugiutil::expect_only_children(sg_list_tag, {"sg_pattern"}, loc_data);

    for (pugi::xml_node sg_tag : sg_list_tag.children()) {
        t_scatter_gather_pattern sg;

        sg.name = pugiutil::get_attribute(sg_tag, "name", loc_data).as_string();

        std::string sg_tag_type = pugiutil::get_attribute(sg_tag, "type", loc_data).as_string();
        if (sg_tag_type == "unidir") {
            sg.type = e_scatter_gather_type::UNIDIR;
        } else if (sg_tag_type == "bidir") {
            sg.type = e_scatter_gather_type::BIDIR;
        } else {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(sg_tag), "Unrecognized type for the <sg_pattern> tag.");
        }

        // Parse gather pattern
        pugi::xml_node gather_node = pugiutil::get_single_child(sg_tag, "gather", loc_data);
        t_wireconn_inf gather_wireconn = parse_wireconn(pugiutil::get_single_child(gather_node, "wireconn", loc_data), loc_data, switches, true);
        if (!gather_wireconn.to_switchpoint_set.empty()) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(sg_tag), "Gather wireconn specification should not set any 'to' switchpoints.");
        }
        if (gather_wireconn.from_switchpoint_set.empty()) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(sg_tag), "Gather wireconn specification does not include any switchpoints.");
        }
        sg.gather_pattern = gather_wireconn;

        // Parse scatter pattern
        pugi::xml_node scatter_node = pugiutil::get_single_child(sg_tag, "scatter", loc_data);
        t_wireconn_inf scatter_wireconn = parse_wireconn(pugiutil::get_single_child(scatter_node, "wireconn", loc_data), loc_data, switches, true);
        if (!scatter_wireconn.from_switchpoint_set.empty()) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(sg_tag), "Scatter wireconn specification should not set any 'from' switchpoints");
        }
        if (scatter_wireconn.to_switchpoint_set.empty()) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(sg_tag), "Scatter wireconn specification does not include any switchpoints.");
        }
        sg.scatter_pattern = scatter_wireconn;

        pugi::xml_node sg_link_list_tag = pugiutil::get_single_child(sg_tag, "sg_link_list", loc_data);
        sg.sg_links = parse_sg_link_tags(sg_link_list_tag, switches, loc_data);

        sg.sg_locations = parse_sg_location_tags(sg_tag, loc_data);

        sg_patterns.push_back(sg);
    }

    arch->scatter_gather_patterns = sg_patterns;
}
