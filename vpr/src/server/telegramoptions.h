#ifndef TELEGRAMOPTIONS_H
#define TELEGRAMOPTIONS_H

#include <convertutils.h>

#include <sstream>
#include <iostream>
#include <set>
#include <vector>
#include <unordered_map>

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
    TelegramOptions(const std::string& data, const std::vector<std::string>& expectedKeys) {
        // parse data string
        std::vector<std::string> options = splitString(data, ';');
        for (const std::string& optionStr: options) {
            std::vector<std::string> fragments = splitString(optionStr, ':');
            if (fragments.size() == TOTAL_INDEXES_NUM) {
                std::string name = fragments[INDEX_NAME];
                Option option{fragments[INDEX_TYPE], fragments[INDEX_VALUE]};
                if (isDataTypeSupported(option.type)) {
                    m_options[name] = option;
                } else {
                    m_errors.emplace_back("bad type for option [" + optionStr + "]");
                }
            } else {
                m_errors.emplace_back("bad option [" + optionStr + "]");
            }
        }

        // check keys presense
        checkKeysPresence(expectedKeys);
    }

    ~TelegramOptions() {}

    bool hasErrors() const { return !m_errors.empty(); }

    std::map<std::size_t, std::set<std::size_t>> getMapOfSets(const std::string& name) {
        std::map<std::size_t, std::set<std::size_t>> result;
        std::string dataStr = getString(name);
        if (!dataStr.empty()) {
            std::vector<std::string> pathes = splitString(dataStr, '|');
            for (const std::string& path: pathes) {
                std::vector<std::string> pathStruct = splitString(path, '#');
                if (pathStruct.size() == 2) {
                    std::string pathIndexStr = pathStruct[0];
                    std::string pathElementIndexesStr = pathStruct[1];
                    std::vector<std::string> pathElementIndexes = splitString(pathElementIndexesStr, ',');
                    std::set<std::size_t> elements;
                    for (const std::string& pathElementIndex: pathElementIndexes) {
                        if (std::optional<int> optValue = tryConvertToInt(pathElementIndex.c_str())) {
                            elements.insert(optValue.value());
                        }
                    }
                    if (std::optional<int> optPathIndex = tryConvertToInt(pathIndexStr.c_str())) {
                        result[optPathIndex.value()] = elements;
                    }
                } else {
                    m_errors.emplace_back("wrong path data structure = " + path);
                }
            }
        }
        return result;
    }

    std::string getString(const std::string& name) {
        std::string result;
        if (auto it = m_options.find(name); it != m_options.end()) {
            result = it->second.value;
        }
        return result;
    }

    int getInt(const std::string& name, int failValue) {
        if (std::optional<int> opt = tryConvertToInt(m_options[name].value)) {
            return opt.value();
        } else {
            m_errors.emplace_back("cannot get int value for option " + name);
            return failValue;
        }
    }

    bool getBool(const std::string& name, bool failValue) {
        if (std::optional<int> opt = tryConvertToInt(m_options[name].value)) {
            return opt.value();
        } else {
            m_errors.emplace_back("cannot get bool value for option " + name);
            return failValue;
        }
    }

    std::string errorsStr() const { 
        std::string result;
        for (const std::string& error: m_errors) {
            result += error + ";";
        }
        return result; 
    }

private:
    std::unordered_map<std::string, Option> m_options;
    std::vector<std::string> m_errors;


    std::vector<std::string> splitString(const std::string& input, char delimiter)
    {
        std::vector<std::string> tokens;
        std::istringstream tokenStream(input);
        std::string token;

        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }

        return tokens;
    }

    bool isDataTypeSupported(const std::string& type) {
        static std::set<std::string> supportedTypes{"int", "string", "bool"};
        return supportedTypes.count(type) != 0;
    }

    bool checkKeysPresence(const std::vector<std::string>& keys) {
        bool result = true;
        for (const std::string& key: keys) {
            if (m_options.find(key) == m_options.end()) {
                m_errors.emplace_back("cannot find required option " + key);
                result = false;
            }
        }
        return result;
    }
};

} // namespace server

#endif // TELEGRAMOPTIONS_H
