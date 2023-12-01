#include "telegramparser.h"

#include <regex>

namespace telegramparser {

int extractJobId(const std::string& message)
{
    static std::regex pattern("\"JOB_ID\":\"(\\d+)\"");
    std::smatch match;
    if (std::regex_search(message, match, pattern)) {
        if (match.size() > 1) {
            return std::atoi(match[1].str().c_str());
        }
    }
    return -1;
}

int extractCmd(const std::string& message)
{
    static std::regex pattern("\"CMD\":\"(\\d+)\"");
    std::smatch match;
    if (std::regex_search(message, match, pattern)) {
        if (match.size() > 1) {
            return std::atoi(match[1].str().c_str());
        }
    }
    return -1;
}

std::string extractOptions(const std::string& message)
{
    static std::regex pattern("\"OPTIONS\":\"(.*?)\"");
    std::smatch match;
    if (std::regex_search(message, match, pattern)) {
        if (match.size() > 1) {
            return match[1].str();
        }
    }
    return "";
}

} // telegramparser
