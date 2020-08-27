#include "analytic_placer.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include <Eigen/Core>
#include <Eigen/IterativeLinearSolvers>
#pragma GCC diagnostic pop
#include <iostream>
#include <vector>
#include <stdint.h>

#include "vpr_types.h"
#include "vtr_time.h"
#include "read_place.h"
#include "globals.h"
#include "vtr_log.h"
#include "cut_spreader.h"
#include "vpr_utils.h"

// Templated struct for constructing and solving matrix equations in analytic placer
template<typename T>
struct EquationSystem {
    EquationSystem(size_t rows, size_t cols) {
        A.resize(cols);
        rhs.resize(rows);
    }

    // Simple sparse format, easy to convert to CCS for solver
    std::vector<std::vector<std::pair<int, T>>> A; // col -> (row, x[row, col]) sorted by row
    std::vector<T> rhs;                            // RHS vector

    void reset() {
        for (auto& col : A)
            col.clear();
        std::fill(rhs.begin(), rhs.end(), T());
    }

    void add_coeff(int row, int col, T val) {
        auto& Ac = A.at(col);
        // Binary search
        int b = 0, e = int(Ac.size()) - 1;
        while (b <= e) {
            int i = (b + e) / 2;
            if (Ac.at(i).first == row) {
                Ac.at(i).second += val;
                return;
            }
            if (Ac.at(i).first > row)
                e = i - 1;
            else
                b = i + 1;
        }
        Ac.insert(Ac.begin() + b, std::make_pair(row, val));
    }

    void add_rhs(int row, T val) { rhs[row] += val; }

    void solve(std::vector<T>& x, float tolerance) {
        using namespace Eigen;
        if (x.empty())
            return;
        VTR_ASSERT(x.size() == A.size());

        VectorXd vx(x.size()), vb(rhs.size());
        SparseMatrix<T> mat(A.size(), A.size());

        std::vector<int> colnnz;
        for (auto& Ac : A)
            colnnz.push_back(int(Ac.size()));
        mat.reserve(colnnz);
        for (int col = 0; col < int(A.size()); col++) {
            auto& Ac = A.at(col);
            for (auto& el : Ac)
                mat.insert(el.first, col) = el.second;
        }

        for (int i = 0; i < int(x.size()); i++)
            vx[i] = x.at(i);
        for (int i = 0; i < int(rhs.size()); i++)
            vb[i] = rhs.at(i);

        ConjugateGradient<SparseMatrix<T>, Lower | Upper> solver;
        solver.setTolerance(tolerance);
        VectorXd xr = solver.compute(mat).solveWithGuess(vb, vx);
        for (int i = 0; i < int(x.size()); i++)
            x.at(i) = xr[i];
    }
};

// helper function to find the index of macro that contains blk
// returns index in placementCtx.pl_macros, -1 if blk not in any macros
int imacro(ClusterBlockId blk) {
    int macro_index;
    get_imacro_from_iblk(&macro_index, blk, g_vpr_ctx.mutable_placement().pl_macros);
    return macro_index;
}

// helper fucntion to find the head of macro containing blk
// returns the ID of the head block
ClusterBlockId macro_head(ClusterBlockId blk) {
    int macro_index = imacro(blk);
    return g_vpr_ctx.mutable_placement().pl_macros[macro_index].members[0].blk_index;
}

/*
 * AnalyticPlacer constructor
 * Currently passing in block locations from initial plament,
 * device information etc. by VprContext
 */
AnalyticPlacer::AnalyticPlacer() {
    //Eigen::initParallel();

    // TODO: PlacerHeapCfg should be externally configured & supplied
    // TODO: tune these parameters for better qor
    tmpCfg.alpha = 0.1;            // anchoring strength
    tmpCfg.beta = 1;               // utilization factor
    tmpCfg.solverTolerance = 1e-5; // solver parameter
    tmpCfg.spread_scale_x = 1;     // refer to CutSpreader::expand_regions()
    tmpCfg.spread_scale_y = 1;
    tmpCfg.criticalityExponent = 1;
    tmpCfg.timingWeight = 10; // refer to AnalyticPlacer::build_equiations(), currently no timing
}

