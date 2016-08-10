#ifndef VTR_UTIL_H
#define VTR_UTIL_H

#include <vector>
#include <string>

namespace vtrutil {

    //Splits the string 'text' along the specified delimiter characters in 'delims'
    //The split strings (excluding the delimiters) are returned
    std::vector<std::string> split(const std::string& text, const std::string delims=" \t\n");

}

#endif
