/**
 * @file
 * @author  Alex Singer and Robert Luo
 * @date    October 2024
 * @brief   The definitions of the analytical solvers used in the AP flow and
 *          their base class.
 */

#include "analytical_solver.h"
#include <cstddef>
#include <cstdio>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include "device_grid.h"
#include "flat_placement_types.h"
#include "partial_placement.h"
#include "ap_netlist.h"
#include "vpr_error.h"
#include "vtr_assert.h"
#include "vtr_time.h"
#include "vtr_vector.h"

#ifdef EIGEN_INSTALLED
// The eigen library contains a warning in GCC13 for a null dereference. This
// causes the CI build to fail due to the warning. Ignoring the warning for
// these include files. Using push to return to the state of GCC diagnostics.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"

#include <Eigen/src/SparseCore/SparseMatrix.h>
#include <Eigen/SVD>
#include <Eigen/Sparse>
#include <Eigen/Eigenvalues>
#include <Eigen/IterativeLinearSolvers>

// Pop the GCC diagnostics state back to what it was before.
#pragma GCC diagnostic pop
#endif // EIGEN_INSTALLED

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_ap_analytical_solver solver_type,
                                                         const APNetlist& netlist,
                                                         const DeviceGrid& device_grid,
                                                         int log_verbosity) {
    // Based on the solver type passed in, build the solver.
    switch (solver_type) {
        case e_ap_analytical_solver::QP_Hybrid:
#ifdef EIGEN_INSTALLED
            return std::make_unique<QPHybridSolver>(netlist, device_grid, log_verbosity);
#else
            (void)netlist;
            (void)device_grid;
            (void)log_verbosity;
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "QP Hybrid Solver requires the Eigen library");
            break;
#endif // EIGEN_INSTALLED
        case e_ap_analytical_solver::LP_B2B:
#ifdef EIGEN_INSTALLED
            return std::make_unique<B2BSolver>(netlist, device_grid, log_verbosity);
#else
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "LP B2B Solver requires the Eigen library");
            break;
#endif // EIGEN_INSTALLED
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized analytical solver type");
            break;
    }
    return nullptr;
}

AnalyticalSolver::AnalyticalSolver(const APNetlist& netlist, int log_verbosity)
    : netlist_(netlist)
    , blk_id_to_row_id_(netlist.blocks().size(), APRowId::INVALID())
    , row_id_to_blk_id_(netlist.blocks().size(), APBlockId::INVALID())
    , log_verbosity_(log_verbosity) {
    // Get the number of moveable blocks in the netlist and create a unique
    // row ID from [0, num_moveable_blocks) for each moveable block in the
    // netlist.
    num_moveable_blocks_ = 0;
    num_fixed_blocks_ = 0;
    size_t current_row_id = 0;
    for (APBlockId blk_id : netlist.blocks()) {
        if (netlist.block_mobility(blk_id) == APBlockMobility::FIXED)
            num_fixed_blocks_++;
        if (netlist.block_mobility(blk_id) != APBlockMobility::MOVEABLE)
            continue;
        APRowId new_row_id = APRowId(current_row_id);
        blk_id_to_row_id_[blk_id] = new_row_id;
        row_id_to_blk_id_[new_row_id] = blk_id;
        current_row_id++;
        num_moveable_blocks_++;
    }
}

#ifdef EIGEN_INSTALLED

/**
 * @brief Helper method to add a connection between a src moveable node and a
 *        target APBlock with the given weight. This updates the tripleList and
 *        the constant vectors with the values necessary to solve the quadratic
 *        objective function.
 *
 * The A_sparse matrix is square and symmetric, so the use of the "row_id" as
 * input is arbitrary; it could easily have been "src_col_id".
 *
 * The src_row_id always represents a moveable node in the linear system. It can
 * represent a moveable APBlock or a star node.
 *
 * The target_blk_id may be either a moveable or fixed block.
 *
 * If the target block (t) is moveable, with source row s:
 *      A[s][s] = A[s][s] + weight
 *      A[t][t] = A[t][t] + weight
 *      A[s][t] = A[s][t] - weight
 *      A[t][s] = A[t][s] - weight
 * If the target block is fixed:
 *      A[s][s] = A[s][s] + weight
 *      b[s] = b[s] + pos[block(t)] * weight
 *
 * These update equations come from taking the partial derivatives of the
 * quadratic objective function w.r.t the moveable block locations. This is
 * explained in detail in the FastPlace paper.
 */
