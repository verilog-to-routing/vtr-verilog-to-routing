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
#include <stdexcept>
#include <cstdio>

#include "pugixml.hpp"

#include "pugixml_loc.hpp"

namespace pugiutil {

//An error produced while getting an XML node/attribute
class XmlError : public std::runtime_error {
  public:
    XmlError(std::string msg = "", std::string new_filename = "", size_t new_linenumber = -1)
        : std::runtime_error(msg)
        , filename_(new_filename)
        , linenumber_(new_linenumber) {}

    //Returns the filename associated with this error
    //returns an empty string if none is specified
    std::string filename() const { return filename_; }
    const char* filename_c_str() const { return filename_.c_str(); }

    //Returns the line number associated with this error
    //returns zero if none is specified
    size_t line() const { return linenumber_; }

  private:
    std::string filename_;
    size_t linenumber_;
};

//Loads the XML file specified by filename into the passed pugi::xml_docment
//
//Returns loc_data look-up for xml node line numbers
loc_data load_xml(pugi::xml_document& doc,     //Document object to be loaded with file contents
                  const std::string filename); //Filename to load from

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
                               const loc_data& loc_data,
                               const ReqOpt req_opt = REQUIRED);

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
                                const ReqOpt req_opt = REQUIRED);

//Counts the number of child nodes of type 'child_name'
//
//  node - The parent xml node
//  child_name - The child tag name
//  loc_data - XML file location data
//  req_opt - Whether the child tag is required (will error if required and not found) or optional. Defaults to REQUIRED
size_t count_children(const pugi::xml_node node,
                      const std::string& child_name,
                      const loc_data& loc_data,
                      const ReqOpt req_opt = REQUIRED);

//Counts the number of child nodes (any type)
//
//  node - The parent xml node
//  loc_data - XML file location data
//  req_opt - Whether the child tag is required (will error if required and not found) or optional. Defaults to REQUIRED
size_t count_children(const pugi::xml_node node,
                      const loc_data& loc_data,
                      const ReqOpt req_opt);

//Throws a well formatted error if the actual count of child nodes named 'child_name' does not equal the 'expected_count'
//
//  node - The parent xml node
//  loc_data - XML file location data
//  expected_count - The expected number of child nodes
void expect_child_node_count(const pugi::xml_node node,
                             std::string child_name,
                             size_t expected_count,
                             const loc_data& loc_data);

//Throws a well formatted error if the actual child count does not equal the 'expected_count'
//
//  node - The parent xml node
//  loc_data - XML file location data
//  expected_count - The expected number of child nodes
void expect_child_node_count(const pugi::xml_node node,
                             size_t expected_count,
                             const loc_data& loc_data);

//Throws a well formatted error if any of node's children are not part of child_names.
//Note this does not check whether the nodes in 'child_names' actually exist.
//
//  node - The parent xml node
//  child_names - expected attribute names
//  loc_data - XML file location data
void expect_only_children(const pugi::xml_node node,
                          std::vector<std::string> child_names,
                          const loc_data& loc_data);

//Throws a well formatted error if any attribute other than those named in 'attribute_names' are found on 'node'.
//Note this does not check whether the attribues in 'attribute_names' actually exist.
//
//  node - The parent xml node
//  attribute_names - expected attribute names
//  loc_data - XML file location data
void expect_only_attributes(const pugi::xml_node node,
                            std::vector<std::string> attribute_names,
                            const loc_data& loc_data);

//Throws a well formatted error if any attribute other than those named in 'attribute_names' are found on 'node' with an additional explanation.
//Note this does not check whether the attribues in 'attribute_names' actually exist.
//
//  node - The parent xml node
//  attribute_names - expected attribute names
//  loc_data - XML file location data
void expect_only_attributes(const pugi::xml_node node,
                            std::vector<std::string> attribute_names,
                            std::string explanation,
                            const loc_data& loc_data);

//Counts the number of attributes on the specified node
//
//  node - The xml node
//  loc_data - XML file location data
//  req_opt - Whether any attributes are required (will error if required and none are found) or optional. Defaults to REQUIRED
size_t count_attributes(const pugi::xml_node node,
                        const loc_data& loc_data,
                        const ReqOpt req_opt = REQUIRED);

//Gets a named property on an node and returns it.
//
//  node - The xml node
//  attr_name - The attribute name
//  loc_data - XML file location data
//  req_opt - Whether the attribute is required (will error if required and not found) or optional. Defaults to REQUIRED
pugi::xml_attribute get_attribute(const pugi::xml_node node,
                                  const std::string& attr_name,
                                  const loc_data& loc_data,
                                  const ReqOpt req_opt = REQUIRED);

//Checks that the given node matches the given tag name.
//
//  node - The xml node
//  tag_name - The expected tag name
//  loc_data - XML file location data
//  req_opt - Whether the tag name is required (will error if required and not found) or optional. Defaults to REQUIRED
bool check_node(const pugi::xml_node node,
                const std::string& tag_name,
                const loc_data& loc_data,
                const ReqOpt req_opt = REQUIRED);

} // namespace pugiutil

#endif
