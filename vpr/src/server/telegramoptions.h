#ifndef TELEGRAMOPTIONS_H
#define TELEGRAMOPTIONS_H

#ifndef NO_SERVER

#include <set>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>

namespace server {
    
/** 
 * @brief Option class Parser
 *
 * Parse the string of options in the format "TYPE:KEY1:VALUE1;TYPE:KEY2:VALUE2", 
 * for example "int:path_num:11;string:path_type:debug;int:details_level:3;bool:is_flat_routing:0".
 * It provides a simple interface to check value presence and access them.
*/

class TelegramOptions {
private:
    enum {
        INDEX_TYPE=0,
        INDEX_NAME,
        INDEX_VALUE,
        TOTAL_INDEXES_NUM
    };

    struct Option {
        std::string type;
        std::string value;
    };

public:
    /**
     * @brief Constructs a TelegramOptions object with the provided data and expected keys.
     *
     * This constructor initializes a TelegramOptions object with the given data string
     * and a vector of expected keys. It parses the data string and validates that such data has all required keys.
     * If some keys are absent, it collects the errors.
     *
     * @param data The data string containing the options.
     * @param expected_keys A vector of strings representing the expected keys in the options.
     */
    TelegramOptions(const std::string& data, const std::vector<std::string>& expected_keys);
    ~TelegramOptions()=default;

    /**
     * @brief Checks if there are any errors present.
     *
     * This function returns true if there are errors present in the error container,
     * otherwise it returns false.
     *
     * @return True if there are errors present, false otherwise.
     */
    bool has_errors() const { return !m_errors.empty(); }

    /**
     * @brief Retrieves a map of sets associated with the specified key.
     *
     * This function retrieves a map of sets associated with the specified key.
     *
     * @note The map of sets is used to store the critical path index (map key) and the associated set of selected sub-path element indexes (map value).
     *
     * @param key The key of the map of sets to retrieve.
     * @return The map of sets associated with the specified key.
     */
    std::map<std::size_t, std::set<std::size_t>> get_map_of_sets(const std::string& key);

    /**
     * @brief Retrieves the string associated with the specified key.
     *
     * This function retrieves the string associated with the specified key.
     * The key is used to identify the desired string value.
     *
     * @param key The key of the string to retrieve.
     * @return The string associated with the specified key.
     */
    std::string get_string(const std::string& key);

    /**
     * @brief Retrieves the integer value associated with the specified key.
     *
     * This function retrieves the integer value associated with the specified key.
     * If the key is found, its corresponding integer value is returned.
     * If the key is not found, the specified fail_value is returned instead.
     *
     * @param key The key whose associated integer value is to be retrieved.
     * @param fail_value The value to return if the key is not found.
     * @return The integer value associated with the specified key, or fail_value if the key is not found.
     */
    int get_int(const std::string& key, int fail_value);

    /**
     * @brief Retrieves the boolean value associated with the specified key.
     *
     * This function retrieves the boolean value associated with the specified key.
     *
     * @param key The key whose associated boolean value is to be retrieved.
     * @param fail_value The value to return if the key is not found.
     * @return The boolean value associated with the specified key, or fail_value if the key is not found.
     */
    bool get_bool(const std::string& key, bool fail_value);

    /**
     * @brief Retrieves a concatenated string of all errors stored in the container.
     *
     * This function retrieves a concatenated string of all errors stored in the container.
     * It concatenates all error strings stored in the container and returns the result.
     * If there are no errors stored in the container, an empty string is returned.
     *
     * @return A concatenated string of all errors stored in the container.
     */
    std::string errors_str() const;

private:
    std::unordered_map<std::string, Option> m_options;
    std::vector<std::string> m_errors;

    bool is_data_type_supported(const std::string& type) const;
    bool check_keys_presence(const std::vector<std::string>& keys);
};

} // namespace server

#endif /* NO_SERVER */

#endif /* TELEGRAMOPTIONS_H */
