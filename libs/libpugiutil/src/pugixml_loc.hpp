#ifndef PUGIXML_LOC_H
#define PUGIXML_LOC_H
/*
 * This file contains utilities for the  PUGI XML parser,
 * hanlding the retrieval of line numbers (useful for error messages)
 */

#include <vector>
#include "pugixml.hpp"

namespace pugiutil {

//pugi offset to line/col data based on: https://stackoverflow.com/questions/21003471/convert-pugixmls-result-offset-to-column-line
class loc_data {
  public:
    loc_data() = default;

    loc_data(std::string filename_val)
        : filename_(filename_val) {
        build_loc_data();
    }

    //The filename this location data is for
    const std::string& filename() const { return filename_; }
    const char* filename_c_str() const { return filename_.c_str(); }

    //Convenience wrapper which takes xml_nodes
    std::size_t line(pugi::xml_node node) const {
        return line(node.offset_debug());
    }

    //Convenience wrapper which takes xml_nodes
    std::size_t col(pugi::xml_node node) const {
        return col(node.offset_debug());
    }

    //Return the line number from the given offset
    std::size_t line(std::ptrdiff_t offset) const;

    //Return the column number from the given offset
    std::size_t col(std::ptrdiff_t offset) const;

  private:
    void build_loc_data();

    std::string filename_;
    std::vector<std::ptrdiff_t> offsets_;
};
} // namespace pugiutil

#endif
