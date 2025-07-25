#include "setup_vib_utils.h"

#include "vpr_error.h"

static void process_from_or_to_tokens(const std::vector<std::string> tokens,
                                      const std::vector<t_physical_tile_type>& physical_tile_types,
                                      const std::vector<t_segment_inf> segments,
                                      std::vector<t_from_or_to_inf>& froms);

static void parse_pin_name(const char* src_string,
                           int* start_pin_index,
                           int* end_pin_index,
                           char* pb_type_name,
                           char* port_name);

static void process_from_or_to_tokens(const std::vector<std::string> tokens,
                                      const std::vector<t_physical_tile_type>& physical_tile_types,
                                      const std::vector<t_segment_inf> segments,
                                      std::vector<t_from_or_to_inf>& froms) {
    for (int i_token = 0; i_token < (int)tokens.size(); i_token++) {
        std::string curr_token = tokens[i_token];
        const char* Token_char = curr_token.c_str();
        std::vector<std::string> splitted_tokens = vtr::StringToken(curr_token).split(".");
        if (splitted_tokens.size() == 1) {
            t_from_or_to_inf from_inf;
            from_inf.type_name = splitted_tokens[0];
            from_inf.from_type = e_multistage_mux_from_or_to_type::MUX;
            froms.push_back(from_inf);
        } else if (splitted_tokens.size() == 2) {
            std::string from_type_name = splitted_tokens[0];
            e_multistage_mux_from_or_to_type from_type;
            for (int i_phy_type = 0; i_phy_type < (int)physical_tile_types.size(); i_phy_type++) {
                if (from_type_name == physical_tile_types[i_phy_type].name) {
                    from_type = e_multistage_mux_from_or_to_type::PB;
                    int start_pin_index, end_pin_index;
                    char *pb_type_name, *port_name;
                    pb_type_name = nullptr;
                    port_name = nullptr;
                    pb_type_name = new char[strlen(Token_char)];
                    port_name = new char[strlen(Token_char)];
                    parse_pin_name(Token_char, &start_pin_index, &end_pin_index, pb_type_name, port_name);

                    std::vector<int> all_sub_tile_to_tile_pin_indices;
                    for (const t_sub_tile& sub_tile : physical_tile_types[i_phy_type].sub_tiles) {
                        int sub_tile_capacity = sub_tile.capacity.total();

                        int start = 0;
                        int end = 0;
                        int i_port = 0;
                        for (; i_port < (int)sub_tile.ports.size(); ++i_port) {
                            if (!strcmp(sub_tile.ports[i_port].name, port_name)) {
                                start = sub_tile.ports[i_port].absolute_first_pin_index;
                                end = start + sub_tile.ports[i_port].num_pins - 1;
                                break;
                            }
                        }
                        if (i_port == (int)sub_tile.ports.size()) {
                            continue;
                        }
                        for (int pin_num = start; pin_num <= end; ++pin_num) {
                            VTR_ASSERT(pin_num < (int)sub_tile.sub_tile_to_tile_pin_indices.size() / sub_tile_capacity);
                            for (int capacity = 0; capacity < sub_tile_capacity; ++capacity) {
                                int sub_tile_pin_index = pin_num + capacity * sub_tile.num_phy_pins / sub_tile_capacity;
                                int physical_pin_index = sub_tile.sub_tile_to_tile_pin_indices[sub_tile_pin_index];
                                all_sub_tile_to_tile_pin_indices.push_back(physical_pin_index);
                            }
                        }
                    }

                    if (start_pin_index == end_pin_index && start_pin_index < 0) {
                        start_pin_index = 0;
                        end_pin_index = all_sub_tile_to_tile_pin_indices.size() - 1;
                    }

                    if ((int)all_sub_tile_to_tile_pin_indices.size() <= start_pin_index || (int)all_sub_tile_to_tile_pin_indices.size() <= end_pin_index) {
                        VTR_LOGF_ERROR(__FILE__, __LINE__,
                                       "The index of pbtype %s : port %s exceeds its total number!\n", pb_type_name, port_name);
                    }

                    for (int i = start_pin_index; i <= end_pin_index; i++) {
                        t_from_or_to_inf from_inf;
                        from_inf.type_name = from_type_name;
                        from_inf.from_type = from_type;
                        from_inf.type_index = i_phy_type;
                        from_inf.phy_pin_index = all_sub_tile_to_tile_pin_indices[i];
                        froms.push_back(from_inf);
                    }
                }
            }
            for (int i_seg_type = 0; i_seg_type < (int)segments.size(); i_seg_type++) {
                if (from_type_name == segments[i_seg_type].name) {
                    from_type = e_multistage_mux_from_or_to_type::SEGMENT;
                    std::string from_detail = splitted_tokens[1];
                    if (from_detail.length() >= 2) {
                        char dir = from_detail.c_str()[0];
                        from_detail.erase(from_detail.begin());
                        int seg_index = std::stoi(from_detail);

                        t_from_or_to_inf from_inf;
                        from_inf.type_name = from_type_name;
                        from_inf.from_type = from_type;
                        from_inf.type_index = i_seg_type;
                        from_inf.seg_dir = dir;
                        from_inf.seg_index = seg_index;
                        froms.push_back(from_inf);
                    }

                    break;
                }
            }
            VTR_ASSERT(from_type == e_multistage_mux_from_or_to_type::PB
                       || from_type == e_multistage_mux_from_or_to_type::SEGMENT);

        } else {
            std::string msg = vtr::string_fmt("Failed to parse vib mux from information '%s'", curr_token.c_str());
            VTR_LOGF_ERROR(__FILE__, __LINE__, msg.c_str());
        }
    }
}

