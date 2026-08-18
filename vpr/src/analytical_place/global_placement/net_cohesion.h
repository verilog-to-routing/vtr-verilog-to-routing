#pragma once
/**
 * @file
 * @author  William Zhang
 * @date    August 2026
 * @brief   Structural net-cohesion detection for the nonlinear Nesterov placer.
 *
 * The smooth wirelength objective lets certain structurally-rigid nets spread:
 * boundary-anchored nets and direct I/O-chain nets (pad/obuf/OCT/termination
 * and delay-chain periphery). Legalization then scatters those structures, which downstream
 * packing and annealing cannot reliably repair. This module owns the detection
 * of those net classes and the extra wirelength-weight multipliers that keep
 * them coherent through the AP-to-APPack handoff; the placer applies the
 * multipliers inside its net-weight refresh.
 */

#include <string>
#include <vector>
#include "ap_netlist.h"
#include "primitive_vector_fwd.h"
#include "vtr_vector.h"

class AtomNetlist;
class FlatPlacementDensityManager;
class LogicalModels;
struct PartialPlacement;

/**
 * @brief True for primitive-model names in the direct I/O-chain family
 *        (pads, buffers, OCT/termination, and I/O delay chains).
 */
bool model_name_is_io_chain(const std::string& model_name);

/**
 * @brief Detects and stores the placer's cohesion net classes.
 *
 * Lifecycle per placement run: construct, then call
 * @ref identify_boundary_confined_dims once the density dimensions are known,
 * and @ref update_boundary_net_flags after the warm-start seed exists.
 * Afterwards @ref net_multiplier yields the combined cohesion weight factor
 * for each net.
 */
class NetCohesion {
  public:
    NetCohesion(const APNetlist& ap_netlist,
                const AtomNetlist& atom_netlist,
                const LogicalModels& models,
                const FlatPlacementDensityManager& density_manager,
                size_t device_grid_width,
                size_t device_grid_height,
                size_t device_grid_num_layers,
                double boundary_net_weight,
                double io_chain_net_weight,
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
     * @brief Mark long boundary-related and direct I/O-chain nets for extra
     *        wirelength cohesion.
     */
    void update_boundary_net_flags(const std::vector<PrimitiveVectorDim>& dimensions,
                                   const PartialPlacement& seed);

    /// @brief Number of flagged direct I/O-chain nets (gates pack-pattern affinity).
    size_t num_io_chain_nets() const { return num_io_chain_cohesion_nets_; }

    /**
     * @brief Combined cohesion weight multiplier for one net (1.0 when unflagged).
     */
    double net_multiplier(APNetId net_id) const {
        double multiplier = 1.0;
        if (static_cast<size_t>(net_id) < boundary_cohesion_nets_.size() && boundary_cohesion_nets_[net_id])
            multiplier *= boundary_net_weight_;
        if (static_cast<size_t>(net_id) < io_chain_cohesion_nets_.size() && io_chain_cohesion_nets_[net_id])
            multiplier *= io_chain_net_weight_;
        return multiplier;
    }

  private:
    /**
     * @brief Return true if all atom primitives represented by the AP block are I/O-chain primitives.
     */
    bool block_is_io_chain_block_(APBlockId blk_id) const;

    const APNetlist& ap_netlist_;
    const AtomNetlist& atom_netlist_;
    const LogicalModels& models_;
    const FlatPlacementDensityManager& density_manager_;
    size_t device_grid_width_ = 0;
    size_t device_grid_height_ = 0;
    size_t device_grid_num_layers_ = 0;
    double boundary_net_weight_ = 1.0;
    double io_chain_net_weight_ = 2.0;
    int log_verbosity_ = 0;

    std::vector<bool> boundary_confined_dims_;
    vtr::vector<APNetId, bool> boundary_cohesion_nets_;
    vtr::vector<APNetId, bool> io_chain_cohesion_nets_;
    size_t num_io_chain_cohesion_nets_ = 0;
};
