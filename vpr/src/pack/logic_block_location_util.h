#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "atom_netlist_fwd.h"

struct t_pb_type;
struct t_pack_patterns;

struct t_logical_location_token {
    std::string name;
    std::optional<int> index;
    std::optional<std::string> mode;
};

std::vector<t_logical_location_token> parse_logical_block_location_tokens(const std::string& location);
std::vector<t_logical_location_token> parse_hierarchical_type_tokens(const std::string& hierarchical_type);

bool token_matches(const t_logical_location_token& want, const t_logical_location_token& got);
bool logical_block_location_matches_hierarchical_type(const std::string& logical_block_location,
                                                      const std::string& hierarchical_type_name);

int score_logical_block_location_pattern_match(const std::vector<t_logical_location_token>& location_tokens,
                                               AtomBlockId blk_id,
                                               const t_pack_patterns& pattern);
