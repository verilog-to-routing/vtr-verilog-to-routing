#include "argparse_util.hpp"
#include <cstring>

namespace argparse {

    std::array<std::string,2> split_leading_dashes(std::string str) {
        auto iter = str.begin();
        while(*iter == '-') {
            ++iter;
        }

        std::string dashes(str.begin(), iter);
        std::string name(iter, str.end());
        std::array<std::string,2> array = {dashes, name};

        return array;
    }

    bool is_argument(std::string str) {
        if (str.size() == 2 && str[0] == '-' && str[1] != '-') {
            return true; //Short option
        } else if (str.size() > 2 && str[0] == '-' && str[1] == '-' && str[2] != '-') {
            return true; //Long option
        }
        return false;
    }

    std::string toupper(std::string str) {
        std::string upper;
        for (size_t i = 0; i < str.size(); ++i) {
            char C = ::toupper(str[i]);
            upper.push_back(C);
        }
        return upper;
    }

    std::string tolower(std::string str) {
        std::string lower;
        for (size_t i = 0; i < str.size(); ++i) {
            char C = ::tolower(str[i]);
            lower.push_back(C);
        }
        return lower;
    }

    char* strdup(const char* str) {
        size_t len = std::strlen(str);
        char* res = new char[len+1]; //+1 for terminator
        std::strcpy(res, str);
        return res;
    }

    std::vector<std::string> wrap_width(std::string str, size_t width, std::vector<std::string> break_strs) {
        std::vector<std::string> wrapped_lines;

        size_t start = 0;
        size_t end = 0;
        size_t last_break = 0;
        for(end = 0; end < str.size(); ++end) {

            size_t len = end - start;

            if (len > width) {
                auto wrapped_line = std::string(str, start, last_break - start) + "\n";
                wrapped_lines.push_back(wrapped_line);
                start = last_break;
            }

            //Find the next break
            for (const auto& brk_str : break_strs) {
                auto pos = str.find(brk_str, end); //FIXME: this is inefficient
                if (pos == end) {
                    last_break = end + 1;
                }
            }

            //If there are embedded new-lines then take them as forced breaks
            char c = str[end];
            if (c == '\n') {
                last_break = end + 1;
                auto wrapped_line = std::string(str, start, last_break - start);
                wrapped_lines.push_back(wrapped_line);
                start = last_break;
            }
        }

        auto last_line = std::string(str, start, end - start);
        wrapped_lines.push_back(last_line);

        return wrapped_lines;
    }

    std::string basename(std::string filepath) {
#ifdef _WIN32
        //Windows uses back-slash as directory divider
        auto pos = filepath.rfind("\\");
#else
        //*nix-like uses forward-slash as directory divider
        auto pos = filepath.rfind("/");
#endif
        if (pos == std::string::npos) {
            pos = 0;
        } else {
            pos += 1;
        }

        return std::string(filepath, pos, filepath.size() - pos);
    }
} //namespace
