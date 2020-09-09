#ifdef ENABLE_ANALYTIC_PLACE

#    include "cut_spreader.h"
#    include <iostream>
#    include <vector>
#    include <queue>
#    include <cstdlib>

#    include "analytic_placer.h"
#    include "vpr_types.h"
#    include "vtr_time.h"
#    include "globals.h"
#    include "vtr_log.h"

// sentinel for base case in CutSpreader (i.e. only 1 block left in region)
constexpr std::pair<int, int> BASE_CASE = {-2, -2};

// sentinel for cut-spreading fail, the other direction is run next
constexpr std::pair<int, int> CUT_FAIL = {-1, -1};

// sentinel for a grid location that is not covered by any regions, for reg_id_at_grid data member
constexpr int AP_NO_REGION = -1;

/*
 * Constructor of CutSpreader
 * @param analytic_placer:	used to access AnalyticPlacer data members (lower-bound solutions)
 * @param blk_t:			logical block type to legalize
 */
CutSpreader::CutSpreader(AnalyticPlacer* analytic_placer, t_logical_block_type_ptr blk_t)
    : ap(analytic_placer)
    , blk_type(blk_t) {
    // builds n_subtiles_at_location data member, which is a quick lookup of number of compatible subtiles at x, y.
    size_t max_x = g_vpr_ctx.device().grid.width();
    size_t max_y = g_vpr_ctx.device().grid.height();
    subtiles_at_location.resize({max_x, max_y});
    for (auto& tile : blk_type->equivalent_tiles) {
        for (auto sub_tile : tile->sub_tiles) {
            // find all sub_tile types compatible with blk_t
            auto result = std::find(sub_tile.equivalent_sites.begin(), sub_tile.equivalent_sites.end(), blk_type);
            if (result != sub_tile.equivalent_sites.end()) {
                for (auto loc : ap->legal_pos.at(tile->index).at(sub_tile.index)) {
                    subtiles_at_location[loc.x][loc.y].push_back(loc);
                }
            }
        }
    }
}

/*
 * @brief: 	Executes the cut-spreader algorithm described in algorithm overview in header file.
 * 			Does not include strict_legalize so placement result is not guaranteed to be legal.
 * 			Strict_legalize must be run after for legal placement result, and for legal placement to
 * 			be passed to annealer through vpr_ctx.
 *
 * 			Input placement is passed by data members (blk_locs) in analytic_placer
 *
 * @return	result placement is passed to strict legalizer by modifying blk_locs in analytic_placer
 */
void CutSpreader::cutSpread() {
    init();                  // initialize data members based on solved solutions from AnalyticPlacer
    find_overused_regions(); //find all overused regions bordered by non-overused regions
    expand_regions();        // expand overused regions until they have enough sub_tiles to accommodate their logic blks

    /*
     * workqueue is a FIFO queue used to recursively cut-spread.
     *
     * In the region vector, the regions not in merged_regions (not absorbed in expansion process)
     * are the initial regions placed in workqueue to cut-spread.
     *
     * After each of these initial regions are cut and spread, their child sub-regions
     * (left and right) are placed at the back of workqueue, with alternated cut direction.
     * This process continues until base case of region with only 1 block is reached,
     * indicated by BASE_CASE return value.
     *
     * Return value of CUT_FAIL indicates that cutting is unsuccessful. This usually happens
     * when regions are quite small: for example, region only has 1 column so a vertical cut
     * is impossible. In this case cut in the other direction is attempted.
     */
    std::queue<std::pair<int, bool>> workqueue;

    // put initial regions into workqueue
    for (auto& r : regions) {
        if (!merged_regions.count(r.id))
            workqueue.emplace(r.id, false);
    }

    while (!workqueue.empty()) {
        auto front = workqueue.front();
        workqueue.pop();
        auto& r = regions.at(front.first);

        auto res = cut_region(r, front.second);
        if (res == BASE_CASE) // only 1 block left, base case
            continue;
        if (res != CUT_FAIL) { // cut-spread successful
                               // place children regions in workqueue
            workqueue.emplace(res.first, !front.second);
            workqueue.emplace(res.second, !front.second);
        } else {                                      // cut-spread unsuccessful
            auto res2 = cut_region(r, !front.second); // try other direction
            if (res2 != CUT_FAIL) {
                // place children regions in workqueue
                workqueue.emplace(res2.first, front.second);
                workqueue.emplace(res2.second, front.second);
            }
        }
    }
}

// setup CutSpreader data structures using information from AnalyticPlacer
void CutSpreader::init() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    size_t max_x = g_vpr_ctx.device().grid.width();
    size_t max_y = g_vpr_ctx.device().grid.height();

    occupancy.resize({max_x, max_y}, 0);
    macro_extent.resize({max_x, max_y});
    reg_id_at_grid.resize({max_x, max_y}, AP_NO_REGION);
    blk_extents.resize(ap->blk_locs.size(), vtr::Rect<int>{-1, -1, -1, -1});
    blks_at_location.resize({max_x, max_y}, std::vector<ClusterBlockId>{});

    // Initialize occupancy matrix, reg_id_at_grid and macros matrix
    for (int x = 0; x < (int)max_x; x++) {
        for (int y = 0; y < (int)max_y; y++) {
            occupancy[x][y] = 0;
            reg_id_at_grid[x][y] = AP_NO_REGION;
            macro_extent[x][y] = {x, y, x, y};
        }
    }

    // lambda function to absorb x, y in blk's macro's extent
    auto set_macro_ext = [&](ClusterBlockId blk, int x, int y) {
        if (blk_extents[blk] == vtr::Rect<int>{-1, -1, -1, -1}) {
            blk_extents.update(blk, {x, y, x, y});
        } else {
            blk_extents[blk].expand_bounding_box({x, y, x, y});
        }
    };

    for (size_t i = 0; i < ap->blk_locs.size(); i++) { // loop through ap->blk_locs
        auto blk = ClusterBlockId{(int)i};
        if (clb_nlist.block_type(blk) == blk_type) {
            auto loc = ap->blk_locs[blk].loc;
            occupancy[loc.x][loc.y]++;
            // compute extent of macro member
            if (imacro(blk) != NO_MACRO) { // if blk is a macro member
                // only update macro heads' extent in blk_extents
                set_macro_ext(macro_head(blk), loc.x, loc.y);
            }
        }
    }

    for (size_t i = 0; i < ap->blk_locs.size(); i++) { // loop through ap->blk_locs
        ClusterBlockId blk = ClusterBlockId{(int)i};
        if (clb_nlist.block_type(blk) == blk_type) {
            // Transfer macro extents to the actual macros structure;
            if (imacro(blk) != NO_MACRO) { // if blk is a macro member
                // update macro_extent for all macro members in macros
                // for single blocks (not in macro), macros[x][y] = {x, y, x, y}
                vtr::Rect<int>& me = blk_extents[macro_head(blk)];
                auto loc = ap->blk_locs[blk].loc;
                auto& lme = macro_extent[loc.x][loc.y];
                lme.expand_bounding_box(me);
            }
        }
    }

    // get solved_solution from AnalyticPlacer
    for (auto blk : ap->solve_blks) {
        if (clb_nlist.block_type(blk) == blk_type)
            blks_at_location[ap->blk_locs[blk].loc.x][ap->blk_locs[blk].loc.y].push_back(blk);
    }
}

