#include "read_xml_arch_file_vib.h"

#include "vtr_assert.h"
#include "vtr_util.h"
#include "arch_error.h"

#include "read_xml_util.h"

using pugiutil::ReqOpt;

static void process_vib(pugi::xml_node Vib_node, std::vector<t_physical_tile_type>& physical_tile_types, t_arch* arch, const pugiutil::loc_data& loc_data);

static void process_first_stage(pugi::xml_node Stage_node, std::vector<t_physical_tile_type>& physical_tile_types, std::vector<t_first_stage_mux_inf>& first_stages, const pugiutil::loc_data& loc_data);

static void process_second_stage(pugi::xml_node Stage_node, std::vector<t_physical_tile_type>& physical_tile_types, std::vector<t_second_stage_mux_inf>& second_stages, const pugiutil::loc_data& loc_data);

static void process_vib_block_type_locs(t_vib_grid_def& grid_def,
                                        int die_number,
                                        vtr::string_internment& strings,
                                        pugi::xml_node layout_block_type_tag,
                                        const pugiutil::loc_data& loc_data);

void process_vib_arch(pugi::xml_node Parent, std::vector<t_physical_tile_type>& physical_tile_types, t_arch* arch, const pugiutil::loc_data& loc_data) {
    int num_vibs = count_children(Parent, "vib", loc_data);
    arch->vib_infs.reserve(num_vibs);
    pugi::xml_node node = get_first_child(Parent, "vib", loc_data);

    for (int i_vib = 0; i_vib < num_vibs; i_vib++) {
        process_vib(node, physical_tile_types, arch, loc_data);
        node = node.next_sibling(node.name());
    }
}

static void process_vib(pugi::xml_node Vib_node, std::vector<t_physical_tile_type>& physical_tile_types, t_arch* arch, const pugiutil::loc_data& loc_data) {
    VibInf vib;

    std::string tmp = get_attribute(Vib_node, "name", loc_data).as_string("");
    if (!tmp.empty()) {
        vib.set_name(tmp);
    } else {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(Vib_node),
                       "No name specified for the vib!\n");
    }

    tmp = get_attribute(Vib_node, "pbtype_name", loc_data).as_string("");
    if (!tmp.empty()) {
        vib.set_pbtype_name(tmp);
    } else {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(Vib_node),
                       "No pbtype_name specified for the vib!\n");
    }

    vib.set_seg_group_num(get_attribute(Vib_node, "vib_seg_group", loc_data).as_int(1));

    tmp = get_attribute(Vib_node, "arch_vib_switch", loc_data).as_string("");

    if (!tmp.empty()) {
        std::string str_tmp;
        str_tmp = tmp;
        vib.set_switch_name(str_tmp);
    } else {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(Vib_node),
                       "No switch specified for the vib!\n");
    }

    expect_only_children(Vib_node, {"seg_group", "multistage_muxs"}, loc_data);

    int group_num = count_children(Vib_node, "seg_group", loc_data);
    VTR_ASSERT(vib.get_seg_group_num() == group_num);
    pugi::xml_node node = get_first_child(Vib_node, "seg_group", loc_data);
    for (int i_group = 0; i_group < group_num; i_group++) {
        t_seg_group seg_group;

        tmp = get_attribute(node, "name", loc_data).as_string("");

        if (!tmp.empty()) {
            seg_group.name = tmp;
        } else {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
                           "No name specified for the vib seg group!\n");
        }

        seg_group.axis = e_parallel_axis_vib::BOTH_DIR; /*DEFAULT value if no axis is specified*/
        tmp = get_attribute(node, "axis", loc_data, ReqOpt::OPTIONAL).as_string("");

        if (!tmp.empty()) {
            if (tmp == "x") {
                seg_group.axis = e_parallel_axis_vib::X;
            } else if (tmp == "y") {
                seg_group.axis = e_parallel_axis_vib::Y;
            } else {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "Unsopported parralel axis type: %s\n", tmp.c_str());
            }
        }

        int track_num = get_attribute(node, "track_nums", loc_data).as_int();
        if (track_num > 0) {
            seg_group.track_num = track_num;
        } else {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
                           "No track_num specified for the vib seg group!\n");
        }

        vib.push_seg_group(seg_group);

        node = node.next_sibling(node.name());
    }

    node = get_single_child(Vib_node, "multistage_muxs", loc_data);
    expect_only_children(node, {"first_stage", "second_stage"}, loc_data);

    pugi::xml_node sub_elem = get_single_child(node, "first_stage", loc_data);
    if (sub_elem) {
        std::vector<t_first_stage_mux_inf> first_stages;
        process_first_stage(sub_elem, physical_tile_types, first_stages, loc_data);

        for (auto first_stage : first_stages) {
            vib.push_first_stage(first_stage);
        }
    }

    sub_elem = get_single_child(node, "second_stage", loc_data);
    if (sub_elem) {
        std::vector<t_second_stage_mux_inf> second_stages;
        process_second_stage(sub_elem, physical_tile_types, second_stages, loc_data);

        for (auto second_stage : second_stages) {
            vib.push_second_stage(second_stage);
        }
    }

    arch->vib_infs.push_back(vib);
}

