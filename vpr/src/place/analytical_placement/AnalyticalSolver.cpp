
#include "AnalyticalSolver.h"
#include <Eigen/src/SparseCore/SparseMatrix.h>
#include <Eigen/SVD>
#include <Eigen/Sparse>
#include <Eigen/Eigenvalues>
#include <Eigen/IterativeLinearSolvers>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <memory>
#include <random>
#include <utility>
#include <vector>
#include "PartialPlacement.h"
#include "ap_netlist.h"
#include "ap_utils.h"
#include "globals.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vtr_assert.h"
#include "vtr_vector.h"

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type, const APNetlist& netlist) {
    if (solver_type == e_analytical_solver::QP_HYBRID)
        return std::make_unique<QPHybridSolver>(netlist);
    else if(solver_type == e_analytical_solver::B2B)
        return std::make_unique<B2BSolver>(netlist);
    VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Unrecognized analytical solver type");
    return nullptr;
}

AnalyticalSolver::AnalyticalSolver(const APNetlist& inetlist) : netlist(inetlist),
                                                                blk_id_to_row_id(netlist.blocks().size(), APRowId::INVALID()),
                                                                row_id_to_blk_id(netlist.blocks().size(), APBlockId::INVALID()) {
    num_moveable_blocks = 0;
    size_t current_row_id = 0;
    for (APBlockId blk_id : netlist.blocks()) {
        if (netlist.block_type(blk_id) != APBlockType::MOVEABLE)
            continue;
        APRowId new_row_id = APRowId(current_row_id);
        blk_id_to_row_id[blk_id] = new_row_id;
        row_id_to_blk_id[new_row_id] = blk_id;
        current_row_id++;
        num_moveable_blocks++;
    }
}

/*
static bool contains_NaN(Eigen::SparseMatrix<double> &mat) {
    bool contains_nan = false;
    for (int k = 0; k < mat.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(mat, k); it; ++it) {
            if (std::isnan(it.value())) {
                contains_nan = true;
                break;
            }
        }
        if (contains_nan) {
            break;
        }
    }
    return contains_nan;
}

static bool isSymmetric(const Eigen::SparseMatrix<double> &A){
    return A.isApprox(A.transpose());
}

static bool isSemiPosDef(const Eigen::SparseMatrix<double> &A){
    // TODO: This is slow, it could be faster if we use Cholesky decomposition (Eigen::SimplicialLDLT), still O(n^3)
    Eigen::SelfAdjointEigenSolver<Eigen::SparseMatrix<double>> solver;
    solver.compute(A);
    VTR_ASSERT(solver.info() == Eigen::Success && "Eigen solver failed!");
    Eigen::VectorXcd eigenvalues = solver.eigenvalues();
    for (int i = 0; i < eigenvalues.size(); ++i) {
        VTR_ASSERT(eigenvalues(i).imag() == 0 && "Eigenvalue not real!");
        if(eigenvalues(i).real()<=0){
            return false;
        }
    }
    return true;
}

static double conditionNumber(const Eigen::SparseMatrix<double> &A) {
    // Since real and sym, instead of sigmamax/sigmamin, lambdamax^2/lambdamin^2 is also ok, could be faster. 
    // Eigen::MatrixXd denseMat = Eigen::MatrixXd(A);
    // Eigen::JacobiSVD<Eigen::MatrixXd> svd(denseMat);
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(A);
    Eigen::VectorXd singularValues = svd.singularValues();
    return singularValues(0)/singularValues(singularValues.size()-1);
}
*/

