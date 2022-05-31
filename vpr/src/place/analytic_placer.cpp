#ifdef ENABLE_ANALYTIC_PLACE

#    include "analytic_placer.h"
#    include <Eigen/Core>
#    include <Eigen/IterativeLinearSolvers>
#    include <iostream>
#    include <vector>
#    include <stdint.h>

#    include "vpr_types.h"
#    include "vtr_time.h"
#    include "read_place.h"
#    include "globals.h"
#    include "vtr_log.h"
#    include "cut_spreader.h"
#    include "vpr_utils.h"
#    include "place_util.h"

// Templated struct for constructing and solving matrix equations in analytic placer
template<typename T>
struct EquationSystem {
    EquationSystem(size_t rows, size_t cols) {
        A.resize(cols);
        rhs.resize(rows);
    }

    // A[col] is an entire column of the sparse matrix
    // each entry in A[col][index] is a pair with {row_number, matrix value}.
    //
    // The strategy of skipping 0 row entries in each column enables easy conversion to
    // Compressed Column Storage scheme supported by Eigen to reduce memory consumption
    // and increase performance
    std::vector<std::vector<std::pair<int, T>>> A;
    // right hand side vector, i.e. b in Ax = b
    std::vector<T> rhs;

    // System of equation is reset by:
    // Clearing all entries in A's column, but size of A (number of columns) is preserved
    // right hand side vector is set to default value of its templated type
    void reset() {
        for (auto& col : A)
            col.clear();
        std::fill(rhs.begin(), rhs.end(), T());
    }

    // Add val to the matrix entry at (row, col)
    // create entry if it doesn't exist
    void add_coeff(int row, int col, T val) {
        auto& A_col = A.at(col);
        // Binary search for the row entry in column col
        int begin_i = 0, end_i = int(A_col.size()) - 1;
        while (begin_i <= end_i) {
            int i = (begin_i + end_i) / 2;
            if (A_col.at(i).first == row) {
                A_col.at(i).second += val;
                return;
            }
            if (A_col.at(i).first > row)
                end_i = i - 1;
            else
                begin_i = i + 1;
        }
        A_col.insert(A_col.begin() + begin_i, std::make_pair(row, val));
    }

    // Add val to the "row"-th entry of right hand side vector
    void add_rhs(int row, T val) { rhs[row] += val; }

    // Solving Ax = b, using current x as an initial guess, returns x by reference.
    // (x must be of correct size, A and rhs must have their entries filled in)
    // tolerance is residual error from solver: |Ax-b|/|b|, 1e-5 works well,
    // can be tuned in ap_cfg in AnalyticPlacer constructor
    void solve(std::vector<T>& x, float tolerance) {
        using namespace Eigen;

        VTR_ASSERT(x.size() == A.size());

        // Converting A into SparseMatrix format from Eigen
        VectorXd vec_x_guess(x.size()), vec_rhs(rhs.size());
        SparseMatrix<T> mat(A.size(), A.size());

        std::vector<int> colnnz; // vector containing number of entries in each column
        for (auto& A_col : A)
            colnnz.push_back(int(A_col.size()));
        mat.reserve(colnnz); // reserve memory for mat depending on number of entries in each row
        for (int col = 0; col < int(A.size()); col++) {
            auto& A_col = A.at(col);
            for (auto& row_entry : A_col)
                mat.insert(row_entry.first, col) = row_entry.second;
        }

        // use current value of x as guess for iterative solver
        for (int i_row = 0; i_row < int(x.size()); i_row++)
            vec_x_guess[i_row] = x.at(i_row);

        for (int i_row = 0; i_row < int(rhs.size()); i_row++)
            vec_rhs[i_row] = rhs.at(i_row);

        ConjugateGradient<SparseMatrix<T>, Lower | Upper> solver;
        solver.setTolerance(tolerance);
        VectorXd x_res = solver.compute(mat).solveWithGuess(vec_rhs, vec_x_guess);
        for (int i_row = 0; i_row < int(x.size()); i_row++)
            x.at(i_row) = x_res[i_row];
    }
};

// helper function to find the index of macro that contains blk
// returns index in placementCtx.pl_macros,
// returns NO_MACRO if blk not in any macros
int imacro(ClusterBlockId blk) {
    int macro_index;
    get_imacro_from_iblk(&macro_index, blk, g_vpr_ctx.mutable_placement().pl_macros);
    return macro_index;
}

// helper fucntion to find the head (first block) of macro containing blk
// returns the ID of the head block
ClusterBlockId macro_head(ClusterBlockId blk) {
    int macro_index = imacro(blk);
    return g_vpr_ctx.mutable_placement().pl_macros[macro_index].members[0].blk_index;
}

