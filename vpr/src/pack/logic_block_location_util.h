#pragma once

#include <optional>
#include <string>
#include <vector>

struct t_logical_location_token {
    std::string name;
    std::optional<int> index;
    std::optional<std::string> mode;
};

struct t_logical_block_location_fields {
    std::string mode_or_pattern_name;
    std::vector<t_logical_location_token> hierarchy_tokens;
};

t_logical_block_location_fields parse_logical_block_location_fields(const std::string& logical_block_location);

std::vector<t_logical_location_token> parse_logical_block_location_tokens(const std::string& location);
std::vector<t_logical_location_token> parse_hierarchical_type_tokens(const std::string& hierarchical_type);

bool token_matches(const t_logical_location_token& want, const t_logical_location_token& got);
bool logical_block_location_matches_hierarchical_type(const std::string& logical_block_location,
                                                      const std::string& hierarchical_type_name);
