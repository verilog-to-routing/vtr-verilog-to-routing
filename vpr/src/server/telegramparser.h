#ifndef TELEGRAMPARSER_H
#define TELEGRAMPARSER_H

#ifndef NO_SERVER

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
    static std::optional<int> try_extract_field_job_id(const std::string& message);
    static std::optional<int> try_extract_field_cmd(const std::string& message);
    static std::optional<std::string> try_extract_field_options(const std::string& message);
    static std::optional<std::string> try_extract_field_data(const std::string& message);
    static std::optional<int> try_extract_field_status(const std::string& message);

private:
    static std::optional<std::string> try_extract_json_value_str(const std::string& json_string, const std::string& key);
};

} // namespace comm

#endif /* NO_SERVER */

#endif /* TELEGRAMPARSER_H */
