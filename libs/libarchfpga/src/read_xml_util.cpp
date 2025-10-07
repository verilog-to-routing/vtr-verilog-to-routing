#include "read_xml_util.h"

#include "vtr_util.h"
#include "arch_error.h"

using pugiutil::ReqOpt;

/* Convert bool to ReqOpt enum */
extern ReqOpt BoolToReqOpt(bool b) {
    if (b) {
        return ReqOpt::REQUIRED;
    }
    return ReqOpt::OPTIONAL;
}

InstPort make_inst_port(std::string str, pugi::xml_node node, const pugiutil::loc_data& loc_data) {
    InstPort inst_port;
    try {
        inst_port = InstPort(str);
    } catch (const ArchFpgaError& e) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
                       "Failed to parse instance port specification '%s' for"
                       " on <%s> tag, %s",
                       str.c_str(), node.name(), e.what());
    }

    return inst_port;
}

InstPort make_inst_port(pugi::xml_attribute attr, pugi::xml_node node, const pugiutil::loc_data& loc_data) {
    InstPort inst_port;
    try {
        inst_port = InstPort(attr.value());
    } catch (const ArchFpgaError& e) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
                       "Failed to parse instance port specification '%s' for"
                       " attribute '%s' on <%s> tag, %s",
                       attr.value(), attr.name(), node.name(), e.what());
    }
    return inst_port;
}

void bad_tag(const pugi::xml_node node,
             const pugiutil::loc_data& loc_data,
             const pugi::xml_node parent_node,
             const std::vector<std::string>& expected_tags) {
    std::string msg = "Unexpected tag ";
    msg += "<";
    msg += node.name();
    msg += ">";

    if (parent_node) {
        msg += " in section <";
        msg += parent_node.name();
        msg += ">";
    }

    if (!expected_tags.empty()) {
        msg += ", expected ";
        for (auto iter = expected_tags.begin(); iter != expected_tags.end(); ++iter) {
            msg += "<";
            msg += *iter;
            msg += ">";

            if (iter < expected_tags.end() - 2) {
                msg += ", ";
            } else if (iter == expected_tags.end() - 2) {
                msg += " or ";
            }
        }
    }

    throw ArchFpgaError(msg, loc_data.filename(), loc_data.line(node));
}

void bad_attribute(const pugi::xml_attribute attr,
                   const pugi::xml_node node,
                   const pugiutil::loc_data& loc_data,
                   const std::vector<std::string>& expected_attributes) {
    std::string msg = "Unexpected attribute ";
    msg += "'";
    msg += attr.name();
    msg += "'";

    if (node) {
        msg += " on <";
        msg += node.name();
        msg += "> tag";
    }

    if (!expected_attributes.empty()) {
        msg += ", expected ";
        for (auto iter = expected_attributes.begin(); iter != expected_attributes.end(); ++iter) {
            msg += "'";
            msg += *iter;
            msg += "'";

            if (iter < expected_attributes.end() - 2) {
                msg += ", ";
            } else if (iter == expected_attributes.end() - 2) {
                msg += " or ";
            }
        }
    }

    throw ArchFpgaError(msg, loc_data.filename(), loc_data.line(node));
}

void bad_attribute_value(const pugi::xml_attribute attr,
                         const pugi::xml_node node,
                         const pugiutil::loc_data& loc_data,
                         const std::vector<std::string>& expected_values) {
    std::string msg = "Invalid value '";
    msg += attr.value();
    msg += "'";
    msg += " for attribute '";
    msg += attr.name();
    msg += "'";

    if (node) {
        msg += " on <";
        msg += node.name();
        msg += "> tag";
    }

    if (!expected_values.empty()) {
        msg += ", expected value ";
        for (auto iter = expected_values.begin(); iter != expected_values.end(); ++iter) {
            msg += "'";
            msg += *iter;
            msg += "'";

            if (iter < expected_values.end() - 2) {
                msg += ", ";
            } else if (iter == expected_values.end() - 2) {
                msg += " or ";
            }
        }
    }

    throw ArchFpgaError(msg, loc_data.filename(), loc_data.line(node));
}

int get_number_of_layers(pugi::xml_node layout_type_tag, const pugiutil::loc_data& loc_data) {
    int max_die_num = -1;

    const auto& layer_tag = layout_type_tag.children("layer");
    for (const auto& layer_child : layer_tag) {
        int die_number = get_attribute(layer_child, "die", loc_data).as_int(0);
        if (die_number > max_die_num) {
            max_die_num = die_number;
        }
    }

    if (max_die_num == -1) {
        // For backwards compatibility, if no die number is specified, assume 1 layer
        return 1;
    } else {
        return max_die_num + 1;
    }
}

t_metadata_dict process_meta_data(vtr::string_internment& strings,
                                  pugi::xml_node Parent,
                                  const pugiutil::loc_data& loc_data) {
    //	<metadata>
    //	  <meta>CLBLL_L_</meta>
    //	</metadata>
    t_metadata_dict data;
    pugi::xml_node metadata = get_single_child(Parent, "metadata", loc_data, ReqOpt::OPTIONAL);
    if (metadata) {
        pugi::xml_node meta_tag = get_first_child(metadata, "meta", loc_data);
        while (meta_tag) {
            std::string key = get_attribute(meta_tag, "name", loc_data).as_string();

            std::string value = meta_tag.child_value();
            data.add(strings.intern_string(key),
                     strings.intern_string(value));
            meta_tag = meta_tag.next_sibling(meta_tag.name());
        }
    }
    return data;
}