int CutSpreader::occ_at(int x, int y) { return occupancy[x][y]; }

int CutSpreader::tiles_at(int x, int y) {
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();
    if (x >= max_x || y >= max_y)
        return 0;
    return int(subtiles_at_location[x][y].size());
}

/*
 * When expanding a region, it might overlap with another region, one of them (merger) will absorb
 * the other (mergee) by merging. @see expand_regions() below;
 *
 * Merge mergee into merged by:
 * * change group id at mergee grids to merged id
 * * adds all n_blks and n_tiles from mergee to merged region
 * * grow merged to include all mergee grids
 */
void CutSpreader::merge_regions(SpreaderRegion& merged, SpreaderRegion& mergee) {
    for (int x = mergee.bb.xmin(); x <= mergee.bb.xmax(); x++)
        for (int y = mergee.bb.ymin(); y <= mergee.bb.ymax(); y++) {
            VTR_ASSERT(reg_id_at_grid[x][y] == mergee.id);
            reg_id_at_grid[x][y] = merged.id; //change group id at mergee grids to merged id
            //adds all n_blks and n_tiles from mergee to merged region
            merged.n_blks += occ_at(x, y);
            merged.n_tiles += tiles_at(x, y);
        }
    merged_regions.insert(mergee.id); // all merged_regions are ignored in main loop
    grow_region(merged, mergee.bb);   // grow merged to include all mergee grids
}

/*
 * grow r to include a rectangular region rect_to_include
 *
 * when init == true, grow_region() initializes SpreaderRegion r
 * in this case, both r and rect_to_include contains the same 1 tile location: the initial overused tile
 * see find_overused_regions where SpreaderRegion r is created.
 * this tile location is processed although it's technically included.
 */
void CutSpreader::grow_region(SpreaderRegion& r, vtr::Rect<int> rect_to_include, bool init) {
    // when given location is within SpreaderRegion
    if ((r.bb.contains(rect_to_include)) && !init)
        return;

    vtr::Rect<int> r_old = r.bb;
    r_old.set_xmin(r.bb.xmin() + (init ? 1 : 0)); // ensure the initial location is processed in the for-loop later, when init == 1
    r.bb.expand_bounding_box(rect_to_include);

    auto process_location = [&](int x, int y) {
        // kicks in only when grid is not claimed, claimed by another region, or part of a macro
        // Merge with any overlapping regions
        if (reg_id_at_grid[x][y] == AP_NO_REGION) {
            r.n_tiles += tiles_at(x, y);
            r.n_blks += occ_at(x, y);
        }
        if (reg_id_at_grid[x][y] != AP_NO_REGION && reg_id_at_grid[x][y] != r.id)
            merge_regions(r, regions.at(reg_id_at_grid[x][y]));
        reg_id_at_grid[x][y] = r.id;
        // Grow to cover any macros
        auto& macro_bb = macro_extent[x][y];
        grow_region(r, macro_bb);
    };
    // process new areas after including rect_to_include, while avoiding double counting old region
    for (int x = r.bb.xmin(); x < r_old.xmin(); x++)
        for (int y = r.bb.ymin(); y <= r.bb.ymax(); y++)
            process_location(x, y);
    for (int x = r_old.xmax() + 1; x <= r.bb.xmax(); x++)
        for (int y = r.bb.ymin(); y <= r.bb.ymax(); y++)
            process_location(x, y);
    for (int y = r.bb.ymin(); y < r_old.ymin(); y++)
        for (int x = r.bb.xmin(); x <= r.bb.xmax(); x++)
            process_location(x, y);
    for (int y = r_old.ymax() + 1; y <= r.bb.ymax(); y++)
        for (int x = r.bb.xmin(); x <= r.bb.xmax(); x++)
            process_location(x, y);
}

// Find overutilized regions surrounded by non-overutilized regions
void CutSpreader::find_overused_regions() {
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();
    for (int x = 0; x < max_x; x++)
        for (int y = 0; y < max_y; y++) {
            if (reg_id_at_grid[x][y] != AP_NO_REGION || (occ_at(x, y) <= tiles_at(x, y)))
                // already in a region or not over-utilized
                continue;

            // create new overused region
            int id = int(regions.size());
            reg_id_at_grid[x][y] = id;
            SpreaderRegion reg;
            reg.id = id;
            reg.bb = {x, y, x, y};
            reg.n_tiles = reg.n_blks = 0;
            reg.n_tiles += tiles_at(x, y);
            reg.n_blks += occ_at(x, y);

            // initialize reg and ensure it covers macros
            grow_region(reg, {x, y, x, y}, true);

            bool expanded = true;
            while (expanded) {
                expanded = false;
                // keep expanding in x and y, until expansion in x, y cannot find overutilised blks

                // try expanding in x
                if (reg.bb.xmax() < max_x - 1) {
                    bool over_occ_x = false;
                    for (int y1 = reg.bb.ymin(); y1 <= reg.bb.ymax(); y1++) {
                        if (occ_at(reg.bb.xmax() + 1, y1) > tiles_at(reg.bb.xmax() + 1, y1)) {
                            over_occ_x = true;
                            break;
                        }
                    }
                    if (over_occ_x) {
                        expanded = true;
                        grow_region(reg, {reg.bb.xmin(), reg.bb.ymin(), reg.bb.xmax() + 1, reg.bb.ymax()});
                    }
                }
                // try expanding in y
                if (reg.bb.ymax() < max_y - 1) {
                    bool over_occ_y = false;
                    for (int x1 = reg.bb.xmin(); x1 <= reg.bb.xmax(); x1++) {
                        if (occ_at(x1, reg.bb.ymax() + 1) > tiles_at(x1, reg.bb.ymax() + 1)) {
                            over_occ_y = true;
                            break;
                        }
                    }
                    if (over_occ_y) {
                        expanded = true;
                        grow_region(reg, {reg.bb.xmin(), reg.bb.ymin(), reg.bb.xmax(), reg.bb.ymax() + 1});
                    }
                }
            }
            regions.push_back(reg);
        }
}