static void process_first_stage(pugi::xml_node Stage_node, std::vector<t_physical_tile_type>& /*physical_tile_types*/, std::vector<t_first_stage_mux_inf>& first_stages, const pugiutil::loc_data& loc_data) {
    expect_only_children(Stage_node, {"mux"}, loc_data);
    int num_mux = count_children(Stage_node, "mux", loc_data);
    first_stages.reserve(num_mux);
    pugi::xml_node node = get_first_child(Stage_node, "mux", loc_data);
    for (int i_mux = 0; i_mux < num_mux; i_mux++) {
        t_first_stage_mux_inf first_stage_mux;
        first_stage_mux.mux_name = get_attribute(node, "name", loc_data).as_string();

        expect_only_children(node, {"from"}, loc_data);
        pugi::xml_node sub_elem = get_first_child(node, "from", loc_data);
        int from_num = count_children(node, "from", loc_data);
        for (int i_from = 0; i_from < from_num; i_from++) {
            std::vector<std::string> from_tokens = vtr::StringToken(sub_elem.child_value()).split(" \t\n");
            first_stage_mux.from_tokens.push_back(from_tokens);
            sub_elem = sub_elem.next_sibling(sub_elem.name());
        }
        first_stages.push_back(first_stage_mux);

        node = node.next_sibling(node.name());
    }
}

static void process_second_stage(pugi::xml_node Stage_node, std::vector<t_physical_tile_type>& /*physical_tile_types*/, std::vector<t_second_stage_mux_inf>& second_stages, const pugiutil::loc_data& loc_data) {
    expect_only_children(Stage_node, {"mux"}, loc_data);
    int num_mux = count_children(Stage_node, "mux", loc_data);
    second_stages.reserve(num_mux);
    pugi::xml_node node = get_first_child(Stage_node, "mux", loc_data);
    for (int i_mux = 0; i_mux < num_mux; i_mux++) {
        t_second_stage_mux_inf second_stage_mux;
        second_stage_mux.mux_name = get_attribute(node, "name", loc_data).as_string();

        expect_only_children(node, {"to", "from"}, loc_data);

        pugi::xml_node sub_elem = get_first_child(node, "to", loc_data);
        int to_num = count_children(node, "to", loc_data);
        VTR_ASSERT(to_num == 1);
        std::vector<std::string> to_tokens = vtr::StringToken(sub_elem.child_value()).split(" \t\n");
        VTR_ASSERT(to_tokens.size() == 1);
        second_stage_mux.to_tokens = to_tokens;

        sub_elem = get_first_child(node, "from", loc_data);
        int from_num = count_children(node, "from", loc_data);
        for (int i_from = 0; i_from < from_num; i_from++) {
            std::vector<std::string> from_tokens = vtr::StringToken(sub_elem.child_value()).split(" \t\n");
            second_stage_mux.from_tokens.push_back(from_tokens);
            sub_elem = sub_elem.next_sibling(sub_elem.name());
        }

        second_stages.push_back(second_stage_mux);

        node = node.next_sibling(node.name());
    }
}

