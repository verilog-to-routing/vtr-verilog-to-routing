/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Defines the DetailedPlacer class which takes a fully legal clustering
 *          and placement and optimizes them while remaining legal.
 */

#pragma once

#include <memory>
#include "ap_flow_enums.h"
#include "clustered_netlist_utils.h"
#include "placer.h"
#include "vpr_utils.h"

/**
 * @brief The detailed placer in an AP flow.
 *
 * Given a fully legal clustering and clustered placement, will optimize the
 * solution while remaining fully legal (able to be used in the rest of the VPR
 * flow).
 */
class DetailedPlacer {
public:
    virtual ~DetailedPlacer() {}

    DetailedPlacer() = default;

    /**
     * @brief Optimize the current legal placement.
     */
    virtual void optimize_placement() = 0;
};

/**
 * @brief A factory method which creates a Detailed Placer of the given type.
 */
std::unique_ptr<DetailedPlacer> make_detailed_placer(
                        e_ap_detailed_placer detailed_placer_type,
                        const BlkLocRegistry& curr_clustered_placement,
                        const AtomNetlist& atom_netlist,
                        const ClusteredNetlist& clustered_netlist,
                        t_vpr_setup& vpr_setup,
                        const t_arch& arch);


/**
 * @brief The Identity Detailed Placer.
 *
 * This detailed placer does literally nothing to the legal placement. This
 * class is used as a placeholder to make the higher-level code easier to work
 * with.
 */
class IdentityDetailedPlacer : public DetailedPlacer {
public:
    using DetailedPlacer::DetailedPlacer;

    void optimize_placement() final {}
};

/**
 * @brief The Annealer Detailed Placer.
 *
 * This Detailed Placer passes the legal solution into the Annealer in the
 * VPR flow (uses the legal solution as the initial placement). This performs
 * the Simulated Annealing algorithm on the solution at the cluster level to
 * try and find a better clustered placement.
 *
 * This Detailed Placer reuses the options from the Placer stage of VPR for this
 * stage. So options passed to the Placer will be used in here.
 */
class AnnealerDetailedPlacer : public DetailedPlacer {
public:
    /**
     * @brief Construct the Annealer Detailed Placer class.
     *
     *  @param curr_clustered_placement
     *      The legalized placement solution to pass as the initial placement
     *      into the annealer.
     *  @param atom_netlist
     *      The netlist of atoms in the circuit.
     *  @param clustered_netlist
     *      The netlist of clusters created by the Full Legalizer.
     *  @param vpr_setup
     *      The setup variables, used to get the params from the user.
     *  @param arch
     *      The FPGA architecture to optimize onto.
     */
    AnnealerDetailedPlacer(const BlkLocRegistry& curr_clustered_placement,
                           const AtomNetlist& atom_netlist,
                           const ClusteredNetlist& clustered_netlist,
                           t_vpr_setup& vpr_setup,
                           const t_arch& arch);

    /**
     * @brief Run the annealer.
     */
    void optimize_placement() final;

private:
    /// @brief The placer class, which contains the annealer.
    std::unique_ptr<Placer> placer_;

    /// @brief A lookup between the block pin indices and pb graph pins.
    IntraLbPbPinLookup pb_gpin_lookup_;

    /// @brief A lookup between CLB pins and atom pins.
    ClusteredPinAtomPinsLookup netlist_pin_lookup_;
};

