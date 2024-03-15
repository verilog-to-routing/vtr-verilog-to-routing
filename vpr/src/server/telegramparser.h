#ifndef TELEGRAMPARSER_H
#define TELEGRAMPARSER_H

#include <string>
#include <optional>

namespace comm {

/**
 * @brief Dummy JSON parser using regular expressions.
 *
 * This module provides helper methods to extract values for a keys as "JOB_ID", "CMD", or "OPTIONS" 
 * from a JSON schema structured as follows: {JOB_ID:num, CMD:enum, OPTIONS:string}.
 */
class TelegramParser {
public:
    static std::optional<int> tryExtractFieldJobId(const std::string& message);
    static std::optional<int> tryExtractFieldCmd(const std::string& message);
    static bool tryExtractFieldOptions(const std::string& message, std::optional<std::string>& result);
    static bool tryExtractFieldData(const std::string& message, std::optional<std::string>& result);
    static std::optional<int> tryExtractFieldStatus(const std::string& message);

private:
    static bool tryExtractJsonValueStr(const std::string& jsonString, const std::string& key, std::optional<std::string>& result);
};

} // namespace comm

#endif // TELEGRAMPARSER_H