static inline void add_connection_to_system(size_t src_row_id,
                                            APBlockId target_blk_id,
                                            double weight,
                                            std::vector<Eigen::Triplet<double>>& tripletList,
                                            Eigen::VectorXd& b_x,
                                            Eigen::VectorXd& b_y,
                                            Eigen::SparseMatrix<double>& A_sparse,
                                            vtr::vector<APBlockId, APRowId>& blk_id_to_row_id,
                                            const APNetlist& netlist) {
    // Verify that this is a valid row.
    VTR_ASSERT_DEBUG(src_row_id < (size_t)A_sparse.rows());
    VTR_ASSERT_DEBUG(A_sparse.rows() == A_sparse.cols());
    // Verify that this is a valid block id.
    VTR_ASSERT_DEBUG(target_blk_id.is_valid());
    // The src_row_id is always a moveable block (rows in the matrix always
    // coorespond to a moveable APBlock or a star node.
    if (netlist.block_mobility(target_blk_id) == APBlockMobility::MOVEABLE) {
        // If the target is also moveable, update the coefficient matrix.
        size_t target_row_id = (size_t)blk_id_to_row_id[target_blk_id];
        VTR_ASSERT_DEBUG(target_row_id < (size_t)A_sparse.rows());
        tripletList.emplace_back(src_row_id, src_row_id, weight);
        tripletList.emplace_back(target_row_id, target_row_id, weight);
        tripletList.emplace_back(src_row_id, target_row_id, -weight);
        tripletList.emplace_back(target_row_id, src_row_id, -weight);
    } else {
        // If the target is fixed, update the coefficient matrix and the
        // constant vectors.
        tripletList.emplace_back(src_row_id, src_row_id, weight);
        VTR_ASSERT_DEBUG(netlist.block_loc(target_blk_id).x >= 0);
        VTR_ASSERT_DEBUG(netlist.block_loc(target_blk_id).y >= 0);
        // FIXME: These fixed block locations are aligned to the anchor of
        //        the tiles they are in. This is not correct. A method
        //        should be added to the netlist class or to a util file
        //        which can get a more accurate position.
        double blk_loc_x = netlist.block_loc(target_blk_id).x;
        double blk_loc_y = netlist.block_loc(target_blk_id).y;
        b_x(src_row_id) += weight * blk_loc_x;
        b_y(src_row_id) += weight * blk_loc_y;
    }
}

void QPHybridSolver::init_linear_system() {
    // Count the number of star nodes that the netlist will have.
    size_t num_star_nodes = 0;
    for (APNetId net_id : netlist_.nets()) {
        if (netlist_.net_pins(net_id).size() > star_num_pins_threshold)
            num_star_nodes++;
    }

    // Initialize the linear system with zeros.
    num_variables_ = num_moveable_blocks_ + num_star_nodes;
    A_sparse = Eigen::SparseMatrix<double>(num_variables_, num_variables_);
    b_x = Eigen::VectorXd::Zero(num_variables_);
    b_y = Eigen::VectorXd::Zero(num_variables_);

    // Create a list of triplets that will be used to create the sparse
    // coefficient matrix. This is the method recommended by Eigen to initialize
    // this matrix.
    // A triplet represents a non-zero entry in a sparse matrix:
    //      (row index, col index, value)
    // Where triplets at the same (row index, col index) are summed together.
    std::vector<Eigen::Triplet<double>> tripletList;
    // Reserve enough space for the triplets. This is just to help with
    // performance.
    // TODO: This can be made more space-efficient by getting the average fanout
    //       of all nets in the APNetlist. Ideally this should be not enough
    //       space, but be within a constant factor.
    size_t num_nets = netlist_.nets().size();
    tripletList.reserve(num_nets);

    // Create the connections using a hybrid connection model of the star and
    // clique connnection models.
    size_t star_node_offset = 0;
    for (APNetId net_id : netlist_.nets()) {
        size_t num_pins = netlist_.net_pins(net_id).size();
        VTR_ASSERT_DEBUG(num_pins > 1);
        if (num_pins > star_num_pins_threshold) {
            // Create a star node and connect each block in the net to the star
            // node.
            // Using the weight from FastPlace
            // TODO: Investigate other weight terms.
            double w = static_cast<double>(num_pins) / static_cast<double>(num_pins - 1);
            size_t star_node_id = num_moveable_blocks_ + star_node_offset;
            for (APPinId pin_id : netlist_.net_pins(net_id)) {
                APBlockId blk_id = netlist_.pin_block(pin_id);
                add_connection_to_system(star_node_id, blk_id, w, tripletList,
                                         b_x, b_y, A_sparse, blk_id_to_row_id_,
                                         netlist_);
            }
            star_node_offset++;
        } else {
            // Create a clique connection where every block in a net connects
            // exactly once to every other block in the net.
            // Using the weight from FastPlace
            // TODO: Investigate other weight terms.
            double w = 1.0 / static_cast<double>(num_pins - 1);
            for (size_t ipin_idx = 0; ipin_idx < num_pins; ipin_idx++) {
                APPinId first_pin_id = netlist_.net_pin(net_id, ipin_idx);
                APBlockId first_blk_id = netlist_.pin_block(first_pin_id);
                for (size_t jpin_idx = ipin_idx + 1; jpin_idx < num_pins; jpin_idx++) {
                    APPinId second_pin_id = netlist_.net_pin(net_id, jpin_idx);
                    APBlockId second_blk_id = netlist_.pin_block(second_pin_id);
                    // Make sure that the first node is moveable. This makes
                    // creating the connection easier.
                    if (netlist_.block_mobility(first_blk_id) == APBlockMobility::FIXED) {
                        // If both blocks are fixed, no connection needs to be
                        // made; just continue.
                        if (netlist_.block_mobility(second_blk_id) == APBlockMobility::FIXED) {
                            continue;
                        }
                        // If the second block is moveable, swap the first and
                        // second block so the first block is the moveable one.
                        std::swap(first_blk_id, second_blk_id);
                    }
                    size_t first_row_id = (size_t)blk_id_to_row_id_[first_blk_id];
                    add_connection_to_system(first_row_id, second_blk_id, w, tripletList,
                                             b_x, b_y, A_sparse, blk_id_to_row_id_,
                                             netlist_);
                }
            }
        }
    }

    // Make sure that the number of star nodes created matches the number of
    // star nodes we pre-calculated we would have.
    VTR_ASSERT_SAFE(num_star_nodes == star_node_offset);

    // Populate the A_sparse matrix using the triplets.
    A_sparse.setFromTriplets(tripletList.begin(), tripletList.end());
}

