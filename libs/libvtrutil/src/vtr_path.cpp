#include "vtr_path.h"

#include "vtr_util.h"

#include <sstream>
#include <filesystem>

namespace vtr {

const std::string PATH_DELIM = "/";

//Splits off the name and extension (including ".") of the specified filename
std::array<std::string, 2> split_ext(const std::string& filename) {
    std::array<std::string, 2> name_ext;
    auto pos = filename.find_last_of('.');

    if (pos == std::string::npos) {
        //No extension
        pos = filename.size();
    }

    name_ext[0] = std::string(filename, 0, pos);
    name_ext[1] = std::string(filename, pos, filename.size() - pos);

    return name_ext;
}

std::string basename(const std::string& path) {
    std::vector<std::string> elements = StringToken(path).split(PATH_DELIM);

    std::string str;
    if (elements.size() > 0) {
        //Return the last path element
        str = elements[elements.size() - 1];
    }

    return str;
}

std::string dirname(const std::string& path) {
    std::vector<std::string> elements = StringToken(path).split(PATH_DELIM);

    std::string str;
    if (elements.size() > 0) {
        //We need to start the dirname with a PATH_DELIM if path started with one
        if (starts_with(path, PATH_DELIM)) {
            str += PATH_DELIM;
        }

        //Join all except the last path element
        str += join(elements.begin(), elements.end() - 1, PATH_DELIM);

        //We append a final PATH_DELIM to allow clients to just append directly to the
        //returned value
        str += PATH_DELIM;
    }

    return str;
}

std::string getcwd() {
    try {
        return std::filesystem::current_path().string();
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error(e.what());
    }
}

} // namespace vtr