// Stop optimizing once this many iterations of solve-legalize lead to negligible wirelength improvement
constexpr int HEAP_STALLED_ITERATIONS_STOP = 15;

/*
 * AnalyticPlacer constructor
 * Currently only initializing AP configuration parameters
 * Placement & device info is accessed via g_vpr_ctx
 */
AnalyticPlacer::AnalyticPlacer() {
    //Eigen::initParallel();

    // TODO: PlacerHeapCfg should be externally configured & supplied
    // TODO: tune these parameters for better performance
    ap_cfg.alpha = 0.1; // anchoring strength, after first AP iteration the legal position of each block
                        // becomes anchors. In the next AP iteration, pseudo-connection between each blocks
                        // current location and its anchor is formed with strength (alph * iter)
                        // @see build_equations()

    ap_cfg.beta = 1; // utilization factor, <= 1, used to determine if a cut-spreading region is
                     // overutilized with the formula: bool overutilized = (num_blks / num_tiles) > beta
                     // for beta < 1, a region must have more tiles than logical blks to not be overutilized

    ap_cfg.solverTolerance = 1e-5; // solver parameter, refers to residual error from solver, defined as |Ax-b|/|b|

    ap_cfg.buildSolveIter = 5; // number of build-solve iteration when calculating placement, used in
                               // build_solve_direction()
                               // for each build-solve iteration, the solution from previous build-solve iteration
                               // is used as a guess for the iterative solver. therefore more buildSolveIter should
                               // should improve result at the expense of runtime

    // following two parameters are used in CutSpreader::expand_regions().
    // they determine the number of steps to expand in x or y direction before switching to expand in the other direction.
    ap_cfg.spread_scale_x = 1;
    ap_cfg.spread_scale_y = 1;

    // following two timing parameters are used to add timing weights in matrix equation, currently not used
    // see comment in add_pin_to_pin_connection() for usage
    ap_cfg.criticalityExponent = 1;
    ap_cfg.timingWeight = 10;
}

/*
 * Main function of analytic placement
 * Takes the random initial placement from place.cpp through g_vpr_ctx
 * Repeat the following until stopping criteria is met:
 * 	* Formulate and solve equations in x & y directions for 1 type of logical block
 * 	* Instantiate CutSpreader to spread and strict_legalize
 *
 * The final legal placement is passed back to annealer in g_vpr_ctx.mutable_placement()
 */