void QPHybridSolver::update_linear_system_with_anchors(
    Eigen::SparseMatrix<double>& A_sparse_diff,
    Eigen::VectorXd& b_x_diff,
    Eigen::VectorXd& b_y_diff,
    PartialPlacement& p_placement,
    unsigned iteration) {
    // Anchor weights grow exponentially with iteration.
    double coeff_pseudo_anchor = anchor_weight_mult_ * std::exp((double)iteration / anchor_weight_exp_fac_);
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks_; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id_[row_id];
        double pseudo_w = coeff_pseudo_anchor;
        A_sparse_diff.coeffRef(row_id_idx, row_id_idx) += pseudo_w;
        b_x_diff(row_id_idx) += pseudo_w * p_placement.block_x_locs[blk_id];
        b_y_diff(row_id_idx) += pseudo_w * p_placement.block_y_locs[blk_id];
    }
}

void QPHybridSolver::init_guesses(const DeviceGrid& device_grid) {
    // If the number of fixed blocks is zero, initialized the guesses to the
    // center of the device.
    if (num_fixed_blocks_ == 0) {
        guess_x = Eigen::VectorXd::Constant(num_variables_, device_grid.width() / 2.0);
        guess_y = Eigen::VectorXd::Constant(num_variables_, device_grid.height() / 2.0);
        return;
    }

    // Compute the centroid of all fixed blocks in the netlist.
    t_flat_pl_loc centroid({0.0f, 0.0f, 0.0f});
    unsigned num_blks_summed = 0;
    for (APBlockId blk_id : netlist_.blocks()) {
        // We only get the centroid of fixed blocks since these are the only
        // blocks with positions that we know.
        if (netlist_.block_mobility(blk_id) != APBlockMobility::FIXED)
            continue;
        // Get the flat location of the fixed block.
        APFixedBlockLoc fixed_blk_loc = netlist_.block_loc(blk_id);
        VTR_ASSERT_SAFE(fixed_blk_loc.x != APFixedBlockLoc::UNFIXED_DIM);
        VTR_ASSERT_SAFE(fixed_blk_loc.y != APFixedBlockLoc::UNFIXED_DIM);
        VTR_ASSERT_SAFE(fixed_blk_loc.layer_num != APFixedBlockLoc::UNFIXED_DIM);
        t_flat_pl_loc flat_blk_loc;
        flat_blk_loc.x = fixed_blk_loc.x;
        flat_blk_loc.y = fixed_blk_loc.y;
        flat_blk_loc.layer = fixed_blk_loc.layer_num;
        // Accumulate into the centroid.
        centroid += flat_blk_loc;
        num_blks_summed++;
    }
    // Divide the sum by the number of fixed blocks.
    VTR_ASSERT_SAFE(num_blks_summed == num_fixed_blocks_);
    centroid /= static_cast<float>(num_blks_summed);

    // Set the guesses to the centroid location.
    guess_x = Eigen::VectorXd::Constant(num_variables_, centroid.x);
    guess_y = Eigen::VectorXd::Constant(num_variables_, centroid.y);
}