/*
 * Expand all utilized regions until they satisfy n_tiles * beta >= n_blocks
 * If overutilized regions overlap in this process, they are merged
 */
void CutSpreader::expand_regions() {
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();

    std::queue<int> overused_regions;
    float beta = ap->ap_cfg.beta;
    for (auto& r : regions)
        // if region is not merged and is overused, move into overused_regions queue
        if (!merged_regions.count(r.id) && r.overused(beta))
            overused_regions.push(r.id);

    while (!overused_regions.empty()) { // expand all overused regions
        int rid = overused_regions.front();
        overused_regions.pop();
        if (merged_regions.count(rid))
            continue;
        auto& reg = regions.at(rid);
        while (reg.overused(beta)) {
            bool changed = false;

            // spread_scale determines steps in x or y direction to expand each time
            for (int j = 0; j < ap->ap_cfg.spread_scale_x; j++) {
                if (reg.bb.xmin() > 0) { // expand in -x direction
                    grow_region(reg, {reg.bb.xmin() - 1, reg.bb.ymin(), reg.bb.xmax(), reg.bb.ymax()});
                    changed = true;
                    if (!reg.overused(beta))
                        break;
                }
                if (reg.bb.xmax() < max_x - 1) { // expand in +x direction
                    grow_region(reg, {reg.bb.xmin(), reg.bb.ymin(), reg.bb.xmax() + 1, reg.bb.ymax()});
                    changed = true;
                    if (!reg.overused(beta))
                        break;
                }
            }

            for (int j = 0; j < ap->ap_cfg.spread_scale_y; j++) {
                if (reg.bb.ymin() > 0) { // expand in -y direction
                    grow_region(reg, {reg.bb.xmin(), reg.bb.ymin() - 1, reg.bb.xmax(), reg.bb.ymax()});
                    changed = true;
                    if (!reg.overused(beta))
                        break;
                }
                if (reg.bb.ymax() < max_y - 1) { // expand in +y direction
                    grow_region(reg, {reg.bb.xmin(), reg.bb.ymin(), reg.bb.xmax(), reg.bb.ymax() + 1});
                    changed = true;
                    if (!reg.overused(beta))
                        break;
                }
            }
            VTR_ASSERT(changed || reg.n_tiles >= reg.n_blks);
        }
        VTR_ASSERT(reg.n_blks <= reg.n_tiles);
    }
}

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
std::pair<int, int> CutSpreader::cut_region(SpreaderRegion& r, bool dir) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    std::vector<ClusterBlockId> cut_blks;
    init_cut_blks(r, cut_blks); // copy all logic blocks to cut into cut_blks

    // Trim the boundaries of the region in axis-of-interest, skipping any rows/cols without any tiles of the right type
    int trimmed_l, trimmed_r;
    std::pair<int&, int&>(trimmed_l, trimmed_r) = trim_region(r, dir);

    // base case (only 1 block left in region)
    if (cut_blks.size() == 1) {
        // ensure placement of last block is on right type of tile
        auto blk = cut_blks.at(0);
        auto& tiles_type = clb_nlist.block_type(blk)->equivalent_tiles;
        auto loc = ap->blk_locs[blk].loc;
        if (std::find(tiles_type.begin(), tiles_type.end(), device_ctx.grid[loc.x][loc.y].type) == tiles_type.end()) {
            // logic block type doesn't match tile type
            // exhaustive search for tile of right type
            // this search should be fast as region must be small at this point (only 1 logic block left)
            for (int x = r.bb.xmin(); x <= r.bb.xmax(); x++)
                for (int y = r.bb.ymin(); y <= r.bb.ymax(); y++) {
                    if (std::find(tiles_type.begin(), tiles_type.end(), device_ctx.grid[x][y].type) != tiles_type.end()) {
                        VTR_ASSERT(blks_at_location[x][y].empty());
                        ap->blk_locs[blk].rawx = x;
                        ap->blk_locs[blk].rawy = y;
                        ap->blk_locs[blk].loc.x = x;
                        ap->blk_locs[blk].loc.y = y;
                        blks_at_location[x][y].push_back(blk);
                        blks_at_location[loc.x][loc.y].clear();
                        return BASE_CASE;
                    }
                }
        }
        return BASE_CASE;
    }

    // sort blks based on raw location
    std::sort(cut_blks.begin(), cut_blks.end(), [&](const ClusterBlockId a, const ClusterBlockId b) {
        return dir ? (ap->blk_locs[a].rawy < ap->blk_locs[b].rawy) : (ap->blk_locs[a].rawx < ap->blk_locs[b].rawx);
    });

    /*
     * Generate initial source cut. It cuts logic blocks in region r into 2 partitions.
     * Initially, ensure that both partitions have similar numbers of logic blocks.
     * Find the midpoint (in terms of total block size, including macros) in sorted cut_blks
     * This is the initial source cut
     */
    int clearance_l, clearance_r;
    int pivot = initial_source_cut(r, cut_blks, dir, clearance_l, clearance_r);

    /*
     * Generate initial target cut. It cuts the physical tiles into 2 sub-areas, into which
     * the 2 partitions of logic blocks will be placed.
     *
     * The difference in utilization (# blocks / # tiles) should be smallest, while meeting
     * clearance requirement for macros
     */
    int left_blks_n, right_blks_n, left_tiles_n, right_tiles_n;
    int best_tgt_cut = initial_target_cut(r, cut_blks, pivot, dir, trimmed_l, trimmed_r,
                                          clearance_l, clearance_r, left_blks_n, right_blks_n, left_tiles_n, right_tiles_n);
    if (best_tgt_cut == -1) // target cut fails clearance requirement for macros
        return CUT_FAIL;

    // Once target_cut is acquired, define left and right subareas
    // The boundaries are defined using the trimmed edges and best target cut
    // The n_tiles will be final while n_blks may change by perturbing the source cut to eliminate
    // overutilization in subareas
    SpreaderRegion rl, rr;
    rl.id = int(regions.size());
    rl.bb = dir ? vtr::Rect<int>{r.bb.xmin(), trimmed_l, r.bb.xmax(), best_tgt_cut}
                : vtr::Rect<int>{trimmed_l, r.bb.ymin(), best_tgt_cut, r.bb.ymax()};
    rl.n_blks = left_blks_n;
    rl.n_tiles = left_tiles_n;
    rr.id = int(regions.size()) + 1;
    rr.bb = dir ? vtr::Rect<int>{r.bb.xmin(), best_tgt_cut + 1, r.bb.xmax(), trimmed_r}
                : vtr::Rect<int>{best_tgt_cut + 1, r.bb.ymin(), trimmed_r, r.bb.ymax()};
    rr.n_blks = right_blks_n;
    rr.n_tiles = right_tiles_n;
    // change the region IDs in each subarea's grid location to subarea's id
    for (int x = rl.bb.xmin(); x <= rl.bb.xmax(); x++)
        for (int y = rl.bb.ymin(); y <= rl.bb.ymax(); y++)
            reg_id_at_grid[x][y] = rl.id;
    for (int x = rr.bb.xmin(); x <= rr.bb.xmax(); x++)
        for (int y = rr.bb.ymin(); y <= rr.bb.ymax(); y++)
            reg_id_at_grid[x][y] = rr.id;

    /*
     * Perturb source cut to eliminate over-utilization
     * This is done by moving logic blocks from overused subarea to the other subarea one at a time
     * until they are no longer overused.
     */
    // while left subarea is over-utilized, move logic blocks to the right subarea one at a time
    while (pivot > 0 && rl.overused(ap->ap_cfg.beta)) {
        auto& move_blk = cut_blks.at(pivot);
        int size = (imacro(move_blk) != NO_MACRO) ? place_ctx.pl_macros[imacro(move_blk)].members.size() : 1;
        rl.n_blks -= size;
        rr.n_blks += size;
        pivot--;
    }
    // while right subarea is over-utilized, move logic blocks to the left subarea one at a time
    while (pivot < int(cut_blks.size()) - 1 && rr.overused(ap->ap_cfg.beta)) {
        auto& move_blk = cut_blks.at(pivot + 1);
        int size = (imacro(move_blk) != NO_MACRO) ? place_ctx.pl_macros[imacro(move_blk)].members.size() : 1;
        rl.n_blks += size;
        rr.n_blks -= size;
        pivot++;
    }

    // within each subarea, spread the logic blocks into bins to make them more evenly spread out
    linear_spread_subarea(cut_blks, dir, 0, pivot + 1, rl);
    linear_spread_subarea(cut_blks, dir, pivot + 1, cut_blks.size(), rr);

    // push subareas back to regions so that they can be accessed by their IDs later
    regions.push_back(rl);
    regions.push_back(rr);

    return std::make_pair(rl.id, rr.id);
}

