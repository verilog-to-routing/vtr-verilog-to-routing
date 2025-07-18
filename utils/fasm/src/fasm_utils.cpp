#include "fasm_utils.h"
#include "vpr_utils.h"

#include <regex>

namespace fasm {

void parse_name_with_optional_index(std::string_view in, std::string *name, int *index) {
  auto in_parts = vtr::StringToken(in).split("[]");

  if(in_parts.size() == 1) {
    *name = in;
    *index = 0;
  } else if(in_parts.size() == 2) {
    *name = in_parts[0];
    *index = vtr::atoi(in_parts[1]);
  } else {
    vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
              "Cannot parse %s.", in.data());
  }
}

std::vector<std::string> split_fasm_entry(std::string entry,
                                          std::string_view delims,
                                          std::string_view ignore) {
  for (size_t ii=0; ii<entry.length(); ii++) {
    while (ignore.find(entry[ii]) != std::string::npos) {
      entry.erase(ii, 1);
    }
  }

  return vtr::StringToken(entry).split(std::string(delims));
}

std::vector<std::string> find_tags_in_feature (std::string_view a_String) {
    const std::regex regex ("(\\{[a-zA-Z0-9_]+\\})");

    std::vector<std::string> tags;
    std::string str(a_String);
    std::smatch match;

    // Search for tags
    while (std::regex_search(str, match, regex)) {
        std::string tag(match.str());

        // Strip braces
        tags.push_back(tag.substr(1, tag.length()-2));
        str = match.suffix().str();
    }

    return tags;
}

std::string substitute_tags (std::string_view a_Feature, const std::map<const std::string, std::string>& a_Tags) {

    // First list tags that are given in the feature string
    auto tags = find_tags_in_feature(a_Feature);
    if (tags.empty()) {
        return std::string{a_Feature};
    }

    // Check if those tags are defined, If not then throw an error
    bool have_errors = false;
    for (const auto& tag: tags) {
        if (a_Tags.count(tag) == 0) {
            vtr::printf_error(__FILE__, __LINE__, "fasm placeholder '%s' not defined!", tag.c_str());
            have_errors = true;
        }
    }

    if (have_errors) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
            "fasm feature '%s' contains placeholders that are not defined for the tile that it is used in!",
            a_Feature.data()
        );
    }

    // Substitute tags
    std::string feature(a_Feature);
    for (const auto& tag: tags) {
        std::regex regex("\\{" + tag + "\\}");
        feature = std::regex_replace(feature, regex, a_Tags.at(tag));
    }

    return feature;
}

} // namespace fasm