void AnalyticPlacer::ap_place() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    vtr::ScopedStartFinishTimer timer("Analytic Placement");

    init(); // transfer placement from g_vpr_ctx to AnalyticPlacer data members
    build_legal_locations();
    int hpwl = total_hpwl();
    VTR_LOG("Creating analytic placement for %d cells, random placement hpwl = %d.\n",
            int(clb_nlist.blocks().size()), int(hpwl));

    // the order in which different logical block types are placed;
    // going through ap_runs once completes 1 iteration of AP
    std::vector<t_logical_block_type_ptr> ap_runs;
    std::unordered_set<t_logical_block_type_ptr> all_blktypes; // set of all logical block types

    // setup ap_runs, run build/solve/legalize once for every block type
    // each type is placed separately, but influenced by the current location of other types
    for (auto blk : place_blks) {
        if (!all_blktypes.count(clb_nlist.block_type(blk))) {
            ap_runs.push_back(clb_nlist.block_type(blk));
            all_blktypes.insert(clb_nlist.block_type(blk));
        }
    }

    // setup and solve matrix multiple times for all logic block types before main loop
    // this helps eliminating randomness from initial placement (when placing one block type, the random placement
    // of the other types may have residual effect on the result, since not all blocks are solved at the same time)
    for (int i = 0; i < 1; i++) { // can tune number of iterations
        for (auto run : ap_runs) {
            build_solve_type(run, -1);
        }
    }

    int iter = 0, stalled = 0;
    // variables for stats
    int solved_hpwl = 0, spread_hpwl = 0, legal_hpwl = 0, best_hpwl = std::numeric_limits<int>::max();
    float iter_start, iter_t, run_start, run_t, solve_t, spread_start, spread_t, legal_start, legal_t;

    print_AP_status_header();

    // main loop for AP
    // stopping criteria: stop after HEAP_STALLED_ITERATIONS_STOP iterations of no improvement
    while (stalled < HEAP_STALLED_ITERATIONS_STOP) {
        // TODO: investigate better stopping criteria
        iter_start = timer.elapsed_sec();
        for (auto blk_type : ap_runs) { // for each type of logic blocks
            run_start = timer.elapsed_sec();

            // lower bound placement for blk_type
            // build and solve matrix equation for blocks of type "blk_type" in both x and y directions
            build_solve_type(blk_type, iter);
            solve_t = timer.elapsed_sec() - run_start;
            solved_hpwl = total_hpwl();
            // lower bound placement complete

            // upper bound placement
            // cut-spreading logic blocks of type "blk_type", this will mostly legalize lower bound placement
            spread_start = timer.elapsed_sec();
            CutSpreader spreader{this, blk_type}; // Legalizer
            if (strcmp(blk_type->name, "io") != 0) {
                /* skip cut-spreading for IO blocks; they tend to cluster on 1 edge of the FPGA due to how cut-spreader works
                 * in HeAP, cut-spreading is invoked only on LUT, DSP, RAM etc.
                 * here, greedy legalization by spreader.strict_legalize() should be sufficient for IOs
                 */
                spreader.cutSpread();
                update_macros();
                spread_hpwl = total_hpwl();
                spread_t = timer.elapsed_sec() - spread_start;
            } else {
                spread_hpwl = -1;
                spread_t = 0;
            }

            // greedy legalizer for fully legal placement
            legal_start = timer.elapsed_sec();
            spreader.strict_legalize(); // greedy legalization snaps blocks to the closest legal location
            update_macros();
            legal_t = timer.elapsed_sec() - legal_start;
            legal_hpwl = total_hpwl();

            // upper bound placement complete

            run_t = timer.elapsed_sec() - run_start;
            print_run_stats(iter, timer.elapsed_sec(), run_t, blk_type->name, solve_blks.size(), solve_t,
                            spread_t, legal_t, solved_hpwl, spread_hpwl, legal_hpwl);
        }

        // TODO: update timing info here after timing weights are implemented in build_equations()

        if (legal_hpwl < best_hpwl) {
            best_hpwl = legal_hpwl;
            stalled = 0;
        } else {
            ++stalled;
        }

        // update legal locations for all blocks for pseudo-connections in next iteration
        for (auto& bl : blk_locs) {
            bl.legal_loc = bl.loc;
        }
        iter_t = timer.elapsed_sec() - iter_start;
        print_iter_stats(iter, iter_t, timer.elapsed_sec(), best_hpwl, stalled);
        ++iter;
    }
}

// build matrix equations and solve for block type "run" in both x and y directions
// macro member positions are updated after solving
void AnalyticPlacer::build_solve_type(t_logical_block_type_ptr run, int iter) {
    setup_solve_blks(run);
    // build and solve matrix equation for both x, y
    // passing -1 as iter to build_solve_direction() signals build_equation() not to add pseudo-connections
    build_solve_direction(false, (iter == 0) ? -1 : iter, ap_cfg.buildSolveIter);
    build_solve_direction(true, (iter == 0) ? -1 : iter, ap_cfg.buildSolveIter);
    update_macros(); // update macro member locations, since only macro head is solved
}

// build legal_pos similar to initial_placement.cpp
// Go through the placement grid and saving all legal positions for each type of sub_tile
// (stored in legal_pos). For a type of sub_tile_t found in tile_t, legal_pos[tile_t][sub_tile_t]
// gives a vector containing all positions (t_pl_loc type) for this sub_tile_t.
void AnalyticPlacer::build_legal_locations() {
    // invoking same function used in initial_placement.cpp (can ignore function name)
    alloc_and_load_legal_placement_locations(legal_pos);
}

// transfer initial placement from g_vpr_ctx to AnalyticPlacer data members, such as: blk_locs, place_blks
// initialize other data members
void AnalyticPlacer::init() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    for (auto blk_id : clb_nlist.blocks()) {
        blk_locs.insert(blk_id, BlockLocation{});
        blk_locs[blk_id].loc = place_ctx.block_locs[blk_id].loc; // transfer of initial placement
        row_num.insert(blk_id, DONT_SOLVE);                      // no blocks are moved by default, until they are setup in setup_solve_blks()
    }

    // only blocks with connections are considered
    auto has_connections = [&](ClusterBlockId blk_id) {
        for (auto pin : clb_nlist.block_pins(blk_id)) {
            int logical_pin_index = clb_nlist.pin_logical_index(pin);
            if (clb_nlist.block_net(blk_id, logical_pin_index) != ClusterNetId::INVALID())
                return true;
        }
        return false;
    };

    for (auto blk_id : clb_nlist.blocks()) {
        if (!place_ctx.block_locs[blk_id].is_fixed && has_connections(blk_id))
            // not fixed and has connections
            // matrix equation is formulated based on connections, so requires at least one connection
            if (imacro(blk_id) == NO_MACRO || macro_head(blk_id) == blk_id) {
                // not in macro or head of macro
                // for macro, only the head (base) block of the macro is a free variable, the location of other macro
                // blocks can be calculated using offset of the head. They are not free variables in the equation system
                place_blks.push_back(blk_id);
            }
    }
}

