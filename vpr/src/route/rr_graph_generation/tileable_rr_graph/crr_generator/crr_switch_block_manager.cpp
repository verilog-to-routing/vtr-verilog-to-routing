#include <regex>

#include <unordered_set>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "vpr_error.h"
#include "crr_switch_block_manager.h"

#include "vtr_assert.h"
#include "vtr_log.h"

namespace crrgenerator {

SwitchBlockManager::SwitchBlockManager(const std::string& sb_maps_file,
                                       const std::string& sb_templates_dir,
                                       const bool annotated_rr_graph,
                                       const int log_verbosity)
    : log_verbosity_(log_verbosity) {
    VTR_LOG("Initializing SwitchBlockManager with maps file: %s\n", sb_maps_file.c_str());

    // Load YAML configuration
    YAML::Node config = YAML::LoadFile(sb_maps_file);
    validate_yaml_structure(config);

    if (!config["SB_MAPS"]) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SB_MAPS section not found in YAML file\n");
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

        std::string full_path = (std::filesystem::path(sb_templates_dir) / sw_template_file).string();
        if (sw_template_file.empty()) {
            full_path = "";
        }

        // Handle escaped asterisks (replace \* with *)
        size_t escape_pos = 0;
        while ((escape_pos = pattern.find("\\*", escape_pos)) != std::string::npos) {
            pattern.erase(escape_pos, 1);
            ++escape_pos;
        }

        ordered_switch_block_patterns_.push_back(pattern);
        pattern_matcher_.register_pattern(pattern);
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

            if (annotated_rr_graph) {
                processor_.update_switch_delays(file_cache_[full_path],
                                                switch_delay_max_ps_,
                                                switch_delay_min_ps_);
            }
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Required switch template file not found: %s\n", full_path.c_str());
        }
    }

    VTR_LOGV_DEBUG(annotated_rr_graph, "Switch delay range across templates: min=%d ps, max=%d ps\n",
                   switch_delay_min_ps_,
                   switch_delay_max_ps_);

    // Map patterns to loaded DataFrames
    for (const auto& [pattern, full_path] : switch_block_to_file_) {
        if (file_cache_.find(full_path) != file_cache_.end()) {
            dataframes_[pattern] = &file_cache_[full_path];
        }
    }

    print_statistics();
}

const DataFrame* SwitchBlockManager::get_dataframe_by_index(size_t pattern_idx) const {
    VTR_ASSERT(pattern_idx < ordered_switch_block_patterns_.size());
    const std::string& pattern = ordered_switch_block_patterns_[pattern_idx];

    auto file_it = switch_block_to_file_.find(pattern);
    if (file_it == switch_block_to_file_.end() || file_it->second.empty()) {
        // The pattern is mapped to an empty/null entry in the SB_MAPS YAML:
        // switch blocks matching it have no connections.
        return nullptr;
    }

    auto df_it = dataframes_.find(pattern);
    if (df_it == dataframes_.end()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "No dataframe found for pattern '%s'\n", pattern.c_str());
    }
    return df_it->second;
}

int SwitchBlockManager::find_matching_pattern_index(size_t x, size_t y) const {
    for (size_t pattern_idx = 0; pattern_idx < ordered_switch_block_patterns_.size(); ++pattern_idx) {
        if (pattern_matcher_.matches(pattern_idx, x, y)) {
            return static_cast<int>(pattern_idx);
        }
    }
    return -1;
}

void SwitchBlockManager::print_statistics() const {
    VTR_LOG("=== CRR Generator Switch Block Manager Statistics ===\n");
    VTR_LOG("Patterns loaded: %zu\n", dataframes_.size());
    VTR_LOG("Unique switch template files: %zu\n", file_cache_.size());

    // Print file details
    for (const auto& [file, df] : file_cache_) {
        VTR_LOG("  %s: %zu connections (%zux%zu)\n",
                std::filesystem::path(file).filename().string().c_str(),
                df.connections, df.rows(), df.cols());
    }
}

void SwitchBlockManager::validate_yaml_structure(const YAML::Node& root) {
    if (!root.IsMap()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "YAML root must be a map\n");
    }

    if (!root["SB_MAPS"]) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Required 'SB_MAPS' section not found in YAML\n");
    }

    if (!root["SB_MAPS"].IsMap()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "'SB_MAPS' must be a map\n");
    }
}

} // namespace crrgenerator
