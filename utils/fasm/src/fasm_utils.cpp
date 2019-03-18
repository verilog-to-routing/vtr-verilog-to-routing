#include "fasm_utils.h"
#include "vpr_utils.h"

namespace fasm {

void parse_name_with_optional_index(const std::string in, std::string *name, int *index) {
  auto in_parts = vtr::split(in, "[]");

  if(in_parts.size() == 1) {
    *name = in;
    *index = 0;
  } else if(in_parts.size() == 2) {
    *name = in_parts[0];
    *index = vtr::atoi(in_parts[1]);
  } else {
    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
              "Cannot parse %s.", in.c_str());
  }
}

std::vector<std::string> split_fasm_entry(std::string entry,
                                                 std::string delims,
                                                 std::string ignore) {
  for (size_t ii=0; ii<entry.length(); ii++) {
    while (ignore.find(entry[ii]) != std::string::npos) {
      entry.erase(ii, 1);
    }
  }

  return vtr::split(entry, delims);
}

} // namespace fasm