// get hpwl of a net, taken from place.cpp get_bb_from_scratch()
// TODO: factor out this function from place.cpp and put into vpr_util
int AnalyticPlacer::get_net_hpwl(ClusterNetId net_id) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();

    // position is not accurate for tiles spanning multiple grid locations
    // need to add pin offset in that case: physical_tile_type(bnum)->pin_width_offset[pnum]
    // see place.cpp get_non_updateable_bb();
    // TODO: map net_pin to tile_pin and add pin offset to x, y locations (refer to place.cpp)
    ClusterBlockId bnum = clb_nlist.net_driver_block(net_id);
    int x = std::max<int>(std::min<int>(blk_locs[bnum].loc.x, max_x - 1), 1);
    int y = std::max<int>(std::min<int>(blk_locs[bnum].loc.y, max_y - 1), 1);

    vtr::Rect<int> bb = {x, y, x, y};

    for (auto pin_id : clb_nlist.net_sinks(net_id)) {
        bnum = clb_nlist.pin_block(pin_id);
        x = std::max<int>(std::min<int>(blk_locs[bnum].loc.x, max_x - 1), 1);
        y = std::max<int>(std::min<int>(blk_locs[bnum].loc.y, max_y - 1), 1);

        bb.expand_bounding_box({x, y, x, y});
    }

    return (bb.ymax() - bb.ymin()) + (bb.xmax() - bb.xmin());
}

// get hpwl for all nets
int AnalyticPlacer::total_hpwl() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    int hpwl = 0;
    for (auto net_id : clb_nlist.nets()) {
        if (!clb_nlist.net_is_ignored(net_id)) {
            hpwl += get_net_hpwl(net_id);
        }
    }
    return hpwl;
}

/*
 * Setup the blocks of type blkTypes (ex. clb, io) to be solved. These blocks are put into
 * solve_blks vector. Each of them is a free variable in the matrix equation (thus excluding
 * macro members, as they are formulated into the equation for the macro's head)
 * A row number is assigned to each of these blocks, which corresponds to its equation in
 * the matrix (the equation acquired from differentiating the objective function w.r.t its
 * x or y location).
 */
void AnalyticPlacer::setup_solve_blks(t_logical_block_type_ptr blkTypes) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    int row = 0;
    solve_blks.clear();
    // clear row_num of all cells, so no blocks are solved
    for (auto& blk : row_num) {
        blk = DONT_SOLVE;
    }
    // update blks to be solved/placed, excluding macro members (macro head included)
    for (auto blk_id : place_blks) { // find blocks of type blkTypes in place_blks
        if (blkTypes == (clb_nlist.block_type(blk_id))) {
            row_num[blk_id] = row++;
            solve_blks.push_back(blk_id);
        }
    }
    // update row_num of macro members
    for (auto& macro : place_ctx.pl_macros)
        for (auto& member : macro.members)
            row_num[member.blk_index] = row_num[macro_head(member.blk_index)];
}

/*
 * Update the location of all members of all macros based on location of macro_head
 * since only macro_head is solved (connections to macro members are also taken into account
 * when formulating the matrix equations), an update for members is necessary
 */
void AnalyticPlacer::update_macros() {
    for (auto& macro : g_vpr_ctx.mutable_placement().pl_macros) {
        ClusterBlockId head_id = macro.members[0].blk_index;
        bool mac_can_be_placed = macro_can_be_placed(macro, blk_locs[head_id].loc, true);

        //if macro can not be placed in this head pos, change the head pos
        if (!mac_can_be_placed) {
            size_t macro_size = macro.members.size();
            blk_locs[head_id].loc -= macro.members[macro_size - 1].offset;
        }

        //macro should be placed successfully after changing the head position
        VTR_ASSERT(macro_can_be_placed(macro, blk_locs[head_id].loc, true));

        //update other member's location based on head pos
        for (auto member = ++macro.members.begin(); member != macro.members.end(); ++member) {
            blk_locs[member->blk_index].loc = blk_locs[head_id].loc + member->offset;
        }
    }
}

