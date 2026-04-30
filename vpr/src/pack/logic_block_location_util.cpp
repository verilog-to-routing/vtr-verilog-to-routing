#include "logic_block_location_util.h"

#include <algorithm>
#include <cctype>
#include <utility>

#include "pack_patterns.h"
#include "physical_types.h"
#include "vpr_utils.h"

enum class e_token_format {
    LOGICAL_LOCATION,
    HIERARCHICAL_TYPE,
};

static int try_parse_int(const std::string& value) {
    if (value.empty()) {
        return -1;
    }
    for (char ch : value) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return -1;
        }
    }
    return std::stoi(value);
}

static t_logical_location_token parse_single_token(const std::string& token, e_token_format format) {
    t_logical_location_token parsed;
    size_t lbr = token.find('[');
    if (lbr == std::string::npos) {
        parsed.name = token;
        return parsed;
    }

    parsed.name = token.substr(0, lbr);
    size_t rbr = token.find(']', lbr + 1);
    if (rbr == std::string::npos) {
        return parsed;
    }

    if (format == e_token_format::LOGICAL_LOCATION) {
        int idx = try_parse_int(token.substr(lbr + 1, rbr - lbr - 1));
        if (idx >= 0) {
            parsed.index = idx;
        }

        size_t lbm = token.find('{', rbr + 1);
        if (lbm != std::string::npos) {
            size_t rbm = token.find('}', lbm + 1);
            if (rbm != std::string::npos && rbm > lbm + 1) {
                parsed.mode = token.substr(lbm + 1, rbm - lbm - 1);
            }
        }
    } else {
        std::string first_bracket = token.substr(lbr + 1, rbr - lbr - 1);
        int idx = try_parse_int(first_bracket);
        if (idx >= 0) {
            parsed.index = idx;
        } else if (!first_bracket.empty()) {
            parsed.mode = first_bracket;
        }

        size_t lbr2 = token.find('[', rbr + 1);
        if (lbr2 != std::string::npos) {
            size_t rbr2 = token.find(']', lbr2 + 1);
            if (rbr2 != std::string::npos && rbr2 > lbr2 + 1) {
                parsed.mode = token.substr(lbr2 + 1, rbr2 - lbr2 - 1);
            }
        }
    }

    return parsed;
}

static std::vector<t_logical_location_token> parse_segmented_tokens(const std::string& input,
                                                                    char separator,
                                                                    e_token_format format) {
    std::vector<t_logical_location_token> tokens;
    size_t start = 0;
    while (start < input.size()) {
        size_t end = input.find(separator, start);
        std::string token = input.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!token.empty()) {
            auto parsed = parse_single_token(token, format);
            if (!parsed.name.empty()) {
                tokens.push_back(std::move(parsed));
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return tokens;
}

std::vector<t_logical_location_token> parse_logical_block_location_tokens(const std::string& location) {
    return parse_segmented_tokens(location, '.', e_token_format::LOGICAL_LOCATION);
}

std::vector<t_logical_location_token> parse_hierarchical_type_tokens(const std::string& hierarchical_type) {
    return parse_segmented_tokens(hierarchical_type, '/', e_token_format::HIERARCHICAL_TYPE);
}

bool token_matches(const t_logical_location_token& want, const t_logical_location_token& got) {
    if (want.name != got.name) {
        return false;
    }
    if (want.index >= 0 && got.index != want.index) {
        return false;
    }
    if (!want.mode.empty() && got.mode != want.mode) {
        return false;
    }
    return true;
}

bool logical_block_location_matches_hierarchical_type(const std::string& logical_block_location,
                                                      const std::string& hierarchical_type_name) {
    auto want_tokens = parse_logical_block_location_tokens(logical_block_location);
    auto got_tokens = parse_hierarchical_type_tokens(hierarchical_type_name);
    if (want_tokens.empty() || got_tokens.empty() || want_tokens.size() > got_tokens.size()) {
        return false;
    }

    for (size_t start = 0; start + want_tokens.size() <= got_tokens.size(); ++start) {
        bool all_match = true;
        for (size_t i = 0; i < want_tokens.size(); ++i) {
            if (!token_matches(want_tokens[i], got_tokens[start + i])) {
                all_match = false;
                break;
            }
        }
        if (all_match) {
            return true;
        }
    }
    return false;
}
