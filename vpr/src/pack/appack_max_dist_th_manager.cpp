/**
 * @file
 * @author  Alex Singer
 * @date    May 2025
 * @breif   Definition of the max distance threshold manager class.
 */

#include "appack_max_dist_th_manager.h"
#include <regex>
#include <stdexcept>
#include <tuple>
#include <vector>
#include "device_grid.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_log.h"

/**
 * @brief Helper method to convert a string into a float with error checking.
 */
static float str_to_float_or_error(const std::string& str);

/**
 * @brief Helper method to parse one term of the user-provided max distance
 *        threshold string.
 *
 * This method decomposes the user string of the form "<regex>:<scale>,<offset>"
 * into its three components.
 */
static std::tuple<std::string, float, float> parse_max_dist_th(const std::string& max_dist_th);

/**
 * @brief Recursive helper method to deduce if the given pb_type is or contains
 *        pb_types which are of the memory class.
 *
 * TODO: This should be a graph traversal instead of a recursive function.
 */
static bool has_memory_pbs(const t_pb_type* pb_type);

void APPackMaxDistThManager::init(const std::vector<std::string>& max_dist_ths,
                                  const std::vector<t_logical_block_type>& logical_block_types,
                                  const DeviceGrid& device_grid) {
    // Automatically set the max distance thresholds.
    auto_set_max_distance_thresholds(logical_block_types, device_grid);

    // If the max distance threshold strings have been set (they are not set to
    // auto), set the max distance thresholds based on the user-provided strings.
    VTR_ASSERT(!max_dist_ths.empty());
    if (max_dist_ths.size() != 1 || max_dist_ths[0] != "auto") {
        set_max_distance_thresholds_from_strings(max_dist_ths, logical_block_types, device_grid);
    }

    // Set the initilized flag to true.
    is_initialized_ = true;

    // Log the max distance thresholds for each logical block type. This is
    // similar to how the input and output pin utilizations are printed.
    VTR_LOG("APPack is using max distance thresholds: ");
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        if (lb_ty.is_empty())
            continue;
        VTR_LOG("%s:%g ",
                lb_ty.name.c_str(),
                get_max_dist_threshold(lb_ty));
    }
    VTR_LOG("\n");
}

void APPackMaxDistThManager::auto_set_max_distance_thresholds(const std::vector<t_logical_block_type>& logical_block_types,
                                                              const DeviceGrid& device_grid) {
    // Compute the max device distance based on the width and height of the
    // device. This is the L1 (manhattan) distance.
    float max_device_distance = device_grid.width() + device_grid.height();

    // Compute the max distance thresholds of the different logical block types.
    float default_max_distance_th = std::max(default_max_dist_th_scale_ * max_device_distance,
                                             default_max_dist_th_offset_);
    float logic_block_max_distance_th = std::max(logic_block_max_dist_th_scale_ * max_device_distance,
                                                 logic_block_max_dist_th_offset_);
    float memory_max_distance_th = std::max(memory_max_dist_th_scale_ * max_device_distance,
                                            memory_max_dist_th_offset_);
    float io_block_max_distance_th = std::max(io_max_dist_th_scale_ * max_device_distance,
                                              io_max_dist_th_offset_);

    // Set all logical block types to have the default max distance threshold.
    logical_block_dist_thresholds_.resize(logical_block_types.size(), default_max_distance_th);

    // Find which (if any) of the logical block types most looks like a CLB block.
    t_logical_block_type_ptr logic_block_type = infer_logic_block_type(device_grid);

    // Go through each of the logical block types.
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        // Skip the empty logical block type. This should not have any blocks.
        if (lb_ty.is_empty())
            continue;

        // Find which type(s) this logical block type looks like.
        bool has_memory = has_memory_pbs(lb_ty.pb_type);
        bool is_logic_block_type = (lb_ty.index == logic_block_type->index);
        bool is_io_block = is_io_type(pick_physical_type(&lb_ty));

        // Update the max distance threshold based on the type. If the logical
        // block type looks like many block types at the same time (for example
        // a CLB which has memory slices within it), then take the average
        // of the max distance thresholds of those types.
        float max_distance_th_sum = 0.0f;
        unsigned block_category_count = 0;
        if (is_logic_block_type) {
            max_distance_th_sum += logic_block_max_distance_th;
            block_category_count++;
        }
        if (has_memory) {
            max_distance_th_sum += memory_max_distance_th;
            block_category_count++;
        }
        if (is_io_block) {
            max_distance_th_sum += io_block_max_distance_th;
            block_category_count++;
        }
        if (block_category_count > 0) {
            logical_block_dist_thresholds_[lb_ty.index] = max_distance_th_sum / static_cast<float>(block_category_count);
        }
    }
}