/*
 * Build and solve in one direction
 * Solved solutions are written back to block_locs[blk].rawx/rawy for double float raw solution,
 * rounded int solutions are written back to block_locs[blk].loc, for each blk in solve_blks
 *
 * yaxis chooses x or y location of each block from blk_locs to formulate the matrix equation.
 * true for y-directed, false for x-directed
 *
 * iter is the number of AnalyticPlacement iterations (solving and legalizing all types of logic
 * blocks once). When iter != -1, at least one iteration has completed. It signals build_equations()
 * to create pseudo-connections between each block and its prior legal position.
 *
 * build_solve_iter determines number of iterations of building and solving for the iterative solver,
 * the solution from the previous build-solve iteration is used as a guess for the iterative solver.
 * More build_solve_iter means better result, with runtime tradeoff. This parameter can be
 * tuned for better performance.
 */
void AnalyticPlacer::build_solve_direction(bool yaxis, int iter, int build_solve_iter) {
    for (int i = 0; i < build_solve_iter; i++) {
        EquationSystem<double> esx(solve_blks.size(), solve_blks.size());
        build_equations(esx, yaxis, iter);
        solve_equations(esx, yaxis);
    }
}

/*
 * stamp 1 weight for a connection on matrix or rhs vector.
 *
 * Block "eqn" specifies which equation (row in matrix system) the weight is added into.
 * let eqn have row_num i, var have row_num j (which is also the column in eqn that corresponds to var).
 *
 * if eqn is not movable, return (eqn doesn't really have an equation as it's not a free variable)
 * if var is movable, weight is added in matrix [j][i]
 * if var is not movable, (var_pos * weight) is added in rhs vector[j]
 * if var is a macro member, weight is added in matrix [j][i], and (-offset_from_head_block * weight) is added to rhs vector[j]
 *
 * for detailed derivation see comment for add_pin_to_pin_connection()
 */
void AnalyticPlacer::stamp_weight_on_matrix(EquationSystem<double>& es,
                                            bool dir,
                                            ClusterBlockId var,
                                            ClusterBlockId eqn,
                                            double weight) {
    PlacementContext& place_ctx = g_vpr_ctx.mutable_placement();

    // Return the x or y position of a block
    auto blk_p = [&](ClusterBlockId blk_id) { return dir ? blk_locs[blk_id].loc.y : blk_locs[blk_id].loc.x; };

    int eqn_row = row_num[eqn];
    if (eqn_row == DONT_SOLVE) // if eqn is not of the right type or is locked down
        return;
    int v_pos = blk_p(var);
    int var_row = row_num[var];
    if (var_row != DONT_SOLVE) { // var is movable, stamp weight on matrix
        es.add_coeff(eqn_row, var_row, weight);
    } else { // var is not movable, stamp weight on rhs vector
        es.add_rhs(eqn_row, -v_pos * weight);
    }
    if (imacro(var) != NO_MACRO) { // var is part of a macro, stamp on rhs vector
        auto& members = place_ctx.pl_macros[imacro(var)].members;
        for (auto& member : members) { // go through macro members to find the right member block
            if (member.blk_index == var)
                es.add_rhs(eqn_row, -(dir ? member.offset.y : member.offset.x) * weight);
        }
    }
}

