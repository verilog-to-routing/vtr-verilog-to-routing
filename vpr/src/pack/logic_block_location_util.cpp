#include "logic_block_location_util.h"

#include <cctype>
#include <utility>

#include "vpr_error.h"

/**
 * @brief Parse a non-negative integer string.
 *
 * In a constraint like `clb[0].fle[0]{n1_lut4}.ble4[0].ff[0]`, this helper
 * is called with extracted text values:
 * - value `0` (from `fle[0]`) returns 0
 * - value `n1_lut4` (non-digit) returns -1
 *
 * @param value Input string to parse.
 *
 * @return Parsed integer value, or -1 when the string is empty or contains
 *         non-digit characters.
 */
static int try_parse_int_impl(const std::string& value) {
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

/**
 * @brief Parse one logical location/hierarchical-type token.
 *
 * Depending on @p format, bracket fields are interpreted as either
 * logical-block index/mode constraints or hierarchical type annotations.
 *
 * Example full paths:
 * - `TokenFormat::LOGICAL_LOCATION`:
 *   `clb[0].fle[0]{n1_lut4}.ble4[0].ff[0]`
 * - `TokenFormat::HIERARCHICAL_TYPE`:
 *   `clb[0][default]/fle[3][n1_lut4]/ble4[0][default]/ff[0]`
 *   `[default]` is the implicit pb_mode name when a pb_type has no explicit `<mode>` in the
 *   architecture; user constraints omit `{...}` at such levels (wildcard), and only specify
 *   `{mode}` when pinning an explicit mode (e.g. `{n1_lut4}` on fle).
 *
 * @param token Input token without path separators.
 * @param format Token syntax interpretation mode.
 *
 * @return Parsed token fields (name/index/mode).
 */
static t_logical_location_token parse_single_token_impl(const std::string& token, LbHierPathParser::TokenFormat format) {
    t_logical_location_token parsed;
    size_t lbr = token.find('[');
    if (lbr == std::string::npos) {
        parsed.name = token;
        return parsed;
    }

    parsed.name = token.substr(0, lbr);
    size_t rbr = token.find(']', lbr + 1);
    if (rbr == std::string::npos) {
        if (format == LbHierPathParser::TokenFormat::LOGICAL_LOCATION) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Invalid logical_block_location token '%s': missing ']' after '['",
                            token.c_str());
        }
        return parsed;
    }

    if (format == LbHierPathParser::TokenFormat::LOGICAL_LOCATION) {
        const std::string bracket_content = token.substr(lbr + 1, rbr - lbr - 1);
        if (!bracket_content.empty()) {
            const int idx = try_parse_int_impl(bracket_content);
            if (idx < 0) {
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "Invalid logical_block_location token '%s': "
                                "expected non-negative integer in '[...]', got '%s'",
                                token.c_str(),
                                bracket_content.c_str());
            }
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
        int idx = try_parse_int_impl(first_bracket);
        if (idx >= 0) {
            parsed.index = idx;
        } else if (!first_bracket.empty()) {
            parsed.mode = first_bracket;
        }
        /*For LbHierPathParser::TokenFormat::HIERARCHICAL_TYPE, it uses [] to indicate the mode*/
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

/**
 * @brief Returns true if two tokens match.
 *
 * Missing index/mode in the expected token are treated as wildcards.
 */
static bool token_matches(const t_logical_location_token& want, const t_logical_location_token& got) {
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

/**
 * @brief Split an input path into non-empty tokens and parse each token.
 *
 * @param input Path-like string to split.
 * @param separator Delimiter used between path segments.
 * @param format Token syntax interpretation mode.
 *
 * @return Parsed token sequence preserving input order.
 */
static std::vector<t_logical_location_token> parse_segmented_tokens_impl(const std::string& input,
                                                                         char separator,
                                                                         LbHierPathParser::TokenFormat format) {
    std::vector<t_logical_location_token> tokens;
    size_t start = 0;
    while (start < input.size()) {
        size_t end = input.find(separator, start);
        std::string token = input.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!token.empty()) {
            auto parsed = parse_single_token_impl(token, format);
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

LbHierPathParser::LbHierPathParser(const std::string& logical_block_location)
    : logical_block_location_(logical_block_location) {}

int LbHierPathParser::parse() {
    want_tokens_ = parse_segmented_tokens_impl(logical_block_location_, '.', TokenFormat::LOGICAL_LOCATION);
    return static_cast<int>(want_tokens_.size());
}

bool LbHierPathParser::matches_hierarchical_type(const std::string& hierarchical_type_name) const {
    const auto got_tokens = parse_segmented_tokens_impl(hierarchical_type_name, '/', TokenFormat::HIERARCHICAL_TYPE);
    if (want_tokens_.empty() || got_tokens.empty() || want_tokens_.size() > got_tokens.size()) {
        return false;
    }

    for (size_t start = 0; start + want_tokens_.size() <= got_tokens.size(); ++start) {
        bool all_match = true;
        for (size_t i = 0; i < want_tokens_.size(); ++i) {
            if (!token_matches(want_tokens_[i], got_tokens[start + i])) {
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

const t_pb_graph_node* LbHierPathParser::find_matching_pb_graph_node(const t_pb_graph_node* root) const {
    if (root == nullptr) {
        return nullptr;
    }
    if (matches_hierarchical_type(root->hierarchical_type_name())) {
        return root;
    }

    if (root->is_primitive() || root->child_pb_graph_nodes == nullptr) {
        return nullptr;
    }

    for (int imode = 0; imode < root->pb_type->num_modes; ++imode) {
        const t_mode& mode = root->pb_type->modes[imode];
        for (int ipb_type = 0; ipb_type < mode.num_pb_type_children; ++ipb_type) {
            for (int ipb = 0; ipb < mode.pb_type_children[ipb_type].num_pb; ++ipb) {
                const t_pb_graph_node& child = root->child_pb_graph_nodes[imode][ipb_type][ipb];
                if (const t_pb_graph_node* match = find_matching_pb_graph_node(&child); match) {
                    return match;
                }
            }
        }
    }

    return nullptr;
}

void LbHierPathParser::validate_logical_block_types(const std::vector<t_logical_block_type>& logical_block_types) {
    if (logical_block_location_.empty()) {
        return;
    }

    if (parse() == 0) {
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Invalid logical_block_location '%s': no valid path tokens",
                        logical_block_location_.c_str());
    }

    for (const t_logical_block_type& lb_type : logical_block_types) {
        if (lb_type.is_empty() || lb_type.pb_graph_head == nullptr) {
            continue;
        }
        if (find_matching_pb_graph_node(lb_type.pb_graph_head) != nullptr) {
            return;
        }
    }

    VPR_FATAL_ERROR(VPR_ERROR_PACK,
                    "Invalid logical_block_location '%s': path does not match any logical block type pb graph",
                    logical_block_location_.c_str());
}
