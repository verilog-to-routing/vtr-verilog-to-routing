#ifndef NO_SERVER

#include "convertutils.h"
#include <sstream>
#include <iomanip>
#include <cstring>

#include "vtr_util.h"
#include "vtr_error.h"

std::optional<int> try_convert_to_int(const std::string& str) {
    try {
        return vtr::atoi(str);
    } catch (const vtr::VtrError&) {
        return std::nullopt;
    }
}

static std::string get_pretty_str_from_double(double value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;  // Set precision to 2 digit after the decimal point
    return ss.str();
}

std::string get_pretty_duration_str_from_ms(int64_t duration_ms) {
    std::string result;
    if (duration_ms >= 1000) {
        result = get_pretty_str_from_double(duration_ms / 1000.0) + " sec";
    } else {
        result = std::to_string(duration_ms);
        result += " ms";
    }
    return result;
}

std::string get_pretty_size_str_from_bytes_num(int64_t bytes_num) {
    std::string result;
    if (bytes_num >= 1024*1024*1024) {
        result = get_pretty_str_from_double(bytes_num / double(1024*1024*1024)) + "Gb";
    } else if (bytes_num >= 1024*1024) {
        result = get_pretty_str_from_double(bytes_num / double(1024*1024)) + "Mb";
    } else if (bytes_num >= 1024) {
        result = get_pretty_str_from_double(bytes_num / double(1024)) + "Kb";
    } else {
        result = std::to_string(bytes_num) + "bytes";
    }
    return result;
}

std::string get_truncated_middle_str(const std::string& src, std::size_t num) {
    std::string result;
    constexpr std::size_t minimal_string_size_to_truncate = 20;
    if (num < minimal_string_size_to_truncate) {
        num = minimal_string_size_to_truncate;
    }
    constexpr const char middle_place_holder[] = "...";
    const std::size_t src_size = src.size();
    if (src_size > num) {
        int prefix_num = num / 2;
        int suffix_num = num / 2 - std::strlen(middle_place_holder);
        result.append(src.substr(0, prefix_num));
        result.append(middle_place_holder);
        result.append(src.substr(src_size - suffix_num));
    } else {
        result = src;
    }

    return result;
}

#endif /* NO_SERVER */