/*
 * Add weights to matrix for the pin-to-pin connection between bound_blk and this_blk (bound2bound model)
 *
 * The matrix A in system of equation Ax=b is a symmetric sparse matrix.
 * Each row of A corresponds to an equation for a free variable. This equation is acquired by differentiating
 * the objective function with respect to the free variable (movable block's x or y location) and setting it
 * to 0.
 *
 * Pin-to-pin connection between 2 movable blocks (call them b1 and b2, with connection weight W12) is the
 * simplest case. Differentiating with respect to b1 and setting to 0 produces W12 * b1 - W12 * b2 = 0, where
 * b1, b2 are the location variables to calculate. When cast into matrix form, the row number of this equation
 * corresponds to b1. Let's assume b1 and b2's equations are in rows i, j. Row number for each free variable also
 * indicates its position in other variable's equation. In our example, assume there are 5 free variables (free
 * blocks), and i=2, j=4. Then, after adding weights to b1's equation, the system will look like the following:
 * 									|   x   x   x   x   x   |   |x |  = | x |
 * 									|   0  W12  0 -W12  0   |   |b1|  = | 0 |
 * 									|   x   x   x   x   x   | * |x |  = | x |
 * 									|   x   x   x   x   x   |   |b2|  = | x |
 * 									|   x   x   x   x   x   |   |x |  = | x |
 * Differentiating with respect to b2 will result in same equation except flipped signs for the weight. This creates
 * symmetry in the matrix, resulting in:
 * 									|   x   x   x   x   x   |   |x |  = | x |
 * 									|   0  W12  0 -W12  0   |   |b1|  = | 0 |
 * 									|   x   x   x   x   x   | * |x |  = | x |
 * 									|   0 -W12  0  W12  0   |   |b2|  = | 0 |
 * 									|   x   x   x   x   x   |   |x |  = | x |
 * To generalize, for movable blocks b1, b2 in row i,j, with connection weight W, the W is added to matrix position
 * [i][i] and [j][j], -W added to [i][j] and [j][i]. This is why stamp_weight_on_matrix is invoked 4 times below.
 *
 * Special Case: immovable/fixed block.
 * Assume b2 in the above example is fixed, then it does not have an equation in the system as it's not a free variable.
 * The new equation is now W12 * b1 = W12 * b2, where b2 is just a constant. (This makes sense as b1=b2 is optimal,
 * since it has wirelength of 0). The matrix equation now looks like the following:
 * 									|   x   x   x   x   x   |   |x |  = |  x   |
 * 									|   0  W12  0   0   0   |   |b1|  = |W12*b2|
 * 									|   x   x   x   x   x   | * |x |  = |  x   |
 * 									|   x   x   x   x   x   |   |x |  = |  x   |
 * 									|   x   x   x   x   x   |   |x |  = |  x   |
 *
 * Special Case: connection to macro member.
 * Assume b1 is the head block of a macro, b3 is its macro member with offset d. b3 has a connection with movable block
 * b2, with weight W23. b3's location is then (b1 + d). The new equation w.r.t. b1 is W23 * (b1 + d - b2) = 0.
 * New equation w.r.t. b3 is symmetrical, producing matrix:
 * 									|   x   x   x   x   x   |   |x |  = |  x   |
 * 									|   0  W23  0 -W23  0   |   |b1|  = |-W23*d|
 * 									|   x   x   x   x   x   | * |x |  = |  x   |
 * 									|   0 -W23  0  W23  0   |   |b2|  = | W23*d|
 * 									|   x   x   x   x   x   |   |x |  = |  x   |
 * As shown here, connection to macro members are formulated into macro's head block's equation. This is why macro members
 * are not formulated in equation system.
 *
 * EquationSystem is passed in for adding weights, dir selects x/y direction, num_pins is used in weight calculation
 * (bound2bound model). bound_pin and this_pin specifies the 2 pins in the connection (one of them is always bound_pin).
 */
void AnalyticPlacer::add_pin_to_pin_connection(EquationSystem<double>& es,
                                               bool dir,
                                               int num_pins,
                                               ClusterPinId bound_pin,
                                               ClusterPinId this_pin) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    if (this_pin == bound_pin)
        // no connection if 2 pins are the same
        return;

    // this_blk and bound_blk locations may not be accurate for larger tiles spanning multiple grid locations
    // need block_locs[blk_id].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum]
    // however, in order to do so, need place_sync_external_block_connections(blk_id) for all blocks
    // TODO: map logical pin to physical pin and add this offset for more accurate pin location
    ClusterBlockId this_blk = clb_nlist.pin_block(this_pin);
    int this_pos = dir ? blk_locs[this_blk].loc.y : blk_locs[this_blk].loc.x;
    ClusterBlockId bound_blk = clb_nlist.pin_block(bound_pin);
    int bound_pos = dir ? blk_locs[bound_blk].loc.y : blk_locs[bound_blk].loc.x;
    // implementing the bound-to-bound net model detailed in HeAP paper, where each bound blk has (num_pins - 1) connections
    // (bound_pos - this_pos) in the denominator "linearizes" the quadratic term (bound_pos - this_pos)^2 in the objective function
    // This ensures that the objective function target HPWL, rather than quadratic wirelength.
    double weight = 1.0 / ((num_pins - 1) * std::max<double>(1, std::abs(bound_pos - this_pos)));

    /*
     * TODO: adding timing weights to matrix entries
     *if (this_pin != 0){
     * weight *= (1.0 + tmpCfg.timingWeight * std::pow(place_crit.criticality(net_id, this_pin), tmgCfg.criticalityExponent));
     * }
     */

    stamp_weight_on_matrix(es, dir, this_blk, this_blk, weight);
    stamp_weight_on_matrix(es, dir, this_blk, bound_blk, -weight);
    stamp_weight_on_matrix(es, dir, bound_blk, bound_blk, weight);
    stamp_weight_on_matrix(es, dir, bound_blk, this_blk, -weight);
}

