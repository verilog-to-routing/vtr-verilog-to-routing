#pragma once
/**
 * @file
 * @author  William Zhang
 * @date    August 2026
 * @brief   Structural net-cohesion detection for the nonlinear Nesterov placer.
 *
 * The smooth wirelength objective lets two-pin nets between periphery-confined
 * blocks spread apart. Legalization then scatters those pairs, which downstream
 * packing and annealing cannot reliably repair. This module owns the detection
 * of that net class and the extra wirelength-weight multiplier that keeps its
 * endpoints together through the AP-to-APPack handoff; the placer applies the
 * multiplier inside its net-weight refresh.
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
 * then @ref update_periphery_pair_nets. Afterwards @ref net_multiplier yields
 * the cohesion weight factor for each net.
 */
class NetCohesion {
  public:
    NetCohesion(const APNetlist& ap_netlist,
                const FlatPlacementDensityManager& density_manager,
                size_t device_grid_width,
                size_t device_grid_height,
                size_t device_grid_num_layers,
                double periphery_pair_weight,
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
     *        resources, and compute their degree damping.
     */
    void update_periphery_pair_nets(const std::vector<PrimitiveVectorDim>& dimensions);

    /// @brief Number of flagged periphery-pair nets (also gates pack-pattern affinity).
    size_t num_periphery_pair_nets() const { return num_periphery_pair_nets_; }

    /**
     * @brief Cohesion weight multiplier for one net (1.0 when unflagged).
     */
    double net_multiplier(APNetId net_id) const {
        if (static_cast<size_t>(net_id) >= periphery_pair_nets_.size() || !periphery_pair_nets_[net_id])
            return 1.;
        // Degree-normalized cohesion. A block on many two-pin periphery nets
        // accumulates the multiplier once per net, so its total pull scales with
        // its degree; a high-fanout periphery structure therefore drags far
        // harder than the simple pad-to-pad pair this class is meant to hold
        // together. Scaling by a reference degree bounds each block's total
        // contribution, which damps high-degree blocks without having to
        // identify them.
        return 1. + (periphery_pair_weight_ - 1.) * periphery_pair_damping_[net_id];
    }

  private:
    const APNetlist& ap_netlist_;
    const FlatPlacementDensityManager& density_manager_;
    size_t device_grid_width_ = 0;
    size_t device_grid_height_ = 0;
    size_t device_grid_num_layers_ = 0;
    double periphery_pair_weight_ = 1.0;
    int log_verbosity_ = 0;

    std::vector<bool> boundary_confined_dims_;
    vtr::vector<APNetId, bool> periphery_pair_nets_;
    /// @brief Per-net degree damping in [0,1]; 1 keeps the full weight.
    vtr::vector<APNetId, double> periphery_pair_damping_;
    size_t num_periphery_pair_nets_ = 0;
};