void QPHybridSolver::solve(unsigned iteration, PartialPlacement& p_placement) {
    // In the first iteration, if the number of fixed blocks is 0, set the
    // placement to be equal to the guess. The solver below will just set the
    // solution to the zero vector if we do not set it to the guess directly.
    if (iteration == 0 && num_fixed_blocks_ == 0) {
        store_solution_into_placement(guess_x, guess_y, p_placement);
        return;
    }

    // Create a temporary linear system which will contain the original linear
    // system which may be updated to include the anchor points.
    Eigen::SparseMatrix<double> A_sparse_diff = Eigen::SparseMatrix<double>(A_sparse);
    Eigen::VectorXd b_x_diff = Eigen::VectorXd(b_x);
    Eigen::VectorXd b_y_diff = Eigen::VectorXd(b_y);
    // In the first iteration, the orginal linear system is used.
    // In any other iteration, use the moveable APBlocks current placement as
    //                         anchor-points (fixed block positions).
    if (iteration != 0) {
        update_linear_system_with_anchors(A_sparse_diff, b_x_diff, b_y_diff,
                                          p_placement, iteration);
    }
    // Verify that the constant vectors are valid.
    VTR_ASSERT_SAFE_MSG(!b_x_diff.hasNaN(), "b_x has NaN!");
    VTR_ASSERT_SAFE_MSG(!b_y_diff.hasNaN(), "b_y has NaN!");

    // Set up the ConjugateGradient Solver using the coefficient matrix.
    // TODO: can change cg.tolerance to increase performance when needed
    //  - This tolerance may need to be a function of the number of nets.
    //  - Instead of normalizing the fixed blocks, the tolerance can be scaled
    //    by the size of the device.
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> cg;
    cg.compute(A_sparse_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at compute!");
    // Use the solver to solve for x and y using the constant vectors
    Eigen::VectorXd x = cg.solveWithGuess(b_x_diff, guess_x);
    total_num_cg_iters_ += cg.iterations();
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_x!");
    Eigen::VectorXd y = cg.solveWithGuess(b_y_diff, guess_y);
    total_num_cg_iters_ += cg.iterations();
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_y!");

    // Write the results back into the partial placement object.
    store_solution_into_placement(x, y, p_placement);

    // Update the guess. The guess for the next iteration is the solution in
    // this iteration.
    guess_x = x;
    guess_y = y;
}

void QPHybridSolver::store_solution_into_placement(const Eigen::VectorXd& x_soln,
                                                   const Eigen::VectorXd& y_soln,
                                                   PartialPlacement& p_placement) {

    // NOTE: The first [0, num_moveable_blocks_) rows always represent the
    //       moveable APBlocks. The star nodes always come after and are ignored
    //       in the solution.
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks_; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id_[row_id];
        VTR_ASSERT_DEBUG(blk_id.is_valid());
        VTR_ASSERT_DEBUG(netlist_.block_mobility(blk_id) == APBlockMobility::MOVEABLE);
        // Due to the iterative nature of CG, it is possible for the solver to
        // overstep 0 and return a negative number by an incredibly small margin.
        // Clamp the number to 0 in this case.
        // TODO: Should investigate good bounds on this, the bounds below were
        //       chosen since any difference higher than 1e-9 would concern me.
        double x_pos = x_soln[row_id_idx];
        if (x_pos < 0.0) {
            VTR_ASSERT_SAFE(std::abs(x_pos) < negative_soln_tolerance_);
            x_pos = 0.0;
        }
        double y_pos = y_soln[row_id_idx];
        if (y_pos < 0.0) {
            VTR_ASSERT_SAFE(std::abs(y_pos) < negative_soln_tolerance_);
            y_pos = 0.0;
        }
        p_placement.block_x_locs[blk_id] = x_pos;
        p_placement.block_y_locs[blk_id] = y_pos;
    }
}

void QPHybridSolver::print_statistics() {
    VTR_LOG("QP-Hybrid Solver Statistics:\n");
    VTR_LOG("\tTotal number of CG iterations: %u\n", total_num_cg_iters_);
}

