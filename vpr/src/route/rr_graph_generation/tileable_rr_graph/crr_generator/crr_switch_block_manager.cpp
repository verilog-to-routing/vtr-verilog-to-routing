#include <regex>

#include <unordered_set>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "crr_switch_block_manager.h"
#include "crr_pattern_matcher.h"

#include "vtr_log.h"

namespace crrgenerator {

static std::string get_switch_block_name(size_t x, size_t y) {
    return "SB_" + std::to_string(x) + "__" + std::to_string(y) + "_";
}

SwitchBlockManager::SwitchBlockManager(int log_verbosity)
    : log_verbosity_(log_verbosity) {}

void SwitchBlockManager::initialize(const std::string& sb_maps_file,
                                    const std::string& sb_templates_dir) {
    VTR_LOG("Initializing SwitchBlockManager with maps file: %s\n", sb_maps_file.c_str());

    // Load YAML configuration
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

        std::string sw_template_file = item.second.as<std::string>();
        if (item.second.IsNull()) {
            sw_template_file = "";
        }

        std::string full_path = std::filesystem::path(sb_templates_dir) / sw_template_file;
        if (sw_template_file.empty()) {
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
            VTR_LOGV(log_verbosity_ > 1, "Attempting to read switch template file: %s\n", full_path.c_str());
            DataFrame df = processor_.read_csv(full_path);
            processor_.process_dataframe(df,
                                         NUM_EMPTY_ROWS,
                                         NUM_EMPTY_COLS);
            file_cache_[full_path] = std::move(df);
            VTR_LOGV(log_verbosity_ > 1, "Processed %zu connections in %s file\n",
                     file_cache_[full_path].connections,
                     std::filesystem::path(full_path).filename().string().c_str());
        } else {
            VTR_LOG_ERROR("Required switch template file not found: %s\n", full_path.c_str());
        }
    }

    // Map patterns to loaded DataFrames
    for (const auto& [pattern, full_path] : switch_block_to_file_) {
        if (file_cache_.find(full_path) != file_cache_.end()) {
            dataframes_[pattern] = &file_cache_[full_path];
        }
    }

    print_statistics();
}

std::string SwitchBlockManager::get_pattern_file_name(const std::string& pattern) const {
    auto it = switch_block_to_file_.find(pattern);

    if (it == switch_block_to_file_.end()) {
        return "";
    } else {
        std::filesystem::path path(it->second);
        return path.filename().string();
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

std::string SwitchBlockManager::find_matching_pattern(size_t x, size_t y) const {
    std::string sw_name = get_switch_block_name(x, y);
    for (const std::string& pattern : ordered_switch_block_patterns_) {
        if (CRRPatternMatcher::matches_pattern(sw_name, pattern)) {
            return pattern;
        }
    }
    return "";
}

void SwitchBlockManager::print_statistics() const {
    VTR_LOG("=== CRR Generator Switch Block Manager Statistics ===\n");
    VTR_LOG("Patterns loaded: %zu\n", dataframes_.size());
    VTR_LOG("Unique switch template files: %zu\n", file_cache_.size());
    VTR_LOG("Total connections: %zu\n", get_total_connections());

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
