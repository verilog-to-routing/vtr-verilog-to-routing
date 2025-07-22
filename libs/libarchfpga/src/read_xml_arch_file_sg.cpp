#include "read_xml_util.h"
#include "parse_switchblocks.h"
#include "pugixml_util.hpp"
#include "arch_error.h"

static std::vector<t_sg_link> parse_sg_link_tags(pugi::xml_node sg_pattern_tag,
                                          const pugiutil::loc_data& loc_data) {
    std::vector<t_sg_link> sg_link_list;
    for (pugi::xml_node node : sg_pattern_tag.children()) {
        if (strcmp(node.name(), "sg_link") != 0) continue;

        const std::vector<std::string> expected_attributes = {"name", "x_offset", "y_offset", "z_offset", "mux", "seg_type"};
        pugiutil::expect_only_attributes(node, expected_attributes, loc_data);

        t_sg_link sg_link;
        sg_link.name = pugiutil::get_attribute(node, "name", loc_data).as_string();
        sg_link.mux_name = pugiutil::get_attribute(node, "mux", loc_data).as_string();
        sg_link.seg_type = pugiutil::get_attribute(node, "seg_type", loc_data).as_string();

        int x_offset = pugiutil::get_attribute(node, "x_offset", loc_data, pugiutil::OPTIONAL).as_int();
        int y_offset = pugiutil::get_attribute(node, "y_offset", loc_data, pugiutil::OPTIONAL).as_int();
        int z_offset = pugiutil::get_attribute(node, "z_offset", loc_data, pugiutil::OPTIONAL).as_int();

        if (x_offset == 0 && y_offset == 0 && z_offset == 0) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "All offset fields in the <sg_link> are non-zero or missing.");
        }

        int offset_length;
        e_sg_link_offset_dim offset_dim;
        if (x_offset != 0) {
            offset_length = x_offset;
            offset_dim = e_sg_link_offset_dim::X;
            if (y_offset != 0 || z_offset != 0) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "More than one of the offset fields in the <sg_link> are non-zero.");
            }
        }
        if (y_offset != 0) {
            offset_length = y_offset;
            offset_dim = e_sg_link_offset_dim::Y;
            if (x_offset != 0 || z_offset != 0) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "More than one of the offset fields in the <sg_link> are non-zero.");
            }
        }
        if (z_offset != 0) {
            offset_length = z_offset;
            offset_dim = e_sg_link_offset_dim::Z;
            if (y_offset != 0 || x_offset != 0) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "More than one of the offset fields in the <sg_link> are non-zero.");
            }
        }

        sg_link.offset_length = offset_length;
        sg_link.offset_dim = offset_dim;

        sg_link_list.push_back(sg_link);
    }

    return sg_link_list;
}
static std::vector<t_sg_location> parse_sg_location_tags(pugi::xml_node sg_pattern_tag,
                                                  const pugiutil::loc_data& loc_data) {
    std::vector<t_sg_location> sg_location_list;
    for (pugi::xml_node node : sg_pattern_tag.children()) {
        if (strcmp(node.name(), "sg_location") != 0) continue;

        t_sg_location sg_location;
        // <sg_location type="EVERYWHERE" num="10" sg_link_name="L1"/>
        sg_location.num = pugiutil::get_attribute(node, "num", loc_data).as_int();
        sg_location.sg_link_name = pugiutil::get_attribute(node, "sg_link_name", loc_data).as_string();
        sg_location.type = sb_location_from_string(pugiutil::get_attribute(node, "type", loc_data).as_string());
        if (sg_location.type == e_sb_location::E_UNRECOGNIZED) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
                           vtr::string_fmt("unrecognized switchblock location: %s\n", pugiutil::get_attribute(node, "type", loc_data).as_string()).c_str());
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
    std::vector<std::string> expected_children = {"sg_pattern"};
    pugiutil::expect_only_children(sg_list_tag, expected_children, loc_data);

    for (pugi::xml_node sg_tag : sg_list_tag.children()) {
        t_scatter_gather_pattern sg;

        sg.name = pugiutil::get_attribute(sg_tag, "name", loc_data).as_string();

        std::string sg_tag_type = pugiutil::get_attribute(sg_tag, "type", loc_data).as_string();
        if(sg_tag_type == "unidir") {
            sg.type = e_scatter_gather_type::UNIDIR;
        } else if (sg_tag_type == "bidir") {
            sg.type = e_scatter_gather_type::BIDIR;
        } else {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(sg_tag), "Unrecognized type for the <sg_pattern> tag.");
        }

        // Parse gather pattern
        pugi::xml_node gather_node = pugiutil::get_single_child(sg_tag, "gather", loc_data);
        t_wireconn_inf gather_wireconn = parse_wireconn(pugiutil::get_single_child(gather_node, "wireconn", loc_data), loc_data, switches);
        sg.gather_pattern = gather_wireconn;
        // TODO: Only 'from' should be set

        // Parse scatter pattern
        pugi::xml_node scatter_node = pugiutil::get_single_child(sg_tag, "scatter", loc_data);
        t_wireconn_inf scatter_wireconn = parse_wireconn(pugiutil::get_single_child(scatter_node, "wireconn", loc_data), loc_data, switches);
        sg.scatter_pattern = scatter_wireconn;
        // TODO: Only 'to' should be set

        sg.sg_links = parse_sg_link_tags(sg_tag, loc_data);
        sg.sg_locations = parse_sg_location_tags(sg_tag, loc_data);

        sg_patterns.push_back(sg);
    }

    arch->scatter_gather_patterns = sg_patterns;
}