void AnalyticPlacer::ap_place() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    vtr::ScopedStartFinishTimer timer("Analytic Placement");

    init();
    build_legal_locations();
    wirelen_t hpwl = total_hpwl();
    VTR_LOG("Creating analytic placement for %d cells, random placement hpwl = %d.\n",
            int(clb_nlist.blocks().size()), int(hpwl));

    std::vector<t_logical_block_type_ptr> heap_runs;
    std::unordered_set<t_logical_block_type_ptr> all_blktypes;

    for (auto blk : place_blks) {
        if (!all_blktypes.count(clb_nlist.block_type(blk))) {
            heap_runs.push_back(clb_nlist.block_type(blk));
            all_blktypes.insert(clb_nlist.block_type(blk));
        }
    }

    for (int i = 0; i < 1; i++) {
        for (auto run : heap_runs) {
            setup_solve_blks(run);
            build_solve_direction(false, -1, 5); // can tune build_solve_iter
            build_solve_direction(true, -1, 5);
            update_macros();
        }
    }

    wirelen_t solved_hpwl = 0, spread_hpwl = 0, legal_hpwl = 0, best_hpwl = std::numeric_limits<wirelen_t>::max();
    int iter = 0, stalled = 0;
    float iter_start, iter_t, run_start, run_t, solve_t, spread_start, spread_t, legal_start, legal_t;

    print_AP_status_header();

    // main loop for AP
    while (stalled < 15) { // stopping criteria
        iter_start = timer.elapsed_sec();

        for (auto run : heap_runs) {
            run_start = timer.elapsed_sec();

            setup_solve_blks(run);
            if (solve_blks.empty()) continue;
            build_solve_direction(false, (iter == 0) ? -1 : iter, 5);
            build_solve_direction(true, (iter == 0) ? -1 : iter, 5);
            update_macros();
            solve_t = timer.elapsed_sec() - run_start;

            solved_hpwl = total_hpwl();

            spread_start = timer.elapsed_sec();
            CutSpreader spreader{this, run};
            if (strcmp(run->name, "io") != 0) {
                spreader.cutSpread();
                update_macros();
                spread_hpwl = total_hpwl();
                spread_t = timer.elapsed_sec() - spread_start;
            } else {
                spread_hpwl = -1;
                spread_t = 0;
            }

            legal_start = timer.elapsed_sec();
            spreader.strict_legalize();
            update_macros();
            legal_t = timer.elapsed_sec() - legal_start;
            legal_hpwl = total_hpwl();

            run_t = timer.elapsed_sec() - run_start;
            print_run_stats(iter, timer.elapsed_sec(), run_t, run->name, solve_blks.size(), solve_t,
                            spread_t, legal_t, solved_hpwl, spread_hpwl, legal_hpwl);
        }

        // TODO: update timing info here after timing weights are implemented in build_equations()

        if (legal_hpwl < best_hpwl) {
            best_hpwl = legal_hpwl;
            stalled = 0;
        } else {
            ++stalled;
        }
        for (auto& bl : blk_locs) {
            bl.second.legal_loc = bl.second.loc;
        }
        iter_t = timer.elapsed_sec() - iter_start;
        print_iter_stats(iter, iter_t, timer.elapsed_sec(), best_hpwl, stalled);
        ++iter;
    }
    free_placement_macros_structs();
}

/*
 * Prints the location of each block, and a simple drawing of FPGA fabric, showing num of blocks on each tile
 * Very useful for debugging
 * Usage:
 *   std::string filename = vtr::string_fmt("%s.post_AP.place", clb_nlist.netlist_name().substr(0, clb_nlist.netlist_name().size()-4).c_str());
 *   print_place(filename.c_str());
 */
