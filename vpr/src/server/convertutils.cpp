#include "convertutils.h"

std::optional<int> tryConvertToInt(const std::string& str)
{
    std::optional<int> result;

    char* endptr; // To store the end of the conversion
    errno = 0; // Set errno to 0 before the call
    int candidate = std::strtol(str.c_str(), &endptr, 10);

    // Check for conversion errors
    if (errno != 0) {
        return result;
    }

    // Check if there were any non-numeric characters
    if (endptr == str.c_str()) {
        return result;
    }

    result = candidate;
    return result;
}