// Build the system of equations for either X or Y
void AnalyticPlacer::build_equations(EquationSystem<double>& es, bool yaxis, int iter) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    // Return the x or y position of a block
    auto blk_p = [&](ClusterBlockId blk_id) { return yaxis ? blk_locs[blk_id].loc.y : blk_locs[blk_id].loc.x; };
    // Return legal position from legalization, after first iteration
    auto legal_p = [&](ClusterBlockId blk_id) { return yaxis ? blk_locs[blk_id].legal_loc.y : blk_locs[blk_id].legal_loc.x; };
    es.reset();

    /*
     * Bound2bound model is used in HeAP:
     * For each net, the left-most and right-most (or down, up in y direction) are bound blocks
     * These 2 blocks form connections with each other and all the other blocks (internal blocks)
     * These connections are used to formulate the matrix equation
     */
    for (auto net_id : clb_nlist.nets()) {
        if (clb_nlist.net_is_ignored(net_id)
            || clb_nlist.net_driver(net_id) == ClusterPinId::INVALID()
            || clb_nlist.net_sinks(net_id).empty()) {
            // ensure net is not ignored (ex. clk nets), has valid driver, has at least 1 sink
            continue;
        }

        // find the 2 bound pins (min and max pin)
        ClusterPinId min_pin = ClusterPinId::INVALID(), max_pin = ClusterPinId::INVALID();
        int min_pos = std::numeric_limits<int>::max(), max_pos = std::numeric_limits<int>::min();
        for (auto pin_id : clb_nlist.net_pins(net_id)) {
            int pos = blk_p(clb_nlist.pin_block(pin_id));
            if (pos < min_pos) {
                min_pos = pos;
                min_pin = pin_id;
            }
            if (pos > max_pos) {
                max_pos = pos;
                max_pin = pin_id;
            }
        }
        VTR_ASSERT(min_pin != ClusterPinId::INVALID());
        VTR_ASSERT(max_pin != ClusterPinId::INVALID());

        int num_pins = clb_nlist.net_pins(net_id).size();
        for (int ipin = 0; ipin < num_pins; ipin++) {
            ClusterPinId pin_id = clb_nlist.net_pin(net_id, ipin);
            // for each pin in net, connect to 2 bound pins (bound2bound model)
            add_pin_to_pin_connection(es, yaxis, num_pins, min_pin, pin_id);
            if (pin_id != min_pin)
                // avoid adding min_pin to max_pin connection twice
                add_pin_to_pin_connection(es, yaxis, num_pins, max_pin, pin_id);
        }
    }

    // Add pseudo-connections to anchor points (legalized position for each block) after first iteration
    // These pseudo-connections pull blocks towards their legal locations, which tends to reduce overlaps in the placement,
    // also so that the next iteration of build-solving matrix doesn't destroy the placement from last iteration.
    // As weight increases with number of iterations, solver's solution converges with the legal placement.
    if (iter != -1) { // if not the first AP iteration
        for (size_t row = 0; row < solve_blks.size(); row++) {
            int l_pos = legal_p(solve_blks.at(row));        // legalized position from last iteration (anchors)
            int solver_blk_pos = blk_p(solve_blks.at(row)); // matrix solved block position from last iteration

            // weight increases with iteration --> psudo-connection strength increases to force convergence to legal placement
            // weight is also higher for blocks that haven't moved much from their solver location to their legal location
            double weight = ap_cfg.alpha * iter / std::max<double>(1, std::abs(l_pos - solver_blk_pos));

            // Adding coefficient to Matrix[row][row] and adding weight to rhs vector is equivalent to adding connection
            // to an immovable block at legal position.
            // The equation becomes Weight * (blk_pos - legal_pos) = 0, where blk_pos is the variable to solve in rhs[row],
            // legal_pos is a constant
            // see comment for add_pin_to_pin_connection() -> special_case: immovable/fixed block
            es.add_coeff(row, row, weight);
            es.add_rhs(row, weight * l_pos);
        }
    }
}

/*
 * Solve the system of equations
 * A formulated system of equation es is passed in
 * yaxis represents if it's x-directed or y-directed location problem
 * Solved solution is moved to loc, rawx, rawy in blk_locs for each block
 */
void AnalyticPlacer::solve_equations(EquationSystem<double>& es, bool yaxis) {
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();

    auto blk_pos = [&](ClusterBlockId blk_id) { return yaxis ? blk_locs[blk_id].rawy : blk_locs[blk_id].rawx; };
    std::vector<double> solve_blks_pos; // each row of solve_blks_pos is a free variable (movable block of the right type to be placed)
    // put current location of solve_blks into solve_blks_pos as guess for iterative solver
    std::transform(solve_blks.begin(), solve_blks.end(), std::back_inserter(solve_blks_pos), blk_pos);
    es.solve(solve_blks_pos, ap_cfg.solverTolerance);

    // move solved locations of solve_blks from solve_blks_pos into blk_locs
    // ensure that new location is strictly within [0, grid.width/height - 1];
    for (size_t i_row = 0; i_row < solve_blks_pos.size(); i_row++)
        if (yaxis) {
            blk_locs[solve_blks.at(i_row)].rawy = std::max(0.0, solve_blks_pos.at(i_row));
            blk_locs[solve_blks.at(i_row)].loc.y = std::min(max_y - 1, std::max(0, int(solve_blks_pos.at(i_row) + 0.5)));
        } else {
            blk_locs[solve_blks.at(i_row)].rawx = std::max(0.0, solve_blks_pos.at(i_row));
            blk_locs[solve_blks.at(i_row)].loc.x = std::min(max_x - 1, std::max(0, int(solve_blks_pos.at(i_row) + 0.5)));
        }
}

