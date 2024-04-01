#ifndef TELEGRAMOPTIONS_H
#define TELEGRAMOPTIONS_H

#include <set>
#include <vector>
#include <unordered_map>
#include <map>

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
    TelegramOptions(const std::string& data, const std::vector<std::string>& expectedKeys);
    ~TelegramOptions()=default;

    bool hasErrors() const { return !m_errors.empty(); }

    std::map<std::size_t, std::set<std::size_t>> getMapOfSets(const std::string& name);

    std::string getString(const std::string& name);

    int getInt(const std::string& name, int failValue);

    bool getBool(const std::string& name, bool failValue);

    std::string errorsStr() const;

private:
    std::unordered_map<std::string, Option> m_options;
    std::vector<std::string> m_errors;

    bool isDataTypeSupported(const std::string& type) const;
    bool checkKeysPresence(const std::vector<std::string>& keys);
};

} // namespace server

#endif // TELEGRAMOPTIONS_H