// copy all logic blocks to cut into cut_blks
void CutSpreader::init_cut_blks(SpreaderRegion& r, std::vector<ClusterBlockId>& cut_blks) {
    cut_blks.clear();
    for (int x = r.bb.xmin(); x <= r.bb.xmax(); x++) {
        for (int y = r.bb.ymin(); y <= r.bb.ymax(); y++) {
            std::copy(blks_at_location[x][y].begin(), blks_at_location[x][y].end(), std::back_inserter(cut_blks));
        }
    }
}

/*
 * Trim the boundaries of the region r in axis-of-interest dir, skipping any rows/cols without
 * tiles of the right type.
 * Afterwards, move blocks in trimmed locations to new trimmed boundaries
 */
std::pair<int, int> CutSpreader::trim_region(SpreaderRegion& r, bool dir) {
    int bb_min = dir ? r.bb.ymin() : r.bb.xmin();
    int bb_max = dir ? r.bb.ymax() : r.bb.xmax();
    int trimmed_l = bb_min, trimmed_r = bb_max;
    bool have_tiles = false;
    while (trimmed_l < bb_max && !have_tiles) { // trim from left
        for (int i = bb_min; i <= bb_max; i++)
            if (tiles_at(dir ? i : trimmed_l, dir ? trimmed_l : i) > 0) {
                have_tiles = true;
                break;
            }
        if (!have_tiles) // trim when the row/col doesn't have tiles
            trimmed_l++;
    }

    have_tiles = false;
    while (trimmed_r > bb_min && !have_tiles) { // trim from right
        for (int i = bb_min; i <= bb_max; i++)
            if (tiles_at(dir ? i : trimmed_r, dir ? trimmed_r : i) > 0) {
                have_tiles = true;
                break;
            }
        if (!have_tiles) // trim when the row/col doesn't have tiles
            trimmed_r--;
    }

    // move blocks from trimmed locations to new boundaries
    for (int x = r.bb.xmin(); x < (dir ? r.bb.xmax() + 1 : trimmed_l); x++) {
        for (int y = r.bb.ymin(); y < (dir ? trimmed_l : r.bb.ymax() + 1); y++) {
            for (auto& blk : blks_at_location[x][y]) {
                // new location is the closest trimmed boundary
                int blk_new_x = dir ? x : trimmed_l, blk_new_y = dir ? trimmed_l : y;
                ap->blk_locs[blk].rawx = blk_new_x;
                ap->blk_locs[blk].rawy = blk_new_y;
                ap->blk_locs[blk].loc.x = blk_new_x;
                ap->blk_locs[blk].loc.y = blk_new_y;
                blks_at_location[blk_new_x][blk_new_y].push_back(blk);
            }
            blks_at_location[x][y].clear(); // clear blocks at old location
        }
    }

    for (int x = (dir ? r.bb.xmin() : trimmed_r + 1); x <= r.bb.xmax(); x++) {
        for (int y = (dir ? trimmed_r + 1 : r.bb.ymin()); y <= r.bb.ymax(); y++) {
            for (auto& blk : blks_at_location[x][y]) {
                // new location is the closest trimmed boundary
                int blk_new_x = dir ? x : trimmed_r, blk_new_y = dir ? trimmed_r : y;
                ap->blk_locs[blk].rawx = blk_new_x;
                ap->blk_locs[blk].rawy = blk_new_y;
                ap->blk_locs[blk].loc.x = blk_new_x;
                ap->blk_locs[blk].loc.y = blk_new_y;
                blks_at_location[blk_new_x][blk_new_y].push_back(blk);
            }
            blks_at_location[x][y].clear(); // clear blocks at old location
        }
    }

    return {trimmed_l, trimmed_r};
}

