#ifndef PUGIXML_UTIL_H
#define PUGIXML_UTIL_H
/*
 * This file contains utilities for the  PUGI XML parser.
 *
 * They primarily relate to:
 *   - Checking for node/attribute exitance and reporting errors if not
 *   - Misc. utilities like counting tags
 *
 * Using these utilities simplifies error handling while manipulating XML
 * since the user doesn't need to explicitly check for node/attribute existance
 * (by default most of these functions will raise exceptions with useful error
 * messages if the requested item does not exists).
 */

#include <vector>
#include <cstdio>

#include "pugixml.hpp"

#include "vtr_error.h"
#include "pugixml_loc.hpp"

namespace pugiutil {

    //An error produced while getting an XML node/attribute
    class XmlError : public VtrError {
        public:
            XmlError(std::string msg="", std::string new_filename="", size_t new_linenumber=-1)
                : VtrError(msg, new_filename, new_linenumber) {}
    };

    //Defines whether something (e.g. a node/attribute) is optional or required.
    //  We use this to improve clarity at the function call site (compared to just 
    //  using boolean values).
    //
    //  For example:
    //
    //      auto node = get_first_child(node, "port", loc_data, true);     
    //
    //  is ambiguous without looking up what the 4th argument represents, where as:
    //
    //      auto node = get_first_child(node, "port", loc_data, REQUIRED);     
    //
    //  is much more explicit.
    enum ReqOpt {
        REQUIRED,
        OPTIONAL
    };

    //Gets the first child element of the given name and returns it.
    //
    //  node - The parent xml node
    //  child_name - The child tag name
    //  loc_data - XML file location data
    //  req_opt - Whether the child tag is required (will error if required and not found) or optional. Defaults to REQUIRED
    pugi::xml_node get_first_child(const pugi::xml_node node, 
                                   const std::string& child_name, 
                                   const pugiloc::loc_data& loc_data,
                                   const ReqOpt req_opt=REQUIRED) {

        pugi::xml_node child = node.child(child_name.c_str());
        if(!child && req_opt == REQUIRED) {
            throw XmlError("Missing required child node '" + child_name + "' in parent node '" + node.name() + "'", 
                           loc_data.filename(), loc_data.line(node));
        }
        return child;
    }

    //Gets the child element of the given name and returns it.
    //Errors if more than one matching child is found.
    //
    //  node - The parent xml node
    //  child_name - The child tag name
    //  loc_data - XML file location data
    //  req_opt - Whether the child tag is required (will error if required and not found) or optional. Defaults to REQUIRED
    pugi::xml_node get_single_child(const pugi::xml_node node, 
                                    const std::string& child_name, 
                                    const pugiloc::loc_data& loc_data,
                                    const ReqOpt req_opt=REQUIRED) {
        pugi::xml_node child = get_first_child(node, child_name, loc_data, req_opt);

        if(child && child.next_sibling(child_name.c_str())) {
            throw XmlError("Multiple child '" + child_name + "' nodes found in parent node '" + node.name() + "' (only one expected)", 
                           loc_data.filename(), loc_data.line(node));
        }

        return child;
    }

    //Counts the number of child nodes
    //
    //  node - The parent xml node
    //  child_name - The child tag name
    //  loc_data - XML file location data
    //  req_opt - Whether the child tag is required (will error if required and not found) or optional. Defaults to REQUIRED
    size_t count_children(const pugi::xml_node node, 
                          const std::string& child_name, 
                          const pugiloc::loc_data& loc_data,
                          const ReqOpt req_opt=REQUIRED) {
        size_t count = 0;

        pugi::xml_node child = get_first_child(node, child_name, loc_data, req_opt);

        while(child) {
            ++count;
            child = child.next_sibling(child_name.c_str());
        }

        return count;
    }
    
    //Gets a named property on an node and returns it.
    //
    //  node - The xml node
    //  attr_name - The attribute name
    //  loc_data - XML file location data
    //  req_opt - Whether the peropry is required (will error if required and not found) or optional. Defaults to REQUIRED
    pugi::xml_attribute get_attribute(const pugi::xml_node node, 
                                      const std::string& attr_name,
                                      const pugiloc::loc_data& loc_data,
                                      const ReqOpt req_opt=REQUIRED) {
        pugi::xml_attribute attr = node.attribute(attr_name.c_str());

        if(!attr && req_opt == REQUIRED) {
            throw XmlError("Expected '" + attr_name + "' attribute on node '" + node.name() + "'",
                           loc_data.filename(), loc_data.line(node));
        }

        return attr;
    }


    //Checks that the given node matches the given tag name.
    //
    //  node - The xml node
    //  tag_name - The expected tag name
    //  loc_data - XML file location data
    //  req_opt - Whether the tag name is required (will error if required and not found) or optional. Defaults to REQUIRED
    bool check_node(const pugi::xml_node node,
                    const std::string& tag_name,
                    const pugiloc::loc_data& loc_data,
                    const ReqOpt req_opt=REQUIRED) {
        if(node.name() == tag_name) {
            return true;
        } else {
            if(req_opt == REQUIRED) {
                throw XmlError(std::string("Unexpected node type '") + node.name() + "' expected '" + tag_name + "'",
                               loc_data.filename(), loc_data.line(node));
            }
            return false;
        }
    }

}

#endif