void B2BSolver::solve(unsigned iteration, PartialPlacement& p_placement) {
    // Store an initial placement into the p_placement object as a starting point
    // for the B2B solver.
    if (iteration == 0) {
        // In the first iteration, we have no prior information.
        // Run the intial placer to get a first guess.
        switch (initial_placement_ty_) {
            case e_initial_placement_type::LeastDense:
                initialize_placement_least_dense(p_placement);
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_AP, "Unknown initial placement type");
        }
    } else {
        // After the first iteration, the prior solved solution will serve as
        // the best starting points for the bounds.

        // Save the legalized solution; we need it for the anchors.
        block_x_locs_legalized = p_placement.block_x_locs;
        block_y_locs_legalized = p_placement.block_y_locs;

        // Store last solved position into p_placement for b2b model
        p_placement.block_x_locs = block_x_locs_solved;
        p_placement.block_y_locs = block_y_locs_solved;
    }

    // Run the B2B solver using p_placement as a starting point.
    b2b_solve_loop(iteration, p_placement);

    // Store the solved solutions for the next iteration.
    block_x_locs_solved = p_placement.block_x_locs;
    block_y_locs_solved = p_placement.block_y_locs;
}

void B2BSolver::initialize_placement_least_dense(PartialPlacement& p_placement) {
    // Find a gap for the blocks such that each block can fit onto the device
    // if they were evenly spaced by this gap.
    double gap = std::sqrt(device_grid_height_ * device_grid_width_ / static_cast<double>(num_moveable_blocks_));

    // Assuming this gap, get how many columns/rows of blocks there will be.
    size_t cols = std::ceil(device_grid_width_ / gap);
    size_t rows = std::ceil(device_grid_height_ / gap);

    // Spread the blocks at these grid coordinates.
    for (size_t r = 0; r <= rows; r++) {
        for (size_t c = 0; c <= cols; c++) {
            size_t i = r * cols + c;
            if (i >= num_moveable_blocks_)
                break;
            APRowId row_id = APRowId(i);
            APBlockId blk_id = row_id_to_blk_id_[row_id];
            p_placement.block_x_locs[blk_id] = c * gap;
            p_placement.block_y_locs[blk_id] = r * gap;
        }
    }
}

