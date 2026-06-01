#pragma once

#include <vector>

#include "atom_netlist_fwd.h"
#include "physical_types.h"

struct t_logical_location_token {
    std::string name;
    int index = -1;
    std::string mode;
};

/**
 * @brief Parses a logical_block_location constraint string and supports matching against
 *        hierarchical type names (e.g. t_pb::hierarchical_type_name()).
 *
 */
class LbHierPathParser {
  public:
    explicit LbHierPathParser(const std::string& logical_block_location);

    /**
     * @brief Parse the logical_block_location into internal tokens.
     *
     * @return Number of parsed tokens.
     */
    int parse();

    /**
     * @brief Returns true if the parsed constraint matches a hierarchical type name.
     */
    bool matches_hierarchical_type(const std::string& hierarchical_type_name) const;

    /**
     * @brief Search the pb graph hierarchy for the first node whose hierarchical_type_name()
     *        matches this constraint. Returns nullptr if no match is found.
     */
    const t_pb_graph_node* find_matching_pb_graph_node(const t_pb_graph_node* root) const;

    /**
     * @brief Validate that this logical_block_location matches at least one pb graph
     *        among the provided logical_block_types.
     */
    void validate_logical_block_types(const std::vector<t_logical_block_type>& logical_block_types);

    enum class TokenFormat {
        LOGICAL_LOCATION,
        HIERARCHICAL_TYPE,
    };

  private:
    std::string logical_block_location_;
    std::vector<t_logical_location_token> want_tokens_;
};
