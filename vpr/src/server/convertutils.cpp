#include "convertutils.h"
#include <sstream>
#include <iomanip>

#include "vtr_util.h"
#include "vtr_error.h"

std::optional<int> tryConvertToInt(const std::string& str)
{
    try {
        return vtr::atoi(str);
    } catch (const vtr::VtrError&) {
        return std::nullopt;
    }
}

static std::string getPrettyStrFromFloat(float value)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;  // Set precision to 2 digit after the decimal point
    return ss.str();
}

std::string getPrettyDurationStrFromMs(int64_t durationMs)
{
    std::string result;
    if (durationMs >= 1000) {
        result = getPrettyStrFromFloat(durationMs / 1000.0f) + " sec";
    } else {
        result = std::to_string(durationMs);
        result += " ms";
    }
    return result;
}

std::string getPrettySizeStrFromBytesNum(int64_t bytesNum)
{
    std::string result;
    if (bytesNum >= 1024*1024*1024) {
        result = getPrettyStrFromFloat(bytesNum / float(1024*1024*1024)) + "Gb";
    } else if (bytesNum >= 1024*1024) {
        result = getPrettyStrFromFloat(bytesNum / float(1024*1024)) + "Mb";
    } else if (bytesNum >= 1024) {
        result = getPrettyStrFromFloat(bytesNum / float(1024)) + "Kb";
    } else {
        result = std::to_string(bytesNum) + "bytes";
    }
    return result;
}


std::string getTruncatedMiddleStr(const std::string& src, std::size_t num) {
    std::string result;
    static std::size_t minimalStringSizeToTruncate = 20;
    if (num < minimalStringSizeToTruncate) {
        num = minimalStringSizeToTruncate;
    }
    static std::string middlePlaceHolder("...");
    const std::size_t srcSize = src.size();
    if (srcSize > num) {
        int prefixNum = num/2;
        int suffixNum = num/2 - middlePlaceHolder.size();
        result.append(src.substr(0, prefixNum));
        result.append(middlePlaceHolder);
        result.append(src.substr(srcSize - suffixNum));
    } else {
        result = src;
    }

    return result;
}
