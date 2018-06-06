#include "argparse_util.hpp"
#include <cstring>
#include <algorithm>

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

    bool is_argument(std::string str, const std::map<std::string,std::shared_ptr<Argument>>& arg_map) {

        for (const auto& kv : arg_map) {
            auto iter = arg_map.find(str);

            if (iter != arg_map.end()) {
                //Exact match to short/long option
                return true;
            }

            if (kv.first.size() == 2 && kv.first[0] == '-') {
                //Check iff this is a short option with no spaces
                if (str[0] == kv.first[0] && str[1] == kv.first[1]) {
                    //Matches first two characters
                    return true;
                }
            }

        }
        return false;
    }

    bool is_valid_choice(std::string str, const std::vector<std::string>& choices) {
        if (choices.empty()) return true;

        auto find_iter = std::find(choices.begin(), choices.end(), str);
        if (find_iter == choices.end()) {
            return false;
        }
        return true;
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
