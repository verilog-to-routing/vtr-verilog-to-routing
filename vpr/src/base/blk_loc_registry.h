#ifndef VTR_BLK_LOC_REGISTRY_H
#define VTR_BLK_LOC_REGISTRY_H

#include "clustered_netlist_fwd.h"
#include "vtr_vector_map.h"
#include "vpr_types.h"
#include "grid_block.h"

struct t_block_loc;

/**
 * @class BlkLocRegistry contains information about the placement of clustered blocks.
 * More specifically:
 *      1) block_locs stores the location where each clustered blocks is placed at.
 *      2) grid_blocks stores which blocks (if any) are placed at a given location.
 *      3) physical_pins stores the mapping between the pins of a clustered block and
 *      the pins of the physical tile where the clustered blocks is placed.
 *
 */
class BlkLocRegistry {
  public:
    BlkLocRegistry() = default;
    ~BlkLocRegistry() = default;
    BlkLocRegistry(const BlkLocRegistry&) = delete;
    BlkLocRegistry& operator=(const BlkLocRegistry&) = default;
    BlkLocRegistry(BlkLocRegistry&&) = delete;
    BlkLocRegistry& operator=(BlkLocRegistry&&) = delete;

  private:
    ///@brief Clustered block placement locations
    vtr::vector_map<ClusterBlockId, t_block_loc> block_locs_;

    ///@brief Clustered block associated with each grid location (i.e. inverse of block_locs)
    GridBlock grid_blocks_;

    ///@brief Clustered pin placement mapping with physical pin
    vtr::vector_map<ClusterPinId, int> physical_pins_;

  public:
    const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs() const;
    vtr::vector_map<ClusterBlockId, t_block_loc>& mutable_block_locs();

    const GridBlock& grid_blocks() const;
    GridBlock& mutable_grid_blocks();

    const vtr::vector_map<ClusterPinId, int>& physical_pins() const;
    vtr::vector_map<ClusterPinId, int>& mutable_physical_pins();

    ///@brief Returns the physical pin of the tile, related to the given ClusterPinId
    int tile_pin_index(const ClusterPinId pin) const;

    ///@brief Returns the physical pin of the tile, related to the given ClusterNedId, and the net pin index.
    int net_pin_to_tile_pin_index(const ClusterNetId net_id, int net_pin_index) const;
};

#endif //VTR_BLK_LOC_REGISTRY_H
