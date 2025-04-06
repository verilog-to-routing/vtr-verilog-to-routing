/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Defines the FullLegalizer class which takes a partial AP placement
 *          and generates a fully legal clustering and placement which can be
 *          routed by VTR.
 */

#pragma once

#include <memory>
#include "ap_flow_enums.h"
#include "vtr_ndmatrix.h"
#include "cluster_legalizer.h"

// Forward declarations
class APNetlist;
class AtomNetlist;
class ClusteredNetlist;
class DeviceGrid;
class PartialPlacement;
class PlaceMacros;
class Prepacker;
struct t_arch;
struct t_vpr_setup;
struct t_pl_loc;
class APBlockId;

/**
 * @brief The full legalizer in an AP flow
 *
 * Given a valid partial placement (of any level of legality), will produce a
 * fully legal clustering and clustered placement for use in the rest of the
 * VTR flow.
 */
class FullLegalizer {
  public:
    virtual ~FullLegalizer() {}

    FullLegalizer(const APNetlist& ap_netlist,
                  const AtomNetlist& atom_netlist,
                  const Prepacker& prepacker,
                  t_vpr_setup& vpr_setup,
                  const t_arch& arch,
                  const DeviceGrid& device_grid)
        : ap_netlist_(ap_netlist)
        , atom_netlist_(atom_netlist)
        , prepacker_(prepacker)
        , vpr_setup_(vpr_setup)
        , arch_(arch)
        , device_grid_(device_grid) {}

    /**
     * @brief Perform legalization on the given partial placement solution
     *
     *  @param p_placement  A valid partial placement (passes verify method).
     *                      This implies that all blocks are placed on the
     *                      device grid and fixed blocks are observed.
     */
    virtual void legalize(const PartialPlacement& p_placement) = 0;

  protected:
    /// @brief The AP Netlist to fully legalize the flat placement of.
    const APNetlist& ap_netlist_;

    /// @brief The Atom Netlist used to generate the AP Netlist.
    const AtomNetlist& atom_netlist_;

    /// @brief The Prepacker used to create molecules from the Atom Netlist.
    const Prepacker& prepacker_;

    /// @brief The VPR setup options passed into the VPR flow. This must be
    ///        mutable since some parts of packing modify the options.
    t_vpr_setup& vpr_setup_;

    /// @brief Information on the architecture of the FPGA.
    const t_arch& arch_;

    /// @brief The device grid which records where clusters can be placed.
    const DeviceGrid& device_grid_;
};

class ClusterGridReconstruction {
public:
    ClusterGridReconstruction() = default;  // <-- ADD THIS ✅
    ClusterGridReconstruction(size_t num_layers, size_t width, size_t height)
        : grid_(num_layers, std::vector<std::vector<std::vector<LegalizationClusterId>>>(
                               width, std::vector<std::vector<LegalizationClusterId>>(
                                          height))) {}

    void initialize_tile(int layer, int x, int y, int num_subtiles) {
        grid_[layer][x][y].resize(num_subtiles, LegalizationClusterId::INVALID());
    }

    LegalizationClusterId get(int layer, int x, int y, int subtile) const {
        if (in_bounds(layer, x, y) && subtile < grid_[layer][x][y].size()) {
            return grid_[layer][x][y][subtile];
        }
        return LegalizationClusterId::INVALID();
    }

    void set(int layer, int x, int y, int subtile, LegalizationClusterId id) {
        if (in_bounds(layer, x, y) && subtile < grid_[layer][x][y].size()) {
            grid_[layer][x][y][subtile] = id;
        }
    }

    void clear() {
        for (auto& layer : grid_) {
            for (auto& column : layer) {
                for (auto& row : column) {
                    std::fill(row.begin(), row.end(), LegalizationClusterId::INVALID());
                }
            }
        }
    }

private:
    std::vector<std::vector<std::vector<std::vector<LegalizationClusterId>>>> grid_;

