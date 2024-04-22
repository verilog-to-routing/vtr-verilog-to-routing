#ifndef TELEGRAMOPTIONS_H
#define TELEGRAMOPTIONS_H

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
    TelegramOptions(const std::string& data, const std::vector<std::string>& expected_keys);
    ~TelegramOptions()=default;

    bool has_errors() const { return !m_errors.empty(); }

    std::map<std::size_t, std::set<std::size_t>> get_map_of_sets(const std::string& name);

    std::string get_string(const std::string& name);

    int get_int(const std::string& name, int fail_value);

    bool get_bool(const std::string& name, bool fail_value);

    std::string errors_str() const;

private:
    std::unordered_map<std::string, Option> m_options;
    std::vector<std::string> m_errors;

    bool is_data_type_supported(const std::string& type) const;
    bool check_keys_presence(const std::vector<std::string>& keys);
};

} // namespace server

#endif /* TELEGRAMOPTIONS_H */
