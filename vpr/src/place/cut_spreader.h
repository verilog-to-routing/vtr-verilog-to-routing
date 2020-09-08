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
 * To resolve overutilization, a recursive partitioning-style placement approach is used. It consists of the following
 * steps:
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
 * being placed; the target cut pertains to the area into which the blocks are placed. The source cut splits the
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
 * original locations in their source bins to new spread location in target bins.
 *
 * This cutting and spreading is repeated recursively. The cut-spreading process returns the left and right
 * (or top and bottom) subareas, which are pushed into a workqueue FIFO. Their direction of cut in the next
 * cut-spreading process is alternated, i.e. if the first cut is in y direction, the resulting left and right
 * sub-areas are further cut in x direction, each producing 2 subareas top and bottom, and so forth.
 * The first region in the FIFO is then popped and goes through cut-spreading. This process is repeated until the base
 * case of only 1 block in the region is reached. At this point the placement is mostly not overutilized and ready
 * for strict legalization.
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
 *   results in the smallest input wirelength (sum of wirelengths to its inputs) for the block is chosen.
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
#    include <queue>

// declaration of used types;
class AnalyticPlacer;
struct t_logical_block_type;

// Cut-spreader, as described in HeAP/SimPL papers
class CutSpreader {
  public:
    /*
     * @brief: Constructor of CutSpreader
     *
     * @param analytic_placer	pointer to the analytic_placer that invokes this instance of CutSpreader.
     * 							passed for CutSpreader to directly access data members in analytic_placer such as
     * 							blk_locs, blk_info, solve_blks, etc, without re-packaging the data to pass to
     * 							CutSpreader.
     *
     * @param blk_t				logical_block_type for CutSpreader to legalize. Currently can only legalize one
     * 							type each time.
     */
    CutSpreader(AnalyticPlacer* analytic_placer, t_logical_block_type_ptr blk_t);

    /*
     * @brief: 	Executes the cut-spreader algorithm described in algorithm overview above.
     * 			Does not include strict_legalize so placement result is not guaranteed to be legal.
     * 			Strict_legalize must run after for legal placement result, and for legal placement to
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
     * 			Input illegal placement from data members (blk_locs) in analytic_placer,
     * 			previously modified by cut_spreader
     *
     * @return: both ap->blk_locs and vpr_ctx.mutable_placement() are modified with legal placement,
     * 			to be used in next solve/spread/legalize iteration or to pass back to annealer.
     */
    void strict_legalize();

  private:
    // pointer to analytic_placer to access its data members
    AnalyticPlacer* ap;

    // block type to legalize
    t_logical_block_type_ptr blk_type;

    // struct describing regions on FPGA to cut_spread
    struct SpreaderRegion {
        int id;              // index of regions in regions vector
        vtr::Rect<int> bb;   // bounding box of the region
        int n_blks, n_tiles; // number of netlist blocks and compatible tiles (placement locations)
        bool overused(float beta) const {
            // determines whether region is overutilized: overused = (Occupancy / Capacity) > beta
            if (n_blks > beta * n_tiles)
                return true;
            else
                return false;
        }
    };

    // Utilization of each tile, indexed by x, y
    vtr::Matrix<int> occupancy;

    // Region ID of each tile, indexed by x, y. AP_NO_REGION if not covered by any region
    // Used to check ownership of a grid position by region.
    vtr::Matrix<int> reg_id_at_grid;

    // Extent of macro at x, y location. If blk is not in any macros, it only covers a single location
    vtr::Matrix<vtr::Rect<int>> macro_extent;

    // List of logic blocks of blk_type at x, y location, indexed by x, y
    // ex. to find all logic blocks occupying location x, y, blks_at_location[x][y] gives a vector of
    // block IDs at that location
    vtr::Matrix<std::vector<ClusterBlockId>> blks_at_location;

    // List of all compatible sub_tiles for the type of blocks being cut-spread, at location x, y.
    // usage: subtiles_at_location[x][y]
    vtr::Matrix<std::vector<t_pl_loc>> subtiles_at_location;

    // List of all SpreaderRegion, index of vector members is the id of the region
    std::vector<SpreaderRegion> regions;

    // List of all merged_regions, these regions are merged in larger regions and should be skipped when
    // recursively cut_spreading. Each entry is the region's ID, which is also the index into the regions vector
    std::unordered_set<int> merged_regions;

    // Lookup of macro's extent by block ID. If block is a single block, it contains only 1 tile location
    vtr::vector_map<ClusterBlockId, vtr::Rect<int>> blk_extents;

    // Setup CutSpreader data structures using information from AnalyticPlacer
    // including blks_at_location, macros, groups, etc.
    void init();

    // Returns number of logical blocks at x, y location
    int occ_at(int x, int y);

    // Returns number of compatible sub_tiles at x, y location
    int tiles_at(int x, int y);

    /*
     * When expanding a region, it might overlap with another region, one of them (merger) will absorb
     * the other (mergee) by merging. @see expand_regions() below;
     *
     * Merge mergee into merged by:
     * * change group id at mergee grids to merged id
     * * adds all n_blks and n_tiles from mergee to merged region
     * * grow merged to include all mergee grids
     */
    void merge_regions(SpreaderRegion& merged, SpreaderRegion& mergee);

    /*
     * Grow region r to include a rectangular region
     * Pass init = true if first time calling for a newly created region
     */
    void grow_region(SpreaderRegion& r, vtr::Rect<int> rect_to_include, bool init = false);

    /*
     * Expand all over-utilized regions until they satisfy n_tiles * beta >= n_blocks
     * If overutilized regions overlap in this process, they are merged
     */
    void expand_regions();

