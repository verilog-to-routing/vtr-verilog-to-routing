#include "convertutils.h"
#include <sstream>
#include <iomanip>

std::optional<int> tryConvertToInt(const std::string& str)
{
    std::optional<int> result;

    std::istringstream iss(str);
    int intValue;
    if (iss >> intValue) {
        if (iss.eof()) {
            result = intValue;
        }
    }
    return result;
}

namespace {
std::string getPrettyStrFromFloat(float value)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;  // Set precision to 2 digit after the decimal point
    return ss.str();
}
} // namespace

std::string getPrettyDurationStrFromMs(int64_t durationMs)
{
    std::string result;
    if (durationMs >= 1000) {
        result = getPrettyStrFromFloat(durationMs/1000.0f) + " sec";
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
        result = getPrettyStrFromFloat(bytesNum/float(1024*1024*1024)) + "Gb";
    } else if (bytesNum >= 1024*1024) {
        result = getPrettyStrFromFloat(bytesNum/float(1024*1024)) + "Mb";
    } else if (bytesNum >= 1024) {
        result = getPrettyStrFromFloat(bytesNum/float(1024)) + "Kb";
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
        result.append(std::move(src.substr(0, prefixNum)));
        result.append(middlePlaceHolder);
        result.append(std::move(src.substr(srcSize - suffixNum)));
    } else {
        result = src;
    }

    return result;
}
