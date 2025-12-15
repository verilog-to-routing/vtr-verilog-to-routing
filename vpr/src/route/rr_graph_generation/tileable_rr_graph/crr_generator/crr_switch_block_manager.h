#pragma once

/**
 * @file crr_switch_block_manager.h
 * @brief Manages switch block configurations and switch template file processing
 *
 * This class handles reading YAML configuration files, processing switch template files
 * containing switch block data, and managing switch block patterns.
 */

#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "data_frame_processor.h"

namespace crrgenerator {

/**
 * @brief Manages switch block configurations and switch template file processing
 *
 * This class handles reading YAML configuration files, processing switch template files
 * containing switch block data, and managing switch block patterns.
 */
class SwitchBlockManager {
  public:
    SwitchBlockManager(int log_verbosity);

    /**
     * @brief Initialize the manager with configuration file
     * @param sb_maps_file Path to the YAML configuration file
     * @param sb_templates_dir Directory containing switch template files
     */
    void initialize(const std::string& sb_maps_file,
                    const std::string& sb_templates_dir);

    /**
     * @brief Get the switch template file name for a given pattern
     */
    std::string get_pattern_file_name(const std::string& pattern) const;

    /**
     * @brief Get DataFrame for a specific switch block pattern
     * @param pattern Switch block pattern name (e.g., "SB_1__2_")
     * @return Pointer to DataFrame or nullptr if not found
     */
    const DataFrame* get_switch_block_dataframe(const std::string& pattern) const;

    /**
     * @brief Check if a pattern exists in the switch block mapping
     * @param pattern Pattern to check
     * @return true if pattern exists
     */
    bool has_pattern(const std::string& pattern) const;

    /**
     * @brief Get all available patterns
     * @return Vector of all pattern names
     */
    std::vector<std::string> get_all_patterns() const;

    /**
     * @brief Find the first matching pattern for the switch block at the given location
     * @param x X coordinate of the switch block location
     * @param y Y coordinate of the switch block location
     * @return Matching pattern or empty string if no match
     */
    std::string find_matching_pattern(size_t x, size_t y) const;

    /**
     * @brief Print statistics about loaded switch blocks
     */
    void print_statistics() const;

    /**
     * @brief Get the total number of connections across all switch blocks
     * @return Total connection count
     */
    size_t get_total_connections() const;

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
     * @brief Verbosity level for logging messages
     */
    int log_verbosity_;

    // Validation
    void validate_yaml_structure(const YAML::Node& root);
};

} // namespace crrgenerator