void B2BSolver::b2b_solve_loop(unsigned iteration, PartialPlacement& p_placement) {
    // Set up the guesses for x and y to help CG converge faster
    // A good guess for B2B is the last solved solution.
    Eigen::VectorXd x_guess(num_moveable_blocks_);
    Eigen::VectorXd y_guess(num_moveable_blocks_);
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks_; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id_[row_id];
        x_guess(row_id_idx) = p_placement.block_x_locs[blk_id];
        y_guess(row_id_idx) = p_placement.block_y_locs[blk_id];
    }

    // Create a timer to keep track of how long each part of the solver take.
    vtr::Timer runtime_timer;

    // To solve B2B, we need to do the following:
    //      1) Set up the connectivity matrix and constant vectors based on the
    //         bounds of the current solution (stored in p_placement).
    //      2) Solve the system of equations using CG and store the result into
    //         p_placement.
    //      3) Repeat. Note: We need to repeat step 1 and 2 iteratively since
    //         the bounds are likely to have changed after step 2.
    // We stop when it looks like the placement is converging (the change in
    // HPWL is sufficiently small for a few iterations).
    double prev_hpwl = std::numeric_limits<double>::max();
    double curr_hpwl = prev_hpwl;
    unsigned num_convergence = 0;
    for (unsigned counter = 0; counter < max_num_bound_updates_; counter++) {
        VTR_LOGV(log_verbosity_ >= 10,
                 "\tPlacement HPWL in b2b loop: %f\n",
                 p_placement.get_hpwl(netlist_));

        // Set up the linear system, including anchor points.
        float build_linear_system_start_time = runtime_timer.elapsed_sec();
        init_linear_system(p_placement);
        if (iteration != 0)
            update_linear_system_with_anchors(iteration);
        total_time_spent_building_linear_system_ += runtime_timer.elapsed_sec() - build_linear_system_start_time;
        VTR_ASSERT_SAFE_MSG(!b_x.hasNaN(), "b_x has NaN!");
        VTR_ASSERT_SAFE_MSG(!b_y.hasNaN(), "b_y has NaN!");
        VTR_ASSERT_SAFE_MSG((b_x.array() >= 0).all(), "b_x has NaN!");
        VTR_ASSERT_SAFE_MSG((b_y.array() >= 0).all(), "b_y has NaN!");

        // Build the solvers for each dimension.
        // Note: Since we have two different connectivity matrices, we need to
        //       different CG solver objects.
        float solve_linear_system_start_time = runtime_timer.elapsed_sec();
        Eigen::VectorXd x, y;
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> cg_x;
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> cg_y;
        cg_x.compute(A_sparse_x);
        cg_y.compute(A_sparse_y);
        VTR_ASSERT_SAFE_MSG(cg_x.info() == Eigen::Success, "Conjugate Gradient failed at compute for A_x!");
        VTR_ASSERT_SAFE_MSG(cg_y.info() == Eigen::Success, "Conjugate Gradient failed at compute for A_y!");
        cg_x.setMaxIterations(max_cg_iterations_);
        cg_y.setMaxIterations(max_cg_iterations_);

        // Solve the x dimension.
        x = cg_x.solveWithGuess(b_x, x_guess);
        total_num_cg_iters_ += cg_x.iterations();
        VTR_LOGV(log_verbosity_ >= 20, "\t\tNum CG-x iter: %zu\n", cg_x.iterations());

        // Solve the y dimension.
        y = cg_y.solveWithGuess(b_y, y_guess);
        total_num_cg_iters_ += cg_y.iterations();
        VTR_LOGV(log_verbosity_ >= 20, "\t\tNum CG-y iter: %zu\n", cg_y.iterations());

        total_time_spent_solving_linear_system_ += runtime_timer.elapsed_sec() - solve_linear_system_start_time;

        // Save the result into the partial placement object.
        store_solution_into_placement(x, y, p_placement);

        // If the current HPWL is larger than the previous HPWL (i.e. the HPWL
        // got worst since last B2B iter) or the gap between the two solutions
        // is small. Increment a counter.
        // TODO: Since, in theory, the HPWL could get worst due to numerical
        //       reasons, should we save the best result? May not be worth it...
        curr_hpwl = p_placement.get_hpwl(netlist_);
        double target_gap = b2b_convergence_gap_fac_ * curr_hpwl;
        if (curr_hpwl > prev_hpwl || std::abs(curr_hpwl - prev_hpwl) < target_gap)
            num_convergence++;

        // If the HPWL got close enough times, stop. This is to allow the HPWL
        // to "bounce", which can happen as it converges.
        // This trades-off quality for run time.
        if (num_convergence >= target_num_b2b_convergences_)
            break;
        prev_hpwl = curr_hpwl;

        // Update the guesses with the most recent answer
        x_guess = x;
        y_guess = y;
    }
}

namespace {
/**
 * @brief Struct used to hold the bounding blocks of an AP net.
 */
struct APNetBounds {
    /// @brief The leftmost block in the net.
    APBlockId min_x_blk;
    /// @brief The rightmost block in the net.
    APBlockId max_x_blk;
    /// @brief The bottom-most block in the net.
    APBlockId min_y_blk;
    /// @brief The top-most block in the net.
    APBlockId max_y_blk;
};

} // namespace

/**
 * @brief Helper method to get the unique bounding blocks of a given net.
 *
 * In the B2B model, we do not want the same block to be the bounds in a given
 * dimension. Therefore, if all blocks share the same x location for example,
 * different bounds will be chosen for the x dimension.
 */
static inline APNetBounds get_unique_net_bounds(APNetId net_id,
                                                const PartialPlacement& p_placement,
                                                const APNetlist& netlist) {
    VTR_ASSERT_SAFE_MSG(netlist.net_pins(net_id).size() != 0,
                        "Cannot get the bounds of an empty net");
    VTR_ASSERT_SAFE_MSG(netlist.net_pins(net_id).size() >= 2,
                        "Expect nets to have at least 2 pins");

    APNetBounds bounds;
    double max_x_pos = std::numeric_limits<double>::lowest();
    double min_x_pos = std::numeric_limits<double>::max();
    double max_y_pos = std::numeric_limits<double>::lowest();
    double min_y_pos = std::numeric_limits<double>::max();

    for (APPinId pin_id : netlist.net_pins(net_id)) {
        // Update the bounds based on the position of the block that has this pin.
        APBlockId blk_id = netlist.pin_block(pin_id);
        double x_pos = p_placement.block_x_locs[blk_id];
        double y_pos = p_placement.block_y_locs[blk_id];
        if (x_pos < min_x_pos) {
            min_x_pos = x_pos;
            bounds.min_x_blk = blk_id;
        }
        if (y_pos < min_y_pos) {
            min_y_pos = y_pos;
            bounds.min_y_blk = blk_id;
        }
        if (x_pos > max_x_pos) {
            max_x_pos = x_pos;
            bounds.max_x_blk = blk_id;
        }
        if (y_pos > max_y_pos) {
            max_y_pos = y_pos;
            bounds.max_y_blk = blk_id;
        }

        // In the case of a tie, we do not want to have the same blocks as bounds.
        // If there is a tie for the max position, and the current min bound is
        // not this block, take the incoming block.
        if (x_pos == max_x_pos && bounds.min_x_blk != blk_id) {
            max_x_pos = x_pos;
            bounds.max_x_blk = blk_id;
        }
        if (y_pos == max_y_pos && bounds.min_y_blk != blk_id) {
            max_y_pos = y_pos;
            bounds.max_y_blk = blk_id;
        }
    }

    // Ensure the same block is set as the bounds.
    // If there is not a bug in the above code, then this could imply that a
    // net only connects to a single APBlock, which does not make sense in this
    // context.
    VTR_ASSERT_SAFE(bounds.min_x_blk != bounds.max_x_blk);
    VTR_ASSERT_SAFE(bounds.min_y_blk != bounds.max_y_blk);

    return bounds;
}