/*
 * generate the initial source_cut for region r, ensure there is enough clearance on either side of the
 * initial cut to accommodate macros
 * returns the initial source cut (index into cut_blks)
 * returns the clearance in clearance_l, clearance_r
 * returns -1 if cannot generate initial source_cut (not enough clearance for macros)
 *
 * see CutSpreader::cut_region() invocation of initial_source_cut for more detail
 */
int CutSpreader::initial_source_cut(SpreaderRegion& r,
                                    std::vector<ClusterBlockId>& cut_blks,
                                    bool dir,
                                    int& clearance_l,
                                    int& clearance_r) {
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    // pivot is the midpoint of cut_blks in terms of total block size (counting macro members)
    // this ensures the initial partitions have similar number of blocks
    int pivot_blks = 0; // midpoint in terms of total number of blocks
    int pivot = 0;      // midpoint in terms of index of cut_blks
    for (auto& blk : cut_blks) {
        // if blk is part of macro (only macro heads in cut_blks, no macro members), add that macro's size
        pivot_blks += (imacro(blk) != NO_MACRO) ? place_ctx.pl_macros[imacro(blk)].members.size() : 1;
        if (pivot_blks >= r.n_blks / 2)
            break;
        pivot++;
    }
    if (pivot >= int(cut_blks.size()))
        pivot = int(cut_blks.size()) - 1;

    // Find clearance required on either side of the pivot
    // i.e. minimum distance from left and right bounds of region to pivot
    // (no cut within clearance to accommodate macros)
    clearance_l = 0, clearance_r = 0;
    for (size_t i = 0; i < cut_blks.size(); i++) {
        int size;
        if (blk_extents.count(cut_blks.at(i))) {
            auto& be = blk_extents[cut_blks.at(i)];
            size = dir ? (be.ymax() - be.ymin() + 1) : (be.xmax() - be.xmin() + 1);
        } else {
            size = 1;
        }
        if (int(i) < pivot)
            clearance_l = std::max(clearance_l, size);
        else
            clearance_r = std::max(clearance_r, size);
    }
    return pivot;
}

/*
 * generate the initial target_cut for region r, ensure that utilization in 2 subareas are closest possible
 * while meeting clearance requirements for macros
 * returns best target cut
 */
int CutSpreader::initial_target_cut(SpreaderRegion& r,
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
                                    int& right_tiles_n) {
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    // To achieve smallest difference in utilization, first move all tiles to right partition
    left_blks_n = 0, right_blks_n = 0;
    left_tiles_n = 0, right_tiles_n = r.n_tiles;
    // count number of blks in each partition, from initial source cut
    for (int i = 0; i <= init_source_cut; i++)
        left_blks_n += (imacro(cut_blks.at(i)) != NO_MACRO) ? place_ctx.pl_macros[imacro(cut_blks.at(i))].members.size() : 1;
    for (int i = init_source_cut + 1; i < int(cut_blks.size()); i++)
        right_blks_n += (imacro(cut_blks.at(i)) != NO_MACRO) ? place_ctx.pl_macros[imacro(cut_blks.at(i))].members.size() : 1;

    int best_tgt_cut = -1;
    double best_deltaU = std::numeric_limits<double>::max();

    // sweep source cut from left to right, moving tiles from right partition to the left
    // calculate the difference in utilization for all target cuts, return the best result
    for (int i = trimmed_l; i <= trimmed_r; i++) {
        int slither_tiles = 0;
        for (int j = dir ? r.bb.xmin() : r.bb.ymin(); j <= (dir ? r.bb.xmax() : r.bb.ymax()); j++) {
            slither_tiles += dir ? tiles_at(j, i) : tiles_at(i, j);
        }

        left_tiles_n += slither_tiles;
        right_tiles_n -= slither_tiles;

        if (((i - trimmed_l) + 1) >= clearance_l && ((trimmed_r - i) + 1) >= clearance_r) {
            // if solution accommodates macro clearances
            // compare difference in utilization
            double tmpU = std::abs(double(left_blks_n) / double(std::max(left_tiles_n, 1)) - double(right_blks_n) / double(std::max(right_tiles_n, 1)));
            if (tmpU < best_deltaU) {
                best_deltaU = tmpU;
                best_tgt_cut = i;
            }
        }
    }

    if (best_tgt_cut == -1) // failed clearance requirement for macros
        return best_tgt_cut;

    // update number of tiles for each subarea
    left_tiles_n = 0, right_tiles_n = 0;
    for (int x = r.bb.xmin(); x <= (dir ? r.bb.xmax() : best_tgt_cut); x++)
        for (int y = r.bb.ymin(); y <= (dir ? best_tgt_cut : r.bb.ymax()); y++)
            left_tiles_n += tiles_at(x, y);
    for (int x = dir ? r.bb.xmin() : (best_tgt_cut + 1); x <= r.bb.xmax(); x++)
        for (int y = dir ? (best_tgt_cut + 1) : r.bb.ymin(); y <= r.bb.ymax(); y++)
            right_tiles_n += tiles_at(x, y);

    if (left_tiles_n == 0 || right_tiles_n == 0)
        // target cut failed since all tiles are still in one subarea
        return -1;

    return best_tgt_cut;
}

/*
 * Spread blocks in subarea by linear interpolation
 * blks_start and blks_end are indices into cut_blks. The blks between these indices will be spread by:
 * * first split the subarea boundaries (area_l and area_r)
 *   into min(number_of_logic_blocks_in_subarea, 10) number of bins.
 * * split the logic blocks into the corresponding number of groups
 * * place the logic blocks from their group to their bin, by linear interpolation using their original
 *   locations to map to a new location in the bin.
 */
