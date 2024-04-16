#include "telegramoptions.h"
#include "convertutils.h"

#include "vtr_util.h"

#include <optional>
#include <sstream>

namespace server {
    
TelegramOptions::TelegramOptions(const std::string& data, const std::vector<std::string_view>& expectedKeys)
{
    // parse data string
    std::vector<std::string> options = vtr::split(data, ";");
    for (const std::string& optionStr: options) {
        std::vector<std::string> fragments = vtr::split(optionStr, ":");
        if (fragments.size() == TOTAL_INDEXES_NUM) {
            const std::string& name = fragments[INDEX_NAME];
            Option option{std::move(fragments[INDEX_TYPE]), std::move(fragments[INDEX_VALUE])};
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

std::map<std::size_t, std::set<std::size_t>> TelegramOptions::getMapOfSets(const std::string_view& name)
{
    std::map<std::size_t, std::set<std::size_t>> result;
    std::string dataStr = getString(name);
    if (!dataStr.empty()) {
        std::vector<std::string> paths = vtr::split(dataStr, "|");
        for (const std::string& path: paths) {
            std::vector<std::string> pathStruct = vtr::split(path, "#");
            if (pathStruct.size() == 2) {
                std::string pathIndexStr = pathStruct[0];
                std::string pathElementIndexesStr = pathStruct[1];
                std::vector<std::string> pathElementIndexes = vtr::split(pathElementIndexesStr, ",");
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

std::string TelegramOptions::getString(const std::string_view& name)
{
    std::string result;
    if (auto it = m_options.find(name); it != m_options.end()) {
        result = it->second.value;
    }
    return result;
}

int TelegramOptions::getInt(const std::string_view& name, int failValue)
{
    if (std::optional<int> opt = tryConvertToInt(m_options[name].value)) {
        return opt.value();
    } else {
        m_errors.emplace_back("cannot get int value for option " + std::string(name));
        return failValue;
    }
}

bool TelegramOptions::getBool(const std::string_view& name, bool failValue)
{
    if (std::optional<int> opt = tryConvertToInt(m_options[name].value)) {
        return opt.value();
    } else {
        m_errors.emplace_back("cannot get bool value for option " + std::string(name));
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

bool TelegramOptions::isDataTypeSupported(const std::string& type) const
{
    static const std::set<std::string> supportedTypes{"int", "string", "bool"};
    return supportedTypes.count(type) != 0;
}

bool TelegramOptions::checkKeysPresence(const std::vector<std::string_view>& keys)
{
    bool result = true;
    for (const std::string_view& key: keys) {
        if (m_options.find(key) == m_options.end()) {
            m_errors.emplace_back("cannot find required option " + std::string(key));
            result = false;
        }
    }
    return result;
}

} // namespace server

