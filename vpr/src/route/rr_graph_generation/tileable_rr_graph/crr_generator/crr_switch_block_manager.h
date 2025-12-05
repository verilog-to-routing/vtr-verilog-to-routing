#pragma once

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
    SwitchBlockManager();

    /**
     * @brief Initialize the manager with configuration file
     * @param sb_maps_file Path to the YAML configuration file
     * @param sb_annotated_dir Directory containing switch template files
     * @param is_annotated Whether the switches are annotated in switch template files
     */
    void initialize(const std::string& sb_maps_file,
                    const std::string& sb_annotated_dir);

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
     * @brief Find the first matching pattern in switch mapping file
     * for a switch block name
     * @param sw_name Switch block name to match
     * @return Matching pattern or empty string if no match
     */
    std::string find_matching_pattern(const std::string& sw_name) const;

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
    std::unordered_map<std::string, std::string> switch_block_to_file_;
    std::unordered_map<std::string, DataFrame*> dataframes_;
    std::unordered_map<std::string, DataFrame> file_cache_;

    DataFrameProcessor processor_;
    std::string annotated_dir_;

    // Validation
    void validate_yaml_structure(const YAML::Node& root);
};

} // namespace crrgenerator
