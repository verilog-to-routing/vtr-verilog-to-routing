#include <regex>

#include <unordered_set>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "crr_switch_block_manager.h"
#include "crr_pattern_matcher.h"

#include "vtr_log.h"

namespace crrgenerator {

SwitchBlockManager::SwitchBlockManager() = default;

void SwitchBlockManager::initialize(const std::string& sb_maps_file,
                                    const std::string& sb_annotated_dir,
                                    const bool is_annotated_excel) {
    VTR_LOG("Initializing SwitchBlockManager with maps file: %s\n", sb_maps_file.c_str());

    annotated_dir_ = sb_annotated_dir;

    // Load YAML configuration
    try {
        YAML::Node config = YAML::LoadFile(sb_maps_file);
        validate_yaml_structure(config);

        if (!config["SB_MAPS"]) {
            VTR_LOG_ERROR("SB_MAPS section not found in YAML file\n");
        }

        YAML::Node sb_maps = config["SB_MAPS"];
        VTR_LOG_DEBUG("Found SB_MAPS section with %zu entries\n", sb_maps.size());

        // Process each switch block mapping
        std::unordered_set<std::string> unique_files;
        for (const auto& item : sb_maps) {
            std::string pattern = item.first.as<std::string>();

            std::string excel_file = item.second.as<std::string>();
            if (item.second.IsNull()) {
                excel_file = "";
            }

            std::string full_path = std::filesystem::path(annotated_dir_) / excel_file;
            if (excel_file.empty()) {
                full_path = "";
            }

            // Handle escaped asterisks (replace \* with *)
            std::regex escaped_asterisk(R"(\\\*)");
            pattern = std::regex_replace(pattern, escaped_asterisk, "*");

            ordered_switch_block_patterns_.push_back(pattern);
            switch_block_to_file_[pattern] = full_path;
            if (!full_path.empty()) {
                unique_files.insert(full_path);
            }
        }

        for (const auto& full_path : unique_files) {
            if (std::filesystem::exists(full_path)) {
                try {
                    VTR_LOG_DEBUG("Attempting to read Excel file: %s\n", full_path.c_str());
                    DataFrame df = processor_.read_csv(full_path);
                    df = processor_.process_dataframe(std::move(df), NUM_EMPTY_ROWS,
                                                      NUM_EMPTY_COLS);
                    file_cache_[full_path] = std::move(df);
                    VTR_LOG_DEBUG("Processed %zu connections in %s file\n",
                                  file_cache_[full_path].connections,
                                  std::filesystem::path(full_path).filename().string().c_str());
                } catch (const std::exception& e) {
                    VTR_LOG_ERROR("Failed to read required Excel file '%s': %s\n", full_path.c_str(), e.what());
                }
            } else {
                VTR_LOG_ERROR("Required Excel file not found: %s\n", full_path.c_str());
            }
        }

        // Map patterns to loaded DataFrames
        for (const auto& [pattern, full_path] : switch_block_to_file_) {
            if (file_cache_.find(full_path) != file_cache_.end()) {
                dataframes_[pattern] = &file_cache_[full_path];
            }
        }

        update_and_set_global_switch_delays(is_annotated_excel);

        print_statistics();
    } catch (const YAML::Exception& e) {
        VTR_LOG_ERROR("Failed to parse YAML file %s: %s\n", sb_maps_file.c_str(), e.what());
    } catch (const std::exception& e) {
        VTR_LOG_ERROR("Failed to initialize SwitchBlockManager: %s\n", e.what());
    }
}

const DataFrame* SwitchBlockManager::get_switch_block_dataframe(const std::string& pattern) const {
    auto it = dataframes_.find(pattern);
    return (it != dataframes_.end()) ? it->second : nullptr;
}

bool SwitchBlockManager::has_pattern(const std::string& pattern) const {
    return dataframes_.find(pattern) != dataframes_.end();
}

std::vector<std::string> SwitchBlockManager::get_all_patterns() const {
    std::vector<std::string> patterns;
    patterns.reserve(dataframes_.size());

    for (const auto& [pattern, _] : dataframes_) {
        patterns.push_back(pattern);
    }

    return patterns;
}

std::string SwitchBlockManager::find_matching_pattern(const std::string& sw_name) const {
    for (const auto& pattern : ordered_switch_block_patterns_) {
        if (PatternMatcher::matches_pattern(sw_name, pattern)) {
            return pattern;
        }
    }
    return "";
}

void SwitchBlockManager::update_and_set_global_switch_delays(const bool is_annotated_excel) {
    if (is_annotated_excel) {
        int switch_delay_max_ps = std::numeric_limits<int>::min();
        int switch_delay_min_ps = std::numeric_limits<int>::max();
        for (const auto& [pattern, df] : dataframes_) {
            processor_.update_switch_delays(*df, switch_delay_max_ps,
                                            switch_delay_min_ps);
        }

        switch_delay_max_ps_ = switch_delay_max_ps;
        switch_delay_min_ps_ = switch_delay_min_ps;
    } else {
        switch_delay_max_ps_ = 1000;
        switch_delay_min_ps_ = 1;
    }

    VTR_LOG("Global switch delay range updated: %d - %d\n", switch_delay_min_ps_, switch_delay_max_ps_);
}

void SwitchBlockManager::print_statistics() const {
    VTR_LOG("=== CRR Generator Switch Block Manager Statistics ===\n");
    VTR_LOG("Patterns loaded: %zu\n", dataframes_.size());
    VTR_LOG("Unique Excel files: %zu\n", file_cache_.size());
    VTR_LOG("Total connections: %zu\n", get_total_connections());
    VTR_LOG("Switch delay range: %d - %d\n", switch_delay_min_ps_, switch_delay_max_ps_);

    // Print file details
    for (const auto& [file, df] : file_cache_) {
        VTR_LOG("  %s: %zu connections (%zux%zu)\n",
                std::filesystem::path(file).filename().string().c_str(),
                df.connections, df.rows(), df.cols());
    }
}

size_t SwitchBlockManager::get_total_connections() const {
    size_t total = 0;
    for (const auto& [file, df] : file_cache_) {
        total += df.connections;
    }
    return total;
}

void SwitchBlockManager::validate_yaml_structure(const YAML::Node& root) {
    if (!root.IsMap()) {
        VTR_LOG_ERROR("YAML root must be a map\n");
    }

    if (!root["SB_MAPS"]) {
        VTR_LOG_ERROR("Required 'SB_MAPS' section not found in YAML\n");
    }

    if (!root["SB_MAPS"].IsMap()) {
        VTR_LOG_ERROR("'SB_MAPS' must be a map\n");
    }
}

} // namespace crrgenerator
