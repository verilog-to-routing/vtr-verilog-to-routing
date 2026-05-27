#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "atom_netlist_fwd.h"
#include "physical_types.h"

struct t_logical_location_token {
    std::string name;
    int index = -1;
    std::string mode;
};

std::vector<t_logical_location_token> parse_logical_block_location_tokens(const std::string& location);
std::vector<t_logical_location_token> parse_hierarchical_type_tokens(const std::string& hierarchical_type);

bool token_matches(const t_logical_location_token& want, const t_logical_location_token& got);
bool logical_block_location_matches_hierarchical_type(const std::string& logical_block_location,
                                                      const std::string& hierarchical_type_name);

/**
 * @brief Parse and verify logical_block_location against the architecture pb graphs.
 *
 * Walks each t_logical_block_type pb graph and compares against
 * t_pb_graph_node::hierarchical_type_name() using logical_block_location_matches_hierarchical_type().
 * Fatal on invalid input.
 */
void validate_logical_block_location(const std::string& logical_block_location,
                                     const std::vector<t_logical_block_type>& logical_block_types);