// FIXME: Make this a member function, this is an insane number of arguments.
static inline void populate_update_hybrid_matrix(Eigen::SparseMatrix<double> &A_sparse_diff,
                                                 Eigen::VectorXd &b_x_diff,
                                                 Eigen::VectorXd &b_y_diff,
                                                 PartialPlacement& p_placement,
                                                 size_t num_moveable_blocks,
                                                 vtr::vector<APRowId, APBlockId> row_id_to_blk_id,
                                                 unsigned iteration) {
    // Aii = Aii+w, bi = bi + wi * xi
    // TODO: verify would it be better if the initial weights are not part of the function.
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

// FIXME: Make this a member function, this is an insane number of arguments.
static inline void populate_hybrid_matrix(Eigen::SparseMatrix<double> &A_sparse,
                                          Eigen::VectorXd &b_x,
                                          Eigen::VectorXd &b_y,
                                          PartialPlacement& p_placement,
                                          const APNetlist& netlist,
                                          size_t num_moveable_blocks,
                                          size_t star_num_pins_threshold,
                                          vtr::vector<APBlockId, APRowId> blk_id_to_row_id) {
    size_t num_star_nodes = 0;
    for (APNetId net_id : netlist.nets()) {
        if (netlist.net_pins(net_id).size() > star_num_pins_threshold)
            num_star_nodes++;
    }

    A_sparse = Eigen::SparseMatrix<double>(num_moveable_blocks + num_star_nodes, num_moveable_blocks + num_star_nodes);
    b_x = Eigen::VectorXd::Zero(num_moveable_blocks + num_star_nodes);
    b_y = Eigen::VectorXd::Zero(num_moveable_blocks + num_star_nodes);

    size_t num_nets = netlist.nets().size();
    std::vector<Eigen::Triplet<double>> tripletList;
    tripletList.reserve(num_moveable_blocks * num_nets);

    size_t star_node_offset = 0;
    for (APNetId net_id : netlist.nets()) {
        size_t num_pins = netlist.net_pins(net_id).size();
        VTR_ASSERT(num_pins > 1);
        if (num_pins > star_num_pins_threshold) {
            // Using the weight from FastPlace
            double w = static_cast<double>(num_pins) / static_cast<double>(num_pins - 1);
            size_t star_node_id = num_moveable_blocks + star_node_offset;
            for (APPinId pin_id : netlist.net_pins(net_id)) {
                APBlockId blk_id = netlist.pin_block(pin_id);
                if (netlist.block_type(blk_id) == APBlockType::MOVEABLE) {
                    size_t row_id = (size_t)blk_id_to_row_id[blk_id];
                    tripletList.emplace_back(star_node_id, star_node_id, w);
                    tripletList.emplace_back(row_id, row_id, w);
                    tripletList.emplace_back(star_node_id, row_id, -w);
                    tripletList.emplace_back(row_id, star_node_id, -w);
                } else {
                    tripletList.emplace_back(star_node_id, star_node_id, w);
                    b_x(star_node_id) += w * p_placement.block_x_locs[blk_id];
                    b_y(star_node_id) += w * p_placement.block_y_locs[blk_id];
                }
            }
            star_node_offset++;
        } else {
            // Using the weight from FastPlace
            double w = 1.0 / static_cast<double>(num_pins - 1);
            for (size_t ipin_idx = 0; ipin_idx < num_pins; ipin_idx++) {
                APPinId first_pin_id = netlist.net_pin(net_id, ipin_idx);
                APBlockId first_blk_id = netlist.pin_block(first_pin_id);
                for (size_t jpin_idx = ipin_idx + 1; jpin_idx < num_pins; jpin_idx++) {
                    APPinId second_pin_id = netlist.net_pin(net_id, jpin_idx);
                    APBlockId second_blk_id = netlist.pin_block(second_pin_id);
                    // Make sure that the first node is moveable. This makes creating the connection easier.
                    if (netlist.block_type(first_blk_id) == APBlockType::FIXED) {
                        if (netlist.block_type(second_blk_id) == APBlockType::FIXED) {
                            continue;
                        }
                        std::swap(first_blk_id, second_blk_id);
                    }
                    size_t first_row_id = (size_t)blk_id_to_row_id[first_blk_id];
                    if (netlist.block_type(second_blk_id) == APBlockType::MOVEABLE) {
                        size_t second_row_id = (size_t)blk_id_to_row_id[second_blk_id];
                        tripletList.emplace_back(first_row_id, first_row_id, w);
                        tripletList.emplace_back(second_row_id, second_row_id, w);
                        tripletList.emplace_back(first_row_id, second_row_id, -w);
                        tripletList.emplace_back(second_row_id, first_row_id, -w);
                    } else {
                        tripletList.emplace_back(first_row_id, first_row_id, w);
                        b_x(first_row_id) += w * p_placement.block_x_locs[second_blk_id];
                        b_y(first_row_id) += w * p_placement.block_y_locs[second_blk_id];
                    }
                }
            }
        }
    }
    A_sparse.setFromTriplets(tripletList.begin(), tripletList.end());
}

void QPHybridSolver::solve(unsigned iteration, PartialPlacement &p_placement) {
    if(iteration == 0)
        populate_hybrid_matrix(A_sparse,b_x, b_y, p_placement, netlist, num_moveable_blocks, star_num_pins_threshold, blk_id_to_row_id);
    Eigen::SparseMatrix<double> A_sparse_diff = Eigen::SparseMatrix<double>(A_sparse);
    Eigen::VectorXd b_x_diff = Eigen::VectorXd(b_x);
    Eigen::VectorXd b_y_diff = Eigen::VectorXd(b_y);
    if(iteration != 0)
        populate_update_hybrid_matrix(A_sparse_diff, b_x_diff, b_y_diff, p_placement, num_moveable_blocks, row_id_to_blk_id, iteration);

    VTR_LOG("Running Quadratic Solver\n");
    
    // Solve Ax=b and fills placement with x.
    Eigen::VectorXd x, y;
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
    // TODO: can change cg.tolerance to increase performance when needed
    VTR_ASSERT_DEBUG(!b_x_diff.hasNaN() && "b_x has NaN!");
    VTR_ASSERT_DEBUG(!b_y_diff.hasNaN() && "b_y has NaN!");
    cg.compute(A_sparse_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at compute!");
    // TODO: Use solve with guess to make this faster. Use the previous placement
    //       as a guess.
    x = cg.solve(b_x_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_x!");
    y = cg.solve(b_y_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_y!");
    // FIXME: num_moveable_blocks should really be called num_rows
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id[row_id];
        p_placement.block_x_locs[blk_id] = x[row_id_idx];
        p_placement.block_y_locs[blk_id] = y[row_id_idx];
    }
}

void B2BSolver::initialize_placement_random_normal(PartialPlacement &p_placement) {
    // FIXME: The global variables should not be accessed from here. Should
    //        put this information as part of the netlist (max_x, min_x, etc.)
    size_t grid_width = g_vpr_ctx.device().grid.width();
    size_t grid_height = g_vpr_ctx.device().grid.height();
    // std::random_device rd;
    // std::mt19937 gen(rd());
    std::mt19937 gen(1894650209824387);
    // FIXME: the standard deviation is too small, [width/height] / [2/3]
    // can spread nodes further aparts.
    std::normal_distribution<> disx((double)grid_width / 2, 1.0);
    std::normal_distribution<> disy((double)grid_height / 2, 1.0);
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id[row_id];
        double x, y;
        do {
            x = disx(gen);
            y = disy(gen);
        } while (!(x < grid_width && y < grid_height && x >= 0 && y >= 0));
        p_placement.block_x_locs[blk_id] = x;
        p_placement.block_y_locs[blk_id] = y;
    }
}

void B2BSolver::initialize_placement_random_uniform(PartialPlacement &p_placement) {
    size_t grid_width = g_vpr_ctx.device().grid.width();
    size_t grid_height = g_vpr_ctx.device().grid.height();
    // std::random_device rd;
    // std::mt19937 gen(rd());
    std::mt19937 gen(1894650209824387);
    std::uniform_real_distribution<> disx(0.0, std::nextafter(grid_width, 0.0));
    std::uniform_real_distribution<> disy(0.0, std::nextafter(grid_height, 0.0));
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id[row_id];
        p_placement.block_x_locs[blk_id] = disx(gen);
        p_placement.block_y_locs[blk_id] = disy(gen);
    }
}

void B2BSolver::initialize_placement_least_dense(PartialPlacement &p_placement) {
    size_t grid_width = g_vpr_ctx.device().grid.width();
    size_t grid_height = g_vpr_ctx.device().grid.height();
    double gap = std::sqrt(grid_height * grid_width / static_cast<double>(num_moveable_blocks));
    size_t cols = std::ceil(grid_width / gap);
    size_t rows = std::ceil(grid_height / gap);
    // VTR_LOG("width: %zu, height: %zu, gap: %f\n", grid_width, grid_height, gap);
    // VTR_LOG("cols: %zu, row: %zu, cols * rows: %zu, numnode: %zu\n", cols, rows, cols*rows, p_placement.num_moveable_nodes);
    // VTR_ASSERT(cols * rows == p_placement.num_moveable_nodes && "number of movable nodes does not match grid size");
    for (size_t r = 0; r <= rows; r++) {
        for (size_t c = 0; c <= cols; c++) {
            size_t i = r * cols + c;
            if (i >= num_moveable_blocks)
                break;
            APRowId row_id = APRowId(i);
            APBlockId blk_id = row_id_to_blk_id[row_id];
            p_placement.block_x_locs[blk_id] = c * gap;
            p_placement.block_y_locs[blk_id] = r * gap;
        }
    }

}

void B2BSolver::populate_matrix(PartialPlacement &p_placement) {
    // Resetting As bs
    A_sparse_x = Eigen::SparseMatrix<double>(A_sparse_x.rows(), A_sparse_x.cols());
    A_sparse_y = Eigen::SparseMatrix<double>(A_sparse_y.rows(), A_sparse_y.cols());
    // A_sparse_x.setZero();
    // A_sparse_y.setZero();
    size_t num_nets = netlist.nets().size();
    std::vector<Eigen::Triplet<double>> tripletList_x;
    tripletList_x.reserve(num_moveable_blocks * num_nets);
    std::vector<Eigen::Triplet<double>> tripletList_y;
    tripletList_y.reserve(num_moveable_blocks * num_nets);
    b_x = Eigen::VectorXd::Zero(num_moveable_blocks);
    b_y = Eigen::VectorXd::Zero(num_moveable_blocks);

    for (APNetId net_id : netlist.nets()) {
        size_t num_pins = netlist.net_pins(net_id).size();
        VTR_ASSERT(num_pins > 1 && "net least has at least 2 pins");
        // Find the bound pins
        double max_x_pos = std::numeric_limits<double>::lowest();
        double min_x_pos = std::numeric_limits<double>::max();
        APBlockId max_x_id;
        APBlockId min_x_id;
        double max_y_pos = std::numeric_limits<double>::lowest();
        double min_y_pos = std::numeric_limits<double>::max();
        APBlockId max_y_id;
        APBlockId min_y_id;
        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            double x_pos = p_placement.block_x_locs[blk_id];
            double y_pos = p_placement.block_y_locs[blk_id];
            if (x_pos > max_x_pos) {
                max_x_pos = x_pos;
                max_x_id = blk_id;
            }
            if (x_pos < min_x_pos) {
                min_x_pos = x_pos;
                min_x_id = blk_id;
            }
            if (y_pos > max_y_pos) {
                max_y_pos = y_pos;
                max_y_id = blk_id;
            }
            if (y_pos < min_y_pos) {
                min_y_pos = y_pos;
                min_y_id = blk_id;
            }
        }
        // assign arbitrary node as bound node when they are all equal
        // TODO: although deterministic, investigate other ways to break ties.
        if (max_x_id == min_x_id) {
            max_x_id = netlist.pin_block(netlist.net_pin(net_id, 0));
            min_x_id = netlist.pin_block(netlist.net_pin(net_id, 1));
        }
        if (max_y_id == min_y_id) {
            max_y_id = netlist.pin_block(netlist.net_pin(net_id, 0));
            min_y_id = netlist.pin_block(netlist.net_pin(net_id, 1));
        }
        // Add edge weights (values in the matrix for a block to block connection.
        auto add_edge = [&](APBlockId first_blk_id, APBlockId second_blk_id, bool is_x){
            if (netlist.block_type(first_blk_id) != APBlockType::MOVEABLE) {
                if (netlist.block_type(second_blk_id) != APBlockType::MOVEABLE) {
                    return;
                }
                std::swap(first_blk_id, second_blk_id);
            }
            // epsilon is needed to prevent numerical instability. If two nodes are on top of each other.
            // The denominator of weight is zero, which causes infinity term in the matrix. Another way of
            // interpreting epsilon is the minimum distance two nodes are considered to be in placement.
            double dist = 0; 
            if (is_x)
                dist = 1.0 / std::max(std::abs(p_placement.block_x_locs[first_blk_id] - p_placement.block_x_locs[second_blk_id]), epsilon);
            else
                dist = 1.0 / std::max(std::abs(p_placement.block_y_locs[first_blk_id] - p_placement.block_y_locs[second_blk_id]), epsilon);
            double w = (2.0 / static_cast<double>(num_pins - 1)) * dist;
            size_t first_row_id = (size_t)blk_id_to_row_id[first_blk_id];
            if (netlist.block_type(second_blk_id) == APBlockType::MOVEABLE) {
                size_t second_row_id = (size_t)blk_id_to_row_id[second_blk_id];
                if (is_x) {
                    tripletList_x.emplace_back(first_row_id, first_row_id, w);
                    tripletList_x.emplace_back(second_row_id, second_row_id, w);
                    tripletList_x.emplace_back(first_row_id, second_row_id, -w);
                    tripletList_x.emplace_back(second_row_id, first_row_id, -w);
                } else {
                    tripletList_y.emplace_back(first_row_id, first_row_id, w);
                    tripletList_y.emplace_back(second_row_id, second_row_id, w);
                    tripletList_y.emplace_back(first_row_id, second_row_id, -w);
                    tripletList_y.emplace_back(second_row_id, first_row_id, -w);
                }
            } else {
                if (is_x){
                    tripletList_x.emplace_back(first_row_id, first_row_id, w);
                    b_x(first_row_id) += w * p_placement.block_x_locs[second_blk_id];
                } else {
                    tripletList_y.emplace_back(first_row_id, first_row_id, w);
                    b_y(first_row_id) += w * p_placement.block_y_locs[second_blk_id];
                }
            }
        };

        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            if (blk_id != max_x_id && blk_id != min_x_id) {
                add_edge(blk_id, max_x_id, true);
                add_edge(blk_id, min_x_id, true);
            } 
            if (blk_id != max_y_id && blk_id != min_y_id) {
                add_edge(blk_id, max_y_id, false);
                add_edge(blk_id, min_y_id, false);
            } 
        }
        add_edge(max_x_id, min_x_id, true);
        add_edge(max_y_id, min_y_id, false);
    }
    A_sparse_x.setFromTriplets(tripletList_x.begin(), tripletList_x.end());
    A_sparse_y.setFromTriplets(tripletList_y.begin(), tripletList_y.end());
}

