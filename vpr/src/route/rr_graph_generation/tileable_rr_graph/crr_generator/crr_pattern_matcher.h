#include <string>
#include <string_view>
#include <regex>
#include <vector>
#include <algorithm>

namespace crrgenerator {

class PatternMatcher {
  private:
    // Helper function to parse range [start:end:step] or comma-separated values [7,20]
    static bool matches_range(int value, const std::string& range_str) {
        // Extract numbers from [start:end:step] or [7,20] format
        size_t start_pos = range_str.find('[');
        size_t end_pos = range_str.find(']');
        if (start_pos == std::string::npos || end_pos == std::string::npos) {
            return false;
        }

        std::string range_content = range_str.substr(start_pos + 1, end_pos - start_pos - 1);

        // Check if it's comma-separated values (e.g., [7,20])
        if (range_content.find(',') != std::string::npos) {
            // Parse comma-separated values
            std::vector<int> values;
            size_t pos = 0;
            size_t comma_pos;

            while ((comma_pos = range_content.find(',', pos)) != std::string::npos) {
                values.push_back(std::stoi(range_content.substr(pos, comma_pos - pos)));
                pos = comma_pos + 1;
            }
            values.push_back(std::stoi(range_content.substr(pos)));

            // Check if value is in the list
            return std::find(values.begin(), values.end(), value) != values.end();
        }
        // Check if it's range notation (e.g., [2:32:3])
        else if (range_content.find(':') != std::string::npos) {
            // Parse start:end:step
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

            if (value < start || value > end) return false;
            if ((value - start) % step != 0) return false;

            return true;
        }
        // Single value (e.g., [5])
        else {
            int single_value = std::stoi(range_content);
            return value == single_value;
        }
    }

    // Convert pattern to regex, handling * and [start:end:step] ranges
    static std::string pattern_to_regex(const std::string& pattern) {
        std::string regex_pattern = "^";

        for (size_t i = 0; i < pattern.length(); ++i) {
            char c = pattern[i];

            if (c == '*') {
                regex_pattern += "([0-9]+)"; // Capture group for numbers
            } else if (c == '[') {
                // Find the matching closing bracket
                size_t close_bracket = pattern.find(']', i);
                if (close_bracket != std::string::npos) {
                    std::string range = pattern.substr(i, close_bracket - i + 1);
                    regex_pattern += "([0-9]+)"; // Capture the number, validate range later
                    i = close_bracket;           // Skip to after the closing bracket
                } else {
                    regex_pattern += "\\["; // Literal bracket if no closing bracket found
                }
            } else if (c == '\\' && i + 1 < pattern.length() && pattern[i + 1] == '*') {
                // Handle escaped asterisk - treat as literal *
                regex_pattern += "\\*";
                ++i; // Skip the next character
            } else {
                // Escape regex special characters
                if (c == '.' || c == '^' || c == '$' || c == '+' || c == '?' || c == '(' || c == ')' || c == '|' || c == '{' || c == '}') {
                    regex_pattern += "\\";
                }
                regex_pattern += c;
            }
        }

        regex_pattern += "$";
        return regex_pattern;
    }

  public:
    static bool matches_pattern(const std::string& name, const std::string& pattern) {
        // Fast path for exact matches (no wildcards or ranges)
        if (pattern.find('*') == std::string::npos && pattern.find('[') == std::string::npos && pattern.find('\\') == std::string::npos) {
            return name == pattern;
        }

        // Compile regex for this pattern
        std::string regex_str = pattern_to_regex(pattern);
        std::regex compiled_regex(regex_str);

        std::smatch matches;
        if (!std::regex_match(name, matches, compiled_regex)) {
            return false;
        }

        // Now validate any range constraints
        std::vector<std::string> captured_numbers;
        for (size_t i = 1; i < matches.size(); ++i) {
            captured_numbers.push_back(matches[i].str());
        }

        // Parse pattern again to find ranges and validate them
        size_t capture_index = 0;
        for (size_t i = 0; i < pattern.length(); ++i) {
            char c = pattern[i];

            if (c == '*') {
                // Simple wildcard, no validation needed
                ++capture_index;
            } else if (c == '[') {
                // Find the matching closing bracket
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
                    i = close_bracket; // Skip to after the closing bracket
                }
            } else if (c == '\\' && i + 1 < pattern.length() && pattern[i + 1] == '*') {
                ++i; // Skip escaped asterisk
            }
        }

        return true;
    }
};

} // namespace crrgenerator