// Debug use, finds # of blocks on each tile location
void AnalyticPlacer::find_overlap(vtr::Matrix<int>& overlap) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    size_t max_x = g_vpr_ctx.device().grid.width();
    size_t max_y = g_vpr_ctx.device().grid.height();

    overlap.resize({max_y, max_x}, 0);

    for (auto blk : clb_nlist.blocks()) {
        overlap[blk_locs[blk].loc.y][blk_locs[blk].loc.x] += 1;
    }
}

// prints a simple figure of FPGA fabric, with numbers on each tile showing usage
// called in AnalyticPlacer::print_place()
std::string AnalyticPlacer::print_overlap(vtr::Matrix<int>& overlap, FILE* fp) {
    int max_x = g_vpr_ctx.device().grid.width();
    int max_y = g_vpr_ctx.device().grid.height();

    std::string out = "";
    fprintf(fp, "%5s", "");
    for (int i = 0; i < max_x; i++) {
        fprintf(fp, "%-5d", i);
    }
    fprintf(fp, "\n%4s", "");
    fprintf(fp, "%s\n", std::string(5 * max_x + 2, '-').c_str());
    for (int i = 0; i < max_y; i++) {
        fprintf(fp, "%-4d|", i);
        for (int j = 0; j < max_x; j++) {
            int count = overlap[i][j];
            fprintf(fp, "%-5s", ((count == 0) ? "0" : std::to_string(count)).c_str());
        }
        fprintf(fp, "|\n");
    }
    fprintf(fp, "%4s", "");
    fprintf(fp, "%s\n", std::string(5 * max_x + 2, '-').c_str());
    return out;
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
    fprintf(fp, "%-25s %-18s %-12s %-25s %-5s %-5s %-10s %-14s %-8s\n",
            "block name",
            "logic block type",
            "pb_type",
            "pb_name",
            "x",
            "y",
            "subblk",
            "block number",
            "is_fixed");
    fprintf(fp, "%-25s %-18s %-12s %-25s %-5s %-5s %-10s %-14s %-8s\n",
            "----------",
            "----------------",
            "-------",
            "-------",
            "--",
            "--",
            "------",
            "------------",
            "--------");

    if (!place_ctx.block_locs.empty()) { //Only if placement exists
        for (auto blk_id : clb_nlist.blocks()) {
            fprintf(fp, "%-25s %-18s %-12s %-25s %-5d %-5d %-10d #%-13zu %-8s\n",
                    clb_nlist.block_name(blk_id).c_str(),
                    clb_nlist.block_type(blk_id)->name,
                    clb_nlist.block_type(blk_id)->pb_type->name,
                    clb_nlist.block_pb(blk_id)->name,
                    blk_locs[blk_id].loc.x,
                    blk_locs[blk_id].loc.y,
                    blk_locs[blk_id].loc.sub_tile,
                    size_t(blk_id),
                    (place_ctx.block_locs[blk_id].is_fixed ? "true" : "false"));
        }
        fprintf(fp, "\ntotal_HPWL: %d\n", total_hpwl());
        vtr::Matrix<int> overlap;
        find_overlap(overlap);
        fprintf(fp, "Occupancy diagram: \n");
        print_overlap(overlap, fp);
    }
    fclose(fp);
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
                                     const int solvedHPWL,
                                     const int spreadHPWL,
                                     const int legalHPWL) {
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
        "%8d "
        "%8d "
        "%8d \n",
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
                                      const int bestHPWL,
                                      const int stall) {
    VTR_LOG(
        "%4zu "
        "%6.3f "
        "%6.3f "
        "%8d "
        "%7d |\n",
        iter,
        time,
        iterTime,
        bestHPWL,
        stall);
    VTR_LOG("                                    |\n");
}

// sentinel for blks not solved in current iteration
int DONT_SOLVE = std::numeric_limits<int>::max();

// sentinel for blks not part of a placement macro
int NO_MACRO = -1;

#endif /* ENABLE_ANALYTIC_PLACE */
