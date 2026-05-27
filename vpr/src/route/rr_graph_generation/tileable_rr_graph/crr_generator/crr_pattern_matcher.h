#pragma once

/**
 * @file crr_pattern_matcher.h
 * @brief Helper class used by the CRR Generator to determine which switch block pattern
 * should be applied to each tile. It does so by finding the first matching pattern
 * for each location.
 */

#include <optional>
#include <string>

#include "vpr_error.h"
#include <string_view>
#include <regex>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace crrgenerator {

/**
 * @brief Helper class used by the CRR Generator to determine which switch block pattern
 * should be applied to each tile. It does so by finding the first matching pattern
 * for each location.
 */
class CRRPatternMatcher {
  public:
    // Pre-compile a pattern and store it in the cache. Call once per pattern at startup.
    void register_pattern(const std::string& pattern) {
        cache_.emplace(pattern, compile(pattern));
    }

    // Returns true if name matches pattern, using the pre-compiled cache.
    // The pattern must have been registered via register_pattern() beforehand;
    // an unregistered pattern raises a fatal error.
    bool matches_pattern(const std::string& name, const std::string& pattern) const {
        auto it = cache_.find(pattern);
        if (it == cache_.end()) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Pattern '%s' was not registered before use\n", pattern.c_str());
        }
        if (!it->second.has_value()) return name == pattern;
        return validate_regex_match(name, pattern, *it->second);
    }

  private:
    std::unordered_map<std::string, std::optional<std::regex>> cache_;

    static std::optional<std::regex> compile(const std::string& pattern) {
        if (pattern.find('*') == std::string::npos
            && pattern.find('[') == std::string::npos
            && pattern.find('\\') == std::string::npos) {
            return std::nullopt;
        }
        return std::regex(pattern_to_regex(pattern));
    }

    // Helper function to parse range [start:end:step] or comma-separated values [7,20]
    static bool matches_range(int value, const std::string& range_str) {
        size_t start_pos = range_str.find('[');
        size_t end_pos = range_str.find(']');
        if (start_pos == std::string::npos || end_pos == std::string::npos) {
            return false;
        }

        std::string range_content = range_str.substr(start_pos + 1, end_pos - start_pos - 1);

        if (range_content.find(',') != std::string::npos) {
            std::vector<int> values;
            size_t pos = 0;
            size_t comma_pos;
            while ((comma_pos = range_content.find(',', pos)) != std::string::npos) {
                values.push_back(std::stoi(range_content.substr(pos, comma_pos - pos)));
                pos = comma_pos + 1;
            }
            values.push_back(std::stoi(range_content.substr(pos)));
            return std::find(values.begin(), values.end(), value) != values.end();
        } else if (range_content.find(':') != std::string::npos) {
            std::vector<int> parts;
            size_t pos = 0;
            size_t colon_pos;
            while ((colon_pos = range_content.find(':', pos)) != std::string::npos) {
                parts.push_back(std::stoi(range_content.substr(pos, colon_pos - pos)));
                pos = colon_pos + 1;
            }
            parts.push_back(std::stoi(range_content.substr(pos)));
            if (parts.size() != 3) return false;
            int start = parts[0];
            int end = parts[1]; // end is NOT exclusive as per your specification
            int step = parts[2];
            if (step == 0) return false;
            if (value < start || value > end) return false;
            if ((value - start) % step != 0) return false;
            return true;
        } else {
            return value == std::stoi(range_content);
        }
    }

    // Convert pattern to regex, handling * and [start:end:step] ranges
    static std::string pattern_to_regex(const std::string& pattern) {
        std::string regex_pattern = "^";
        for (size_t i = 0; i < pattern.length(); ++i) {
            char c = pattern[i];
            if (c == '*') {
                regex_pattern += "([0-9]+)";
            } else if (c == '[') {
                size_t close_bracket = pattern.find(']', i);
                if (close_bracket != std::string::npos) {
                    regex_pattern += "([0-9]+)";
                    i = close_bracket;
                } else {
                    regex_pattern += "\\[";
                }
            } else if (c == '\\' && i + 1 < pattern.length() && pattern[i + 1] == '*') {
                regex_pattern += "\\*";
                ++i;
            } else {
                if (c == '.' || c == '^' || c == '$' || c == '+' || c == '?' || c == '(' || c == ')' || c == '|' || c == '{' || c == '}') {
                    regex_pattern += "\\";
                }
                regex_pattern += c;
            }
        }
        regex_pattern += "$";
        return regex_pattern;
    }

    static bool validate_regex_match(const std::string& name,
                                     const std::string& pattern,
                                     const std::regex& compiled_regex) {
        std::smatch matches;
        if (!std::regex_match(name, matches, compiled_regex)) {
            return false;
        }

        std::vector<std::string> captured_numbers;
        for (size_t i = 1; i < matches.size(); ++i) {
            captured_numbers.push_back(matches[i].str());
        }

        size_t capture_index = 0;
        for (size_t i = 0; i < pattern.length(); ++i) {
            char c = pattern[i];
            if (c == '*') {
                ++capture_index;
            } else if (c == '[') {
                size_t close_bracket = pattern.find(']', i);
                if (close_bracket != std::string::npos) {
                    std::string range = pattern.substr(i, close_bracket - i + 1);
                    if (capture_index < captured_numbers.size()) {
                        int captured_value = std::stoi(captured_numbers[capture_index]);
                        if (!matches_range(captured_value, range)) {
                            return false;
                        }
                    }
                    ++capture_index;
                    i = close_bracket;
                }
            } else if (c == '\\' && i + 1 < pattern.length() && pattern[i + 1] == '*') {
                ++i;
            }
        }

        return true;
    }
};

} // namespace crrgenerator
