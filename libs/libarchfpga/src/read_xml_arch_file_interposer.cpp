#include "read_xml_arch_file_interposer.h"
#include <vector>
#include "interposer_types.h"
#include "read_xml_util.h"
#include "pugixml_util.hpp"
#include "arch_error.h"

t_interposer_cut_inf parse_interposer_cut_tag(pugi::xml_node interposer_cut_tag, const pugiutil::loc_data& loc_data) {
    t_interposer_cut_inf interposer;

    pugiutil::expect_only_attributes(interposer_cut_tag, {"dim", "loc"}, loc_data);

    std::string interposer_dim = pugiutil::get_attribute(interposer_cut_tag, "dim", loc_data).as_string();
    if (interposer_dim.size() != 1 || !CHAR_INTERPOSER_DIM_MAP.contains(interposer_dim[0])) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(interposer_cut_tag), "Interposer tag dimension must be a single character of either X, x, Y or y.");
    }

    interposer.dim = CHAR_INTERPOSER_DIM_MAP.at(interposer_dim[0]);

    interposer.loc = pugiutil::get_attribute(interposer_cut_tag, "loc", loc_data).as_int();
    if (interposer.loc < 0) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(interposer_cut_tag), "Interposer location must be positive.");
    }

    pugiutil::expect_only_children(interposer_cut_tag, {"interdie_wire"}, loc_data);

    for (pugi::xml_node interdie_wire_tag : interposer_cut_tag.children()) {
        const std::vector<std::string> interdie_wire_attributes = {{"sg_name", "sg_link", "offset_start", "offset_end", "offset_increment", "num"}};
        pugiutil::expect_only_attributes(interdie_wire_tag, interdie_wire_attributes, loc_data);

        t_interdie_wire_inf interdie_wire;

        interdie_wire.sg_name = pugiutil::get_attribute(interdie_wire_tag, "sg_name", loc_data).as_string();
        interdie_wire.sg_link = pugiutil::get_attribute(interdie_wire_tag, "sg_link", loc_data).as_string();

        interdie_wire.offset_definition.start_expr = pugiutil::get_attribute(interdie_wire_tag, "offset_start", loc_data).as_string();
        interdie_wire.offset_definition.end_expr = pugiutil::get_attribute(interdie_wire_tag, "offset_end", loc_data).as_string();
        interdie_wire.offset_definition.incr_expr = pugiutil::get_attribute(interdie_wire_tag, "offset_increment", loc_data).as_string();
        
        interdie_wire.num = pugiutil::get_attribute(interdie_wire_tag, "num", loc_data).as_int();

        interposer.interdie_wires.push_back(interdie_wire);
    }

    return interposer;
}