// This function adds anchors for legalized solution. Anchors are treated as fixed node,
// each connecting to a movable node. Number of nodes in a anchor net is always 2.
void B2BSolver::populate_matrix_anchor(PartialPlacement& p_placement, unsigned iteration) {
    // double coeff_pseudo_anchor = 0.001 * std::exp((double)iteration/29.0);
    // double coeff_pseudo_anchor = std::exp((double)iteration/1.0);

    // Using alpha from the SimPL paper
    double coeff_pseudo_anchor = 0.01 * (1.0 + static_cast<double>(iteration));
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id[row_id];
        // Anchor node are always 2 pins.
        double pseudo_w_x = coeff_pseudo_anchor*2.0/std::max(std::abs(p_placement.block_x_locs[blk_id] - block_x_locs_legalized[blk_id]), epsilon);
        double pseudo_w_y = coeff_pseudo_anchor*2.0/std::max(std::abs(p_placement.block_y_locs[blk_id] - block_y_locs_legalized[blk_id]), epsilon);
        A_sparse_x.coeffRef(row_id_idx, row_id_idx) += pseudo_w_x;
        A_sparse_y.coeffRef(row_id_idx, row_id_idx) += pseudo_w_y;
        b_x(row_id_idx) += pseudo_w_x * block_x_locs_legalized[blk_id];
        b_y(row_id_idx) += pseudo_w_y * block_y_locs_legalized[blk_id];
    }
    // Clique formulation anchor points
    // double coeff_pseudo_anchor = 0.01 * std::exp((double)iteration/5.0);
    // for (size_t i = 0; i < p_placement.num_moveable_nodes; i++){
    //     double pseudo_w = coeff_pseudo_anchor; 
    //     A_sparse_x.coeffRef(i, i) += pseudo_w;
    //     A_sparse_y.coeffRef(i, i) += pseudo_w;
    //     b_x(i) += pseudo_w * node_loc_x_legalized[i];
    //     b_y(i) += pseudo_w *  node_loc_x_legalized[i];
    // }
}