    bool in_bounds(int layer, int x, int y) const {
        return layer >= 0 && layer < int(grid_.size()) &&
               x >= 0 && x < int(grid_[layer].size()) &&
               y >= 0 && y < int(grid_[layer][x].size());
    }
};
/**
 * @brief A factory method which creates a Full Legalizer of the given type.
 */
std::unique_ptr<FullLegalizer> make_full_legalizer(e_ap_full_legalizer full_legalizer_type,
                                                   const APNetlist& ap_netlist,
                                                   const AtomNetlist& atom_netlist,
                                                   const Prepacker& prepacker,
                                                   t_vpr_setup& vpr_setup,
                                                   const t_arch& arch,
                                                   const DeviceGrid& device_grid);

class BasicMinDisturbance : public FullLegalizer {

// TODO: Need to determine which of them to be private or other type. Commenting.

public:
    using FullLegalizer::FullLegalizer;

    void legalize(const PartialPlacement& p_placement) final; // what does final mean here?


    void initialize_cluster_grids();

    std::vector<APBlockId> pack_recontruction_pass(ClusterLegalizer& cluster_legalizer,
                                                        const PartialPlacement& p_placement);

    void place_clusters(const ClusteredNetlist& clb_nlist,
                        const PlaceMacros& place_macros,
                        std::unordered_map<AtomBlockId, LegalizationClusterId> atom_to_legalization_map);

    void place_clusters_naive(const ClusteredNetlist& clb_nlist,
        const PlaceMacros& place_macros,
        const PartialPlacement& p_placement);

    //vtr::NdMatrix<std::vector<LegalizationClusterId>, 3> cluster_grids;
    ClusterGridReconstruction cluster_grids;
    std::unordered_map<LegalizationClusterId, std::tuple<int, int, int, int>> cluster_location_map;
    vtr::NdMatrix<t_physical_tile_type_ptr, 3> tile_type;

    

    bool try_pack_molecule_at_location(const t_physical_tile_loc& tile_loc, const PackMoleculeId& mol_id, 
                                                            const APNetlist& ap_netlist_, const Prepacker& prepacker_, ClusterLegalizer& cluster_legalizer,
                                                            std::map<const t_model*, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types);
    std::vector<t_physical_tile_loc> get_neighbor_locations(const t_physical_tile_loc& tile_loc, int window_size);

};

/**
 * @brief The Naive Full Legalizer.
 *
 * This Full Legalizer will try to create clusters exactly where they want to
 * according to the Partial Placement. It will grow clusters from atoms that
 * are placed in the same tile, then it will try to place the cluster at that
 * location. If a location cannot be found, once all other clusters have tried
 * to be placed, it will try to find anywhere the cluster will fit and place it
 * there.
 */
class NaiveFullLegalizer : public FullLegalizer {
  public:
    using FullLegalizer::FullLegalizer;

    /**
     * @brief Perform naive full legalization.
     */
    void legalize(const PartialPlacement& p_placement) final;

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
                        const PlaceMacros& place_macros,
                        const PartialPlacement& p_placement);
};

/**
 * @brief APPack: A flat-placement-informed Packer Placer.
 *
 * The idea of APPack is to use the flat-placement information generated by the
 * AP Flow to guide the Packer and Placer to a better solution.
 *
 * In the Packer, the flat-placement can provide more context for the clusters
 * to pull in atoms that want to be near the other atoms in the cluster, and
 * repell atoms that are far apart. This can potentially make better clusters
 * than a Packer that does not know that information.
 *
 * In the Placer, the flat-placement can help decide where clusters of atoms
 * want to be placed. If this placement is then fed into a Simulated Annealing
 * based Detailed Placement step, this would enable it to converge on a better
 * answer faster.
 */
class APPack : public FullLegalizer {
  public:
    using FullLegalizer::FullLegalizer;

    /**
     * @brief Run APPack.
     *
     * This will call the Packer and Placer using the options provided by the
     * user for these stages in VPR.
     */
    void legalize(const PartialPlacement& p_placement) final;
};