/* Process vib layout */
void process_vib_layout(pugi::xml_node vib_layout_tag, t_arch* arch, const pugiutil::loc_data& loc_data) {
    VTR_ASSERT(vib_layout_tag.name() == std::string("vib_layout"));

    size_t auto_layout_cnt = 0;
    size_t fixed_layout_cnt = 0;
    for (auto layout_type_tag : vib_layout_tag.children()) {
        if (layout_type_tag.name() == std::string("auto_layout")) {
            ++auto_layout_cnt;
        } else if (layout_type_tag.name() == std::string("fixed_layout")) {
            ++fixed_layout_cnt;
        } else {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(layout_type_tag),
                           "Unexpected tag type '<%s>', expected '<auto_layout>' or '<fixed_layout>'", layout_type_tag.name());
        }
    }

    if (auto_layout_cnt == 0 && fixed_layout_cnt == 0) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(vib_layout_tag),
                       "Expected either an <auto_layout> or <fixed_layout> tag");
    }
    if (auto_layout_cnt > 1) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(vib_layout_tag),
                       "Expected at most one <auto_layout> tag");
    }
    VTR_ASSERT_MSG(auto_layout_cnt == 0 || auto_layout_cnt == 1, "<auto_layout> may appear at most once");

    int num_of_avail_layer;

    for (auto vib_layout_type_tag : vib_layout_tag.children()) {
        t_vib_grid_def grid_def = process_vib_grid_layout(arch->strings, vib_layout_type_tag, loc_data, arch, num_of_avail_layer);

        arch->vib_grid_layouts.emplace_back(std::move(grid_def));
    }
}

t_vib_grid_def process_vib_grid_layout(vtr::string_internment& strings, pugi::xml_node layout_type_tag, const pugiutil::loc_data& loc_data, t_arch* arch, int& num_of_avail_layer) {
    t_vib_grid_def grid_def;
    num_of_avail_layer = get_number_of_layers(layout_type_tag, loc_data);
    bool has_layer = layout_type_tag.child("layer");

    //Determine the grid specification type
    if (layout_type_tag.name() == std::string("auto_layout")) {
        //expect_only_attributes(layout_type_tag, {"aspect_ratio"}, loc_data);

        grid_def.grid_type = VibGridDefType::VIB_AUTO;
        grid_def.name = "auto";

        for (size_t i = 0; i < arch->grid_layouts.size(); i++) {
            if (arch->grid_layouts[i].name == grid_def.name) {
                grid_def.aspect_ratio = arch->grid_layouts[i].aspect_ratio;
            }
        }
        //grid_def.aspect_ratio = get_attribute(layout_type_tag, "aspect_ratio", loc_data, ReqOpt::OPTIONAL).as_float(1.);

    } else if (layout_type_tag.name() == std::string("fixed_layout")) {
        expect_only_attributes(layout_type_tag, {"name"}, loc_data);

        grid_def.grid_type = VibGridDefType::VIB_FIXED;
        //grid_def.width = get_attribute(layout_type_tag, "width", loc_data).as_int();
        //grid_def.height = get_attribute(layout_type_tag, "height", loc_data).as_int();
        std::string name = get_attribute(layout_type_tag, "name", loc_data).value();

        if (name == "auto") {
            //We name <auto_layout> as 'auto', so don't allow a user to specify it
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(layout_type_tag),
                           "The name '%s' is reserved for auto-sized layouts; please choose another name");
        }

        for (size_t i = 0; i < arch->grid_layouts.size(); i++) {
            if (arch->grid_layouts[i].name == name) {
                grid_def.width = arch->grid_layouts[i].width;
                grid_def.height = arch->grid_layouts[i].height;
            }
        }
        grid_def.name = name;

    } else {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(layout_type_tag),
                       "Unexpected tag '<%s>'. Expected '<auto_layout>' or '<fixed_layout>'.",
                       layout_type_tag.name());
    }

    grid_def.layers.resize(num_of_avail_layer);
    arch->layer_global_routing.resize(num_of_avail_layer);
    //No layer tag is specified (only one die is specified in the arch file)
    //Need to process layout_type_tag children to get block types locations in the grid
    if (has_layer) {
        std::set<int> seen_die_numbers; //Check that die numbers in the specific layout tag are unique
        //One or more than one layer tag is specified
        auto layer_tag_specified = layout_type_tag.children("layer");
        for (auto layer_child : layer_tag_specified) {
            int die_number;
            bool has_global_routing;
            //More than one layer tag is specified, meaning that multi-die FPGA is specified in the arch file
            //Need to process each <layer> tag children to get block types locations for each grid
            die_number = get_attribute(layer_child, "die", loc_data).as_int(0);
            has_global_routing = get_attribute(layer_child, "has_prog_routing", loc_data, ReqOpt::OPTIONAL).as_bool(true);
            arch->layer_global_routing.at(die_number) = has_global_routing;
            VTR_ASSERT(die_number >= 0 && die_number < num_of_avail_layer);
            auto insert_res = seen_die_numbers.insert(die_number);
            VTR_ASSERT_MSG(insert_res.second, "Two different layers with a same die number may have been specified in the Architecture file");
            process_vib_block_type_locs(grid_def, die_number, strings, layer_child, loc_data);
        }
    } else {
        //if only one die is available, then global routing resources must exist in that die
        int die_number = 0;
        arch->layer_global_routing.at(die_number) = true;
        process_vib_block_type_locs(grid_def, die_number, strings, layout_type_tag, loc_data);
    }
    return grid_def;
}

