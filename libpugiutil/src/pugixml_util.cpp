#include "pugixml_util.hpp"

namespace pugiutil {

    //Loads the XML file specified by filename into the passed pugi::xml_docment
    //
    //Returns loc_data look-up for xml node line numbers
    loc_data load_xml(pugi::xml_document& doc,  //Document object to be loaded with file contents
                               const std::string filename) { //Filename to load from
        auto location_data = loc_data(filename);

        auto load_result = doc.load_file(filename.c_str());
        if(!load_result) {
            std::string msg = load_result.description();
            auto line = location_data.line(load_result.offset);
            auto col = location_data.col(load_result.offset);
            throw XmlError("Unable to load XML file '" + filename + "', " + msg 
                           + " (line: " + std::to_string(line) + " col: " + std::to_string(col) + ")", 
                           filename.c_str(), line);
        }

        return location_data;
    }

    //Gets the first child element of the given name and returns it.
    //
    //  node - The parent xml node
    //  child_name - The child tag name
    //  loc_data - XML file location data
    //  req_opt - Whether the child tag is required (will error if required and not found) or optional. Defaults to REQUIRED
    pugi::xml_node get_first_child(const pugi::xml_node node, 
                                   const std::string& child_name, 
                                   const loc_data& loc_data,
                                   const ReqOpt req_opt) {

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
                                    const loc_data& loc_data,
                                    const ReqOpt req_opt) {
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
                          const loc_data& loc_data,
                          const ReqOpt req_opt) {
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
                                      const loc_data& loc_data,
                                      const ReqOpt req_opt) {
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
                    const loc_data& loc_data,
                    const ReqOpt req_opt) {
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
