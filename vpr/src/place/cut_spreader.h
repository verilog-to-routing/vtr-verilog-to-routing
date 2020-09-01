#ifndef VPR_SRC_PLACE_LEGALIZER_H_
#define VPR_SRC_PLACE_LEGALIZER_H_

#ifdef ENABLE_ANALYTIC_PLACE

/**
 * @file
 * @brief This file defines the cut-spreader class with a greedy legalizer as a member method.
 * Cut-spreader roughly legalizes overutilized tiles present in illegal placement from the matrix equation
 * solution (lower-bound placement), using geometric partitioning to recursively cut and spread tiles within
 * these regions, eliminating most overutilizations.
 * Legalizer then strictly legalizes the placement using a greedy strategy, ensuring logic block to physical
 * subtile type-matching and eliminating all overutilizations. This completes the lower-bound placement.
 *
 **************************************************************************************************************
 * 											Algorithm Overview												  *
 **************************************************************************************************************
 * The solution produced by the solver almost always contains 2 types of illegality: overutilization and
 * logical-physical type mismatch.
 *
 * Cut-Spreader
 * ============
 * To resolve overutilization, a recursive partitioning-style placement approach is used.
 *
 * find_overused_regions & expand_regions
 * --------------------------------------
 * @see find_overused_regions()
 * @see expand_regions()
 * The first step is to find an area of the FPGA that is overutilized for which the blocks contained within
 * must be spread to a larger area. To obtain this overutilized area, adjacent locations on the FPGA that are
 * occupied by more than one block (also overutilized) are repeatedly clustered together, until all clusters
 * are bordered on all sides by non-overutilized locations. Next, the area is expanded in both the x and y
 * directions until it's large enough to accommodate all blocks contained. Overutilization is defined as follows:
 * (Occupancy / Capacity) > beta, where beta is a constant <=1, currently defined in AnalyticPlacer::PlacerHeapCfg.
 *
 * cut_region
 * ----------
 * @see cut_region()
 * @see run()
 * In the second step, two cuts are generated: a source cut and a target cut. The source cut pertains to the blocks
 * being places; the target cut pertains to the area into which the blocks are placed. The source cut splits the
 * blocks into two partitions, while the target cut splits the area into two sub-areas, into which the blocks in
 * each partition are spread. Two objectives are minimized during this process: the imbalance between the number of
 * blocks in each partition, and the difference in the utilization (Occupancy / Capacity) of each subarea.
 *
 * To generate the source cut, the logic blocks are sorted by their raw_x or raw_y location, depending on the
 * orientation of the desired cut. After sorting, a pivot is chosen, all blocks to the left of the pivot are
 * assigned to the left partition, and all blocks to the right are assigned to the right partition (we use left
 * for left/top, right for right/bottom in x/y directions respectively in the implementation).
 *
 * The target cut is an x or y cut of the area such that all blocks in each partition fit in their respective
 * subareas, and the difference in utilization is minimized. This is difficult due to the discrete nature of FPGA
 * architecture. To achieve this, first assign all tiles to right region, then move tiles to the left one by one,
 * calculating the difference in utilization for each move. The move with smallest delta_utilization is chosen as
 * the target cut. To eliminate possible overutilization in either subarea, perturb the source cut by moving a
 * single block from the over-utilized sub-area to the other, until neither is overutilized.
 *
 * Next, the blocks in sub-areas are spread to distribute them evenly. We split the sub-area into 10 equally-sized
 * bins and logic blocks into 10 equal-capacity source bins. Then linearly interpolate to map blocks from their
 * original_locations in their source bins to new spread location in target bins.
 *
 * This cut and spreading is repeated recursively. The cut spreading process returns the left and right subareas,
 * which are pushed into a FIFO. Then a region is popped from the FIFO and goes through cut-spreading. This process
 * continues until a base case of only 1 block in the region is reached. At this point the placement is mostly not
 * overutilized and ready for strict legalization.
 *
 * **************************************************************************************************************
 *
 * Strict Legalizer
 * ================
 * @see strict_legalize()
 * Strict Legalizer ensures that the placement is strictly legal. It does so using a greedy approach.
 *
 * All blocks are sorted in descending macro lengths order and put in a priority queue (only macro heads are
 * considered, while the rest of the macro members are ignored; single blocks have length 1). Each block goes through
 * the following procedure:
 *
 * * Find all compatible sub_tile types, based on which all potential sub_tile locations are found (this process is
 *   made computationally cheap by legal_pos data member in AnalyticPlacer)
 * * Within a radius (starting from 0) of the block's currently location, randomly choose a location as candidate.
 * * If the block is a single block (not in macro), multiple candidates are potentially chosen, and the one that
 *   results in the smallest input wirelength for the block is chosen.
 * * If the block is a macro head, the location for all member blocks are calculated using member offsets. If all
 *   member locations have compatible sub_tile and does not overlap with another macro, then place the macro.
 * * In either case, if the candidate fails to satisfy legality constraints, the radius may increase (depending on
 *   number of iterations at current radius), and a new candidate will be chosen.
 *
 *
 * @cite SimPL
 * Original analytic placer with cut-spreading legalizing was intended for ASIC design, proposed in SimPL.
 * SimPL: An Effective Placement Algorithm, Myung-Chul Kim, Dong-Jin Lee and Igor L. Markov
 * http://www.ece.umich.edu/cse/awards/pdfs/iccad10-simpl.pdf
 *
 * @cite HeAP
 * FPGA adaptation of SimPL, targeting FPGAs with heterogeneous blocks located at discrete locations.
 * Analytical Placement for Heterogeneous FPGAs, Marcel Gort and Jason H. Anderson
 * https://janders.eecg.utoronto.ca/pdfs/marcelfpl12.pdf
 *
 * @cite nextpnr
 * An implementation of HeAP, which the cut-spreader and legalizer here is based off of. Implementation details
 * have been modified for the architecture and netlist specification of VTR, and better performance.
 * nextpnr -- Next Generation Place and Route,  placer_heap, David Shah <david@symbioticeda.com>
 * https://github.com/YosysHQ/nextpnr
 *
 */