void AnalyticPlacer::print_place(const char* place_file) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    FILE* fp;

    fp = fopen(place_file, "w");

    fprintf(fp, "Netlist_File: %s Netlist_ID: %s\n",
            clb_nlist.netlist_name().c_str(),
            clb_nlist.netlist_id().c_str());
    fprintf(fp, "Array size: %zu x %zu logic blocks\n\n", device_ctx.grid.width(), device_ctx.grid.height());
    fprintf(fp, "#block name\t\tlogic block type\tpb_type\tpb_name\t\tx\ty\tsubblk\tblock number\tis_fixed\n");
    fprintf(fp, "#----------\t\t----------------\t-------\t-------\t\t--\t--\t------\t------------\t---------\n");

    if (!place_ctx.block_locs.empty()) { //Only if placement exists
        for (auto blk_id : clb_nlist.blocks()) {
            fprintf(fp, "%s\t", clb_nlist.block_name(blk_id).c_str());
            if (strlen(clb_nlist.block_name(blk_id).c_str()) >= 14)
                ;
            else if (strlen(clb_nlist.block_name(blk_id).c_str()) < 7)
                fprintf(fp, "\t\t");
            else
                fprintf(fp, "\t");

            fprintf(fp, "%s\t\t\t", clb_nlist.block_type(blk_id)->name);
            fprintf(fp, "%s\t\t", clb_nlist.block_type(blk_id)->pb_type->name);

            fprintf(fp, "%s\t", clb_nlist.block_pb(blk_id)->name);
            if (strlen(clb_nlist.block_pb(blk_id)->name) < 7)
                fprintf(fp, "\t\t");
            else if (strlen(clb_nlist.block_pb(blk_id)->name) >= 14)
                ;
            else
                fprintf(fp, "\t");
            // fprintf(fp, "%s\t\t", cluster_ctx.clb_nlist.block_type(blk_id)->)
            fprintf(fp, "%d\t%d\t%d", blk_locs[blk_id].loc.x, blk_locs[blk_id].loc.y, blk_locs[blk_id].loc.sub_tile);
            fprintf(fp, "\t#%zu", size_t(blk_id));
            fprintf(fp, "\t\t%d\n", place_ctx.block_locs[blk_id].is_fixed);
        }
        fprintf(fp, "\ntotal_HPWL: %ld\n", total_hpwl());
        fprintf(fp, "overlap: %f\n", find_overlap());
        fprintf(fp, "%s", print_overlap().c_str());
    }
    fclose(fp);
}

// build num_legal_pos, legal_pos like initial_placement.cpp
void AnalyticPlacer::build_legal_locations() {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();
    int max_x = device_ctx.grid.width();
    int max_y = device_ctx.grid.height();

    std::vector<std::vector<int>> index; // helps in building legal_pos
    int num_tile_type = device_ctx.physical_tile_types.size();

    legal_pos.resize(num_tile_type);
    num_legal_pos.resize(num_tile_type);
    index.resize(num_tile_type);

    for (const auto& tile : device_ctx.physical_tile_types) {
        num_legal_pos[tile.index].resize(tile.sub_tiles.size());
        index[tile.index].resize(tile.sub_tiles.size());
    }

    for (int i = 0; i < max_x; i++) {
        for (int j = 0; j < max_y; j++) {
            auto tile = device_ctx.grid[i][j].type;

            for (auto sub_tile : tile->sub_tiles) {
                auto capacity = sub_tile.capacity;

                for (int k = 0; k < capacity.total(); k++) {
                    if (place_ctx.grid_blocks[i][j].blocks[k + capacity.low] != INVALID_BLOCK_ID) {
                        if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
                            num_legal_pos[tile->index][sub_tile.index]++;
                        }
                    }
                }
            }
        }
    }

    for (const auto& type : device_ctx.physical_tile_types) {
        legal_pos[type.index].resize(type.sub_tiles.size());

        for (auto sub_tile : type.sub_tiles) {
            legal_pos[type.index][sub_tile.index].resize(num_legal_pos[type.index][sub_tile.index]);
        }
    }

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            auto tile = device_ctx.grid[i][j].type;

            for (auto sub_tile : tile->sub_tiles) {
                auto capacity = sub_tile.capacity;

                for (int k = 0; k < capacity.total(); k++) {
                    if (place_ctx.grid_blocks[i][j].blocks[k + capacity.low] == INVALID_BLOCK_ID) {
                        continue;
                    }

                    if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
                        int itype = tile->index;
                        int isub_tile = sub_tile.index;
                        legal_pos[itype][isub_tile][index[itype][isub_tile]].x = i;
                        legal_pos[itype][isub_tile][index[itype][isub_tile]].y = j;
                        legal_pos[itype][isub_tile][index[itype][isub_tile]].sub_tile = k + capacity.low;
                        index[itype][isub_tile]++;
                    }
                }
            }
        }
    }
}

