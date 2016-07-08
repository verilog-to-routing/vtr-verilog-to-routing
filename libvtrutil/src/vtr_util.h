#ifndef VTR_UTIL_H
#define VTR_UTIL_H

#include <vector>
#include <string>

namespace vtrutil {

    //Splits the string 'text' along the specified delimiter 'delim'
    std::vector<std::string> split(const std::string& text, const std::string delims);

}

#endif
