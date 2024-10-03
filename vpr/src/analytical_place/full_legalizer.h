/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Defines the FullLegalizer class which takes a partial AP placement
 *          and generates a fully legal clustering and placement which can be
 *          routed by VTR.
 */

#pragma once

#include <vector>

// Forward declarations
class APNetlist;
class AtomNetlist;
class ClusteredNetlist;
class DeviceGrid;
class PartialPlacement;
class Prepacker;
struct t_arch;
struct t_lb_type_rr_node;
struct t_logical_block_type;
struct t_model;
struct t_packer_opts;
struct t_vpr_setup;

/**
 * @brief The full legalizer in an AP flow
 *
 * Given a valid partial placement (of any level of legality), will produce a
 * fully legal clustering and placement for use in the rest of the VTR flow.
 */
class FullLegalizer {
public:
    /**
     * @brief Constructor of the Full Legalizer class.
     *
     * Brings in all the necessary state here. This is the state needed from the
     * AP Context. the Packer Context, and the Placer Context.
     */
    FullLegalizer(const APNetlist& ap_netlist,
                  t_vpr_setup& vpr_setup,
                  const DeviceGrid& device_grid,
                  const t_arch* arch,
                  const AtomNetlist& atom_netlist,
                  const Prepacker& prepacker,
                  const std::vector<t_logical_block_type>& logical_block_types,
                  std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                  const t_model* user_models,
                  const t_model* library_models,
                  const t_packer_opts& packer_opts)
            : ap_netlist_(ap_netlist),
              vpr_setup_(vpr_setup),
              device_grid_(device_grid),
              arch_(arch),
              atom_netlist_(atom_netlist),
              prepacker_(prepacker),
              logical_block_types_(logical_block_types),
              lb_type_rr_graphs_(lb_type_rr_graphs),
              user_models_(user_models),
              library_models_(library_models),
              packer_opts_(packer_opts) {}

    /**
     * @brief Perform legalization on the given partial placement solution
     *
     *  @param p_placement  A valid partial placement (passes verify method).
     *                      This implies that all blocks are placed on the
     *                      device grid and fixed blocks are observed.
     */
    void legalize(const PartialPlacement& p_placement);

private:
    /**
     * @brief Helper method to create the clusters from the given partial
     *        placement.
     * TODO: Should return a ClusteredNetlist object, but need to wait until
     *       it is separated from load_cluster_constraints.
     */
    void create_clusters(const PartialPlacement& p_placement);

    /**
     * @brief Helper method to place the clusters based on the given partial
     *        placement.
     */
    void place_clusters(const ClusteredNetlist& clb_nlist,
                        const PartialPlacement& p_placement);

    // AP Context Info
    const APNetlist& ap_netlist_;
    // Overall Setup Info
    // FIXME: I do not like bringing all of this in. Perhaps clean up the methods
    //        that use it.
    t_vpr_setup& vpr_setup_;
    // Device Context Info
    const DeviceGrid& device_grid_;
    const t_arch* arch_;
    // Packing Context Info
    const AtomNetlist& atom_netlist_;
    const Prepacker& prepacker_;
    const std::vector<t_logical_block_type>& logical_block_types_;
    std::vector<t_lb_type_rr_node>* lb_type_rr_graphs_;
    const t_model* user_models_;
    const t_model* library_models_;
    const t_packer_opts& packer_opts_;
    // Placement Context Info
    // TODO: Populate this once the placer is cleaned up some.
};