// build blk_locs based on initial placement;
// build place_blks;
void AnalyticPlacer::init() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    for (auto blk_id : clb_nlist.blocks()) {
        blk_locs[blk_id].loc = place_ctx.block_locs[blk_id].loc;
        blk_row_num[blk_id] = dont_solve;
    }

    auto has_connections = [&](ClusterBlockId blk_id) {
        for (auto pin : clb_nlist.block_pins(blk_id)) {
            int logical_pin_index = clb_nlist.pin_logical_index(pin);
            if (clb_nlist.block_net(blk_id, logical_pin_index) != ClusterNetId::INVALID())
                return true;
        }
        return false;
    };

    for (auto blk_id : clb_nlist.blocks()) {
        if (imacro(blk_id) == -1 || macro_head(blk_id) == blk_id)                    // not in macro or head of macro
            if (!place_ctx.block_locs[blk_id].is_fixed && has_connections(blk_id)) { // not fixed and has connections
                place_blks.push_back(blk_id);
            }
    }
}

// get hpwl of a net, taken from place.cpp get_bb_from_scratch()
wirelen_t AnalyticPlacer::get_net_hpwl(ClusterNetId net_id) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    int xmax, ymax, xmin, ymin, x, y;

    ClusterBlockId bnum = clb_nlist.net_driver_block(net_id);
    x = blk_locs[bnum].loc.x;
    y = blk_locs[bnum].loc.y;

    xmin = x;
    ymin = y;
    xmax = x;
    ymax = y;

    for (auto pin_id : clb_nlist.net_sinks(net_id)) {
        bnum = clb_nlist.pin_block(pin_id);
        x = blk_locs[bnum].loc.x;
        y = blk_locs[bnum].loc.y;

        if (x < xmin) {
            xmin = x;
        } else if (x > xmax) {
            xmax = x;
        }

        if (y < ymin) {
            ymin = y;
        } else if (y > ymax) {
            ymax = y;
        }
    }

    xmin = std::max<int>(xmin, 1);
    ymin = std::max<int>(ymin, 1);
    xmax = std::max<int>(xmax, 1);
    ymax = std::max<int>(ymax, 1);

    return (ymax - ymin) + (xmax - xmin);
}

wirelen_t AnalyticPlacer::total_hpwl() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    wirelen_t hpwl = 0;
    for (auto net_id : clb_nlist.nets()) {
        if (!clb_nlist.net_is_ignored(net_id)) {
            hpwl += get_net_hpwl(net_id);
        }
    }
    return hpwl;
}

// Setup the cells to be solved, returns the number of rows (number of blks to solve)
int AnalyticPlacer::setup_solve_blks(t_logical_block_type_ptr blkTypes) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    int row = 0;
    solve_blks.clear();
    // clear udata of all cells
    for (auto& blk : blk_row_num) {
        blk.second = dont_solve;
    }
    // update cells to be placed, excluding cell children
    for (auto blk_id : place_blks) {
        if (blkTypes != (clb_nlist.block_type(blk_id)))
            continue;
        blk_row_num[blk_id] = row++;
        solve_blks.push_back(blk_id);
    }
    // update udata of children
    for (auto& macro : place_ctx.pl_macros)
        for (auto& member : macro.members)
            blk_row_num[member.blk_index] = blk_row_num[macro_head(member.blk_index)];

    return row;
}

// Update the location of all members of all macros
void AnalyticPlacer::update_macros() {
    for (auto& macro : g_vpr_ctx.mutable_placement().pl_macros) {
        ClusterBlockId head_id = macro.members[0].blk_index;
        for (auto member = ++macro.members.begin(); member != macro.members.end(); ++member) {
            blk_locs[member->blk_index].loc = blk_locs[head_id].loc + member->offset;
        }
    }
}

// Build and solve in one direction
void AnalyticPlacer::build_solve_direction(bool yaxis, int iter, int build_solve_iter) {
    for (int i = 0; i < build_solve_iter; i++) {
        EquationSystem<double> esx(solve_blks.size(), solve_blks.size());
        build_equations(esx, yaxis, iter);
        solve_equations(esx, yaxis);
    }
}

