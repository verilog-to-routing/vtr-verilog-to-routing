#ifndef PUGIXML_LOC_H
#define PUGIXML_LOC_H
/*
 * This file contains utility wrapper functions around the PUGI XML parser,
 * hanlding things like retrieving line numbers, and producing errors for
 * missing nodes/attributes
 */

#include <vector>
#include <cstdio>
#include "pugixml.hpp"

namespace pugiloc {

    //pugi offset to line/col data based on: https://stackoverflow.com/questions/21003471/convert-pugixmls-result-offset-to-column-line
    class loc_data {
        public:
            loc_data(std::string filename_val)
                : filename_(filename_val) {
                build_loc_data();
            }

            const std::string& filename() const { return filename_; }
            const char* filename_c_str() const { return filename_.c_str(); }

            size_t line(pugi::xml_node node) const {
                return line(node.offset_debug());
            }

            size_t col(pugi::xml_node node) const {
                return col(node.offset_debug());

            }

            size_t line(ptrdiff_t offset) const {
                auto it = std::lower_bound(offsets_.begin(), offsets_.end(), offset);
                size_t index = it - offsets_.begin();

                return 1 + index;

            }

            size_t col(ptrdiff_t offset) const {
                auto it = std::lower_bound(offsets_.begin(), offsets_.end(), offset);
                size_t index = it - offsets_.begin();

                return index == 0 ? offset + 1 : offset - offsets_[index - 1];

            }

        private:
            void build_loc_data() {
                FILE* f = fopen(filename_.c_str(), "rb");

                ptrdiff_t offset = 0;

                char buffer[1024];
                size_t size;

                while ((size = fread(buffer, 1, sizeof(buffer), f)) > 0)
                {
                for (size_t i = 0; i < size; ++i)
                    if (buffer[i] == '\n')
                        offsets_.push_back(offset + i);

                    offset += size;
                }

                fclose(f);
            }

            std::string filename_;
            std::vector<ptrdiff_t> offsets_;
    };
}

#endif
