#ifndef VPR_SRC_PLACE_LEGALIZER_H_
#define VPR_SRC_PLACE_LEGALIZER_H_

#include "vpr_context.h"

class AnalyticPlacer;
struct t_logical_block_type;

// Cut-spreader, as described in HeAP/SimPL papers
class CutSpreader {
  public:
    // can refactor to placement_hints data structure instead of AnalyticPlacer.
    CutSpreader(AnalyticPlacer* analytic_placer, VprContext& vpr_ctx, t_logical_block_type_ptr cell_t);

    void run();

    void strict_legalize();

  private:
    int32_t dont_solve = std::numeric_limits<int32_t>::max();

    AnalyticPlacer* ap;
    const DeviceContext& device_ctx;
    const ClusteredNetlist& clb_nlist;
    PlacementContext& place_ctx;
    t_logical_block_type_ptr blk_type;

    struct MacroExtent {
        int x0, y0, x1, y1;
    };

    struct SpreaderRegion {
        int id;
        int x0, y0, x1, y1;
        int n_blks, n_tiles;
        bool overused(float beta) const {
            //if (n_tiles < 4) return true;
            if (n_blks > beta * n_tiles) return true;
            return false;
        }
    };

    std::vector<std::vector<int>> occupancy;
    std::vector<std::vector<int>> groups;
    std::vector<std::vector<MacroExtent>> macros;
    std::vector<std::vector<std::vector<ClusterBlockId>>> blks_at_location;
    std::vector<std::vector<std::vector<t_pl_loc>>> ft;

    std::vector<SpreaderRegion> regions;
    std::unordered_set<int> merged_regions;

    std::map<ClusterBlockId, MacroExtent> blk_extents;

    std::vector<ClusterBlockId> cut_blks;

    void init();

    int occ_at(int x, int y);

    int tiles_at(int x, int y);

    void merge_regions(SpreaderRegion& merged, SpreaderRegion& mergee);

    void grow_region(SpreaderRegion& r, int x0, int y0, int x1, int y1, bool init = false);

    void expand_regions();

    void find_overused_regions();

    std::pair<int, int> cut_region(SpreaderRegion& r, bool dir);
};

#endif /* VPR_SRC_PLACE_LEGALIZER_H_ */