void CutSpreader::linear_spread_subarea(std::vector<ClusterBlockId>& cut_blks,
                                        bool dir,
                                        int blks_start,
                                        int blks_end,
                                        SpreaderRegion& sub_area) {
    double area_l = dir ? sub_area.bb.ymin() : sub_area.bb.xmin(); // left boundary
    double area_r = dir ? sub_area.bb.ymax() : sub_area.bb.xmax(); // right boundary
    int N = blks_end - blks_start;                                 // number of logic blocks in subarea
    if (N <= 2) {                                                  // only 1 bin, skip binning and directly linear interpolate
        for (int i = blks_start; i < blks_end; i++) {
            auto& pos = dir ? ap->blk_locs[cut_blks.at(i)].rawy
                            : ap->blk_locs[cut_blks.at(i)].rawx;
            pos = area_l + (i - blks_start) * ((area_r - area_l) / N);
        }
    } else {
        // Split tiles into K bins, split blocks into K groups
        // Since cut_blks are sorted, to specify block groups, only need the index of the left and right block
        // Each block group has its original left and right bounds, the goal is to map this group's bound into
        // bin's bounds, and assign new locations to blocks using linear interpolation
        int K = std::min<int>(N, 10);                   // number of bins/groups
        std::vector<std::pair<int, double>> bin_bounds; // (0-th group's first block, 0-th bin's left bound)
        bin_bounds.emplace_back(blks_start, area_l);
        for (int i_bin = 1; i_bin < K; i_bin++)
            // find i-th group's first block, i-th bin's left bound
            bin_bounds.emplace_back(blks_start + (N * i_bin) / K, area_l + ((area_r - area_l + 0.99) * i_bin) / K);
        bin_bounds.emplace_back(blks_end, area_r + 0.99); // find K-th group's last block, K-th bin's right bound
        for (int i_bin = 0; i_bin < K; i_bin++) {
            auto &bl = bin_bounds.at(i_bin), br = bin_bounds.at(i_bin + 1); // i-th bin's left and right bound
            // i-th group's original bounds (left and right most block's original location)
            double group_left = dir ? ap->blk_locs[cut_blks.at(bl.first)].rawy
                                    : ap->blk_locs[cut_blks.at(bl.first)].rawx;
            double group_right = dir ? ap->blk_locs[cut_blks.at(br.first - 1)].rawy
                                     : ap->blk_locs[cut_blks.at(br.first - 1)].rawx;
            double bin_left = bl.second;
            double bin_right = br.second;
            // mapping from i-th block group's original bounds to i-th bin's bounds
            double mapping = (bin_right - bin_left) / std::max(0.00001, group_right - group_left); // prevent division by 0
            // map blks in i-th group to new location in i-th bin using linear interpolation
            for (int i_blk = bl.first; i_blk < br.first; i_blk++) {
                // new location is stored back into rawx/rawy
                auto& blk_pos = dir ? ap->blk_locs[cut_blks.at(i_blk)].rawy
                                    : ap->blk_locs[cut_blks.at(i_blk)].rawx;
                VTR_ASSERT(blk_pos >= group_left && blk_pos <= group_right);
                blk_pos = bin_left + mapping * (blk_pos - group_left); // linear interpolation
            }
        }
    }

    // Update blks_at_location for each block with their new location
    for (int x = sub_area.bb.xmin(); x <= sub_area.bb.xmax(); x++)
        for (int y = sub_area.bb.ymin(); y <= sub_area.bb.ymax(); y++) {
            blks_at_location[x][y].clear();
        }
    for (int i_blk = blks_start; i_blk < blks_end; i_blk++) {
        auto& bl = ap->blk_locs[cut_blks[i_blk]];
        bl.loc.x = std::min(sub_area.bb.xmax(), std::max(sub_area.bb.xmin(), int(bl.rawx)));
        bl.loc.y = std::min(sub_area.bb.ymax(), std::max(sub_area.bb.ymin(), int(bl.rawy)));
        blks_at_location[bl.loc.x][bl.loc.y].push_back(cut_blks[i_blk]);
    }
}

/*
 * @brief: 	Greedy strict legalize using algorithm described in algorithm overview above.
 *
 * 			Input illegal placement from data members (blk_locs) in analytic_placer
 *
 * @return: both ap->blk_locs and vpr_ctx.mutable_placement() are modified with legal placement,
 * 			to be used in next solve/spread/legalize iteration or to pass back to annealer.
 */
