#pragma once
/**
 * @file
 * @author  William Zhang
 * @date    August 2026
 * @brief   Structural net-cohesion detection for the nonlinear Nesterov placer.
 *
 * The smooth wirelength objective lets two-pin nets between periphery-confined
 * blocks spread apart. This module owns the detection of that net class; the
 * placer uses the detected count to gate its pack-pattern affinity springs.
 *
 * The class is derived entirely from the parsed architecture: a resource
 * dimension is "boundary-confined" when the grid gives it capacity only near
 * the device edge. No primitive, model, or block-type name is consulted, so the
 * detection transfers to any architecture whose periphery resources are laid
 * out that way, and simply selects nothing on architectures where they are not.
 */

#include <vector>
#include "ap_netlist.h"
#include "primitive_vector_fwd.h"
#include "vtr_vector.h"

class FlatPlacementDensityManager;

/**
 * @brief Detects and stores the placer's cohesion net class.
 *
 * Lifecycle per placement run: construct, call
 * @ref identify_boundary_confined_dims once the density dimensions are known,
 * then @ref update_periphery_pair_nets. @ref num_periphery_pair_nets then
 * reports how many nets were flagged.
 */
class NetCohesion {
  public:
    NetCohesion(const APNetlist& ap_netlist,
                const FlatPlacementDensityManager& density_manager,
                size_t device_grid_width,
                size_t device_grid_height,
                size_t device_grid_num_layers,
                int log_verbosity);

    /**
     * @brief Identify resource dimensions whose target capacity is almost
     *        entirely on the device boundary; stores the result.
     */
    void identify_boundary_confined_dims(const std::vector<PrimitiveVectorDim>& dimensions);

    /// @brief [dim index] true if target capacity lies almost entirely on the device boundary.
    const std::vector<bool>& boundary_confined_dims() const { return boundary_confined_dims_; }

    /**
     * @brief Return true if a block has any mass in boundary-confined dimensions.
     */
    bool block_has_boundary_mass(APBlockId blk_id,
                                 const std::vector<PrimitiveVectorDim>& dimensions) const;

    /**
     * @brief Flag the two-pin nets whose endpoints both sit on boundary-confined
     *        resources.
     */
    void update_periphery_pair_nets(const std::vector<PrimitiveVectorDim>& dimensions);

    /// @brief Number of flagged periphery-pair nets (also gates pack-pattern affinity).
    size_t num_periphery_pair_nets() const { return num_periphery_pair_nets_; }

  private:
    const APNetlist& ap_netlist_;
    const FlatPlacementDensityManager& density_manager_;
    size_t device_grid_width_ = 0;
    size_t device_grid_height_ = 0;
    size_t device_grid_num_layers_ = 0;
    int log_verbosity_ = 0;

    std::vector<bool> boundary_confined_dims_;
    vtr::vector<APNetId, bool> periphery_pair_nets_;
    size_t num_periphery_pair_nets_ = 0;
};
