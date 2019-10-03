#ifndef UTILS_FASM_FASM_UTILS_H_
#define UTILS_FASM_FASM_UTILS_H_

#include <string>
#include <vector>
#include <map>

namespace fasm {

// Parse a port name that may have an index.
//
// in="A" parts to *name="A", *index=0
// in="A[5]" parts to *name="A", *index=5
//
// Throws vpr exception if parsing fails.
void parse_name_with_optional_index(const std::string in, std::string *name, int *index);

// Split FASM entry into parts.
//
// delims - Characters to split on.
// ignore - Characters to ignore.
std::vector<std::string> split_fasm_entry(std::string entry,
                                                 std::string delims,
                                                 std::string ignore);

// Substitutes tags found in a string with their values provided by the map.
// Thorws an error if a tag is found in the string and its value is not present
// in the map.
//
// a_Feature - Fasm feature string (or any other string)
// a_Tags    - Map with tags and their values
std::string substitute_tags (const std::string& a_Feature,
                             const std::map<const std::string, std::string>& a_Tags);

} // namespace fasm

#endif /* UTILS_FASM_FASM_UTILS_H_ */
