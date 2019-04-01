#ifndef UTILS_FASM_FASM_UTILS_H_
#define UTILS_FASM_FASM_UTILS_H_

#include <string>
#include <vector>

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

} // namespace fasm

#endif /* UTILS_FASM_FASM_UTILS_H_ */