void CutSpreader::strict_legalize() {
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();

    // clear the location of all blocks in place_ctx
    for (auto blk : clb_nlist.blocks()) {
        if (!place_ctx.block_locs[blk].is_fixed && (ap->row_num[blk] != DONT_SOLVE || (imacro(blk) != NO_MACRO && ap->row_num[macro_head(blk)] != DONT_SOLVE))) {
            unbind_tile(place_ctx.block_locs[blk].loc);
        }
    }

    // Greedy largest-macro-first approach
    // put all blocks being placed in current AP in priority_queue "remaining" with the priority being the
    // length of the macro they are in (for single blocks, priority = 1).
    // This prioritizes the placement of longest macros over single blocks
    std::priority_queue<std::pair<int, ClusterBlockId>> remaining;
    for (auto blk : ap->solve_blks) {
        if (imacro(blk) != NO_MACRO) // blk is head block of a macro (only head blks are solved)
            remaining.emplace(place_ctx.pl_macros[imacro(blk)].members.size(), blk);
        else
            remaining.emplace(1, blk);
    }

    /*
     * ripup_radius determines at which point already placed single logic blocks will be "ripped up" for placement of
     * the current block. Specifically, when radius of random selection (determined by availability of compatible sub_tiles)
     * is larger than ripup_radius, occupied sub_tiles are also considered for blk's placement (not just unoccupied sub_tiles).
     *
     * Therefore, a small ripup_radius honors the current location of blk (from spreading) more, as it allows placement at
     * occupied sub_tiles when random selection radius around current location is still small. When ripup_radius is large,
     * blk can only search unoccupied sub_tiles in a large area before it can rip up placed blks. This will make blk more likely
     * to stray far from current location.
     *
     * ripup_radius is doubled every time outer while-loop executes (ap->solve_blks.size()) times,
     * i.e. after trying to place each block once, if there's still block to place (some block displaced/ripped up other blocks),
     * ripup_radius is doubled, allowing these ripped up blocks to look for unoccupied sub_tiles in a larger area.
     *
     * Only applies for single blocks
     */
    int ripup_radius = 2;
    // num of iters of outer most while loop, cleared when it equals the number of blocks that needs to be place for this
    // build-solve-legalize iteration. When cleared, ripup_radius is doubled.
    int total_iters = 0;
    // total_iters without clearing, used for time-out
    int total_iters_noreset = 0;

    // outer while loop, each loop iteration aims to place one solve_blk (either single blk or head blk of a macro)
    while (!remaining.empty()) {
        auto top = remaining.top();
        remaining.pop();
        ClusterBlockId blk = top.second;

        if (is_placed(blk)) // ignore if already placed
            continue;

        int radius = 0; // radius of 0 means initial candidate location is the current location of blk after spreading
        int iter = 0;   // iterations of the inner while-loop, used for timeout

        /*
         * iter_at_radius: number of inner-loop iterations (number of proposed candidate locations) at current radius
         * used to determine whether to explore more candidate locations (iter_at_radius < explore limit)
         * or take the current best_subtile for blk
         *
         * only applies for single blocks
         */
        int iter_at_radius = 0;
        bool placed = false;                                // flag for inner-loop
        t_pl_loc best_subtile = t_pl_loc{};                 // current best candidate with smallest best_inp_len, only for single blocks
        int best_inp_len = std::numeric_limits<int>::max(); // used to choose best_subtile, only for single blocks

        total_iters++;
        total_iters_noreset++;

        // clear total_iters and double ripup_radius when all solve_blks have been attempted to place once
        if (total_iters > int(ap->solve_blks.size())) {
            total_iters = 0;
            ripup_radius = std::min(std::max(max_x - 1, max_y - 1), ripup_radius * 2);
        }

        // timeout
        VTR_ASSERT(total_iters_noreset <= std::max(5000, 8 * int(clb_nlist.blocks().size())));

        while (!placed) { // while blk is not placed
            // timeout
            VTR_ASSERT(iter <= std::max(10000, 3 * int(clb_nlist.blocks().size())));

            // randomly choose a location within radius around current location (given by spreading)
            int nx = rand() % (2 * radius + 1) + std::max(ap->blk_locs[blk].loc.x - radius, 0);
            int ny = rand() % (2 * radius + 1) + std::max(ap->blk_locs[blk].loc.y - radius, 0);

            iter++;
            iter_at_radius++;
            if (iter >= (10 * (radius + 1))) { // a heuristic to determine when to increase radius
                // check if there's sub_tiles of right type within radius.
                // If no, increase radius until at least 1 compatible sub_tile is found
                radius = std::min(std::max(max_x - 1, max_y - 1), radius + 1);
                while (radius < std::max(max_x - 1, max_y - 1)) {
                    // search every location within radius for compatible sub_tiles
                    for (int x = std::max(0, ap->blk_locs[blk].loc.x - radius);
                         x <= std::min(max_x - 1, ap->blk_locs[blk].loc.x + radius);
                         x++) {
                        for (int y = std::max(0, ap->blk_locs[blk].loc.y - radius);
                             y <= std::min(max_y - 1, ap->blk_locs[blk].loc.y + radius);
                             y++) {
                            if (subtiles_at_location[x][y].size() > 0) // compatible sub_tiles found within radius
                                goto notempty;
                        }
                    }
                    // no sub_tiles found, increase radius
                    radius = std::min(std::max(max_x - 1, max_y - 1), radius + 1);
                }
            notempty:
                iter_at_radius = 0;
                iter = 0;
            }

            if (nx < 0 || nx >= max_x || ny < 0 || ny >= max_y || subtiles_at_location[nx][ny].empty())
                // try another random location if candidate location is illegal or has no sub_tiles
                continue;

            /*
             * explore_limit determines when to stop exploring for better sub_tiles for blk
             * When explore_limit is not met (iter_at_radius < explore_limit), each candidate sub_tile is evaluated based on
             * their resulting total input wirelength (a heuristic) for blk.
             * When explore_limit is met and a best_sub_tile is found, blk is placed there.
             *
             * Only applies for single blocks
             * @see comments for try_place_blk()
             */
            int explore_limit = 2 * radius;

            // if blk is not a macro member
            if (imacro(blk) == NO_MACRO) {
                placed = try_place_blk(blk,
                                       nx,
                                       ny,
                                       radius > ripup_radius,           // bool ripup_radius_met
                                       iter_at_radius >= explore_limit, // bool exceeds_explore_limit
                                       best_inp_len,
                                       best_subtile,
                                       remaining);
            } else {
                placed = try_place_macro(blk,
                                         nx,
                                         ny,
                                         remaining);
            }
        }
    }
}

/*
 * Helper function in strict_legalize()
 * Place blk on sub_tile location by modifying place_ctx.grid_blocks, place_ctx.block_locs, and ap->blk_locs[blk].loc
 */
void CutSpreader::bind_tile(t_pl_loc sub_tile, ClusterBlockId blk) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    VTR_ASSERT(place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] == EMPTY_BLOCK_ID);
    VTR_ASSERT(place_ctx.block_locs[blk].is_fixed == false);
    place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] = blk;
    place_ctx.block_locs[blk].loc = sub_tile;
    place_ctx.grid_blocks[sub_tile.x][sub_tile.y].usage++;
    ap->blk_locs[blk].loc = sub_tile;
}

/*
 * Helper function in strict_legalize()
 * Remove placement at sub_tile location by clearing place_ctx.block_locs and place_Ctx.grid_blocks
 */
void CutSpreader::unbind_tile(t_pl_loc sub_tile) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    VTR_ASSERT(place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] != EMPTY_BLOCK_ID);
    ClusterBlockId blk = place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile];
    VTR_ASSERT(place_ctx.block_locs[blk].is_fixed == false);
    place_ctx.block_locs[blk].loc = t_pl_loc{};
    place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] = EMPTY_BLOCK_ID;
    place_ctx.grid_blocks[sub_tile.x][sub_tile.y].usage--;
}

/*
 * Helper function in strict_legalze()
 * Check if the block is placed in place_ctx (place_ctx.block_locs[blk] has a location that matches
 * the block in place_ctx.grid_blocks)
 */
bool CutSpreader::is_placed(ClusterBlockId blk) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    if (place_ctx.block_locs[blk].loc != t_pl_loc{}) {
        auto loc = place_ctx.block_locs[blk].loc;
        VTR_ASSERT(place_ctx.grid_blocks[loc.x][loc.y].blocks[loc.sub_tile] == blk);
        return true;
    }
    return false;
}

/*
 * Sub-routine of strict_legalize()
 * Tries to place a single block "blk" at a candidate location nx, ny. Returns whether the blk is successfully placed.
 *
 * If number of iterations at current radius has exceeded the exploration limit (exceeds_explore_limit),
 * and a candidate sub_tile is already found (best_subtile), then candidate location is ignored, and blk is
 * placed in best_subtile.
 *
 * Else, if exploration limit is not exceeded, the sub_tiles at nx, ny are evaluated on the blk's resulting total
 * input wirelength (a heuristic). If this total input wirelength is shorter than current best_inp_len, it becomes
 * the new best_subtile.
 * If exploration limit is exceeded and no candidate sub_tile is available in (best_subtile), then blk is placed at
 * next compatible sub_tile at candidate location nx, ny.
 *
 * If blk displaces a logic block by taking its sub_tile, the displaced logic block is put back into remaining queue.
 */
