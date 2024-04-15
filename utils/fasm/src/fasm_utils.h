#ifndef UTILS_FASM_FASM_UTILS_H_
#define UTILS_FASM_FASM_UTILS_H_

#include <string>
#include <string_view>
#include <vector>
#include <map>

namespace fasm {

// Parse a port name that may have an index.
//
// in="A" parts to *name="A", *index=0
// in="A[5]" parts to *name="A", *index=5
//
// Throws vpr exception if parsing fails.
void parse_name_with_optional_index(std::string_view in, std::string *name, int *index);

// Split FASM entry into parts.
//
// delims - Characters to split on.
// ignore - Characters to ignore.
std::vector<std::string> split_fasm_entry(std::string entry,
                                          std::string_view delims,
                                          std::string_view ignore);

// Searches for tags in given string, returns their names in a vector.
std::vector<std::string> find_tags_in_feature(std::string_view a_String);

// Substitutes tags found in a string with their values provided by the map.
// Throws an error if a tag is found in the string and its value is not present
// in the map.
//
// a_Feature - Fasm feature string (or any other string)
// a_Tags    - Map with tags and their values
std::string substitute_tags(std::string_view a_Feature,
                            const std::map<const std::string, std::string>& a_Tags);

} // namespace fasm

#endif /* UTILS_FASM_FASM_UTILS_H_ */
