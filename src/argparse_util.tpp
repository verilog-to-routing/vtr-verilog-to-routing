#include <sstream>
#include "argparse.hpp"

namespace argparse {

    template<typename T> 
    T as(std::string str) {
        
        std::stringstream ss(str);

        T val;
        ss >> val;

        if (!ss.good()) {
            std::stringstream msg;
            msg << "Failed to convert value '" << str << "'";
            throw ArgParseError(msg.str().c_str());
        }
    }

    template<typename Container>
    std::string join(Container container, std::string join_str) {
        std::stringstream ss;

        bool first = true;
        for (const auto& val : container) {
            if (!first) ss << join_str;
            ss << val;
            first = false;
        }

        return ss.str();
    }
}