#    include "vpr_context.h"

// declaration of used types;
class AnalyticPlacer;
struct t_logical_block_type;

// Cut-spreader, as described in HeAP/SimPL papers
class CutSpreader {
  public:
    /*
     * @brief: Constructor of CutSpreader
     *
     * @param analytic_placer	pointer to the analytic_placer that invokes this instance of CutSpreader
     * 							passed for CutSpreader to directly access data members in analytic_placer such as
     * 							blk_locs, blk_info, solve_blks, etc, without re-packaging the data to pass to
     * 							CutSpreader.
     *
     * @param vpr_context		legal placement is returned by modifying vpr_context.mutable_placement(), which
     * 							annealer then operates on. CutSpreader also gets netlist & device info from
     * 							vpr_context.clustering().clb_nlist and vpr_context.device().
     *
     * @param blk_t				logical_block_type for CutSpreader to legalize. Currently can only legalize one
     * 							type each time.
     */
    CutSpreader(AnalyticPlacer* analytic_placer, t_logical_block_type_ptr blk_t);

    /*
     * @brief: 	Executes the cut-spreader algorithm described in algorithm overview above.
     * 			Does not include strict_legalize so placement result is not guaranteed to be legal.
     * 			Strict_legalize must be run after for legal placement result, and for legal placement to
     * 			be passed to annealer through vpr_ctx.
     *
     * 			Input placement is passed by data members (blk_locs) in analytic_placer
     *
     * @return	result placement is passed to strict legalizer by modifying blk_locs in analytic_placer
     */
    void cutSpread();

    /*
     * @brief: 	Greedy strict legalize using algorithm described in algorithm overview above.
     *
     * 			Input illegal placement from data members (blk_locs) in analytic_placer
     *
     * @return	result placement is passed to annealer by modifying vpr_ctx.mutable_placement()
     */
    void strict_legalize();