// Build the system of equations for either X or Y
void AnalyticPlacer::build_equations(EquationSystem<double>& es, bool yaxis, int iter) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    // Return the x or y position of a block
    auto blk_p = [&](ClusterBlockId blk_id) { return yaxis ? blk_locs.at(blk_id).loc.y : blk_locs.at(blk_id).loc.x; };
    // Return legal position from legalization, after first iteration
    auto legal_p = [&](ClusterBlockId blk_id) { return yaxis ? blk_locs.at(blk_id).legal_loc.y : blk_locs.at(blk_id).legal_loc.x; };
    es.reset();

    for (auto net_id : clb_nlist.nets()) {
        if (clb_nlist.net_is_ignored(net_id)) continue;
        VTR_ASSERT(clb_nlist.net_driver(net_id) != ClusterPinId::INVALID());
        VTR_ASSERT(!clb_nlist.net_sinks(net_id).empty());
        if (clb_nlist.net_driver(net_id) == ClusterPinId::INVALID()) continue;
        if (clb_nlist.net_sinks(net_id).empty()) continue;

        ClusterPinId lbpin = ClusterPinId::INVALID(), ubpin = ClusterPinId::INVALID();
        int lbpos = std::numeric_limits<int>::max(), ubpos = std::numeric_limits<int>::min();
        for (auto pin_id : clb_nlist.net_pins(net_id)) {
            int pos = blk_p(clb_nlist.pin_block(pin_id));
            if (pos < lbpos) {
                lbpos = pos;
                lbpin = pin_id;
            }
            if (pos > ubpos) {
                ubpos = pos;
                ubpin = pin_id;
            }
        }
        VTR_ASSERT(lbpin != ClusterPinId::INVALID());
        VTR_ASSERT(ubpin != ClusterPinId::INVALID());

        auto stamp_equation = [&](ClusterPinId var, ClusterPinId eqn, double weight) {
            int eqn_row = blk_row_num[clb_nlist.pin_block(eqn)];
            if (eqn_row == dont_solve) return;
            ClusterBlockId var_blk = clb_nlist.pin_block(var);
            int v_pos = blk_p(var_blk);
            int var_row = blk_row_num[var_blk];
            if (var_row != dont_solve) { // var is movable
                es.add_coeff(eqn_row, var_row, weight);
            } else { // var is not movable
                es.add_rhs(eqn_row, -v_pos * weight);
            }
            if (imacro(var_blk) != -1) { // var is part of a macro
                auto& members = place_ctx.pl_macros[imacro(var_blk)].members;
                for (auto& member : members) {
                    if (member.blk_index == var_blk)
                        es.add_rhs(eqn_row, -(yaxis ? member.offset.y : member.offset.x) * weight);
                }
            }
        };

        int pin_n = clb_nlist.net_pins(net_id).size();
        for (int ipin = 0; ipin < pin_n; ipin++) {
            int this_pos = blk_p(clb_nlist.net_pin_block(net_id, ipin));
            ClusterPinId pin_id = clb_nlist.net_pin(net_id, ipin);
            auto process_arc = [&](ClusterPinId other) {
                if (other == pin_id || pin_id == ubpin) // to avoid bound pins connect to itself && double counting lbpin-to-ubpin connection
                    return;
                int o_pos = blk_p(clb_nlist.pin_block(other));
                double weight = 1.0 / ((pin_n - 1) * std::max<double>(1, std::abs(o_pos - this_pos)));

                // TODO: adding timing weights to matrix entries
                /*if (ipin != 0){
                 * weight *= (1.0 + tmpCfg.timingWeight * std::pow(place_crit.criticality(net_id, ipin), tmgCfg.criticalityExponent));
                 * } */

                stamp_equation(pin_id, pin_id, weight);
                stamp_equation(pin_id, other, -weight);
                stamp_equation(other, other, weight);
                stamp_equation(other, pin_id, -weight);
            };
            process_arc(lbpin);
            process_arc(ubpin);
        }
    }
    if (iter != -1) {
        for (size_t row = 0; row < solve_blks.size(); row++) {
            int l_pos = legal_p(solve_blks.at(row));
            int b_pos = blk_p(solve_blks.at(row));

            double weight = tmpCfg.alpha * iter / std::max<double>(1, std::abs(l_pos - b_pos));
            // Add psudo-connections for legalization
            es.add_coeff(row, row, weight);
            es.add_rhs(row, weight * l_pos);
        }
    }
}

// Solve the system of equations
void AnalyticPlacer::solve_equations(EquationSystem<double>& es, bool yaxis) {
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();

    auto blk_pos = [&](ClusterBlockId blk_id) { return yaxis ? blk_locs.at(blk_id).loc.y : blk_locs.at(blk_id).loc.x; };
    std::vector<double> vals;
    std::transform(solve_blks.begin(), solve_blks.end(), std::back_inserter(vals), blk_pos);
    es.solve(vals, tmpCfg.solverTolerance);

    for (size_t i = 0; i < vals.size(); i++)
        if (yaxis) {
            blk_locs.at(solve_blks.at(i)).rawy = vals.at(i);
            blk_locs.at(solve_blks.at(i)).loc.y = std::min(max_y - 1, std::max(0, int(vals.at(i) + 0.5)));
        } else {
            blk_locs.at(solve_blks.at(i)).rawx = vals.at(i);
            blk_locs.at(solve_blks.at(i)).loc.x = std::min(max_x - 1, std::max(0, int(vals.at(i) + 0.5)));
        }
}