bool CutSpreader::try_place_blk(ClusterBlockId blk,
                                int nx,
                                int ny,
                                bool ripup_radius_met,
                                bool exceeds_explore_limit,
                                int& best_inp_len,
                                t_pl_loc& best_subtile,
                                std::priority_queue<std::pair<int, ClusterBlockId>>& remaining) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    // iteration at current radius has exceed exploration limit, and a candidate sub_tile (best_subtile) is found
    // then blk is placed in best_subtile
    if (exceeds_explore_limit && best_subtile != t_pl_loc{}) {
        // find the logic block bound to (placed on) best_subtile
        ClusterBlockId bound_blk = place_ctx.grid_blocks[best_subtile.x][best_subtile.y].blocks[best_subtile.sub_tile];
        if (bound_blk != EMPTY_BLOCK_ID) {   // if best_subtile has a logic block
            unbind_tile(best_subtile);       // clear bound_block and best_subtile's placement info
            remaining.emplace(1, bound_blk); // put bound_blk back into remaining blocks to place
        }
        bind_tile(best_subtile, blk); // place blk on best_subtile
        return true;
    }

    // if exploration limit is not met or a candidate sub_tile is not found yet
    for (auto sub_t : subtiles_at_location[nx][ny]) {                                              // for each available sub_tile at random location
        ClusterBlockId bound_blk = place_ctx.grid_blocks[sub_t.x][sub_t.y].blocks[sub_t.sub_tile]; // logic blk at [nx, ny]
        if (bound_blk == EMPTY_BLOCK_ID
            || ripup_radius_met
            || rand() % (20000) < 10) {
            /* conditions when a sub_tile at nx, ny is considered:
             * - sub_tile is not occupied (no bound_blk)
             * - occupied sub_tile is considered when:
             *     1) current radius > ripup-radius. (see strict_legalize() for more details)
             *     OR
             *     2) a 0.05% chance of acceptance.
             */
            if (bound_blk != EMPTY_BLOCK_ID && imacro(bound_blk) != NO_MACRO)
                // do not sub_tiles when the block placed on it is part of a macro, as they have higher priority
                continue;
            if (!exceeds_explore_limit) { // if still in exploration phase, find best_subtile with smallest best_inp_len
                int input_len = 0;
                // find all input pins and add up input wirelength
                for (auto pin : clb_nlist.block_input_pins(blk)) {
                    ClusterNetId net = clb_nlist.pin_net(pin);
                    if (net == ClusterNetId::INVALID()
                        || clb_nlist.net_is_ignored(net)
                        || clb_nlist.net_driver(net) == ClusterPinId::INVALID())
                        continue;
                    ClusterBlockId driver = clb_nlist.pin_block(clb_nlist.net_driver(net));
                    auto driver_loc = ap->blk_locs[driver].loc;
                    input_len += std::abs(driver_loc.x - nx) + std::abs(driver_loc.y - ny);
                }
                if (input_len < best_inp_len) {
                    // update best_subtile
                    best_inp_len = input_len;
                    best_subtile = sub_t;
                }
                break;
            } else { // exploration phase passed and still no best_subtile yet, choose the next compatible sub_tile
                if (bound_blk != EMPTY_BLOCK_ID) {
                    remaining.emplace(1, bound_blk);
                    unbind_tile(sub_t); // remove bound_blk and place blk on sub_t
                }
                bind_tile(sub_t, blk);
                return true;
            }
        }
    }
    return false;
}

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
bool CutSpreader::try_place_macro(ClusterBlockId blk,
                                  int nx,
                                  int ny,
                                  std::priority_queue<std::pair<int, ClusterBlockId>>& remaining) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    for (auto sub_t : subtiles_at_location[nx][ny]) {
        std::vector<std::pair<ClusterBlockId, t_pl_loc>> targets; // contains the target placement location for each macro block
        std::queue<std::pair<ClusterBlockId, t_pl_loc>> visit;    // visit goes through all macro members once
        visit.emplace(blk, sub_t);                                // push head block and target sub_tile first
        bool placement_impossible = false;                        // once set to true, break while loop and try next sub_t
        while (!visit.empty()) {                                  // go through every macro block
            ClusterBlockId visit_blk = visit.front().first;
            VTR_ASSERT(!is_placed(visit_blk));
            t_pl_loc target = visit.front().second; // target location
            visit.pop();

            // ensure the target location has compatible tile
            auto blk_t = clb_nlist.block_type(blk);
            auto result = std::find(blk_t->equivalent_tiles.begin(), blk_t->equivalent_tiles.end(), g_vpr_ctx.device().grid[target.x][target.y].type);
            if (result == blk_t->equivalent_tiles.end()) {
                placement_impossible = true;
                break;
            }

            // if the target location has a logic block, ensure it's not part of a macro
            // because a macro placed before the current one has higher priority (longer chain)
            ClusterBlockId bound = place_ctx.grid_blocks[target.x][target.y].blocks[target.sub_tile];
            if (bound != EMPTY_BLOCK_ID && imacro(bound) != NO_MACRO) {
                placement_impossible = true;
                break;
            }
            // place macro block into target vector along with its target location
            targets.emplace_back(visit_blk, target);
            if (macro_head(visit_blk) == visit_blk) { // if visit_blk is the head block of the macro
                // push all macro members to visit queue along with their calculated positions
                const std::vector<t_pl_macro_member>& members = place_ctx.pl_macros[imacro(blk)].members;
                for (auto member = members.begin() + 1; member != members.end(); ++member) {
                    t_pl_loc mloc = target + member->offset; // calculate member_loc using (head blk location + offset)
                    visit.emplace(member->blk_index, mloc);
                }
            }
        }

        if (!placement_impossible) { // if placement is possible, apply this placement
            for (auto& target : targets) {
                ClusterBlockId bound = place_ctx.grid_blocks[target.second.x][target.second.y].blocks[target.second.sub_tile];
                if (bound != EMPTY_BLOCK_ID) {
                    // if target location has a logic block, displace it and put it in remaining queue to be placed later
                    unbind_tile(target.second);
                    remaining.emplace(1, bound);
                }
                bind_tile(target.second, target.first);
            }
            return true;
        }
    }
    return false;
}

#endif /* ENABLE_ANALYTIC_PLACE */
