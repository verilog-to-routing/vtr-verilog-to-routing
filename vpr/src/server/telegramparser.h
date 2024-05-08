#ifndef TELEGRAMPARSER_H
#define TELEGRAMPARSER_H

#ifndef NO_SERVER

#include <string>
#include <optional>

namespace comm {

/**
 * @brief Dummy JSON parser.
 *
 * This class provides helper methods to extract values for a keys as "JOB_ID", "CMD", "OPTIONS", "DATA", or "STATUS
 * from a JSON schema structured as follows: {JOB_ID:num, CMD:enum, OPTIONS:string, DATA:string, STATUS:num}.
 */
class TelegramParser {
public:
    /**
     * @brief Attempts to extract the JOB_ID field from a given message.
     *
     * This function parses the provided message and attempts to extract the JOB_ID field from it.
     * If the JOB_ID field is found and successfully extracted, it is returned as an optional integer.
     * If the JOB_ID field is not found or cannot be parsed as an integer, an empty optional is returned.
     *
     * @param message The message from which to extract the JOB_ID field.
     * @return An optional integer containing the extracted JOB_ID if successful, otherwise an empty optional.
     */
    static std::optional<int> try_extract_field_job_id(const std::string& message);

    /**
     * @brief Attempts to extract the CMD field from a given message.
     *
     * This function parses the provided message and attempts to extract the CMD field from it.
     * If the CMD field is found and successfully extracted, it is returned as an optional integer.
     * If the CMD field is not found or cannot be parsed as an integer, an empty optional is returned.
     *
     * @param message The message from which to extract the CMD field.
     * @return An optional integer containing the extracted CMD if successful, otherwise an empty optional.
     */
    static std::optional<int> try_extract_field_cmd(const std::string& message);

    /**
     * @brief Attempts to extract the OPTIONS field from a given message.
     *
     * This function parses the provided message and attempts to extract the OPTIONS field from it.
     * If the OPTIONS field is found and successfully extracted, it is returned as an optional string.
     * If the OPTIONS field is not found an empty optional is returned.
     *
     * @param message The message from which to extract the OPTIONS field.
     * @return An optional string containing the extracted OPTIONS if successful, otherwise an empty optional.
     */
    static std::optional<std::string> try_extract_field_options(const std::string& message);

    /**
     * @brief Attempts to extract the DATA field from a given message.
     *
     * This function parses the provided message and attempts to extract the DATA field from it.
     * If the DATA field is found and successfully extracted, it is returned as an optional string.
     * If the DATA field is not found an empty optional is returned.
     *
     * @param message The message from which to extract the DATA field.
     * @return An optional string containing the extracted DATA if successful, otherwise an empty optional.
     */
    static std::optional<std::string> try_extract_field_data(const std::string& message);

    /**
     * @brief Attempts to extract the STATUS field from a given message.
     *
     * This function parses the provided message and attempts to extract the STATUS field from it.
     * If the STATUS field is found and successfully extracted, it is returned as an optional integer.
     * If the STATUS field is not found or cannot be parsed as an integer, an empty optional is returned.
     *
     * @param message The message from which to extract the STATUS field.
     * @return An optional integer containing the extracted STATUS if successful, otherwise an empty optional.
     */
    static std::optional<int> try_extract_field_status(const std::string& message);

private:
    static std::optional<std::string> try_extract_json_value_str(const std::string& json_string, const std::string& key);
};

} // namespace comm

#endif /* NO_SERVER */

#endif /* TELEGRAMPARSER_H */
