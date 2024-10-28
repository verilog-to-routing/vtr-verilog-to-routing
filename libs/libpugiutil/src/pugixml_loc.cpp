#include <cstdio>
#include <algorithm>
#include "pugixml_util.hpp"
#include "pugixml_loc.hpp"

namespace pugiutil {

//Return the line number from the given offset
std::size_t loc_data::line(std::ptrdiff_t offset) const {
    auto it = std::lower_bound(offsets_.begin(), offsets_.end(), offset);
    std::size_t index = it - offsets_.begin();

    return 1 + index;
}

//Return the column number from the given offset
std::size_t loc_data::col(std::ptrdiff_t offset) const {
    auto it = std::lower_bound(offsets_.begin(), offsets_.end(), offset);
    std::size_t index = it - offsets_.begin();

    return index == 0 ? offset + 1 : offset - offsets_[index - 1];
}

void loc_data::build_loc_data() {
    FILE* f = fopen(filename_.c_str(), "rb");

    if (f == nullptr) {
        throw XmlError("Failed to open file", filename_);
    }

    std::ptrdiff_t offset = 0;

    char buffer[1024];
    std::size_t size;

    while ((size = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        for (std::size_t i = 0; i < size; ++i) {
            if (buffer[i] == '\n') {
                offsets_.push_back(offset + i);
            }
        }

        offset += size;
    }

    fclose(f);
}

} // namespace pugiutil
