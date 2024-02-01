#include "telegramparser.h"
#include "convertutils.h"
#include "commconstants.h"

namespace comm {

std::optional<std::string> TelegramParser::tryExtractJsonValueStr(const std::string& jsonString, const std::string& key) {
    std::optional<std::string> result;

    // Find the position of the key
    size_t keyPos = jsonString.find("\"" + key + "\":");

    if (keyPos == std::string::npos) {
        // Key not found
        return result;
    }

    // Find the position of the value after the key
    size_t valuePosStart = jsonString.find("\"", keyPos + key.length() + std::string("\":\"").size());

    if (valuePosStart == std::string::npos) {
        // Value not found
        return result;
    }

    // Find the position of the closing quote for the value
    size_t valueEnd = jsonString.find("\"", valuePosStart + std::string("\"").size());

    if (valueEnd == std::string::npos) {
        // Closing quote not found
        return result;
    }

    // Extract the value substring
    result = jsonString.substr(valuePosStart + 1, (valueEnd - valuePosStart) - 1);
    return result;
}

std::optional<int> TelegramParser::tryExtractFieldJobId(const std::string& message) {
    std::optional<int> result;
    if (std::optional<std::string> strOpt = tryExtractJsonValueStr(message, comm::KEY_JOB_ID)) {
        result = tryConvertToInt(strOpt.value());
    }
    return result;
}

std::optional<int> TelegramParser::tryExtractFieldCmd(const std::string& message) {
    std::optional<int> result;
    if (std::optional<std::string> strOpt = tryExtractJsonValueStr(message, comm::KEY_CMD)) {
        result = tryConvertToInt(strOpt.value());
    }
    return result;
}

std::optional<std::string> TelegramParser::tryExtractFieldOptions(const std::string& message) {
    return tryExtractJsonValueStr(message, comm::KEY_OPTIONS);
}

std::optional<std::string> TelegramParser::tryExtractFieldData(const std::string& message) {
    return tryExtractJsonValueStr(message, comm::KEY_DATA);
}

std::optional<int> TelegramParser::tryExtractFieldStatus(const std::string& message) {
    std::optional<int> result;
    if (std::optional<std::string> strOpt = tryExtractJsonValueStr(message, comm::KEY_STATUS)) {
        result = tryConvertToInt(strOpt.value());
    }
    return result;
}

} // namespace comm