// This function is the inner loop that alternate between building matrix based on placement, and solving model to get placement.
void B2BSolver::b2b_solve_loop(unsigned iteration, PartialPlacement &p_placement){
    // Set up the guesses for x and y to help CG converge faster
    // A good guess for B2B is the last solved solution.
    Eigen::VectorXd x_guess(num_moveable_blocks);
    Eigen::VectorXd y_guess(num_moveable_blocks);
    for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks; row_id_idx++) {
        APRowId row_id = APRowId(row_id_idx);
        APBlockId blk_id = row_id_to_blk_id[row_id];
        x_guess(row_id_idx) = p_placement.block_x_locs[blk_id];
        y_guess(row_id_idx) = p_placement.block_y_locs[blk_id];
    }
    double previous_hpwl, current_hpwl = std::numeric_limits<double>::max();
    (void)previous_hpwl;
    (void)current_hpwl;
    for(unsigned counter = 0; counter < inner_iterations; counter++) {
        previous_hpwl = current_hpwl;
        VTR_LOG("placement hpwl in b2b loop: %f\n", get_hpwl(p_placement, netlist));
        // FIXME: Should move the global reference to the device context out.
        // This assert is misleading and should not be here. It is possible for
        // the inner B2B loop to have an invalid placement, but ultimately the
        // outter loop is the one that must be valid.
        // VTR_ASSERT(p_placement.verify(netlist, g_vpr_ctx.device()) && "did not produce a valid placement in b2b solve loop");
        populate_matrix(p_placement);
        if (iteration != 0)
            populate_matrix_anchor(p_placement, iteration);
        Eigen::VectorXd x, y;
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg_x;
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg_y;
        VTR_ASSERT(!b_x.hasNaN() && "b_x has NaN!");
        VTR_ASSERT(!b_y.hasNaN() && "b_y has NaN!");
        VTR_ASSERT((b_x.array() >= 0).all() && "b_x has NaN!");
        VTR_ASSERT((b_y.array() >= 0).all() && "b_y has NaN!");
        cg_x.compute(A_sparse_x);
        cg_y.compute(A_sparse_y);
        // Not worth the time
        // having more cg iteration does not help with over convergence
        // cg_x.setMaxIterations(cg_x.maxIterations() * 100);
        // cg_x.setTolerance(cg_x.tolerance() * 100);
        // cg_y.setMaxIterations(cg_y.maxIterations() * 100);
        // cg_y.setTolerance(cg_y.tolerance() * 100);
        VTR_ASSERT(cg_x.info() == Eigen::Success && "Conjugate Gradient failed at compute for A_x!");
        VTR_ASSERT(cg_y.info() == Eigen::Success && "Conjugate Gradient failed at compute for A_y!");
        x = cg_x.solveWithGuess(b_x, x_guess);
        y = cg_y.solveWithGuess(b_y, y_guess);
        // These assertion are not valid because we do not need cg to converge. And often they do not converge with in default max iterations.
        // VTR_ASSERT(cg_x.info() == Eigen::Success && "Conjugate Gradient failed at solving b_x!");
        // VTR_ASSERT(cg_y.info() == Eigen::Success && "Conjugate Gradient failed at solving b_y!");
        for (size_t row_id_idx = 0; row_id_idx < num_moveable_blocks; row_id_idx++) {
            APRowId row_id = APRowId(row_id_idx);
            APBlockId blk_id = row_id_to_blk_id[row_id];
            p_placement.block_x_locs[blk_id] = x[row_id_idx];
            p_placement.block_y_locs[blk_id] = y[row_id_idx];
        }
        // Update the guesses with the most recent answer
        x_guess = x;
        y_guess = y;
        current_hpwl = get_hpwl(p_placement, netlist);
    // This would not work because sometimes they stay in close proximity before growing or shrinking
    // }while(std::abs(current_hpwl - previous_hpwl) < 5 && counter < 100);
    // current_hpwl > previous_hpwl - 10 would not work when this tries to grow and converge to legalized solution
    // simplest one for now
    }
}

