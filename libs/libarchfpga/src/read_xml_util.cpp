#include <sstream>
#include "read_xml_util.h"

#include "vtr_util.h"
#include "arch_error.h"

using namespace pugiutil;

/* Convert bool to ReqOpt enum */
extern ReqOpt BoolToReqOpt(bool b) {
	if (b) {
		return REQUIRED;
	}
	return OPTIONAL;
}

InstPort::InstPort(std::string str) {
    std::vector<std::string> inst_port = vtr::split(str, ".");

    if(inst_port.size() == 1) {
        instance_ = {"", -1, -1};
        port_ = parse_name_index(inst_port[0]);

    } else if(inst_port.size() == 2) {
        instance_ = parse_name_index(inst_port[0]);
        port_ = parse_name_index(inst_port[1]);
    } else {
        std::string msg = vtr::string_fmt("Failed to parse instance port specification '%s'",
                                str.c_str());
        throw ArchFpgaError(msg);
    }
}

InstPort::InstPort(std::string str, pugi::xml_node node, const pugiutil::loc_data& loc_data) {
    try {
        *this = InstPort(str);
    } catch (const ArchFpgaError& e) {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
				"Failed to parse instance port specification '%s' for"
                " on <%s> tag, %s",
				str.c_str(), node.name(), e.what());
    }
}

InstPort::InstPort(pugi::xml_attribute attr, pugi::xml_node node, const pugiutil::loc_data& loc_data) {
    try {
        *this = InstPort(attr.value());
    } catch (const ArchFpgaError& e) {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(node),
				"Failed to parse instance port specification '%s' for"
                " attribute '%s' on <%s> tag, %s",
				attr.value(), attr.name(), node.name(), e.what());
    }
}

InstPort::name_index InstPort::parse_name_index(std::string str) {
    auto open_bracket_pos = str.find("[");
    auto close_bracket_pos = str.find("]");
    auto colon_pos = str.find(":");

    //Parse checks
    if(open_bracket_pos == std::string::npos && close_bracket_pos != std::string::npos) {
        //Close brace only
        std::string msg = "near '" + str + "', missing '['";
        throw ArchFpgaError(msg);
    }

    if(open_bracket_pos != std::string::npos && close_bracket_pos == std::string::npos) {
        //Open brace only
        std::string msg = "near '" + str + "', missing ']'";
        throw ArchFpgaError(msg);
    }

    if(open_bracket_pos != std::string::npos && close_bracket_pos != std::string::npos) {
        //Have open and close braces, close must be after open
        if(open_bracket_pos > close_bracket_pos) {
            std::string msg = "near '" + str + "', '[' after ']'";
            throw ArchFpgaError(msg);
        }
    }

    if(colon_pos != std::string::npos) {
        //Have a colon, it must be between open/close braces
        if(colon_pos > close_bracket_pos || colon_pos < open_bracket_pos) {
            std::string msg = "near '" + str + "', found ':' but not between '[' and ']'";
            throw ArchFpgaError(msg);
        }
    }


    //Extract the name and index info
    std::string name = str.substr(0,open_bracket_pos);
    std::string first_idx_str;
    std::string second_idx_str;

    if(colon_pos == std::string::npos && open_bracket_pos == std::string::npos && close_bracket_pos == std::string::npos) {
    } else if(colon_pos == std::string::npos) {
        //No colon, implies a single element
        first_idx_str = str.substr(open_bracket_pos + 1, close_bracket_pos);
        second_idx_str = first_idx_str;
    } else {
        //Colon, implies a range
        first_idx_str = str.substr(open_bracket_pos + 1, colon_pos);
        second_idx_str = str.substr(colon_pos + 1, close_bracket_pos);
    }

    int first_idx = UNSPECIFIED;
    if(!first_idx_str.empty()) {
        std::stringstream ss(first_idx_str);
        size_t idx;
        ss >> idx;
        if(!ss.good()) {
            std::string msg = "near '" + str + "', expected positive integer";
            throw ArchFpgaError(msg);
        }
        first_idx = idx;
    }

    int second_idx = UNSPECIFIED;
    if(!second_idx_str.empty()) {
        std::stringstream ss(second_idx_str);
        size_t idx;
        ss >> idx;
        if(!ss.good()) {
            std::string msg = "near '" + str + "', expected positive integer";
            throw ArchFpgaError(msg);
        }
        second_idx = idx;
    }

    return {name, std::min(first_idx, second_idx), std::max(first_idx, second_idx)};

}


void bad_tag(const pugi::xml_node node,
                const pugiutil::loc_data& loc_data,
                const pugi::xml_node parent_node,
                const std::vector<std::string> expected_tags) {

    std::string msg = "Unexpected tag ";
    msg += "<";
    msg += node.name();
    msg += ">";

    if(parent_node) {
        msg += " in section <";
        msg += parent_node.name();
        msg += ">";
    }

    if(!expected_tags.empty()) {
        msg += ", expected ";
        for(auto iter = expected_tags.begin(); iter != expected_tags.end(); ++iter) {
            msg += "<";
            msg += *iter;
            msg += ">";

            if(iter < expected_tags.end() - 2) {
                msg += ", ";
            } else if(iter == expected_tags.end() - 2) {
                msg += " or ";
            }
        }
    }

    throw ArchFpgaError(msg, loc_data.filename(), loc_data.line(node));
}

void bad_attribute(const pugi::xml_attribute attr,
                const pugi::xml_node node,
                const pugiutil::loc_data& loc_data,
                const std::vector<std::string> expected_attributes) {

    std::string msg = "Unexpected attribute ";
    msg += "'";
    msg += attr.name();
    msg += "'";

    if(node) {
        msg += " on <";
        msg += node.name();
        msg += "> tag";
    }

    if(!expected_attributes.empty()) {
        msg += ", expected ";
        for(auto iter = expected_attributes.begin(); iter != expected_attributes.end(); ++iter) {
            msg += "'";
            msg += *iter;
            msg += "'";

            if(iter < expected_attributes.end() - 2) {
                msg += ", ";
            } else if(iter == expected_attributes.end() - 2) {
                msg += " or ";
            }
        }
    }

    throw ArchFpgaError(msg, loc_data.filename(), loc_data.line(node));
}

void bad_attribute_value(const pugi::xml_attribute attr,
                const pugi::xml_node node,
                const pugiutil::loc_data& loc_data,
                const std::vector<std::string> expected_values) {

    std::string msg = "Invalid value '";
    msg += attr.value();
    msg += "'";
    msg += " for attribute '";
    msg += attr.name();
    msg += "'";

    if(node) {
        msg += " on <";
        msg += node.name();
        msg += "> tag";
    }

    if(!expected_values.empty()) {
        msg += ", expected value ";
        for(auto iter = expected_values.begin(); iter != expected_values.end(); ++iter) {
            msg += "'";
            msg += *iter;
            msg += "'";

            if(iter < expected_values.end() - 2) {
                msg += ", ";
            } else if(iter == expected_values.end() - 2) {
                msg += " or ";
            }
        }
    }

    throw ArchFpgaError(msg, loc_data.filename(), loc_data.line(node));
}
