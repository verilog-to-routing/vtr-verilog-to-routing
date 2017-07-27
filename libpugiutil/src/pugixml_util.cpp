#include "pugixml_util.hpp"

namespace pugiutil {

    //Loads the XML file specified by filename into the passed pugi::xml_document
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

    //Counts the number of child nodes of type 'child_name'
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

        //Note that we don't do any error checking here since get_first_child does the existance check

        return count;
    }

    //Counts the number of child nodes (any type)
    //
    //  node - The parent xml node
    //  loc_data - XML file location data
    //  req_opt - Whether the child tag is required (will error if required and not found) or optional. Defaults to REQUIRED
    size_t count_children(const pugi::xml_node node, 
                          const loc_data& loc_data,
                          const ReqOpt req_opt) {
        size_t count = std::distance(node.begin(), node.end());

        if (count == 0 && req_opt == REQUIRED) {
            throw XmlError("Expected child node(s) in node '" + std::string(node.name()) + "'",
                           loc_data.filename(), loc_data.line(node));
        }

        return count;
    }

    //Throws a well formatted error if the actual child count does not equal the 'expected_count'
    //
    //  node - The parent xml node
    //  loc_data - XML file location data
    //  expected_count - The expected number of child nodes
    void expect_child_node_count(const pugi::xml_node node,
                            size_t expected_count,
                            const loc_data& loc_data) {
        size_t actual_count = count_children(node, loc_data, OPTIONAL);

        if (actual_count != expected_count) {
            //throw XmlError("Expected " + std::to_string(expected_count) 
                            //+ " child node(s) of node "
                            //+ "'" + std::string(node.name()) + "'"
                            //+ "(found " + std::to_string(actual_count) + ")",
                           //loc_data.filename(), loc_data.line(node));
            
            throw XmlError("Found " + std::to_string(actual_count)
                            + " child node(s) of "
                            + "'" + std::string(node.name()) + "'"
                            + "(expected " + std::to_string(expected_count) + ")",
                           loc_data.filename(), loc_data.line(node));
        }
    }

    //Counts the number of attributes on the specified node
    //
    //  node - The xml node
    //  loc_data - XML file location data
    //  req_opt - Whether any attributes are required (will error if required and none are found) or optional. Defaults to REQUIRED
    size_t count_attributes(const pugi::xml_node node,
                            const loc_data& loc_data,
                            const ReqOpt req_opt) {

        size_t count = std::distance(node.attributes_begin(), node.attributes_end());

        if (count == 0 && req_opt == REQUIRED) {
            throw XmlError("Expected attributes on node'" + std::string(node.name()) + "'",
                           loc_data.filename(), loc_data.line(node));
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
