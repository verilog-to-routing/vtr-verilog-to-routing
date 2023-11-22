#pragma once 

#include <sstream>
#include <iostream>
#include <set>
#include <vector>
#include <unordered_map>

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
    TelegramOptions(const std::string& data) {
        std::vector<std::string> options = splitString(data, ';');
        for (const std::string& optionStr: options) {
            std::vector<std::string> fragments = splitString(optionStr, ':');
            if (fragments.size() == TOTAL_INDEXES_NUM) {
                std::string name = fragments[INDEX_NAME];
                Option option{fragments[INDEX_TYPE], fragments[INDEX_VALUE]};
                if (validateType(option.type)) {
                    m_options[name] = option;
                } else {
                    m_errors.emplace_back("bad type for option [" + optionStr + "]");
                }
            } else {
                m_errors.emplace_back("bad option [" + optionStr + "]");
            }
        }
    }

    ~TelegramOptions() {}

    bool hasErrors() const { return !m_errors.empty(); }

    bool validateNamePresence(const std::vector<std::string>& names) {
        bool result = true;
        for (const std::string& name: names) {
            if (m_options.find(name) == m_options.end()) {
                m_errors.emplace_back("cannot find required option " + name);
                result = false;
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

    int getInt(const std::string& name, int defaultValue) {
        int result = defaultValue;
        try {
            result = std::atoi(m_options[name].value.c_str());
        } catch(...) {
            m_errors.emplace_back("cannot get int value for option " + name);
        }
        return result;
    }

    bool getBool(const std::string& name, bool defaultValue) {
        bool result = defaultValue;
        try {
            result = std::atoi(m_options[name].value.c_str());
        } catch(...) {
            m_errors.emplace_back("cannot get bool value for option " + name);
        }
        return result;
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

    bool validateType(const std::string& type) {
        static std::set<std::string> supportedTypes{"int", "string", "bool"};
        return supportedTypes.count(type) != 0;
    }
};