void B2BSolver::add_connection_to_system(APBlockId first_blk_id,
                                         APBlockId second_blk_id,
                                         size_t num_pins,
                                         const vtr::vector<APBlockId, double>& blk_locs,
                                         std::vector<Eigen::Triplet<double>>& triplet_list,
                                         Eigen::VectorXd& b) {
    // To make the code below simpler, we assume that the first block is always
    // moveable.
    if (netlist_.block_mobility(first_blk_id) != APBlockMobility::MOVEABLE) {
        if (netlist_.block_mobility(second_blk_id) != APBlockMobility::MOVEABLE) {
            // If both blocks are fixed, do not connect them.
            return;
        }
        // If the first block is fixed and the second block is moveable, swap them.
        std::swap(first_blk_id, second_blk_id);
    }

    // Compute the weight of the connection.
    //  From the Kraftwerk2 paper:
    //          w = (2 / (P - 1)) * (1 / distance)
    //
    // epsilon is needed to prevent numerical instability. If two nodes are on top of each other.
    // The denominator of weight is zero, which causes infinity term in the matrix. Another way of
    // interpreting epsilon is the minimum distance two nodes are considered to be in placement.
    double dist = std::max(std::abs(blk_locs[first_blk_id] - blk_locs[second_blk_id]), distance_epsilon_);
    double w = (2.0 / static_cast<double>(num_pins - 1)) * (1.0 / dist);

    // Update the connectivity matrix and the constant vector.
    // This is similar to how connections are added for the quadratic formulation.
    size_t first_row_id = (size_t)blk_id_to_row_id_[first_blk_id];
    if (netlist_.block_mobility(second_blk_id) == APBlockMobility::MOVEABLE) {
        size_t second_row_id = (size_t)blk_id_to_row_id_[second_blk_id];
        triplet_list.emplace_back(first_row_id, first_row_id, w);
        triplet_list.emplace_back(second_row_id, second_row_id, w);
        triplet_list.emplace_back(first_row_id, second_row_id, -w);
        triplet_list.emplace_back(second_row_id, first_row_id, -w);
    } else {
        triplet_list.emplace_back(first_row_id, first_row_id, w);
        b(first_row_id) += w * blk_locs[second_blk_id];
    }
}