static bool has_memory_pbs(const t_pb_type* pb_type) {
    if (pb_type == nullptr)
        return false;

    // Check if this pb_type is a memory class. If so return true. This acts as
    // a base case for the recursion.
    if (pb_type->class_type == e_pb_type_class::MEMORY_CLASS)
        return true;

    // Go through all modes of this pb_type and check if any of those modes'
    // children have memory pb_types, if so return true.
    for (int mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
        const t_mode& mode = pb_type->modes[mode_idx];
        for (int child_idx = 0; child_idx < mode.num_pb_type_children; child_idx++) {
            if (has_memory_pbs(&mode.pb_type_children[child_idx]))
                return true;
        }
    }

    // If this pb_type is not a memory and its modes do not have memory pbs in
    // them, then this pb_type is not a memory.
    return false;
}

void APPackMaxDistThManager::set_max_distance_thresholds_from_strings(
    const std::vector<std::string>& max_dist_ths,
    const std::vector<t_logical_block_type>& logical_block_types,
    const DeviceGrid& device_grid) {

    // Go through each of the user-provided strings.
    for (const std::string& max_dist_th : max_dist_ths) {
        // If any of them are the word "auto", this was a user error and should
        // be flagged.
        // TODO: Maybe move this and other semantic checks up to the checker of
        //       VPR's command line.
        if (max_dist_th == "auto") {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "APPack: Cannot provide both auto and other max distance threshold strings");
        }

        // Parse the string for the regex, scale, and offset.
        std::string logical_block_regex_str;
        float logical_block_max_dist_th_scale;
        float logical_block_max_dist_th_offset;
        std::tie(logical_block_regex_str,
                 logical_block_max_dist_th_scale,
                 logical_block_max_dist_th_offset) = parse_max_dist_th(max_dist_th);

        // Setup the regex for the logical blocks the user wants to set the
        // thresholds for.
        std::regex logical_block_regex(logical_block_regex_str);

        // Compute the max distance threshold the user selected.
        float max_device_distance = device_grid.width() + device_grid.height();
        float logical_block_max_dist_th = std::max(max_device_distance * logical_block_max_dist_th_scale,
                                                   logical_block_max_dist_th_offset);

        // Search through all logical blocks and set the thresholds of any matches
        // to the threshold the user selected.
        bool found_match = false;
        for (const t_logical_block_type& lb_ty : logical_block_types) {
            bool is_match = std::regex_match(lb_ty.name, logical_block_regex);
            if (!is_match)
                continue;

            logical_block_dist_thresholds_[lb_ty.index] = logical_block_max_dist_th;
            found_match = true;
        }
        // If no match is found, send a warning to the user.
        if (!found_match) {
            VTR_LOG_WARN("Unable to find logical block type for max distance threshold regex string: %s\n",
                         logical_block_regex_str.c_str());
        }
    }
}

static std::tuple<std::string, float, float> parse_max_dist_th(const std::string& max_dist_th) {
    // Verify the format of the string. It must have one and only one colon.
    unsigned colon_count = 0;
    for (char c : max_dist_th) {
        if (c == ':')
            colon_count++;
    }
    if (colon_count != 1) {
        VTR_LOG_ERROR("Invalid max distance threshold string: %s\n",
                      max_dist_th.c_str());
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Error when parsing APPack max distance threshold string");
    }

    // Split the string along the colon.
    auto del_pos = max_dist_th.find(':');
    std::string logical_block_regex_str = max_dist_th.substr(0, del_pos);
    std::string lb_max_dist_th_str = max_dist_th.substr(del_pos + 1, std::string::npos);

    // Split along the comma for the scale/offset.
    // Verify that the comma only appears once in the scale/offset string.
    unsigned comma_count = 0;
    for (char c : lb_max_dist_th_str) {
        if (c == ',')
            comma_count++;
    }
    if (comma_count != 1) {
        VTR_LOG_ERROR("Invalid max distance threshold string: %s\n",
                      max_dist_th.c_str());
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Error when parsing APPack max distance threshold string");
    }

    // Split the string along the comma.
    auto comma_pos = lb_max_dist_th_str.find(',');
    std::string lb_max_dist_th_scale_str = lb_max_dist_th_str.substr(0, comma_pos);
    std::string lb_max_dist_th_offset_str = lb_max_dist_th_str.substr(comma_pos + 1, std::string::npos);

    // Convert the scale and offset into floats (error checking to be safe).
    float lb_max_dist_th_scale = str_to_float_or_error(lb_max_dist_th_scale_str);
    float lb_max_dist_th_offset = str_to_float_or_error(lb_max_dist_th_offset_str);

    // Return the results as a tuple.
    return std::make_tuple(logical_block_regex_str, lb_max_dist_th_scale, lb_max_dist_th_offset);
}

static float str_to_float_or_error(const std::string& str) {
    float val = -1;
    try {
        val = std::stof(str);
    } catch (const std::invalid_argument& e) {
        VTR_LOG_ERROR("Error while parsing max distance threshold value: %s\n"
                      "Failed with invalid argument: %s\n",
                      str.c_str(),
                      e.what());
    } catch (const std::out_of_range& e) {
        VTR_LOG_ERROR("Error while parsing max distance threshold value: %s\n"
                      "Failed with out of range: %s\n",
                      str.c_str(),
                      e.what());
    }
    if (val < 0.0f) {
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Error when parsing APPack max distance threshold string");
    }
    return val;
}
