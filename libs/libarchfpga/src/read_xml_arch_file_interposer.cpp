#include "read_xml_arch_file_interposer.h"

#include <vector>

#include "interposer_types.h"
#include "read_xml_util.h"
#include "pugixml_util.hpp"
#include "arch_error.h"

t_interposer_cut_inf parse_interposer_cut_tag(pugi::xml_node interposer_cut_tag, const pugiutil::loc_data& loc_data) {
    t_interposer_cut_inf interposer;

    pugiutil::expect_only_attributes(interposer_cut_tag, {"x", "y"}, loc_data);

    pugi::xml_attribute x_attr = pugiutil::get_attribute(interposer_cut_tag, "x", loc_data, pugiutil::ReqOpt::OPTIONAL);
    pugi::xml_attribute y_attr = pugiutil::get_attribute(interposer_cut_tag, "y", loc_data, pugiutil::ReqOpt::OPTIONAL);

    const bool has_x = !x_attr.empty();
    const bool has_y = !y_attr.empty();

    if (has_x && has_y) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(interposer_cut_tag),
                       "Interposer cut tag must specify where the cut is to appear using only one of `x` or `y` attributes.");
    }
    if (!has_x && !has_y) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(interposer_cut_tag),
                       "Interposer cut tag must specify where the cut is to appear using `x` or `y` attributes.");
    }

    interposer.loc = has_x ? std::string(x_attr.value()) : std::string(y_attr.value());
    interposer.dim = has_x ? e_interposer_cut_type::VERT : e_interposer_cut_type::HORZ;

    pugiutil::expect_only_children(interposer_cut_tag, {"interdie_wire"}, loc_data);

    for (pugi::xml_node interdie_wire_tag : interposer_cut_tag.children()) {
        const std::vector<std::string> interdie_wire_attributes = {"sg_name", "sg_link", "offset_start", "offset_end", "offset_increment", "num"};
        pugiutil::expect_only_attributes(interdie_wire_tag, interdie_wire_attributes, loc_data);

        t_interdie_wire_inf interdie_wire;

        interdie_wire.sg_name = pugiutil::get_attribute(interdie_wire_tag, "sg_name", loc_data).as_string();
        interdie_wire.sg_link = pugiutil::get_attribute(interdie_wire_tag, "sg_link", loc_data).as_string();

        interdie_wire.offset.start = pugiutil::get_attribute(interdie_wire_tag, "offset_start", loc_data).as_int();
        interdie_wire.offset.end = pugiutil::get_attribute(interdie_wire_tag, "offset_end", loc_data).as_int();
        interdie_wire.offset.increment = pugiutil::get_attribute(interdie_wire_tag, "offset_increment", loc_data).as_int();

        if (interdie_wire.offset.increment < 0) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(interdie_wire_tag),
                           "The `offset_increment` attribute of an `interdie_wire` must be a positive number.");
        }

        if (interdie_wire.offset.start > interdie_wire.offset.end) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(interdie_wire_tag),
                           "The `offset_start` attribute of an `interdie_wire` must be smaller than the `offset_start` attribute.");
        }

        interdie_wire.num = pugiutil::get_attribute(interdie_wire_tag, "num", loc_data).as_int();

        if (interdie_wire.num <= 0) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(interdie_wire_tag),
                           "The `num` attribute of an `interdie_wire` must be specified with a positive number.");
        }

        interposer.interdie_wires.push_back(interdie_wire);
    }

    return interposer;
}
