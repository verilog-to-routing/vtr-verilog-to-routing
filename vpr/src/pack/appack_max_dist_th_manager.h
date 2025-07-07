#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    May 2025
 * @brief   Declaration of a class that manages the max candidate distance
 *          thresholding optimization used within APPack.
 */

#include <string>
#include <vector>
#include "physical_types.h"
#include "vtr_assert.h"

// Forward declarations.
class DeviceGrid;

/**
 * @brief Manager class which manages parsing and getting the max candidate
 *        distance thresholds for each of the logical block types in the
 *        architecture.
 *
 * The initializer of this class will set the max candidate distance thresholds
 * based on the arguments passed by the user.
 *
 * Within the packer, the get_max_dist_threshold method can be used to get the
 * max distance threshold for a given logical block type.
 */
class APPackMaxDistThManager {
    // To compute the max distance threshold, we use two numbers to compute it:
    //  - Scale: This is what fraction of the device the distance should be.
    //           i.e. the max distance threshold will be scale * (W + H) since
    //           W+H is the farthes L1 distance possible on the device.
    //  - Offset: This is the minimum threshold it can have. This prevents small
    //            devices with small scales having thresholds that are too small.
    // The following scales and offsets are set for interesting logical blocks
    // when the "auto" selection mode is used. The following numbers were
    // empirically found to work well.

    // This is the default scale and offset. Logical blocks that we do not
    // recognize as being of the special categories will have this threshold.
    static constexpr float default_max_dist_th_scale_ = 0.1f;
    static constexpr float default_max_dist_th_offset_ = 10.0f;

    // Logic blocks (such as CLBs and LABs) tend to have more resources on the
    // device, thus they have tighter thresholds. This was found to work well.
    static constexpr float logic_block_max_dist_th_scale_ = 0.06f;
    static constexpr float logic_block_max_dist_th_offset_ = 15.0f;

    // Memory blocks (i.e. blocks that contain pb_types of the memory class)
    // seem to have very touchy packing; thus these do not have the max
    // threshold to prevent them from creating too many clusters.
    static constexpr float memory_max_dist_th_scale_ = 1.0f;
    static constexpr float memory_max_dist_th_offset_ = 0.0f;

    // IO blocks tend to have very sparse resources and setting the offset too
    // low can create too many blocks. Set this to a higher value.
    static constexpr float io_max_dist_th_scale_ = 0.5f;
    static constexpr float io_max_dist_th_offset_ = 15.0f;

  public:
    APPackMaxDistThManager() = default;

    /**
     * @brief Initializer for the manager class. The thresholds for each logical
     *        block type is selected here.
     *
     *  @param should_initialize
     *      Whether to compute the thresholds for each logical block or not. This
     *      is to allow the class to be passed around without AP being enabled.
     *  @param max_dist_ths
     *      An array of strings representing the user-defined max distance
     *      thresholds. This is passed from the command line.
     *  @param logical_block_types
     *      An array of all logical block types in the architecture.
     *  @param device_grid
     */
    void init(const std::vector<std::string>& max_dist_ths,
              const std::vector<t_logical_block_type>& logical_block_types,
              const DeviceGrid& device_grid);

    /**
     * @brief Get the max distance threshold of the given logical block type.
     */
    inline float get_max_dist_threshold(const t_logical_block_type& logical_block_ty) const {
        VTR_ASSERT_SAFE_MSG(is_initialized_,
                            "APPackMaxDistThManager has not been initialized, cannot call this method");
        VTR_ASSERT_SAFE_MSG((size_t)logical_block_ty.index < logical_block_dist_thresholds_.size(),
                            "Logical block type does not have a max distance threshold");

        return logical_block_dist_thresholds_[logical_block_ty.index];
    }

    /**
     * @brief Get the maximum distance possible on the device. This is the
     *        manhattan distance from the bottom-left corner of the device to
     *        the top-right.
     */
    inline float get_max_device_distance() const {
        VTR_ASSERT_SAFE_MSG(is_initialized_,
                            "APPackMaxDistThManager has not been initialized, cannot call this method");

        return max_distance_on_device_;
    }

    /**
     * @brief Set the max distance threshold of the given logical block type.
     */
    inline void set_max_dist_threshold(const t_logical_block_type& logical_block_ty,
                                       float new_threshold) {
        VTR_ASSERT_SAFE_MSG(is_initialized_,
                            "APPackMaxDistThManager has not been initialized, cannot call this method");
        VTR_ASSERT_SAFE_MSG((size_t)logical_block_ty.index < logical_block_dist_thresholds_.size(),
                            "Logical block type does not have a max distance threshold");

        logical_block_dist_thresholds_[logical_block_ty.index] = new_threshold;
    }

  private:
    /**
     * @brief Helper method that initializes the thresholds of all logical
     *        block types to reasonable numbers based on the characteristics
     *        of the logical block type.
     */
    void auto_set_max_distance_thresholds(const std::vector<t_logical_block_type>& logical_block_types,
                                          const DeviceGrid& device_grid);

    /**
     * @brief Helper method that sets the thresholds based on the user-provided
     *        strings.
     */
    void set_max_distance_thresholds_from_strings(const std::vector<std::string>& max_dist_ths,
                                                  const std::vector<t_logical_block_type>& logical_block_types);

    /// @brief A flag which shows if the thesholds have been computed or not.
    bool is_initialized_ = false;

    /// @brief The max distance thresholds of all logical blocks in the architecture.
    ///        This is initialized in the constructor and accessed during packing.
    std::vector<float> logical_block_dist_thresholds_;

    /// @brief This is the maximum manhattan distance possible on the device. This
    ///        is the distance of traveling from the bottom-left corner of the device
    ///        to the top right.
    float max_distance_on_device_;
};
