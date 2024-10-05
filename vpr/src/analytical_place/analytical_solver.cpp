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
#include <memory>
#include <utility>
#include <vector>
#include "partial_placement.h"
#include "ap_netlist.h"
#include "vpr_error.h"
#include "vtr_assert.h"
#include "vtr_vector.h"

#ifdef EIGEN_INSTALLED
#include <Eigen/src/SparseCore/SparseMatrix.h>
#include <Eigen/SVD>
#include <Eigen/Sparse>
#include <Eigen/Eigenvalues>
#include <Eigen/IterativeLinearSolvers>
#endif // EIGEN_INSTALLED

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type,
                                                         const APNetlist& netlist) {
    // Based on the solver type passed in, build the solver.
    switch (solver_type) {
        case e_analytical_solver::QP_HYBRID:
#ifdef EIGEN_INSTALLED
            return std::make_unique<QPHybridSolver>(netlist);
#else
            (void)netlist;
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "QP Hybrid Solver requires the Eigen library");
            break;
#endif // EIGEN_INSTALLED
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized analytical solver type");
            break;
    }
    return nullptr;
}

AnalyticalSolver::AnalyticalSolver(const APNetlist& netlist)
    : netlist_(netlist),
      blk_id_to_row_id_(netlist.blocks().size(), APRowId::INVALID()),
      row_id_to_blk_id_(netlist.blocks().size(), APBlockId::INVALID()) {
    // Get the number of moveable blocks in the netlist and create a unique
    // row ID from [0, num_moveable_blocks) for each moveable block in the
    // netlist.
    num_moveable_blocks_ = 0;
    size_t current_row_id = 0;
    for (APBlockId blk_id : netlist.blocks()) {
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
    size_t num_variables = num_moveable_blocks_ + num_star_nodes;
    A_sparse = Eigen::SparseMatrix<double>(num_variables, num_variables);
    b_x = Eigen::VectorXd::Zero(num_variables);
    b_y = Eigen::VectorXd::Zero(num_variables);

    // Create a list of triplets that will be used to create the sparse
    // coefficient matrix. This is the method recommended by Eigen to initialize
    // this matrix.
    // A triplet represents a non-zero entry in a sparse matrix:
    //      (row index, col index, value)
    // Where triplets at the same (row index, col index) are summed together.
    std::vector<Eigen::Triplet<double>> tripletList;
    // Reserve enough space for the triplets. This is just to help with
    // performance.
    // This is an over-estimate that assumes that each net connnects to all
    // moveable blocks using a star node.
    // TODO: This can be made more space-efficient by getting the average fanout
    //       of all nets in the APNetlist. Ideally this should be not enough
    //       space, but be within a constant factor.
    size_t num_nets = netlist_.nets().size();
    tripletList.reserve(num_moveable_blocks_ * num_nets);

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

/**
 * @brief Helper method to update the linear system with anchors to the current
 *        partial placement.
 *
 * For each moveable block (with row = i) in the netlist:
 *      A[i][i] = A[i][i] + coeff_pseudo_anchor;
 *      b[i] = b[i] + pos[block(i)] * coeff_pseudo_anchor;
 * Where coeff_pseudo_anchor grows with each iteration.
 *
 * This is basically a fast way of adding a connection between all moveable
 * blocks in the netlist and their target fixed placement location.
 *
 * See add_connection_to_system.
 *
 *  @param A_sparse_diff    The ceofficient matrix to update.
 *  @param b_x_diff         The x-dimension constant vector to update.
 *  @param b_y_diff         The y-dimension constant vector to update.
 *  @param p_placement      The location the moveable blocks should be anchored
 *                          to.
 *  @param num_moveable_blocks  The number of moveable blocks in the netlist.
 *  @param row_id_to_blk_id     Lookup for the row id from the APBlock Id.
 *  @param iteration        The current iteration of the Global Placer.
 */
static inline void update_linear_system_with_anchors(
                               Eigen::SparseMatrix<double> &A_sparse_diff,
                               Eigen::VectorXd &b_x_diff,
                               Eigen::VectorXd &b_y_diff,
                               PartialPlacement& p_placement,
                               size_t num_moveable_blocks,
                               vtr::vector<APRowId, APBlockId> row_id_to_blk_id,
                               unsigned iteration) {
    // Anchor weights grow exponentially with iteration.
    double coeff_pseudo_anchor = 0.01 * std::exp((double)iteration/5);
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id[row_id];
        double pseudo_w = coeff_pseudo_anchor;
        A_sparse_diff.coeffRef(row_id_idx, row_id_idx) += pseudo_w;
        b_x_diff(row_id_idx) += pseudo_w * p_placement.block_x_locs[blk_id];
        b_y_diff(row_id_idx) += pseudo_w * p_placement.block_y_locs[blk_id];
    }
}

void QPHybridSolver::solve(unsigned iteration, PartialPlacement &p_placement) {
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
                                          p_placement, num_moveable_blocks_,
                                          row_id_to_blk_id_, iteration);
    }
    // Verify that the constant vectors are valid.
    VTR_ASSERT_DEBUG(!b_x_diff.hasNaN() && "b_x has NaN!");
    VTR_ASSERT_DEBUG(!b_y_diff.hasNaN() && "b_y has NaN!");

    // Set up the ConjugateGradient Solver using the coefficient matrix.
    // TODO: can change cg.tolerance to increase performance when needed
    //  - This tolerance may need to be a function of the number of nets.
    //  - Instead of normalizing the fixed blocks, the tolerance can be scaled
    //    by the size of the device.
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
    cg.compute(A_sparse_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at compute!");
    // Use the solver to solve for x and y using the constant vectors
    // TODO: Use solve with guess to make this faster. Use the previous placement
    //       as a guess.
    Eigen::VectorXd x = cg.solve(b_x_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_x!");
    Eigen::VectorXd y = cg.solve(b_y_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_y!");

    // Write the results back into the partial placement object.
    // NOTE: The first [0, num_moveable_blocks_) rows always represent the
    //       moveable APBlocks. The star nodes always come after and are ignored
    //       in the solution.
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks_; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id_[row_id];
        VTR_ASSERT_DEBUG(blk_id.is_valid());
        VTR_ASSERT_DEBUG(netlist_.block_mobility(blk_id) == APBlockMobility::MOVEABLE);
        p_placement.block_x_locs[blk_id] = x[row_id_idx];
        p_placement.block_y_locs[blk_id] = y[row_id_idx];
    }
}

#endif // EIGEN_INSTALLED

