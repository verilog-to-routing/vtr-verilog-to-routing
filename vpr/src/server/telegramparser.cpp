#include "telegramparser.h"

#include <regex>

namespace server {

int TelegramParser::extractJobId(const std::string& message)
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

int TelegramParser::extractCmd(const std::string& message)
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

std::string TelegramParser::extractOptions(const std::string& message)
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

} // namespace server
