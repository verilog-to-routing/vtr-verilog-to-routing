#ifndef NO_SERVER

#include "telegramoptions.h"
#include "convertutils.h"

#include "vtr_util.h"

#include <optional>

namespace server {
    
TelegramOptions::TelegramOptions(const std::string& data, const std::vector<std::string>& expected_keys) {
    // parse data string
    std::vector<std::string> options = vtr::split(data, ";");
    for (const std::string& option_str: options) {
        std::vector<std::string> fragments = vtr::split(option_str, ":");
        if (fragments.size() == TOTAL_INDEXES_NUM) {
            std::string name{std::move(fragments[INDEX_NAME])};
            Option option{std::move(fragments[INDEX_TYPE]), std::move(fragments[INDEX_VALUE])};
            if (is_data_type_supported(option.type)) {
                m_options.emplace(name, std::move(option));
            } else {
                m_errors.emplace_back("bad type for option [" + option_str + "]");
            }
        } else {
            m_errors.emplace_back("bad option [" + option_str + "]");
        }
    }

    // check keys presence
    check_keys_presence(expected_keys);
}

std::map<std::size_t, std::set<std::size_t>> TelegramOptions::get_map_of_sets(const std::string& name) {
    std::map<std::size_t, std::set<std::size_t>> result;
    std::string data_str = get_string(name);
    if (!data_str.empty()) {
        std::vector<std::string> paths = vtr::split(data_str, "|");
        for (const std::string& path: paths) {
            std::vector<std::string> path_struct = vtr::split(path, "#");
            if (path_struct.size() == 2) {
                std::string path_index_str = path_struct[0];
                std::string path_element_indexes_str = path_struct[1];
                std::vector<std::string> path_element_indexes = vtr::split(path_element_indexes_str, ",");
                std::set<std::size_t> elements;
                for (const std::string& path_element_index_Str: path_element_indexes) {
                    if (std::optional<int> opt_value = try_convert_to_int(path_element_index_Str)) {
                        elements.insert(opt_value.value());
                    } else {
                        m_errors.emplace_back("cannot extract path element index from " + path_element_index_Str);
                    }
                }
                if (std::optional<int> opt_path_index = try_convert_to_int(path_index_str)) {
                    result[opt_path_index.value()] = elements;
                } else {
                    m_errors.emplace_back("cannot extract path index from " + path_index_str);
                }
            } else {
                m_errors.emplace_back("wrong path data structure = " + path);
            }
        }
    }
    return result;
}

std::string TelegramOptions::get_string(const std::string& name) {
    std::string result;
    if (auto it = m_options.find(name); it != m_options.end()) {
        result = it->second.value;
    }
    return result;
}

int TelegramOptions::get_int(const std::string& name, int fail_value) {
    if (std::optional<int> opt = try_convert_to_int(m_options[name].value)) {
        return opt.value();
    } else {
        m_errors.emplace_back("cannot get int value for option " + name);
        return fail_value;
    }
}

bool TelegramOptions::get_bool(const std::string& name, bool fail_value) {
    if (std::optional<int> opt = try_convert_to_int(m_options[name].value)) {
        return opt.value();
    } else {
        m_errors.emplace_back("cannot get bool value for option " + name);
        return fail_value;
    }
}

std::string TelegramOptions::errors_str() const {
    std::string result;
    for (const std::string& error: m_errors) {
        result += error + ';';
    }
    return result; 
}

bool TelegramOptions::is_data_type_supported(const std::string& type) const {
    constexpr std::array<std::string_view, 3> supported_types{"int", "string", "bool"};
    return std::find(supported_types.begin(), supported_types.end(), type) != supported_types.end();
}

bool TelegramOptions::check_keys_presence(const std::vector<std::string>& keys) {
    bool result = true;
    for (const std::string& key: keys) {
        if (m_options.find(key) == m_options.end()) {
            m_errors.emplace_back("cannot find required option " + std::string(key));
            result = false;
        }
    }
    return result;
}

} // namespace server

#endif /* NO_SERVER */
