#ifndef NO_SERVER

#include "telegramparser.h"
#include "convertutils.h"
#include "commconstants.h"

#include <cstring>

namespace comm {

std::optional<std::string> TelegramParser::try_extract_json_value_str(const std::string& json_string, const std::string& key) {
    constexpr const char end_key_pattern[] = {"\":\""};

    // Find the position of the key
    size_t key_pos = json_string.find('\"' + key + end_key_pattern);

    if (key_pos == std::string::npos) {
        // Key not found
        return std::nullopt;
    }

    // Find the position of the value after the key
    size_t value_pos_start = json_string.find('\"', key_pos + key.length() + std::strlen(end_key_pattern));

    if (value_pos_start == std::string::npos) {
        // Value not found
        return std::nullopt;
    }

    // Find the position of the closing quote for the value
    size_t value_end = json_string.find('\"', value_pos_start + sizeof('\"'));

    if (value_end == std::string::npos) {
        // Closing quote not found
        return std::nullopt;
    }

    // Extract the value substring
    return json_string.substr(value_pos_start + 1, (value_end - value_pos_start) - 1);
}

std::optional<int> TelegramParser::try_extract_field_job_id(const std::string& message) {
    if (std::optional<std::string> str_opt = try_extract_json_value_str(message, comm::KEY_JOB_ID)) {
        return try_convert_to_int(str_opt.value());
    }
    return std::nullopt;
}

std::optional<int> TelegramParser::try_extract_field_cmd(const std::string& message) {
    if (std::optional<std::string> str_opt = try_extract_json_value_str(message, comm::KEY_CMD)) {
        return try_convert_to_int(str_opt.value());
    }
    return std::nullopt;
}

std::optional<std::string> TelegramParser::try_extract_field_options(const std::string& message) {
    return try_extract_json_value_str(message, comm::KEY_OPTIONS);
}

std::optional<std::string> TelegramParser::try_extract_field_data(const std::string& message) {
    return try_extract_json_value_str(message, comm::KEY_DATA);
}

std::optional<int> TelegramParser::try_extract_field_status(const std::string& message) {
    if (std::optional<std::string> str_opt = try_extract_json_value_str(message, comm::KEY_STATUS)) {
        return try_convert_to_int(str_opt.value());
    }
    return std::nullopt;
}

} // namespace comm

#endif /* NO_SERVER */