// Debug use, finds # of blocks on 1 tile for all tiles
float AnalyticPlacer::find_overlap() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();

    int OL = 0;
    overlap.resize(max_y);
    for (auto& row : overlap) {
        row.resize(max_x);
        std::fill(row.begin(), row.end(), 0);
    }

    for (auto blk : clb_nlist.blocks()) {
        overlap.at(blk_locs[blk].loc.y).at(blk_locs[blk].loc.x) += 1;
    }

    std::cout << "overlap:" << std::endl;
    for (auto& row : overlap) {
        for (int count : row) {
            if (count > 0) OL++;
        }
    }

    return ((float)clb_nlist.blocks().size()) / (float)OL;
}

// prints a simple figure of FPGA fabric, with numbers on each tile showing usage
// called in AnalyticPlacer::print_place()
std::string AnalyticPlacer::print_overlap() {
    int max_x = g_vpr_ctx.device().grid.width();

    std::string out = "";
    out.append(5, ' ');
    for (int i = 0; i < max_x; i++) {
        std::string is = std::to_string(i);
        out.append(is);
        out.append(5 - strlen(is.c_str()), ' ');
    }
    out.append("\n");
    out.append(4, ' ');
    out.append(5 * max_x + 2, '-');
    out.append("\n");
    int row_n = 0;
    for (auto& row : overlap) {
        out.append(std::to_string(row_n));
        out.append(4 - strlen(std::to_string(row_n).c_str()), ' ');
        out.append("|");
        for (int count : row) {
            out.append((count == 0) ? " " : std::to_string(count));
            std::string space = std::string(5 - strlen(std::to_string(count).c_str()), ' ');
            out.append(space);
        }
        out.append("|");
        out.append("\n");
        row_n++;
    }
    out.append(4, ' ');
    out.append(5 * max_x + 2, '-');
    return out;
}

void AnalyticPlacer::print_AP_status_header() {
    VTR_LOG("\n");
    VTR_LOG("---- ------ ------ -------- ------- | ------ --------- ------ ------ ------ ------ -------- -------- --------\n");
    VTR_LOG("Iter   Time   Iter     Best   Stall |    Run BlockType  Solve  Solve Spread  Legal   Solved   Spread    Legal\n");
    VTR_LOG("              Time     hpwl         |   Time            Block   Time   Time   Time     hpwl     hpwl     hpwl\n");
    VTR_LOG("      (sec)  (sec)                  |  (sec)              Num  (sec)  (sec)  (sec)                           \n");
    VTR_LOG("---- ------ ------ -------- ------- | ------ --------- ------ ------ ------ ------ -------- -------- --------\n");
}

void AnalyticPlacer::print_run_stats(const int iter,
                                     const float time,
                                     const float runTime,
                                     const char* blockType,
                                     const int blockNum,
                                     const float solveTime,
                                     const float spreadTime,
                                     const float legalTime,
                                     const wirelen_t solvedHPWL,
                                     const wirelen_t spreadHPWL,
                                     const wirelen_t legalHPWL) {
    VTR_LOG(
        "%4zu "
        "%6.3f "
        "                        | "
        "%6.3f "
        "%9s "
        "%6d "
        "%6.3f "
        "%6.3f "
        "%6.3f "
        "%8ld "
        "%8ld "
        "%8ld \n",
        iter,
        time,
        runTime,
        blockType,
        blockNum,
        solveTime,
        spreadTime,
        legalTime,
        solvedHPWL,
        spreadHPWL,
        legalHPWL);
}

void AnalyticPlacer::print_iter_stats(const int iter,
                                      const float iterTime,
                                      const float time,
                                      const wirelen_t bestHPWL,
                                      const int stall) {
    VTR_LOG(
        "%4zu "
        "%6.3f "
        "%6.3f "
        "%8ld "
        "%7d |\n",
        iter,
        time,
        iterTime,
        bestHPWL,
        stall);
    VTR_LOG("                                    |\n");
}