void B2BSolver::solve(unsigned iteration, PartialPlacement &p_placement) {
    // Need an initial placement to decide who are the bounding nodes.
    if (iteration == 0){
        initialize_placement_least_dense(p_placement);
        A_sparse_x = Eigen::SparseMatrix<double>(num_moveable_blocks, num_moveable_blocks);
        A_sparse_y = Eigen::SparseMatrix<double>(num_moveable_blocks, num_moveable_blocks);
        b_x = Eigen::VectorXd::Zero(num_moveable_blocks);
        b_y = Eigen::VectorXd::Zero(num_moveable_blocks);
        b2b_solve_loop(iteration, p_placement);
        // store solved node in data sturcture of this class.
        block_x_locs_solved = p_placement.block_x_locs;
        block_y_locs_solved = p_placement.block_y_locs;
        return;
    }
    // p_placement was legalized outside of solve, save the legalized positions.
    block_x_locs_legalized = p_placement.block_x_locs;
    block_y_locs_legalized = p_placement.block_y_locs;
    // store last solved position into p_placement for b2b model
    p_placement.block_x_locs = block_x_locs_solved;
    p_placement.block_y_locs = block_y_locs_solved;
    b2b_solve_loop(iteration, p_placement);
    // store solved position in data structure of this class
    block_x_locs_solved = p_placement.block_x_locs;
    block_y_locs_solved = p_placement.block_y_locs;
}

