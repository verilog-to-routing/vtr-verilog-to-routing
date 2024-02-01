#include "convertutils.h"
#include <sstream>

std::optional<int> tryConvertToInt(const std::string& str) {
    std::optional<int> result;

    std::istringstream iss(str);
    int intValue;
    if (iss >> intValue) {
        // Check if there are no any characters left in the stream
        char remaining;
        if (!(iss >> remaining)) {
            result = intValue;
        }
    }
    return result;
}
