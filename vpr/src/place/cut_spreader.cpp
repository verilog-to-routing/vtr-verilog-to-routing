#include "cut_spreader.h"
#include <iostream>
#include <vector>
#include <queue>
#include <cstdlib>

#include "analytic_placer.h"
#include "vpr_types.h"
#include "vtr_time.h"
#include "globals.h"
#include "vtr_log.h"

CutSpreader::CutSpreader(AnalyticPlacer* analytic_placer, VprContext& vpr_ctx, t_logical_block_type_ptr cell_t)
    : ap(analytic_placer)
    , device_ctx(vpr_ctx.device())
    , clb_nlist(vpr_ctx.clustering().clb_nlist)
    , place_ctx(vpr_ctx.mutable_placement())
    , blk_type(cell_t) {
    // get "fast_tile" for blk_type, quick look up by x, y coords
    ft.resize(ap->max_x, std::vector<std::vector<t_pl_loc>>(ap->max_y));
    for (auto& tile : blk_type->equivalent_tiles) {
        for (auto sub_tile : tile->sub_tiles) {
            auto result = std::find(sub_tile.equivalent_sites.begin(), sub_tile.equivalent_sites.end(), blk_type);
            if (result != sub_tile.equivalent_sites.end()) {
                for (auto loc : ap->legal_pos.at(tile->index).at(sub_tile.index)) {
                    ft.at(loc.x).at(loc.y).emplace_back(loc);
                }
            }
        }
    }
}

void CutSpreader::run() {
    init();
    find_overused_regions();
    expand_regions();
    std::queue<std::pair<int, bool>> workqueue;

    for (auto& r : regions) {
        if (merged_regions.count(r.id)) continue;
        workqueue.emplace(r.id, false);
    }
    while (!workqueue.empty()) {
        auto front = workqueue.front();
        workqueue.pop();
        auto& r = regions.at(front.first);
        // if (r.n_blks < 2) continue;

        auto res = cut_region(r, front.second);
        if (res == std::pair<int, int>{-2, -2}) continue;
        if (res != std::pair<int, int>{-1, -1}) {
            workqueue.emplace(res.first, !front.second);
            workqueue.emplace(res.second, !front.second);
        } else {
            auto res2 = cut_region(r, !front.second);
            if (res2 != std::pair<int, int>{-1, -1}) {
                workqueue.emplace(res2.first, front.second);
                workqueue.emplace(res2.second, front.second);
            }
        }
    }
}

// setup CutSpreader data structures using information from AnalyticPlacer
void CutSpreader::init() {
    occupancy.resize(ap->max_x, std::vector<int>(ap->max_y, 0));
    macros.resize(ap->max_x, std::vector<MacroExtent>(ap->max_y));
    groups.resize(ap->max_x, std::vector<int>(ap->max_y, -1));
    blks_at_location.resize(ap->max_x, std::vector<std::vector<ClusterBlockId>>(ap->max_y));

    // Initialize occupancy matrix and macros matrix
    for (int x = 0; x < ap->max_x; x++)
        for (int y = 0; y < ap->max_y; y++) {
            occupancy.at(x).at(y) = 0;
            groups.at(x).at(y) = -1;
            macros.at(x).at(y) = {x, y, x, y};
        }

    auto set_macro_ext = [&](ClusterBlockId blk, int x, int y) {
        if (!blk_extents.count(blk))
            blk_extents[blk] = {x, y, x, y};
        else {
            blk_extents[blk].x0 = std::min(blk_extents[blk].x0, x);
            blk_extents[blk].y0 = std::min(blk_extents[blk].y0, y);
            blk_extents[blk].x1 = std::max(blk_extents[blk].x1, x);
            blk_extents[blk].y1 = std::max(blk_extents[blk].y1, y);
        }
    };

    for (auto& blk : ap->blk_locs) {
        if (ap->blk_info[blk.first].type != blk_type)
            continue;
        occupancy.at(blk.second.loc.x).at(blk.second.loc.y)++;
        // compute extent of chain member
        if (ap->blk_info[blk.first].macroInfo) // if blk is a macro member
            set_macro_ext(ap->macro_blks[blk.first].macro_head->blkId, blk.second.loc.x, blk.second.loc.y);
    }

    for (auto& blk : ap->blk_locs) {
        if (ap->blk_info[blk.first].type != blk_type) continue;
        // Transfer macro extents to the actual macros structure;
        MacroExtent* me = nullptr;
        if (ap->blk_info[blk.first].macroInfo) {
            me = &(blk_extents.at(ap->macro_blks[blk.first].macro_head->blkId));
            auto& lme = macros.at(blk.second.loc.x).at(blk.second.loc.y);
            lme.x0 = std::min(lme.x0, me->x0);
            lme.y0 = std::min(lme.y0, me->y0);
            lme.x1 = std::max(lme.x1, me->x1);
            lme.y1 = std::max(lme.y1, me->y1);
        }
    }

    for (auto blk : ap->solve_blks) {
        if (ap->blk_info[blk].type != blk_type) continue;
        blks_at_location.at(ap->blk_locs.at(blk).loc.x).at(ap->blk_locs.at(blk).loc.y).push_back(blk);
    }
}

