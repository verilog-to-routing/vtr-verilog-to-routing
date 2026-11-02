#pragma once

#include <vector>

#include "atom_netlist_fwd.h"
#include "physical_types.h"

/**
 * @brief One hierarchy step parsed from a logical_block_location constraint string.
 *
 * LbHierPathParser splits the '.'-separated path into a vector of these tokens.
 * Each step is block_name[instance_index] with an optional {mode_name} suffix.
 *
 * Example: "clb[0].fle[2]{n1_lut4}.ble4[0].ff[0]" parses to:
 *   - {name: "clb",  index: 0, mode: ""}
 *   - {name: "fle",  index: 2, mode: "n1_lut4"}
 *   - {name: "ble4", index: 0, mode: ""}
 *   - {name: "ff",   index: 0, mode: ""}
 */
struct t_logical_location_token {
    std::string name; ///< pb_type name at this hierarchy level
    int index = -1;   ///< instance index from [...]; -1 if omitted
    std::string mode; ///< architecture mode from {...}; empty if omitted
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

    /**
     * @brief Syntax used when parsing a '.'- or '/'-separated hierarchy path into tokens.
     */
    enum class TokenFormat {
        LOGICAL_LOCATION,  ///< User constraint format: '.'-separated, index in [...], mode in {...}
                           ///< e.g. "clb[0].fle[2]{n1_lut4}.ble4[0].ff[0]"
        HIERARCHICAL_TYPE, ///< t_pb::hierarchical_type_name() format: '/'-separated, index and mode in [...]
                           ///< e.g. "clb[0][default]/fle[2][n1_lut4]/ble4[0][default]/ff[0]"
    };

  private:
    /**
     * @brief User constraint path before parsing (e.g. `clb[0].fle[0]{n1_lut4}.ble4[0].ff[0]`).
     */
    std::string logical_block_location_;
    /**
     * @brief Parsed hierarchy levels ("want" tokens) from parse(); all matching compares these to candidate pb paths.
     *        Empty until parse() is called.
     */
    std::vector<t_logical_location_token> want_tokens_;
};