  private:
    // marker for blocks that wasn't solved (blocks of different type, macro members)
    int32_t dont_solve = std::numeric_limits<int32_t>::max();

    // pointer to analytic_placer to access its data members
    AnalyticPlacer* ap;

    // block type to legalize
    t_logical_block_type_ptr blk_type;

    // struct that specifies the region occupied by a macro
    struct MacroExtent {
        int x0, y0, x1, y1;
    };

    // struct describing regions on FPGA to cut_spread
    struct SpreaderRegion {
        int id;              // index of regions in regions vector
        int x0, y0, x1, y1;  // extent of the region
        int n_blks, n_tiles; // number of blocks and compatible tiles
        bool overused(float beta) const {
            // determines whether region is overutilized:
            // overused = (Occupancy / Capacity) > beta
            if (n_blks > beta * n_tiles) return true;
            return false;
        }
    };

    // Utilization of each tile, indexed by x, y
    std::vector<std::vector<int>> occupancy;

    // Region ID of each tile, indexed by x, y. -1 if not covered by any region
    std::vector<std::vector<int>> groups;

    // Extent of macro at x, y location. If blk is not in any macros, MacroExtent only covers a single locations
    std::vector<std::vector<MacroExtent>> macros;

    // List of logic blocks of blk_type at x, y location, indexed by x, y
    std::vector<std::vector<std::vector<ClusterBlockId>>> blks_at_location;

    // List of sub_tiles compatible with blk_type at x, y location, indexed by x, y
    std::vector<std::vector<std::vector<t_pl_loc>>> ft;

    // List of all SpreaderRegion, index of vector members is the id of the region
    std::vector<SpreaderRegion> regions;

    // List of all merged_regions, these regions are merged in larger regions and should be skipped when
    // recursively cur_spreading. Each entry is the region's ID, which is also the index into the regions vector
    std::unordered_set<int> merged_regions;

    // Lookup of macro's extent by block ID. If block is a single block, MacroExtent contains only 1 tile location
    std::map<ClusterBlockId, MacroExtent> blk_extents;

    // Setup CutSpreader data structures using information from AnalyticPlacer
    // including blks_at_location, macros, groups, etc.
    void init();

    // Returns number of logical blocks at x, y location
    int occ_at(int x, int y);

    // Returns number of compatible sub_tiles at x, y location
    int tiles_at(int x, int y);

    /*
     * Merge mergee into merged by:
     * * change group id at mergee grids to merged id
     * * adds all n_blks and n_tiles from mergee to merged region
     * * grow merged to include all mergee grids
     */
    void merge_regions(SpreaderRegion& merged, SpreaderRegion& mergee);

    /*
     * Grow region r to include a rectangular region (x0, y0, x1, y1)
     * Pass init = true if first time calling for a newly created region
     */
    void grow_region(SpreaderRegion& r, int x0, int y0, int x1, int y1, bool init = false);

    /*
     * Expand all utilized regions until they satisfy n_tiles * beta >= n_blocks
     * If overutilized regions overlap in this process, they are merged
     */
    void expand_regions();

    /*
     * Find overutilized regions surrounded by non-overutilized regions
     * Start off at an overutilized tile and expand in x, y directions until the region is surrounded by
     * non-overutilized regions.
     */
    void find_overused_regions();

    /*
     * Recursive cut-based spreading in HeAP paper
     * "left" denotes "-x, -y", "right" denotes "+x, +y" depending on dir
     *
     *  @param r	regrion to cut & spread
     *  @param dir	direction, true for y, false for x
     *
     *  @return		a pair of sub-region IDs created from cutting region r.
     *  			{-2, -2} if base case is reached
     *  			{-1, -1} if cut unsuccessful, need to cut in the other direction
     */
    std::pair<int, int> cut_region(SpreaderRegion& r, bool dir);
};

#endif /* ENABLE_ANALYTIC_PLACE */

#endif /* VPR_SRC_PLACE_LEGALIZER_H_ */