void B2BSolver::init_linear_system(PartialPlacement& p_placement) {
    // Reset the linear system
    A_sparse_x = Eigen::SparseMatrix<double>(num_moveable_blocks_, num_moveable_blocks_);
    A_sparse_y = Eigen::SparseMatrix<double>(num_moveable_blocks_, num_moveable_blocks_);
    b_x = Eigen::VectorXd::Zero(num_moveable_blocks_);
    b_y = Eigen::VectorXd::Zero(num_moveable_blocks_);

    // Create triplet lists to store the sparse positions to update and reserve
    // space for them.
    size_t num_nets = netlist_.nets().size();
    std::vector<Eigen::Triplet<double>> triplet_list_x;
    triplet_list_x.reserve(num_nets);
    std::vector<Eigen::Triplet<double>> triplet_list_y;
    triplet_list_y.reserve(num_nets);

    for (APNetId net_id : netlist_.nets()) {
        size_t num_pins = netlist_.net_pins(net_id).size();
        VTR_ASSERT_SAFE_MSG(num_pins > 1, "net must have at least 2 pins");

        // Find the bounding blocks
        APNetBounds net_bounds = get_unique_net_bounds(net_id, p_placement, netlist_);

        // Add an edge from every block to their bounds (ignoring the bounds
        // themselves for now).
        // FIXME: If one block has multiple pins, it may connect to the bounds
        //        multiple times. Should investigate the effect of this.
        for (APPinId pin_id : netlist_.net_pins(net_id)) {
            APBlockId blk_id = netlist_.pin_block(pin_id);
            if (blk_id != net_bounds.max_x_blk && blk_id != net_bounds.min_x_blk) {
                add_connection_to_system(blk_id, net_bounds.max_x_blk, num_pins, p_placement.block_x_locs, triplet_list_x, b_x);
                add_connection_to_system(blk_id, net_bounds.min_x_blk, num_pins, p_placement.block_x_locs, triplet_list_x, b_x);
            }
            if (blk_id != net_bounds.max_y_blk && blk_id != net_bounds.min_y_blk) {
                add_connection_to_system(blk_id, net_bounds.max_y_blk, num_pins, p_placement.block_y_locs, triplet_list_y, b_y);
                add_connection_to_system(blk_id, net_bounds.min_y_blk, num_pins, p_placement.block_y_locs, triplet_list_y, b_y);
            }
        }

        // Connect the bounds to each other. Its just easier to put these here
        // instead of in the for loop above.
        add_connection_to_system(net_bounds.max_x_blk, net_bounds.min_x_blk, num_pins, p_placement.block_x_locs, triplet_list_x, b_x);
        add_connection_to_system(net_bounds.max_y_blk, net_bounds.min_y_blk, num_pins, p_placement.block_y_locs, triplet_list_y, b_y);
    }

    // Build the sparse connectivity matrices from the triplets.
    A_sparse_x.setFromTriplets(triplet_list_x.begin(), triplet_list_x.end());
    A_sparse_y.setFromTriplets(triplet_list_y.begin(), triplet_list_y.end());
}

// This function adds anchors for legalized solution. Anchors are treated as fixed node,
// each connecting to a movable node. Number of nodes in a anchor net is always 2.
void B2BSolver::update_linear_system_with_anchors(unsigned iteration) {
    VTR_ASSERT_SAFE_MSG(iteration != 0,
                        "no fixed solution to anchor to in the first iteration");
    // Get the anchor weight based on the iteration number. We want the anchor
    // weights to get stronger as we get later in global placement. Found that
    // an exponential weight term worked well for this.
    double coeff_pseudo_anchor = anchor_weight_mult_ * std::exp((double)iteration / anchor_weight_exp_fac_);

    // Add an anchor for each moveable block to its solved position.
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks_; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id_[row_id];
        double pseudo_w_x = coeff_pseudo_anchor * 2.0;
        double pseudo_w_y = coeff_pseudo_anchor * 2.0;
        A_sparse_x.coeffRef(row_id_idx, row_id_idx) += pseudo_w_x;
        A_sparse_y.coeffRef(row_id_idx, row_id_idx) += pseudo_w_y;
        b_x(row_id_idx) += pseudo_w_x * block_x_locs_legalized[blk_id];
        b_y(row_id_idx) += pseudo_w_y * block_y_locs_legalized[blk_id];
    }
}

void B2BSolver::store_solution_into_placement(Eigen::VectorXd& x_soln,
                                              Eigen::VectorXd& y_soln,
                                              PartialPlacement& p_placement) {
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks_; row_id_idx++) {
        // Since we are capping the number of iterations, the solver may not
        // have enough time to converge on a solution that is on the device.
        // We just clamp the solution to zero for now.
        // TODO: Should handle this better. If the solution is very negative
        //       it may indicate a bug.
        if (x_soln[row_id_idx] < 0.0)
            x_soln[row_id_idx] = 0.0;
        if (y_soln[row_id_idx] < 0.0)
            y_soln[row_id_idx] = 0.0;

        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id_[row_id];
        p_placement.block_x_locs[blk_id] = x_soln[row_id_idx];
        p_placement.block_y_locs[blk_id] = y_soln[row_id_idx];
    }
}

void B2BSolver::print_statistics() {
    VTR_LOG("B2B Solver Statistics:\n");
    VTR_LOG("\tTotal number of CG iterations: %u\n", total_num_cg_iters_);
    VTR_LOG("\tTotal time spent building linear system: %g seconds\n",
            total_time_spent_building_linear_system_);
    VTR_LOG("\tTotal time spent solving linear system: %g seconds\n",
            total_time_spent_solving_linear_system_);
}

#endif // EIGEN_INSTALLED
