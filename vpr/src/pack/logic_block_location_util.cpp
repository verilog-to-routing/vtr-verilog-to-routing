#include "logic_block_location_util.h"

#include <algorithm>
#include <cctype>
#include <queue>
#include <unordered_set>
#include <utility>

#include "pack_patterns.h"
#include "physical_types.h"
#include "vpr_utils.h"

enum class e_token_format {
    LOGICAL_LOCATION,
    HIERARCHICAL_TYPE,
};

static std::optional<int> try_parse_int(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    for (char ch : value) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return std::nullopt;
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
        parsed.index = try_parse_int(token.substr(lbr + 1, rbr - lbr - 1));

        size_t lbm = token.find('{', rbr + 1);
        if (lbm != std::string::npos) {
            size_t rbm = token.find('}', lbm + 1);
            if (rbm != std::string::npos && rbm > lbm + 1) {
                parsed.mode = token.substr(lbm + 1, rbm - lbm - 1);
            }
        }
    } else {
        std::string first_bracket = token.substr(lbr + 1, rbr - lbr - 1);
        auto idx = try_parse_int(first_bracket);
        if (idx) {
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
    if (want.index && (!got.index || *want.index != *got.index)) {
        return false;
    }
    if (want.mode && (!got.mode || *want.mode != *got.mode)) {
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

static std::unordered_set<t_pb_type*> collect_pattern_pb_types(const t_pack_patterns& pattern) {
    std::unordered_set<t_pb_type*> pattern_blocks;
    if (!pattern.root_block) {
        return pattern_blocks;
    }

    std::queue<t_pack_pattern_block*> block_queue;
    std::unordered_set<t_pack_pattern_block*> visited_blocks;
    block_queue.push(pattern.root_block);
    visited_blocks.insert(pattern.root_block);

    while (!block_queue.empty()) {
        t_pack_pattern_block* block = block_queue.front();
        block_queue.pop();

        if (block->pb_type) {
            pattern_blocks.insert(const_cast<t_pb_type*>(block->pb_type));
        }

        for (auto conn = block->connections; conn != nullptr; conn = conn->next) {
            t_pack_pattern_block* next = (conn->from_block == block) ? conn->to_block : conn->from_block;
            if (next && !visited_blocks.contains(next)) {
                visited_blocks.insert(next);
                block_queue.push(next);
            }
        }
    }

    return pattern_blocks;
}
static bool leaf_token_matches_relaxed(const t_logical_location_token& leaf_token,
                                       const t_logical_location_token& pb_token) {
    if (leaf_token.name != pb_token.name) {
        return false;
    }
    // Pattern tokens from pb_type names often omit index/mode. Treat those as wildcards.
    if (pb_token.index && leaf_token.index && *pb_token.index != *leaf_token.index) {
        return false;
    }
    if (pb_token.mode && leaf_token.mode && *pb_token.mode != *leaf_token.mode) {
        return false;
    }
    return true;
}

static bool pb_type_matches_location_mode_hint(const t_pb_type* pb_type,
                                               const std::vector<t_logical_location_token>& location_tokens) {
    if (!pb_type) {
        return false;
    }
    for (const auto& token : location_tokens) {
        if (!token.mode || token.mode->empty()) {
            continue;
        }
        for (const t_mode* mode = pb_type->parent_mode; mode != nullptr; mode = mode->parent_pb_type ? mode->parent_pb_type->parent_mode : nullptr) {
            if (*token.mode == mode->name) {
                return true;
            }
        }
    }
    return false;
}

int score_logical_block_location_pattern_match(const std::vector<t_logical_location_token>& location_tokens,
                                               AtomBlockId blk_id,
                                               const t_pack_patterns& pattern) {
    if (location_tokens.empty()) {
        return -1;
    }

    auto pattern_blocks = collect_pattern_pb_types(pattern);
    if (pattern_blocks.empty()) {
        return -1;
    }

    const auto& leaf_token = location_tokens.back();
    bool matches_atom = false;
    bool leaf_token_match = false;
    int best_leaf_specificity = 0;

    for (auto* pb_type : pattern_blocks) {
        if (primitive_type_feasible(blk_id, pb_type)) {
            matches_atom = true;
        }

        auto pb_tokens = parse_hierarchical_type_tokens(pb_type->name);
        if (pb_tokens.empty()) {
            t_logical_location_token name_only_token;
            name_only_token.name = pb_type->name;
            pb_tokens.push_back(std::move(name_only_token));
        }

        for (const auto& pb_token : pb_tokens) {
            if (leaf_token_matches_relaxed(leaf_token, pb_token)) {
                leaf_token_match = true;
                int specificity = 0;
                if (pb_token.index && leaf_token.index && *pb_token.index == *leaf_token.index) {
                    specificity += 1;
                }
                if (pb_type_matches_location_mode_hint(pb_type, location_tokens)) {
                    specificity += 1;
                }
                best_leaf_specificity = best_leaf_specificity + specificity;
            }
        }
    }

    // Require both architectural feasibility and a leaf primitive match.
    if (!matches_atom && !leaf_token_match) {
        return -1;
    }
    
    return best_leaf_specificity; // Keep minimal tie-break information.
}
