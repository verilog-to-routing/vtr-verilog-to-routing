/**
 * @file
 * @author  Alex Singer
 * @date    April 2025
 * @brief   Manager class for pre-cluster (primitive-level) timing analysis.
 */

#pragma once

#include <memory>
#include <string>
#include "vpr_types.h"
#include "vtr_assert.h"

// Forward declarations.
class AtomLookup;
class AtomNetlist;
class PreClusterDelayCalculator;
class Prepacker;
class SetupTimingInfo;

/**
 * @brief Pre-cluster timing manager class.
 *
 * This class encapsulates the timing computations used prior to clustering.
 * This maintains all of the state necessary to perform these timing computations.
 */
class PreClusterTimingManager {
  public:
    /**
     * @brief Constructor for the manager class.
     *
     * If timing_driven is set to true, this constructor will perform a setup
     * timing analysis with a pre-clustered delay model. The delay model uses
     * the primitive delays specified in the architecture file and a simple
     * estimate of routing (a typical routing delay based on the wire delays
     * found in the architecture, and more specific delays for direct connections
     * like carry chains whose use we already know from the pre-packing).
     *
     *  @param timing_driven
     *          Whether this class should compute timing information or not. This
     *          may seem counter-intuitive, but this class still needs to exist
     *          even if timing is turned off. This will not initialize anything
     *          and set the valid flag to false if we are not timing driven.
     *  @param atom_netlist
     *          The primitive netlist to perform timing analysis over.
     *  @param atom_lookup
     *          A lookup between the primitives and their timing nodes.
     *  @param prepacker
     *          The prepacker object used to prepack primitives into molecules.
     *  @param timing_update_type
     *          The type of timing update this class should perform.
     *  @param arch
     *          The architecture.
     *  @param routing_arch
     *          The routing architecture.
     *  @param analysis opts
     *          Options for the timing analysis in VPR.
     */
    PreClusterTimingManager(bool timing_driven,
                            const AtomNetlist& atom_netlist,
                            const AtomLookup& atom_lookup,
                            const Prepacker& prepacker,
                            e_timing_update_type timing_update_type,
                            const t_arch& arch,
                            const t_det_routing_arch& routing_arch,
                            const std::string& device_layout,
                            const t_analysis_opts& analysis_opts);

    /**
     * @brief Calculates the setup criticality of the given primitive block.
     *
     * Currently defined as the maximum criticality over the block inputs.
     */
    float calc_atom_setup_criticality(AtomBlockId blk_id,
                                      const AtomNetlist& atom_netlist) const;

    /**
     * @brief Returns whether or not the pre-cluster timing manager was
     *        initialized (i.e. timing information can be computed).
     */
    bool is_valid() const {
        return is_valid_;
    }

    /**
     * @brief Get a reference to the setup timing info.
     */
    const SetupTimingInfo& get_timing_info() const {
        VTR_ASSERT_SAFE_MSG(is_valid_,
                            "Timing manager has not been initialized");
        return *timing_info_;
    }

  private:
    /// @brief A valid flag used to signify if the pre-cluster timing manager
    ///        class has been initialized or not. For example, if the flow is
    ///        not timing-driven, then this class will just be a shell which
    ///        should not have any timing information (but the object exists).
    bool is_valid_;

    /// @brief The delay calculator used for computing timing.
    std::shared_ptr<PreClusterDelayCalculator> clustering_delay_calc_;

    /// @brief The setup timing info used for getting the timing of edges
    ///        in the timing graph.
    std::shared_ptr<SetupTimingInfo> timing_info_;
};