int CutSpreader::occ_at(int x, int y) { return occupancy.at(x).at(y); }

int CutSpreader::tiles_at(int x, int y) {
    if (x >= ap->max_x || y >= ap->max_y) return 0;
    return int(ft.at(x).at(y).size());
}

/*
 * Merges 2 regions by:
 * change group id at mergee grids to merged id
 * adds all n_blks and n_tiles from mergee to merged region
 * grow merged to include all mergee grids
 */
void CutSpreader::merge_regions(SpreaderRegion& merged, SpreaderRegion& mergee) {
    for (int x = mergee.x0; x <= mergee.x1; x++)
        for (int y = mergee.y0; y <= mergee.y1; y++) {
            VTR_ASSERT(groups.at(x).at(y) == mergee.id);
            groups.at(x).at(y) = merged.id;
            merged.n_blks += occ_at(x, y);
            merged.n_tiles += tiles_at(x, y);
        }
    merged_regions.insert(mergee.id);
    grow_region(merged, mergee.x0, mergee.y0, mergee.x1, mergee.y1);
}

/*
 * grow r to include a rectangular region (x0, y0, x1, y1)
 */
void CutSpreader::grow_region(SpreaderRegion& r, int x0, int y0, int x1, int y1, bool init) {
    // when given location is whithin SpreaderRegion
    if ((x0 >= r.x0 && y0 >= r.y0 && x1 <= r.x1 && y1 <= r.y1) && !init) return;

    int old_x0 = r.x0 + (init ? 1 : 0), old_y0 = r.y0, old_x1 = r.x1, old_y1 = r.y1;
    r.x0 = std::min(r.x0, x0);
    r.y0 = std::min(r.y0, y0);
    r.x1 = std::max(r.x1, x1);
    r.y1 = std::max(r.y1, y1);

    auto process_location = [&](int x, int y) {
        // kicks in only when grid is not claimed, claimed by another region, or part of a macro
        // Merge with any overlapping regions

        if (groups.at(x).at(y) == -1) {
            r.n_tiles += tiles_at(x, y);
            r.n_blks += occ_at(x, y);
        }
        if (groups.at(x).at(y) != -1 && groups.at(x).at(y) != r.id)
            merge_regions(r, regions.at(groups.at(x).at(y)));
        groups.at(x).at(y) = r.id;
        // Grow to cover any macros
        auto& macro = macros.at(x).at(y);
        grow_region(r, macro.x0, macro.y0, macro.x1, macro.y1);
    };
    // avoid double counting old region
    for (int x = r.x0; x < old_x0; x++)
        for (int y = r.y0; y <= r.y1; y++)
            process_location(x, y);
    for (int x = old_x1 + 1; x <= x1; x++)
        for (int y = r.y0; y <= r.y1; y++)
            process_location(x, y);
    for (int y = r.y0; y < old_y0; y++)
        for (int x = r.x0; x <= r.x1; x++)
            process_location(x, y);
    for (int y = old_y1 + 1; y <= r.y1; y++)
        for (int x = r.x0; x <= r.x1; x++)
            process_location(x, y);
}