    /*
     * Find overutilized regions surrounded by non-overutilized regions
     * Start off at an overutilized tile and expand in x, y directions 1 step at a time in both directions
     * until the region is surrounded by non-overutilized regions.
     */
    void find_overused_regions();

    // copy all logic blocks that needs to be cut into cut_blks
    void init_cut_blks(SpreaderRegion& r, std::vector<ClusterBlockId>& cut_blks);

    /*
     * generate the initial source_cut for region r, ensure there is enough clearance on either side of the
     * initial cut to accommodate macros
     * returns the initial source cut (index into cut_blks)
     * returns the clearance in clearance_l, clearance_r
     * returns -1 if cannot generate initial source_cut (not enough clearance for macros)
     */
    int initial_source_cut(SpreaderRegion& r,
                           std::vector<ClusterBlockId>& cut_blks,
                           bool dir,
                           int& clearance_l,
                           int& clearance_r);

    /*
     * generate the initial target_cut for region r, ensure that utilization in 2 subareas are closest possible
     * while meeting clearance requirements for macros
     * returns best target cut
     * returns the resulting number of blocks in left and right partitions in left_blks_n, right_blks_n
     * returns the resulting number of tiles in left and right subareas in left_tiles_n, right_tiles_n
     */
    int initial_target_cut(SpreaderRegion& r,
                           std::vector<ClusterBlockId>& cut_blks,
                           int init_source_cut,
                           bool dir,
                           int trimmed_l,
                           int trimmed_r,
                           int clearance_l,
                           int clearance_r,
                           int& left_blks_n,
                           int& right_blks_n,
                           int& left_tiles_n,
                           int& right_tiles_n);

    /*
     * Trim the boundaries of the region in axis-of-interest, skipping any rows/cols without any tiles
     * of the right type.
     * Afterwards, move blocks in trimmed locations to new trimmed boundaries
     */
    std::pair<int, int> trim_region(SpreaderRegion& r, bool dir);

    /*
     * Spread blocks in subarea by linear interpolation
     * blks_start and blks_end are indices into cut_blks. The blks between these indices will be spread by:
     * * first split the subarea (between boundaries area_l and area_r) into
     *   min(number_of_logic_blocks_in_subarea, 10) number of bins.
     * * split the logic blocks into the corresponding number of groups
     * * place the logic blocks from their group to their bin, by linear interpolation using their original
     *   locations to map to a new location in the bin.
     */
    void linear_spread_subarea(std::vector<ClusterBlockId>& cut_blks,
                               bool dir,
                               int blks_start,
                               int blks_end,
                               SpreaderRegion& sub_area);

    /*
     * Recursive cut-based spreading in HeAP paper
     * "left" denotes "-x, -y", "right" denotes "+x, +y" depending on dir
     *
     *  @param r	region to cut & spread
     *  @param dir	direction, true for y, false for x
     *
     *  @return		a pair of sub-region IDs created from cutting region r.
     *  			BASE_CASE if base case is reached
     *  			CUT_FAIL if cut unsuccessful, need to cut in the other direction
     */
    std::pair<int, int> cut_region(SpreaderRegion& r, bool dir);

    /*
     * Helper function in strict_legalize()
     * Place blk on sub_tile location by modifying place_ctx.grid_blocks and place_ctx.block_locs
     */
    void bind_tile(t_pl_loc sub_tile, ClusterBlockId blk);

    /*
     * Helper function in strict_legalize()
     * Remove placement at sub_tile location by clearing place_ctx.block_locs and place_Ctx.grid_blocks
     */
    void unbind_tile(t_pl_loc sub_tile);

    /*
     * Helper function in strict_legalize()
     * Check if the block is placed in place_ctx (place_ctx.block_locs[blk] has a location that matches
     * the block in place_ctx.grid_blocks)
     */
    bool is_placed(ClusterBlockId blk);

    /*
     * Sub-routine of strict_legalize()
     * Tries to place a single block "blk" at a candidate location nx, ny. Returns whether the blk is succesfully placed.
     *
     * If number of iterations at current radius has exceeded the exploration limit (exceeds_explore_limit),
     * and a candidate sub_tile is already found (best_subtile), then candidate location is ignored, and blk is
     * placed in best_subtile.
     *
     * Else, if exploration limit is not exceeded, the subtiles at nx, ny are evaluated on the blk's resulting total
     * input wirelength (a heuristic). If this total input wirelength is shorter than current best_inp_len, it becomes
     * the new best_subtile.
     * If exploration limit is exceeded and no candidate sub_tile is available in (best_subtile), then blk is placed at
     * next sub_tile at candidate location nx, ny.
     *
     * If blk displaces a logic block by taking its sub_tile, the displaced logic block is put back into remaining queue.
     */
    bool try_place_blk(ClusterBlockId blk,
                       int nx,
                       int ny,
                       bool ripup_radius_met,
                       bool exceeds_need_to_explore,
                       int& best_inp_len,
                       t_pl_loc& best_subtile,
                       std::priority_queue<std::pair<int, ClusterBlockId>>& remaining);

    /*
     * Sub-routine of strict_legalize()
     *
     * Tries to place the macro with the head block on candidate location nx, ny. Returns if the macro is successfully placed.
     *
     * For each possible macro placement starting from nx, ny, if any block's position in the macro does not have compatible
     * sub_tiles or overlaps with another macro, the placement is impossible.
     *
     * If a possible placement is found, it's applied to all blocks.
     */
    bool try_place_macro(ClusterBlockId blk,
                         int nx,
                         int ny,
                         std::priority_queue<std::pair<int, ClusterBlockId>>& remaining);
};
#endif /* ENABLE_ANALYTIC_PLACE */

#endif /* VPR_SRC_PLACE_LEGALIZER_H_ */
