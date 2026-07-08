#pragma once

/**
 * @file crr_switch_block_manager.h
 * @brief Manages switch block configurations and switch template file processing
 *
 * This class handles reading YAML configuration files, processing switch template files
 * containing switch block data, and managing switch block patterns.
 */

#include <limits>
#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "data_frame_processor.h"
#include "crr_pattern_matcher.h"

namespace crrgenerator {

/**
 * @brief Manages switch block configurations and switch template file processing
 *
 * This class handles reading YAML configuration files, processing switch template files
 * containing switch block data, and managing switch block patterns.
 */
class SwitchBlockManager {
  public:
    /**
     * @brief Constructor
     * @param sb_maps_file Path to the YAML configuration file
     * @param sb_templates_dir Directory containing switch template files
     * @param annotated_rr_graph When true, scan all loaded template files to
     *                           record the min/max switch delay (in ps).
     * @param log_verbosity Verbosity level for logging messages
     */
    SwitchBlockManager(const std::string& sb_maps_file,
                       const std::string& sb_templates_dir,
                       const bool annotated_rr_graph,
                       const int log_verbosity);

    /**
     * @brief Number of registered switch block patterns (in SB_MAPS order)
     */
    size_t num_patterns() const { return ordered_switch_block_patterns_.size(); }

    /**
     * @brief Get the DataFrame of the pattern with the given index
     * @param pattern_idx Pattern index (in SB_MAPS order)
     * @return The pattern's switch template DataFrame, or nullptr when the
     *         pattern has no template file (no connections)
     */
    const DataFrame* get_dataframe_by_index(size_t pattern_idx) const;

    /**
     * @brief Find the first matching pattern for the switch block at the given location
     * @param x X coordinate of the switch block location
     * @param y Y coordinate of the switch block location
     * @return Index of the matching pattern (in SB_MAPS order), or -1 if no match
     */
    int find_matching_pattern_index(size_t x, size_t y) const;

    /**
     * @brief Print statistics about loaded switch blocks
     */
    void print_statistics() const;

    /**
     * @brief Get the total number of connections across all switch blocks
     * @return Total connection count
     */
    size_t get_total_connections() const;

    /**
     * @brief Get the minimum switch delay (ps) observed across all loaded
     *        template files. Only valid when the manager was constructed
     *        with annotated_rr_graph = true.
     */
    int get_switch_delay_min_ps() const { return switch_delay_min_ps_; }

    /**
     * @brief Get the maximum switch delay (ps) observed across all loaded
     *        template files. Only valid when the manager was constructed
     *        with annotated_rr_graph = true.
     */
    int get_switch_delay_max_ps() const { return switch_delay_max_ps_; }

  private:
    /**
     * @brief Ordered list of switch block patterns
     *
     * We need to store the switch blocks in the same order defined in the YAML
     * file. Later, if a switch block match to multiple patterns defined in the
     * YAML file, the pattern defined earliest in the list will be used.
     */
    std::vector<std::string> ordered_switch_block_patterns_;

    /**
     * @brief Maps switch block patterns to their corresponding full file paths.
     */
    std::unordered_map<std::string, std::string> switch_block_to_file_;

    /**
     * @brief Maps switch block patterns to their corresponding dataframes.
     */
    std::unordered_map<std::string, DataFrame*> dataframes_;

    /**
     * @brief Maps full file paths to their corresponding dataframes.
     */
    std::unordered_map<std::string, DataFrame> file_cache_;

    /**
     * @brief Processor for reading and processing switch block template files.
     */
    DataFrameProcessor processor_;

    /**
     * @brief Pattern matcher with compiled regex cache.
     */
    CRRPatternMatcher pattern_matcher_;

    /**
     * @brief Verbosity level for logging messages
     */
    int log_verbosity_;

    /**
     * @brief Min/max switch delay (ps) accumulated across every loaded
     *        template file when annotated_rr_graph is enabled.
     *        Initialized to INT_MAX/INT_MIN so the first observed value
     *        always wins.
     */
    int switch_delay_min_ps_ = std::numeric_limits<int>::max();
    int switch_delay_max_ps_ = std::numeric_limits<int>::min();

    // Validation
    void validate_yaml_structure(const YAML::Node& root);
};

} // namespace crrgenerator
