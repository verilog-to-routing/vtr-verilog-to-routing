#include <cstdio>
#include <algorithm>
#include <vector>
#include "pugixml_util.hpp"
#include "pugixml_loc.hpp"

// The size of the read buffer when reading from a file.
#ifndef PUGI_UTIL_READ_BUF_SIZE
#define PUGI_UTIL_READ_BUF_SIZE 1048576
#endif // PUGI_UTIL_READ_BUF_SIZE

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

    std::vector<char> buffer(PUGI_UTIL_READ_BUF_SIZE);
    std::size_t size;

    while ((size = fread(buffer.data(), 1, buffer.size() * sizeof(char), f)) > 0) {
        for (std::size_t i = 0; i < size; ++i) {
            if (buffer[i] == '\n') {
                offsets_.push_back(offset + i);
            }
        }

        offset += size;
    }

    fclose(f);
}

void loc_data::build_loc_data_from_string(char* filename_val, size_t buffersize) {
    //std::ptrdiff_t offset = 0;
    //char buffer[1024];
    //std::size_t size;
    for (std::size_t i = 0; i < buffersize; ++i) {
        if (filename_val[i] == '\0')
            break;
        else if (filename_val[i] == '\n') {
            offsets_.push_back(i);
        }
    }
}
} // namespace pugiutil