static void process_vib_block_type_locs(t_vib_grid_def& grid_def,
                                        int die_number,
                                        vtr::string_internment& strings,
                                        pugi::xml_node layout_block_type_tag,
                                        const pugiutil::loc_data& loc_data) {
    // Helper struct to define coordinate parameters
    struct CoordParams {
        std::string x_start, x_end, y_start, y_end;
        std::string x_repeat = "", x_incr = "", y_repeat = "", y_incr = "";

        CoordParams(const std::string& xs, const std::string& xe, const std::string& ys, const std::string& ye)
            : x_start(xs)
            , x_end(xe)
            , y_start(ys)
            , y_end(ye) {}

        CoordParams() = default;
    };

    //Process all the block location specifications
    for (auto loc_spec_tag : layout_block_type_tag.children()) {
        auto loc_type = loc_spec_tag.name();
        auto type_name = get_attribute(loc_spec_tag, "type", loc_data).value();
        int priority = get_attribute(loc_spec_tag, "priority", loc_data).as_int();
        t_metadata_dict meta = process_meta_data(strings, loc_spec_tag, loc_data);

        auto& loc_defs = grid_def.layers.at(die_number).loc_defs;

        if (loc_type == std::string("perimeter")) {
            expect_only_attributes(loc_spec_tag, {"type", "priority"}, loc_data);

            const std::vector<CoordParams> perimeter_edges = {
                {"0", "0", "0", "H - 1"},         // left (including corners)
                {"W - 1", "W - 1", "0", "H - 1"}, // right (including corners)
                {"1", "W - 2", "0", "0"},         // bottom (excluding corners)
                {"1", "W - 2", "H - 1", "H - 1"}  // top (excluding corners)
            };

            for (const auto& edge : perimeter_edges) {
                t_vib_grid_loc_def edge_def(type_name, priority);
                edge_def.x.start_expr = edge.x_start;
                edge_def.x.end_expr = edge.x_end;
                edge_def.y.start_expr = edge.y_start;
                edge_def.y.end_expr = edge.y_end;
                loc_defs.emplace_back(std::move(edge_def));
            }
        } else if (loc_type == std::string("corners")) {
            expect_only_attributes(loc_spec_tag, {"type", "priority"}, loc_data);

            const std::vector<CoordParams> corner_positions = {
                {"0", "0", "0", "0"},        // bottom_left
                {"0", "0", "H-1", "H-1"},    // top_left
                {"W-1", "W-1", "0", "0"},    // bottom_right
                {"W-1", "W-1", "H-1", "H-1"} // top_right
            };

            for (const auto& corner : corner_positions) {
                t_vib_grid_loc_def corner_def(type_name, priority);
                corner_def.x.start_expr = corner.x_start;
                corner_def.x.end_expr = corner.x_end;
                corner_def.y.start_expr = corner.y_start;
                corner_def.y.end_expr = corner.y_end;
                loc_defs.emplace_back(std::move(corner_def));
            }
        } else if (loc_type == std::string("fill")) {
            expect_only_attributes(loc_spec_tag, {"type", "priority"}, loc_data);

            t_vib_grid_loc_def fill_def(type_name, priority);
            fill_def.x.start_expr = "0";
            fill_def.x.end_expr = "W - 1";
            fill_def.y.start_expr = "0";
            fill_def.y.end_expr = "H - 1";
            loc_defs.push_back(std::move(fill_def));
        } else if (loc_type == std::string("single")) {
            expect_only_attributes(loc_spec_tag, {"type", "priority", "x", "y"}, loc_data);

            const std::string x_pos = get_attribute(loc_spec_tag, "x", loc_data).value();
            const std::string y_pos = get_attribute(loc_spec_tag, "y", loc_data).value();

            t_vib_grid_loc_def single_def(type_name, priority);
            single_def.x.start_expr = x_pos;
            single_def.x.end_expr = x_pos + " + w - 1";
            single_def.y.start_expr = y_pos;
            single_def.y.end_expr = y_pos + " + h - 1";
            loc_defs.push_back(std::move(single_def));
        } else if (loc_type == std::string("col")) {
            expect_only_attributes(loc_spec_tag, {"type", "priority", "startx", "repeatx", "starty", "incry"}, loc_data);

            const std::string start_x = get_attribute(loc_spec_tag, "startx", loc_data).value();

            t_vib_grid_loc_def col_def(type_name, priority);
            col_def.x.start_expr = start_x;
            col_def.x.end_expr = start_x + " + w - 1";

            // Handle optional attributes
            auto repeat_attr = get_attribute(loc_spec_tag, "repeatx", loc_data, ReqOpt::OPTIONAL);
            if (repeat_attr) {
                col_def.x.repeat_expr = repeat_attr.value();
            }

            auto start_y_attr = get_attribute(loc_spec_tag, "starty", loc_data, ReqOpt::OPTIONAL);
            if (start_y_attr) {
                col_def.y.start_expr = start_y_attr.value();
            }

            auto incr_y_attr = get_attribute(loc_spec_tag, "incry", loc_data, ReqOpt::OPTIONAL);
            if (incr_y_attr) {
                col_def.y.incr_expr = incr_y_attr.value();
            }

            loc_defs.push_back(std::move(col_def));

        } else if (loc_type == std::string("row")) {
            expect_only_attributes(loc_spec_tag, {"type", "priority", "starty", "repeaty", "startx", "incrx"}, loc_data);

            const std::string start_y = get_attribute(loc_spec_tag, "starty", loc_data).value();

            t_vib_grid_loc_def row_def(type_name, priority);
            row_def.y.start_expr = start_y;
            row_def.y.end_expr = start_y + " + h - 1";

            // Handle optional attributes
            auto repeat_attr = get_attribute(loc_spec_tag, "repeaty", loc_data, ReqOpt::OPTIONAL);
            if (repeat_attr) {
                row_def.y.repeat_expr = repeat_attr.value();
            }

            auto start_x_attr = get_attribute(loc_spec_tag, "startx", loc_data, ReqOpt::OPTIONAL);
            if (start_x_attr) {
                row_def.x.start_expr = start_x_attr.value();
            }

            auto incr_x_attr = get_attribute(loc_spec_tag, "incrx", loc_data, ReqOpt::OPTIONAL);
            if (incr_x_attr) {
                row_def.x.incr_expr = incr_x_attr.value();
            }

            loc_defs.push_back(std::move(row_def));
        } else if (loc_type == std::string("region")) {
            expect_only_attributes(loc_spec_tag,
                                   {"type", "priority",
                                    "startx", "endx", "repeatx", "incrx",
                                    "starty", "endy", "repeaty", "incry"},
                                   loc_data);
            t_vib_grid_loc_def region_def(type_name, priority);

            // Helper lambda to set optional attribute
            auto set_optional_attr = [&](const char* attr_name, std::string& target) {
                auto attr = get_attribute(loc_spec_tag, attr_name, loc_data, ReqOpt::OPTIONAL);
                if (attr) {
                    target = attr.value();
                }
            };

            // Set all optional region attributes
            set_optional_attr("startx", region_def.x.start_expr);
            set_optional_attr("endx", region_def.x.end_expr);
            set_optional_attr("repeatx", region_def.x.repeat_expr);
            set_optional_attr("incrx", region_def.x.incr_expr);
            set_optional_attr("starty", region_def.y.start_expr);
            set_optional_attr("endy", region_def.y.end_expr);
            set_optional_attr("repeaty", region_def.y.repeat_expr);
            set_optional_attr("incry", region_def.y.incr_expr);

            loc_defs.push_back(std::move(region_def));
        } else {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(loc_spec_tag),
                           "Unrecognized grid location specification type '%s'\n", loc_type);
        }
    }
}
