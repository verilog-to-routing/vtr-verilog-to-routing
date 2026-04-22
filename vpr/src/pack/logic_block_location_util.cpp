#include "logic_block_location_util.h"

#include <algorithm>
#include <cctype>
#include <functional>
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

std::vector<t_logical_location_token> parse_hierarchical_type_tokens(const t_pb_type& pb_type) {
    std::vector<t_logical_location_token> tokens;

    std::vector<const t_pb_type*> chain_leaf_to_root;
    for (const t_pb_type* curr = &pb_type; curr != nullptr;) {
        chain_leaf_to_root.push_back(curr);
        curr = (curr->parent_mode) ? curr->parent_mode->parent_pb_type : nullptr;
    }

    for (auto it = chain_leaf_to_root.rbegin(); it != chain_leaf_to_root.rend(); ++it) {
        t_logical_location_token token;
        token.name = (*it)->name ? (*it)->name : "";
        tokens.push_back(std::move(token));
    }

    // Recover parent mode selection from each child pb_type's parent_mode.
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        const t_pb_type* child_pb_type = *(chain_leaf_to_root.rbegin() + i + 1);
        if (child_pb_type->parent_mode && child_pb_type->parent_mode->name) {
            tokens[i].mode = std::string(child_pb_type->parent_mode->name);
        }
    }

    return tokens;
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

bool token_matches_name_and_mode(const t_logical_location_token& want, const t_logical_location_token& got) {
    if (want.name != got.name) {
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

static std::unordered_set<const t_pb_type*> collect_pattern_pb_types(const t_pack_patterns& pattern) {
    std::unordered_set<const t_pb_type*> pattern_blocks;
    if (!pattern.root_block) {
        return pattern_blocks;
    }

    std::queue<const t_pack_pattern_block*> block_queue;
    std::unordered_set<const t_pack_pattern_block*> visited_blocks;
    std::unordered_set<const t_pb_type*> seen_pb_types;
    block_queue.push(pattern.root_block);
    visited_blocks.insert(pattern.root_block);

    while (!block_queue.empty()) {
        const t_pack_pattern_block* block = block_queue.front();
        block_queue.pop();

        if (block->pb_type && seen_pb_types.insert(block->pb_type).second) {
            pattern_blocks.insert(block->pb_type);
        }

        for (auto conn = block->connections; conn != nullptr; conn = conn->next) {
            const t_pack_pattern_block* next = (conn->from_block == block) ? conn->to_block : conn->from_block;
            if (next && !visited_blocks.contains(next)) {
                visited_blocks.insert(next);
                block_queue.push(next);
            }
        }
    }

    return pattern_blocks;
}

int score_logical_block_location_pattern_match(const std::vector<t_logical_location_token>& location_tokens,
                                               AtomBlockId blk_id,
                                               const t_pack_patterns& pattern) {
    auto pattern_blocks = collect_pattern_pb_types(pattern);
    if (pattern_blocks.empty()) {
        return -1;
    }

    int best_score = -1;
    for (const t_pb_type* pb_type : pattern_blocks) {
        if (!primitive_type_feasible(blk_id, pb_type)) {
            continue;
        }

        auto pb_tokens = parse_hierarchical_type_tokens(*pb_type);
        if (pb_tokens.empty()) {
            continue;
        }

        if (location_tokens.size() > pb_tokens.size()) {
            continue;
        }

        // Token-by-token from the front, now matching name + optional index + optional mode.
        bool all_match = true;
        int score = 0;
        for (size_t i = 0; i < location_tokens.size(); ++i) {
            if (!token_matches(location_tokens[i], pb_tokens[i])) {
                all_match = false;
                break;
            }

            // Name is mandatory; explicit index/mode constraints increase specificity.
            score += 1; // name match
            if (location_tokens[i].index >= 0) {
                score += 1; // explicit index match
            }
            if (!location_tokens[i].mode.empty()) {
                score += 1; // explicit mode match
            }
        }

        if (all_match && score > best_score) {
            best_score = score;
        }
    }

    return best_score;
}