// Find overutilized regions surrounded by non-overutilized regions
void CutSpreader::find_overused_regions() {
    for (int x = 0; x < ap->max_x; x++)
        for (int y = 0; y < ap->max_y; y++) {
            // Either already in a group, or not overutilized.

            if (groups.at(x).at(y) != -1) continue;
            bool overutilized = false;
            if (occ_at(x, y) > tiles_at(x, y)) overutilized = true;

            if (!overutilized) continue;

            int id = int(regions.size());
            groups.at(x).at(y) = id;
            SpreaderRegion reg;
            reg.id = id;
            reg.x0 = reg.x1 = x;
            reg.y0 = reg.y1 = y;
            reg.n_tiles = reg.n_blks = 0;
            reg.n_tiles += tiles_at(x, y);
            reg.n_blks += occ_at(x, y);

            // make sure it covers carries
            grow_region(reg, reg.x0, reg.y0, reg.x1, reg.y1, true);

            bool expanded = true;
            while (expanded) {
                expanded = false;
                // keep trying expansion in x and y, until find no over-occupancy blks
                // or hit grouped blks

                // try expanding in x
                if (reg.x1 < ap->max_x - 1) {
                    bool over_occ_x = false;
                    for (int y1 = reg.y0; y1 <= reg.y1; y1++) {
                        if (occ_at(reg.x1 + 1, y1) > tiles_at(reg.x1 + 1, y1)) {
                            over_occ_x = true;

                            break;
                        }
                    }
                    if (over_occ_x) {
                        expanded = true;
                        grow_region(reg, reg.x0, reg.y0, reg.x1 + 1, reg.y1);
                    }
                }
                // try expanding in y
                if (reg.y1 < ap->max_y - 1) {
                    bool over_occ_y = false;
                    for (int x1 = reg.x0; x1 <= reg.x1; x1++) {
                        if (occ_at(x1, reg.y1 + 1) > tiles_at(x1, reg.y1 + 1)) {
                            over_occ_y = true;
                            break;
                        }
                    }
                    if (over_occ_y) {
                        expanded = true;
                        grow_region(reg, reg.x0, reg.y0, reg.x1, reg.y1 + 1);
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
    std::queue<int> overused_regions;
    float beta = ap->tmpCfg.beta;
    for (auto& r : regions)
        // if region is not merged and overused
        if (!merged_regions.count(r.id) && r.overused(beta)) overused_regions.push(r.id);

    while (!overused_regions.empty()) {
        int rid = overused_regions.front();
        overused_regions.pop();
        if (merged_regions.count(rid)) continue;
        auto& reg = regions.at(rid);
        while (reg.overused(beta)) {
            bool changed = false;
            for (int j = 0; j < ap->tmpCfg.spread_scale_x; j++) {
                if (reg.x0 > 0) {
                    grow_region(reg, reg.x0 - 1, reg.y0, reg.x1, reg.y1);
                    changed = true;
                    if (!reg.overused(beta)) break;
                }
                if (reg.x1 < ap->max_x - 1) {
                    grow_region(reg, reg.x0, reg.y0, reg.x1 + 1, reg.y1);
                    changed = true;
                    if (!reg.overused(beta)) break;
                }
            }
            // if (!reg.overused(beta)) break;
            for (int j = 0; j < ap->tmpCfg.spread_scale_y; j++) {
                if (reg.y0 > 0) {
                    grow_region(reg, reg.x0, reg.y0 - 1, reg.x1, reg.y1);
                    changed = true;
                    if (!reg.overused(beta))
                        break;
                }
                if (reg.y1 < ap->max_y - 1) {
                    grow_region(reg, reg.x0, reg.y0, reg.x1, reg.y1 + 1);
                    changed = true;
                    if (!reg.overused(beta)) break;
                }
            }
            VTR_ASSERT(changed || reg.n_tiles >= reg.n_blks);
        }
        VTR_ASSERT(reg.n_blks <= reg.n_tiles);
    }
}

// Recursive cut-based spreading in HeAP paper
// "left" denotes "-x, -y", "right" denotes "+x, +y" depending on dir
std::pair<int, int> CutSpreader::cut_region(SpreaderRegion& r, bool dir) {
    cut_blks.clear();
    int total_blks = 0, total_tiles = 0;
    for (int x = r.x0; x <= r.x1; x++) {
        for (int y = r.y0; y <= r.y1; y++) {
            std::copy(blks_at_location.at(x).at(y).begin(), blks_at_location.at(x).at(y).end(), std::back_inserter(cut_blks));
            total_tiles += tiles_at(x, y);
        }
    }

    for (auto& blk : cut_blks) {
        // blks_at_location is made from solve_blks, i.e. only the head of the macro is counted
        total_blks += (ap->blk_info[blk].macroInfo && ap->blk_info[blk].macroInfo->imember == 0) ? ap->macro_blks[blk].pl_macro->members.size() : 1;
    }

    // First trim the boundaries of the region in axis-of-interest, skipping any rows/cols without any tiles
    // of the right type
    int trimmed_l = dir ? r.y0 : r.x0, trimmed_r = dir ? r.y1 : r.x1;
    while (trimmed_l < (dir ? r.y1 : r.x1)) {
        bool have_tiles = false;
        for (int i = dir ? r.x0 : r.y0; i <= (dir ? r.x1 : r.y1); i++)
            if (tiles_at(dir ? i : trimmed_l, dir ? trimmed_l : i) > 0) {
                have_tiles = true;
                break;
            }
        if (have_tiles) break;
        trimmed_l++;
    }
    while (trimmed_r > (dir ? r.y0 : r.x0)) {
        bool have_tiles = false;
        for (int i = dir ? r.x0 : r.y0; i <= (dir ? r.x1 : r.y1); i++)
            if (tiles_at(dir ? i : trimmed_r, dir ? trimmed_r : i) > 0) {
                have_tiles = true;
                break;
            }
        if (have_tiles) break;
        trimmed_r--;
    }

    // move blocks on trimmed locations to new boundaries
    for (int x = r.x0; x < (dir ? r.x1 + 1 : trimmed_l); x++)
        for (int y = r.y0; y < (dir ? trimmed_l : r.y1 + 1); y++) {
            for (auto& blk : blks_at_location.at(x).at(y)) {
                ap->blk_locs.at(blk).rawx = dir ? x : trimmed_l;
                ap->blk_locs.at(blk).rawy = dir ? trimmed_l : y;
                ap->blk_locs.at(blk).loc.x = dir ? x : trimmed_l;
                ap->blk_locs.at(blk).loc.y = dir ? trimmed_l : y;
                blks_at_location.at(dir ? x : trimmed_l).at(dir ? trimmed_l : y).push_back(blk);
            }
            blks_at_location.at(x).at(y).clear();
        }

    for (int x = (dir ? r.x0 : trimmed_r + 1); x <= r.x1; x++)
        for (int y = (dir ? trimmed_r + 1 : r.y0); y <= r.y1; y++) {
            for (auto& blk : blks_at_location.at(x).at(y)) {
                ap->blk_locs.at(blk).rawx = dir ? x : trimmed_r;
                ap->blk_locs.at(blk).rawy = dir ? trimmed_r : y;
                ap->blk_locs.at(blk).loc.x = dir ? x : trimmed_r;
                ap->blk_locs.at(blk).loc.y = dir ? trimmed_r : y;
                blks_at_location.at(dir ? x : trimmed_r).at(dir ? trimmed_r : y).push_back(blk);
            }
            blks_at_location.at(x).at(y).clear();
        }

    // sort blks based on raw loc
    std::sort(cut_blks.begin(), cut_blks.end(), [&](const ClusterBlockId a, const ClusterBlockId b) {
        return dir ? (ap->blk_locs[a].rawy < ap->blk_locs[b].rawy) : (ap->blk_locs[a].rawx < ap->blk_locs[b].rawx);
    });

    // base case
    if (cut_blks.size() == 1) {
        // ensure placement of last block is on right type of tile
        auto blk = cut_blks.at(0);
        auto& tiles_type = ap->blk_info[blk].type->equivalent_tiles;
        auto loc = ap->blk_locs[blk].loc;
        if (std::find(tiles_type.begin(), tiles_type.end(), device_ctx.grid[loc.x][loc.y].type) != tiles_type.end())
            return {-2, -2};
        else { // exhaustive search for right tile
            for (int x = r.x0; x <= r.x1; x++)
                for (int y = r.y0; y <= r.y1; y++) {
                    if (std::find(tiles_type.begin(), tiles_type.end(), device_ctx.grid[x][y].type) != tiles_type.end()) {
                        VTR_ASSERT(blks_at_location[x][y].empty());
                        ap->blk_locs.at(blk).rawx = x;
                        ap->blk_locs.at(blk).rawy = y;
                        ap->blk_locs.at(blk).loc.x = x;
                        ap->blk_locs.at(blk).loc.y = y;
                        blks_at_location.at(x).at(y).push_back(blk);
                        blks_at_location.at(loc.x).at(loc.y).clear();
                        return {-2, -2};
                    }
                }
        }
        return {-2, -2};
    }

    // Find the blks midpoint, counting macros in terms of their total size
    // this is the initial source cut
    int pivot_blks = 0; // midpoint in terms of total number of blocks
    int pivot = 0;      // midpoint in terms of index of cut_blks
    for (auto& blk : cut_blks) {
        pivot_blks += (ap->blk_info[blk].macroInfo) ? ap->macro_blks[blk].pl_macro->members.size() : 1;
        if (pivot_blks >= total_blks / 2) break;
        pivot++;
    }
    if (pivot >= int(cut_blks.size())) pivot = int(cut_blks.size()) - 1;

    // Find clearance required on either side of the pivot
    // i.e. minimum distance from left and right bounds of region to pivot
    // (no cut within clearance because of macros)
    int clearance_l = 0, clearance_r = 0;
    for (size_t i = 0; i < cut_blks.size(); i++) {
        int size;
        if (blk_extents.count(cut_blks.at(i))) {
            auto& be = blk_extents.at(cut_blks.at(i));
            size = dir ? (be.y1 - be.y0 + 1) : (be.x1 - be.x0 + 1);
        } else {
            size = 1;
        }
        if (int(i) < pivot)
            clearance_l = std::max(clearance_l, size);
        else
            clearance_r = std::max(clearance_r, size);
    }
    // Find target cut that minimizes difference in utilization, while ensuring all macros still fit

    if ((trimmed_r - trimmed_l + 1) <= std::max(clearance_l, clearance_r)) return {-1, -1};

    // Find the initial target cut that minimizes utilization imbalance
    // while meeting the clearance requirements for macros
    int left_blks_n = 0, right_blks_n = 0;
    int left_tiles_n = 0, right_tiles_n = r.n_tiles;
    for (int i = 0; i <= pivot; i++)
        left_blks_n += ap->blk_info[cut_blks.at(i)].macroInfo ? ap->macro_blks[cut_blks.at(i)].pl_macro->members.size() : 1;
    for (int i = pivot + 1; i < int(cut_blks.size()); i++)
        right_blks_n += ap->blk_info[cut_blks.at(i)].macroInfo ? ap->macro_blks[cut_blks.at(i)].pl_macro->members.size() : 1;

    int best_tgt_cut = -1;
    double best_deltaU = std::numeric_limits<double>::max();

    for (int i = trimmed_l; i <= trimmed_r; i++) {
        int slither_tiles = 0;
        for (int j = dir ? r.x0 : r.y0; j <= (dir ? r.x1 : r.y1); j++) {
            slither_tiles += dir ? tiles_at(j, i) : tiles_at(i, j);
        }

        left_tiles_n += slither_tiles;
        right_tiles_n -= slither_tiles;

        if (((i - trimmed_l) + 1) >= clearance_l && ((trimmed_r - i) + 1) >= clearance_r) {
            // solution accommodates macro clearances
            // compare difference in utilization
            double tmpU = std::abs(double(left_blks_n) / double(std::max(left_tiles_n, 1)) - double(right_blks_n) / double(std::max(right_tiles_n, 1)));
            if (tmpU < best_deltaU) {
                best_deltaU = tmpU;
                best_tgt_cut = i;
            }
        }
    }
    if (best_tgt_cut == -1) return {-1, -1};

    left_tiles_n = 0;
    right_tiles_n = 0;

    for (int x = r.x0; x <= (dir ? r.x1 : best_tgt_cut); x++)
        for (int y = r.y0; y <= (dir ? best_tgt_cut : r.y1); y++)
            left_tiles_n += tiles_at(x, y);

    for (int x = dir ? r.x0 : (best_tgt_cut + 1); x <= r.x1; x++)
        for (int y = dir ? (best_tgt_cut + 1) : r.y0; y <= r.y1; y++)
            right_tiles_n += tiles_at(x, y);

    if (left_tiles_n == 0 || right_tiles_n == 0) return {-1, -1};

    // Perturb source cut to eliminate over-utilization
    auto is_part_overutil = [&](bool right_reg) {
        if (left_blks_n > left_tiles_n || right_blks_n > right_tiles_n) {
            double delta = double(left_blks_n) / double(std::max(left_tiles_n, 1)) - double(right_blks_n) / double(std::max(right_tiles_n, 1));
            return right_reg ? delta < 0 : delta > 0;
        }
        return false;
    };

    // while left region is over-utilized
    while (pivot > 0 && is_part_overutil(false)) {
        auto& move_blk = cut_blks.at(pivot);
        int size = ap->blk_info[move_blk].macroInfo ? ap->macro_blks[move_blk].pl_macro->members.size() : 1;
        left_blks_n -= size;
        right_blks_n += size;
        pivot--;
    }
    // while right region is over-utilized
    while (pivot < int(cut_blks.size()) - 1 && is_part_overutil(true)) {
        auto& move_blk = cut_blks.at(pivot + 1);
        int size = ap->blk_info[move_blk].macroInfo ? ap->macro_blks[move_blk].pl_macro->members.size() : 1;
        left_blks_n += size;
        right_blks_n -= size;
        pivot++;
    }

    // split regions into bins, then spread blks by linear interpolation within those bins
    auto spread_bin_linInt = [&](int blks_start, int blks_end, double area_l, double area_r) {
        int N = blks_end - blks_start;
        if (N <= 2) {
            for (int i = blks_start; i < blks_end; i++) {
                auto& pos = dir ? ap->blk_locs.at(cut_blks.at(i)).rawy
                                : ap->blk_locs.at(cut_blks.at(i)).rawx;
                pos = area_l + (i - blks_start) * ((area_r - area_l) / N);
            }
            return;
        }
        // Split region into bins
        int K = std::min<int>(N, 10);
        std::vector<std::pair<int, double>> bin_bounds; // [(blk start, area start)]
        bin_bounds.emplace_back(blks_start, area_l);
        for (int i = 1; i < K; i++)
            bin_bounds.emplace_back(blks_start + (N * i) / K, area_l + ((area_r - area_l + 0.99) * i) / K);
        bin_bounds.emplace_back(blks_end, area_r + 0.99);
        for (int i = 0; i < K; i++) {
            auto &bl = bin_bounds.at(i), br = bin_bounds.at(i + 1);
            double orig_left = dir ? ap->blk_locs[cut_blks.at(bl.first)].rawy
                                   : ap->blk_locs[cut_blks.at(bl.first)].rawx;
            double orig_right = dir ? ap->blk_locs[cut_blks.at(br.first - 1)].rawy
                                    : ap->blk_locs[cut_blks.at(br.first - 1)].rawx;
            double m = (br.second - bl.second) / std::max(0.00001, orig_right - orig_left);
            for (int j = bl.first; j < br.first; j++) {
                auto& pos = dir ? ap->blk_locs.at(cut_blks.at(j)).rawy
                                : ap->blk_locs.at(cut_blks.at(j)).rawx;
                VTR_ASSERT(pos >= orig_left && pos <= orig_right);
                pos = bl.second + m * (pos - orig_left);
            }
        }
    };
    spread_bin_linInt(0, pivot + 1, trimmed_l, best_tgt_cut);
    spread_bin_linInt(pivot + 1, int(cut_blks.size()), best_tgt_cut + 1, trimmed_r);

    // Update data structure
    for (int x = r.x0; x <= r.x1; x++)
        for (int y = r.y0; y <= r.y1; y++) {
            blks_at_location.at(x).at(y).clear();
        }
    for (auto blk : cut_blks) {
        auto& bl = ap->blk_locs.at(blk);
        bl.loc.x = std::min(r.x1, std::max(r.x0, int(bl.rawx)));
        bl.loc.y = std::min(r.y1, std::max(r.y0, int(bl.rawy)));
        blks_at_location.at(bl.loc.x).at(bl.loc.y).push_back(blk);
    }
    SpreaderRegion rl, rr;
    rl.id = int(regions.size());
    rl.x0 = dir ? r.x0 : trimmed_l;
    rl.y0 = dir ? trimmed_l : r.y0;
    rl.x1 = dir ? r.x1 : best_tgt_cut;
    rl.y1 = dir ? best_tgt_cut : r.y1;
    rl.n_blks = left_blks_n;
    rl.n_tiles = left_tiles_n;
    rr.id = int(regions.size()) + 1;
    rr.x0 = dir ? r.x0 : (best_tgt_cut + 1);
    rr.y0 = dir ? (best_tgt_cut + 1) : r.y0;
    rr.x1 = dir ? r.x1 : trimmed_r;
    rr.y1 = dir ? trimmed_r : r.y1;
    rr.n_blks = right_blks_n;
    rr.n_tiles = right_tiles_n;
    regions.push_back(rl);
    regions.push_back(rr);

    for (int x = rl.x0; x <= rl.x1; x++)
        for (int y = rl.y0; y <= rl.y1; y++)
            groups.at(x).at(y) = rl.id;
    for (int x = rr.x0; x <= rr.x1; x++)
        for (int y = rr.y0; y <= rr.y1; y++)
            groups.at(x).at(y) = rr.id;
    return std::make_pair(rl.id, rr.id);
}

/*
 * A greedy strict legalizer that honors raw locs as much as possible
 * Priorities macros over single blks
 */
void CutSpreader::strict_legalize() {
    auto bindTile = [&](t_pl_loc sub_tile, ClusterBlockId blk) {
        VTR_ASSERT(place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] == EMPTY_BLOCK_ID);
        VTR_ASSERT(place_ctx.block_locs[blk].is_fixed == false);
        place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] = blk;
        place_ctx.block_locs[blk].loc = sub_tile;
        place_ctx.grid_blocks[sub_tile.x][sub_tile.y].usage++;
    };

    auto unbindTile = [&](t_pl_loc sub_tile) {
        VTR_ASSERT(place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] != EMPTY_BLOCK_ID);
        ClusterBlockId blk = place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile];
        VTR_ASSERT(place_ctx.block_locs[blk].is_fixed == false);
        place_ctx.block_locs[blk].loc = t_pl_loc{};
        place_ctx.grid_blocks[sub_tile.x][sub_tile.y].blocks[sub_tile.sub_tile] = EMPTY_BLOCK_ID;
        place_ctx.grid_blocks[sub_tile.x][sub_tile.y].usage--;
    };

    auto isPlaced = [&](ClusterBlockId blk) {
        if (place_ctx.block_locs[blk].loc != t_pl_loc{}) {
            auto loc = place_ctx.block_locs[blk].loc;
            VTR_ASSERT(place_ctx.grid_blocks[loc.x][loc.y].blocks[loc.sub_tile] == blk);
            return true;
        }
        return false;
    };

    // unbind all tiles
    for (auto blk : clb_nlist.blocks()) {
        if (!place_ctx.block_locs[blk].is_fixed && (ap->blk_info[blk].udata != dont_solve || (ap->blk_info[blk].macroInfo && ap->macro_blks[blk].macro_head->udata != dont_solve)))
            unbindTile(place_ctx.block_locs[blk].loc);
    }

    // simple greedy largest-macro-first approach
    std::priority_queue<std::pair<int, ClusterBlockId>> remaining;
    for (auto blk : ap->solve_blks) {
        if (ap->blk_info[blk].macroInfo)
            remaining.emplace(ap->macro_blks[blk].pl_macro->members.size(), blk);
        else
            remaining.emplace(1, blk);
    }
    // increment (double) when total_iters > solve_blks.size()
    int ripup_radius = 2;
    // num of iters of outer most while loop, cleared when ripup radius gets incremented
    int total_iters = 0;
    // total_iters without clearing
    int total_iters_noreset = 0;

    while (!remaining.empty()) {
        auto top = remaining.top();
        remaining.pop();

        ClusterBlockId blk = top.second;
        // ignore if placed now
        if (isPlaced(blk)) continue;

        std::map<int, std::unordered_set<int>> tiles_map;
        for (auto& tile : ap->blk_info[blk].type->equivalent_tiles) {
            for (auto sub_tile : tile->sub_tiles) {
                auto result = std::find(sub_tile.equivalent_sites.begin(), sub_tile.equivalent_sites.end(), ap->blk_info[blk].type);
                if (result != sub_tile.equivalent_sites.end())
                    tiles_map[tile->index].insert(sub_tile.index);
            }
        }
        // move potential sub_tiles to fast_t;
        std::vector<std::vector<std::vector<t_pl_loc>>> fast_t(ap->max_x, std::vector<std::vector<t_pl_loc>>(ap->max_y, std::vector<t_pl_loc>{}));
        for (auto& tile : tiles_map) {
            for (auto& sub_tile : tile.second) {
                for (t_pl_loc& pos : ap->legal_pos.at(tile.first).at(sub_tile)) {
                    fast_t.at(pos.x).at(pos.y).emplace_back(pos);
                }
            }
        }

        int radius = 0;
        // iterations of the inner while-loop
        int iter = 0;
        int iter_at_radius = 0;
        bool placed = false;
        t_pl_loc best_subtile = t_pl_loc{};
        int best_inp_len = std::numeric_limits<int>::max();

        total_iters++;
        total_iters_noreset++;
        if (total_iters > int(ap->solve_blks.size())) {
            total_iters = 0;
            ripup_radius = std::min(std::max(ap->max_x - 1, ap->max_y - 1), ripup_radius * 2);
        }

        // timeout
        VTR_ASSERT(total_iters_noreset <= std::max(5000, 8 * int(clb_nlist.blocks().size())));

        while (!placed) {
            // timeout
            VTR_ASSERT(iter <= std::max(10000, 3 * int(clb_nlist.blocks().size())));

            int rx = radius, ry = radius;

            int nx = rand() % (2 * rx + 1) + std::max(ap->blk_locs.at(blk).loc.x - rx, 0);
            int ny = rand() % (2 * ry + 1) + std::max(ap->blk_locs.at(blk).loc.y - ry, 0);

            iter++;
            iter_at_radius++;
            if (iter >= (10 * (radius + 1))) {
                // check if there's subtile of right type within radius.
                // If no, increase radius, else reset iter and continue generating random position
                radius = std::min(std::max(ap->max_x - 1, ap->max_y - 1), radius + 1);
                while (radius < std::max(ap->max_x - 1, ap->max_y - 1)) {
                    for (int x = std::max(0, ap->blk_locs.at(blk).loc.x - radius);
                         x <= std::min(ap->max_x - 1, ap->blk_locs.at(blk).loc.x + radius); x++) {
                        if (x >= int(ap->max_x)) break;
                        for (int y = std::max(0, ap->blk_locs.at(blk).loc.y - radius);
                             y <= std::min(ap->max_y - 1, ap->blk_locs.at(blk).loc.y + radius); y++) {
                            if (y >= int(ap->max_y)) break;
                            if (fast_t.at(x).at(y).size() > 0) goto notempty;
                        }
                    }
                    radius = std::min(std::max(ap->max_x - 1, ap->max_y - 1), radius + 1);
                }
            notempty:
                iter_at_radius = 0;
                iter = 0;
            }

            if (nx < 0 || nx >= ap->max_x) continue;
            if (ny < 0 || ny >= ap->max_y) continue;
            if (fast_t.at(nx).at(ny).empty()) continue;

            int need_to_explore = 2 * radius;

            if (iter_at_radius >= need_to_explore && best_subtile != t_pl_loc{}) {
                ClusterBlockId bound = place_ctx.grid_blocks[best_subtile.x][best_subtile.y].blocks[best_subtile.sub_tile];
                if (bound != EMPTY_BLOCK_ID) {
                    unbindTile(best_subtile);
                    remaining.emplace(ap->blk_info[bound].macroInfo ? ap->macro_blks[bound].pl_macro->members.size() : 1, bound);
                }
                bindTile(best_subtile, blk);
                placed = true;
                ap->blk_locs[blk].loc = best_subtile;
                break;
            }

            // if blk is not a macro member
            if (!ap->blk_info[blk].macroInfo) {
                for (auto sub_t : fast_t.at(nx).at(ny)) {                                                // availalbe subtiles at random location
                    if (place_ctx.grid_blocks[sub_t.x][sub_t.y].blocks[sub_t.sub_tile] == EMPTY_BLOCK_ID // if sub_tile available
                        || (radius > ripup_radius || rand() % (20000) < 10)) {
                        ClusterBlockId bound = place_ctx.grid_blocks[sub_t.x][sub_t.y].blocks[sub_t.sub_tile];
                        if (bound != EMPTY_BLOCK_ID) {
                            if (ap->blk_info[bound].macroInfo) continue;
                            unbindTile(sub_t); // else unbind sub_tile from bound
                        }
                        bindTile(sub_t, blk);
                        if (iter_at_radius < need_to_explore) {
                            unbindTile(sub_t);
                            if (bound != EMPTY_BLOCK_ID) bindTile(sub_t, bound);
                            int input_len = 0;
                            for (auto pin : clb_nlist.block_input_pins(blk)) {
                                ClusterNetId net = clb_nlist.pin_net(pin);
                                if (net == ClusterNetId::INVALID() || clb_nlist.net_is_ignored(net) || clb_nlist.net_driver(net) == ClusterPinId::INVALID()) continue;
                                ClusterBlockId driver = clb_nlist.pin_block(clb_nlist.net_driver(net));
                                auto driver_loc = ap->blk_locs[driver].loc;
                                input_len += std::abs(driver_loc.x - nx) + std::abs(driver_loc.y - ny);
                            }
                            if (input_len < best_inp_len) {
                                best_inp_len = input_len;
                                best_subtile = sub_t;
                            }
                            break;
                        } else {
                            if (bound != EMPTY_BLOCK_ID)
                                remaining.emplace(ap->blk_info[bound].macroInfo ? ap->macro_blks[bound].pl_macro->members.size() : 1, bound);
                            ap->blk_locs[blk].loc = sub_t;
                            placed = true;
                            break;
                        }
                    }
                }
            } else {
                for (auto sub_t : fast_t.at(nx).at(ny)) {
                    std::vector<std::pair<ClusterBlockId, t_pl_loc>> targets;
                    std::vector<std::pair<t_pl_loc, ClusterBlockId>> swaps_made;
                    std::queue<std::pair<ClusterBlockId, t_pl_loc>> visit;
                    visit.emplace(blk, sub_t);
                    while (!visit.empty()) {
                        ClusterBlockId vb = visit.front().first;
                        VTR_ASSERT(!isPlaced(vb));
                        t_pl_loc target = visit.front().second;
                        visit.pop();
                        auto log_type = clb_nlist.block_type(blk);
                        auto result = std::find(log_type->equivalent_tiles.begin(), log_type->equivalent_tiles.end(), device_ctx.grid[target.x][target.y].type);
                        if (result == log_type->equivalent_tiles.end()) goto fail;
                        ClusterBlockId bound = place_ctx.grid_blocks[target.x][target.y].blocks[target.sub_tile];
                        if (bound != EMPTY_BLOCK_ID)
                            if (ap->blk_info[bound].macroInfo) goto fail;
                        targets.emplace_back(vb, target);
                        if (ap->macro_blks[vb].imember == 0) {
                            const std::vector<t_pl_macro_member>& members = ap->macro_blks[blk].pl_macro->members;
                            for (auto member = members.begin() + 1; member != members.end(); ++member) {
                                t_pl_loc mloc = target + member->offset;
                                visit.emplace(member->blk_index, mloc);
                            }
                        }
                    }

                    for (auto& target : targets) {
                        ClusterBlockId bound = place_ctx.grid_blocks[target.second.x][target.second.y].blocks[target.second.sub_tile];
                        if (bound != EMPTY_BLOCK_ID) unbindTile(target.second);
                        bindTile(target.second, target.first);
                        swaps_made.emplace_back(target.second, bound);
                    }

                    if (false) {
                    fail:
                        for (auto& swap : swaps_made) {
                            unbindTile(swap.first);
                            if (swap.second != EMPTY_BLOCK_ID)
                                bindTile(swap.first, swap.second);
                        }
                        continue;
                    }
                    for (auto& target : targets) {
                        ap->blk_locs[target.first].loc = target.second;
                    }
                    for (auto& swap : swaps_made) {
                        if (swap.second != EMPTY_BLOCK_ID)
                            remaining.emplace(ap->blk_info[swap.second].macroInfo ? ap->macro_blks[swap.second].pl_macro->members.size() : 1, swap.second);
                    }

                    placed = true;
                    break;
                }
            }
        }
    }
}
