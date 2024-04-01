#include "telegramoptions.h"
#include "convertutils.h"

#include <sstream>

namespace server {
    
TelegramOptions::TelegramOptions(const std::string& data, const std::vector<std::string>& expectedKeys) 
{
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

    // check keys presence
    checkKeysPresence(expectedKeys);
}

std::map<std::size_t, std::set<std::size_t>> TelegramOptions::getMapOfSets(const std::string& name) 
{
    std::map<std::size_t, std::set<std::size_t>> result;
    std::string dataStr = getString(name);
    if (!dataStr.empty()) {
        std::vector<std::string> paths = splitString(dataStr, '|');
        for (const std::string& path: paths) {
            std::vector<std::string> pathStruct = splitString(path, '#');
            if (pathStruct.size() == 2) {
                std::string pathIndexStr = pathStruct[0];
                std::string pathElementIndexesStr = pathStruct[1];
                std::vector<std::string> pathElementIndexes = splitString(pathElementIndexesStr, ',');
                std::set<std::size_t> elements;
                for (const std::string& pathElementIndexStr: pathElementIndexes) {
                    if (std::optional<int> optValue = tryConvertToInt(pathElementIndexStr)) {
                        elements.insert(optValue.value());
                    } else {
                        m_errors.emplace_back("cannot extract path element index from " + pathElementIndexStr);
                    }
                }
                if (std::optional<int> optPathIndex = tryConvertToInt(pathIndexStr)) {
                    result[optPathIndex.value()] = elements;
                } else {
                    m_errors.emplace_back("cannot extract path index from " + pathIndexStr);
                }
            } else {
                m_errors.emplace_back("wrong path data structure = " + path);
            }
        }
    }
    return result;
}

std::string TelegramOptions::getString(const std::string& name) 
{
    std::string result;
    if (auto it = m_options.find(name); it != m_options.end()) {
        result = it->second.value;
    }
    return result;
}

int TelegramOptions::getInt(const std::string& name, int failValue) 
{
    if (std::optional<int> opt = tryConvertToInt(m_options[name].value)) {
        return opt.value();
    } else {
        m_errors.emplace_back("cannot get int value for option " + name);
        return failValue;
    }
}

bool TelegramOptions::getBool(const std::string& name, bool failValue) 
{
    if (std::optional<int> opt = tryConvertToInt(m_options[name].value)) {
        return opt.value();
    } else {
        m_errors.emplace_back("cannot get bool value for option " + name);
        return failValue;
    }
}

std::string TelegramOptions::errorsStr() const 
{ 
    std::string result;
    for (const std::string& error: m_errors) {
        result += error + ';';
    }
    return result; 
}

std::vector<std::string> TelegramOptions::splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream tokenStream(input);
    std::string token;

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

bool TelegramOptions::isDataTypeSupported(const std::string& type) const 
{
    static const std::set<std::string> supportedTypes{"int", "string", "bool"};
    return supportedTypes.count(type) != 0;
}

bool TelegramOptions::checkKeysPresence(const std::vector<std::string>& keys) 
{
    bool result = true;
    for (const std::string& key: keys) {
        if (m_options.find(key) == m_options.end()) {
            m_errors.emplace_back("cannot find required option " + key);
            result = false;
        }
    }
    return result;
}

} // namespace server