static void parse_pin_name(const char* src_string, int* start_pin_index, int* end_pin_index, char* pb_type_name, char* port_name) {
    // Parses out the pb_type_name and port_name
    // If the start_pin_index and end_pin_index is specified, parse them too.
    // Return the values parsed by reference.

    std::string source_string;

    // parse out the pb_type and port name, possibly pin_indices
    const char* find_format = strstr(src_string, "[");
    if (find_format == nullptr) {
        /* Format "pb_type_name.port_name" */
        *start_pin_index = *end_pin_index = -1;

        source_string = src_string;

        for (size_t ichar = 0; ichar < source_string.size(); ichar++) {
            if (source_string[ichar] == '.')
                source_string[ichar] = ' ';
        }

        int match_count = sscanf(source_string.c_str(), "%s %s", pb_type_name, port_name);
        if (match_count != 2) {
            VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                            "Invalid pin - %s, name should be in the format "
                            "\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
                            "The end_pin_index and start_pin_index can be the same.\n",
                            src_string);
        }
    } else {
        /* Format "pb_type_name.port_name[end_pin_index:start_pin_index]" */
        source_string = src_string;
        for (size_t ichar = 0; ichar < source_string.size(); ichar++) {
            //Need white space between the components when using %s with
            //sscanf
            if (source_string[ichar] == '.')
                source_string[ichar] = ' ';
            if (source_string[ichar] == '[')
                source_string[ichar] = ' ';
        }

        int match_count = sscanf(source_string.c_str(), "%s %s %d:%d]",
                                 pb_type_name, port_name,
                                 end_pin_index, start_pin_index);
        if (match_count != 4) {
            match_count = sscanf(source_string.c_str(), "%s %s %d]",
                                 pb_type_name, port_name,
                                 end_pin_index);
            *start_pin_index = *end_pin_index;
            if (match_count != 3) {
                VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                                "Invalid pin - %s, name should be in the format "
                                "\"pb_type_name\".\"port_name\" or \"pb_type_name\".\"port_name[end_pin_index:start_pin_index]\". "
                                "The end_pin_index and start_pin_index can be the same.\n",
                                src_string);
            }
        }
        if (*end_pin_index < 0 || *start_pin_index < 0) {
            VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                            "Invalid pin - %s, the pin_index in "
                            "[end_pin_index:start_pin_index] should not be a negative value.\n",
                            src_string);
        }
        if (*end_pin_index < *start_pin_index) {
            int temp;
            temp = *end_pin_index;
            *end_pin_index = *start_pin_index;
            *start_pin_index = temp;
        }
    }
}

void setup_vib_inf(const std::vector<t_physical_tile_type>& physical_tile_types,
                   const std::vector<t_arch_switch_inf>& switches,
                   const std::vector<t_segment_inf>& segments,
                   std::vector<VibInf>& vib_infs) {
    VTR_ASSERT(!vib_infs.empty());
    for (VibInf& vib_inf : vib_infs) {
        for (size_t i_switch = 0; i_switch < switches.size(); i_switch++) {
            if (vib_inf.get_switch_name() == switches[i_switch].name) {
                vib_inf.set_switch_idx(i_switch);
                break;
            }
        }

        std::vector<t_seg_group> seg_groups = vib_inf.get_seg_groups();
        for (t_seg_group& seg_group : seg_groups) {
            for (int i_seg = 0; i_seg < (int)segments.size(); i_seg++) {
                if (segments[i_seg].name == seg_group.name) {
                    seg_group.seg_index = i_seg;
                    break;
                }
            }
        }
        vib_inf.set_seg_groups(seg_groups);

        std::vector<t_first_stage_mux_inf> first_stages = vib_inf.get_first_stages();
        for (t_first_stage_mux_inf& first_stage : first_stages) {
            std::vector<std::vector<std::string>>& from_tokens = first_stage.from_tokens;
            for (const std::vector<std::string>& from_token : from_tokens) {
                process_from_or_to_tokens(from_token, physical_tile_types, segments, first_stage.froms);
            }
        }
        vib_inf.set_first_stages(first_stages);

        std::vector<t_second_stage_mux_inf> second_stages = vib_inf.get_second_stages();
        for (t_second_stage_mux_inf& second_stage : second_stages) {
            std::vector<t_from_or_to_inf> tos;

            process_from_or_to_tokens(second_stage.to_tokens, physical_tile_types, segments, tos);
            for (t_from_or_to_inf& to : tos) {
                VTR_ASSERT(to.from_type == e_multistage_mux_from_or_to_type::SEGMENT
                           || to.from_type == e_multistage_mux_from_or_to_type::PB);
                second_stage.to.push_back(to);
            }

            std::vector<std::vector<std::string>> from_tokens = second_stage.from_tokens;
            for (const std::vector<std::string>& from_token : from_tokens) {
                process_from_or_to_tokens(from_token, physical_tile_types, segments, second_stage.froms);
            }
        }
        vib_inf.set_second_stages(second_stages);
    }
}
