#include "AnalyticalSolver.h"
#include <Eigen/SVD>
#include <Eigen/Sparse>
#include <Eigen/Eigenvalues>
#include <Eigen/IterativeLinearSolvers>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <memory>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>
#include "PartialPlacement.h"
#include "atom_netlist.h"
#include "globals.h"
#include "partition_region.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type) {
    if (solver_type == e_analytical_solver::QP_HYBRID)
        return std::make_unique<QPHybridSolver>();
    else if(solver_type == e_analytical_solver::B2B)
        return std::make_unique<B2BSolver>();
    VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Unrecognized analytical solver type");
    return nullptr;
}

bool contains_NaN(Eigen::SparseMatrix<double> &mat) {
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

bool isSymmetric(const Eigen::SparseMatrix<double> &A){
    return A.isApprox(A.transpose());
}

bool isSemiPosDef(const Eigen::SparseMatrix<double> &A){
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

double conditionNumber(const Eigen::SparseMatrix<double> &A) {
    // Since real and sym, instead of sigmamax/sigmamin, lambdamax^2/lambdamin^2 is also ok, could be faster. 
    // Eigen::MatrixXd denseMat = Eigen::MatrixXd(A);
    // Eigen::JacobiSVD<Eigen::MatrixXd> svd(denseMat);
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(A);
    Eigen::VectorXd singularValues = svd.singularValues();
    return singularValues(0)/singularValues(singularValues.size()-1);
}

static inline void populate_update_hybrid_matrix(Eigen::SparseMatrix<double> &A_sparse_diff,
                                          Eigen::VectorXd &b_x_diff,
                                          Eigen::VectorXd &b_y_diff,
                                          PartialPlacement& p_placement,
                                          unsigned iteration) {
    // Aii = Aii+w, bi = bi + wi * xi
    // TODO: verify would it be better if the initial weights are not part of the function.
    double coeff_pseudo_anchor = 0.01 * std::exp((double)iteration/5);
    for (size_t i = 0; i < p_placement.num_moveable_nodes; i++){
        double pseudo_w = coeff_pseudo_anchor; 
        A_sparse_diff.coeffRef(i, i) += pseudo_w;
        b_x_diff(i) += pseudo_w * p_placement.node_loc_x[i];
        b_y_diff(i) += pseudo_w * p_placement.node_loc_y[i];
    }
}


static inline void populate_hybrid_matrix(Eigen::SparseMatrix<double> &A_sparse,
                                          Eigen::VectorXd &b_x,
                                          Eigen::VectorXd &b_y,
                                          PartialPlacement& p_placement) {
    size_t num_star_nodes = 0;
    const AtomNetlist& netlist = p_placement.atom_netlist;
    for (AtomNetId net_id : netlist.nets()) {
        if (p_placement.net_is_ignored_for_placement(net_id))
            continue;
        if (netlist.net_pins(net_id).size() > 3)
            num_star_nodes++;
    }

    size_t num_moveable_nodes = p_placement.num_moveable_nodes;
    A_sparse = Eigen::SparseMatrix<double>(num_moveable_nodes + num_star_nodes, num_moveable_nodes + num_star_nodes);
    b_x = Eigen::VectorXd::Zero(num_moveable_nodes + num_star_nodes);
    b_y = Eigen::VectorXd::Zero(num_moveable_nodes + num_star_nodes);

    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    std::vector<Eigen::Triplet<double>> tripletList;
    tripletList.reserve(num_moveable_nodes * netlist.nets().size());

    size_t star_node_offset = 0;
    // FIXME: Instead of iterating over the whole nelist and reverse looking up
    //        it may make more sense to pre-compute the netlist.
    for (AtomNetId net_id : netlist.nets()) {
        if (p_placement.net_is_ignored_for_placement(net_id))
            continue;
        int num_pins = netlist.net_pins(net_id).size();
        VTR_ASSERT(num_pins > 1);
        if (num_pins > 3) {
            // FIXME: THIS WAS DIRECTLY COPIED FROM THE STAR FORMULATION. MOVE TO OWN FUNCTION.
            //          (with the exeption of the star node offset).
            // Using the weight from FastPlace
            double w = static_cast<double>(num_pins) / static_cast<double>(num_pins - 1);
            size_t star_node_id = num_moveable_nodes + star_node_offset;
            for (AtomPinId pin_id : netlist.net_pins(net_id)) {
                AtomBlockId blk_id = netlist.pin_block(pin_id);
                size_t node_id = p_placement.get_node_id_from_blk(blk_id, atom_ctx.atom_molecules);
                // Note: the star node is always moveable
                if (p_placement.is_moveable_node(node_id)) {
                    tripletList.emplace_back(star_node_id, star_node_id, w);
                    tripletList.emplace_back(node_id, node_id, w);
                    tripletList.emplace_back(star_node_id, node_id, -w);
                    tripletList.emplace_back(node_id, star_node_id, -w);
                } else {
                    tripletList.emplace_back(star_node_id, star_node_id, w);
                    b_x(star_node_id) += w * p_placement.node_loc_x[node_id];
                    b_y(star_node_id) += w * p_placement.node_loc_y[node_id];
                }
            }
            star_node_offset++;
        } else {
            // FIXME: THIS WAS DIRECTLY COPIED FROM THE CLIQUE FORMULATION. MOVE TO OWN FUNCTION.
            // Using the weight from FastPlace
            double w = 1.0 / static_cast<double>(num_pins - 1);

            for (int ipin = 0; ipin < num_pins; ipin++) {
                // FIXME: Is it possible for two pins to be connected to the same block?
                //        I am wondering if this doesnt matter because it would appear as tho
                //        this block really wants to be connected lol.
                AtomBlockId first_block_id = netlist.net_pin_block(net_id, ipin);
                size_t first_node_id = p_placement.get_node_id_from_blk(first_block_id, atom_ctx.atom_molecules);
                for (int jpin = ipin + 1; jpin < num_pins; jpin++) {
                    AtomBlockId second_block_id = netlist.net_pin_block(net_id, jpin);
                    size_t second_node_id = p_placement.get_node_id_from_blk(second_block_id, atom_ctx.atom_molecules);
                    // Make sure that the first node is moveable. This makes creating the connection easier.
                    if (!p_placement.is_moveable_node(first_node_id)) {
                        if (!p_placement.is_moveable_node(second_node_id)) {
                            continue;
                        }
                        std::swap(first_node_id, second_node_id);
                    }
                    if (p_placement.is_moveable_node(second_node_id)) {
                        tripletList.emplace_back(first_node_id, first_node_id, w);
                        tripletList.emplace_back(second_node_id, second_node_id, w);
                        tripletList.emplace_back(first_node_id, second_node_id, -w);
                        tripletList.emplace_back(second_node_id, first_node_id, -w);
                    } else {
                        tripletList.emplace_back(first_node_id, first_node_id, w);
                        b_x(first_node_id) += w * p_placement.node_loc_x[second_node_id];
                        b_y(first_node_id) += w * p_placement.node_loc_y[second_node_id];
                    }
                }
            }
        }
    }
    A_sparse.setFromTriplets(tripletList.begin(), tripletList.end());
}

void QPHybridSolver::solve(unsigned iteration, PartialPlacement &p_placement) {
    if(iteration == 0)
        populate_hybrid_matrix(A_sparse,b_x, b_y, p_placement);
    Eigen::SparseMatrix<double> A_sparse_diff = Eigen::SparseMatrix<double>(A_sparse);
    Eigen::VectorXd b_x_diff = Eigen::VectorXd(b_x);
    Eigen::VectorXd b_y_diff = Eigen::VectorXd(b_y);
    if(iteration != 0)
        populate_update_hybrid_matrix(A_sparse_diff, b_x_diff, b_y_diff, p_placement, iteration);

    VTR_LOG("Running Quadratic Solver\n");
    
    // Solve Ax=b and fills placement with x.
    Eigen::VectorXd x, y;
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
    // TODO: can change cg.tolerance to increase performance when needed
    VTR_ASSERT_DEBUG(!b_x_diff.hasNaN() && "b_x has NaN!");
    VTR_ASSERT_DEBUG(!b_y_diff.hasNaN() && "b_y has NaN!");
    cg.compute(A_sparse_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at compute!");
    x = cg.solve(b_x_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_x!");
    y = cg.solve(b_y_diff);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_y!");
    for (size_t node_id = 0; node_id < p_placement.num_moveable_nodes; node_id++) {
        p_placement.node_loc_x[node_id] = x[node_id];
        p_placement.node_loc_y[node_id] = y[node_id];
    }
}

void B2BSolver::b2b_solve_loop(unsigned iteration, PartialPlacement &p_placement, double hpwl){
    VTR_LOG("placement hpwl in b2b loop: %f\n", p_placement.get_HPWL());
    VTR_ASSERT(p_placement.is_valid_partial_placement() && "did not produce a valid placement in b2b solve loop");
    populate_matrix(p_placement);
    if(iteration != 0)
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
    // cg_x.setMaxIterations(cg_x.maxIterations() * 10);
    // cg_x.setTolerance(cg_x.tolerance() * 100);
    // cg_y.setMaxIterations(cg_y.maxIterations() * 10);
    // cg_y.setTolerance(cg_y.tolerance() * 100);
    VTR_ASSERT(cg_x.info() == Eigen::Success && "Conjugate Gradient failed at compute for A_x!");
    VTR_ASSERT(cg_y.info() == Eigen::Success && "Conjugate Gradient failed at compute for A_y!");
    x = cg_x.solve(b_x);
    y = cg_y.solve(b_y);
    // VTR_ASSERT(cg_x.info() == Eigen::Success && "Conjugate Gradient failed at solving b_x!");
    // VTR_ASSERT(cg_y.info() == Eigen::Success && "Conjugate Gradient failed at solving b_y!");
    for (size_t node_id = 0; node_id < p_placement.num_moveable_nodes; node_id++) {
        p_placement.node_loc_x[node_id] = x[node_id];
        p_placement.node_loc_y[node_id] = y[node_id];
    }
    double current_hpwl = p_placement.get_HPWL();
    if (current_hpwl > (hpwl - 10)){
        return;
    }
    b2b_solve_loop(iteration, p_placement, current_hpwl);
}

void B2BSolver::storeXY(PartialPlacement &p_placement){
    node_loc_x = p_placement.node_loc_x;
    node_loc_y = p_placement.node_loc_y;
}

void B2BSolver::swapXY(PartialPlacement &p_placement){
    node_loc_x.swap(p_placement.node_loc_x);
    node_loc_y.swap(p_placement.node_loc_y);
}

// Starting B2B solver
void B2BSolver::solve(unsigned iteration, PartialPlacement &p_placement) {
    // Need an initial placement to decide who are the bounding nodes.
    if (iteration == 0){
        initialize_placement(p_placement);
        size_t ASize = p_placement.num_moveable_nodes;
        A_sparse_x = Eigen::SparseMatrix<double>(ASize, ASize);
        A_sparse_y = Eigen::SparseMatrix<double>(ASize, ASize);
        b_x = Eigen::VectorXd::Zero(ASize);
        b_y = Eigen::VectorXd::Zero(ASize);
        b2b_solve_loop(iteration, p_placement, p_placement.get_HPWL());
        storeXY(p_placement);
        return;
    }
    swapXY(p_placement);
    b2b_solve_loop(iteration, p_placement, std::numeric_limits<double>::max());
    storeXY(p_placement);
}

void B2BSolver::initialize_placement(PartialPlacement &p_placement) {
    size_t grid_width = g_vpr_ctx.device().grid.width();
    size_t grid_height = g_vpr_ctx.device().grid.height();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> disx((double)grid_width / 2, 1.0);
    std::normal_distribution<> disy((double)grid_height / 2, 1.0);
    for (size_t i = 0; i < p_placement.num_moveable_nodes; i++) {
        double x, y;
        do {
            x = disx(gen);
            y = disy(gen);
        } while (!(x < grid_width && y < grid_height && x >= 0 && y >= 0));
        p_placement.node_loc_x[i] = x;
        p_placement.node_loc_y[i] = y;
    }
}

// This function return the two nodes on the bound of a netlist, (max, min)
std::pair<size_t, size_t> B2BSolver::boundNode(std::vector<size_t>& node_ids, std::vector<double>& node_locs){
    auto compare = [&node_locs](int a, int b) {
        return node_locs[a] < node_locs[b];
    };
    auto maxIt = std::max_element(node_ids.begin(), node_ids.end(), compare);
    auto minIt = std::min_element(node_ids.begin(), node_ids.end(), compare);
    VTR_ASSERT(node_locs[*maxIt] >= node_locs[*minIt] && "max note should be greater than min node");
    return std::make_pair(*maxIt, *minIt);
}

void B2BSolver::populate_matrix(PartialPlacement &p_placement) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const AtomNetlist& netlist = p_placement.atom_netlist;
    
    std::vector<Eigen::Triplet<double>> tripletList_x;
    tripletList_x.reserve(p_placement.num_moveable_nodes * netlist.nets().size());
    std::vector<Eigen::Triplet<double>> tripletList_y;
    tripletList_y.reserve(p_placement.num_moveable_nodes * netlist.nets().size());
    b_x = Eigen::VectorXd::Zero(p_placement.num_moveable_nodes);
    b_y = Eigen::VectorXd::Zero(p_placement.num_moveable_nodes);

    for (AtomNetId net_id : netlist.nets()) {
        if (p_placement.net_is_ignored_for_placement(net_id))
            continue;

        int num_pins = netlist.net_pins(net_id).size();
        VTR_ASSERT(num_pins > 1 && "net least has at least 2 pins");
        
        std::vector<size_t> node_ids;
        for (AtomPinId pin_id : netlist.net_pins(net_id)) {
            AtomBlockId blk_id = netlist.pin_block(pin_id);
            size_t node_id = p_placement.get_node_id_from_blk(blk_id, atom_ctx.atom_molecules);
            node_ids.push_back(node_id);
        }
        std::set<size_t> node_ids_set(node_ids.begin(), node_ids.end());
        std::vector<size_t> node_ids_no_duplicate(node_ids_set.begin(), node_ids_set.end());
        
        auto [maxXId, minXId] = boundNode(node_ids_no_duplicate, p_placement.node_loc_x);
        auto [maxYId, minYId] = boundNode(node_ids_no_duplicate, p_placement.node_loc_y);
        if (node_ids_no_duplicate.size() <= 1){
            continue;
        }
        if (maxXId == minXId) {
            maxXId = node_ids_no_duplicate[0];
            minXId = node_ids_no_duplicate[1];
        }
        if (maxYId == minYId) {
            maxYId = node_ids_no_duplicate[0];
            minYId = node_ids_no_duplicate[1];
        }
        auto add_node = [&](size_t first_node_id, size_t second_node_id, unsigned num_nodes, bool is_x){
            if (!p_placement.is_moveable_node(first_node_id)) {
                if (!p_placement.is_moveable_node(second_node_id)) {
                    return;
                }
                std::swap(first_node_id, second_node_id);
            }
            double epsilon = 1e-6;
            double dist = 0; 
            if(is_x)
                dist = 1.0/std::max(std::abs(p_placement.node_loc_x[first_node_id] - p_placement.node_loc_x[second_node_id]), epsilon);
            else
                dist = 1.0/std::max(std::abs(p_placement.node_loc_y[first_node_id] - p_placement.node_loc_y[second_node_id]), epsilon);
            double w = (2.0/static_cast<double>(num_nodes - 1)) * dist;
            if (p_placement.is_moveable_node(second_node_id)) {
                if (is_x) {
                    tripletList_x.emplace_back(first_node_id, first_node_id, w);
                    tripletList_x.emplace_back(second_node_id, second_node_id, w);
                    tripletList_x.emplace_back(first_node_id, second_node_id, -w);
                    tripletList_x.emplace_back(second_node_id, first_node_id, -w);
                }else{
                    tripletList_y.emplace_back(first_node_id, first_node_id, w);
                    tripletList_y.emplace_back(second_node_id, second_node_id, w);
                    tripletList_y.emplace_back(first_node_id, second_node_id, -w);
                    tripletList_y.emplace_back(second_node_id, first_node_id, -w);
                }
            } else {
                if (is_x){
                    tripletList_x.emplace_back(first_node_id, first_node_id, w);
                    b_x(first_node_id) += w * p_placement.node_loc_x[second_node_id];
                }else{
                    tripletList_y.emplace_back(first_node_id, first_node_id, w);
                    b_y(first_node_id) += w * p_placement.node_loc_y[second_node_id];
                }
            }
        };
        for (size_t node_id = 0; node_id < node_ids_no_duplicate.size(); node_id++) {
            if (node_id != maxXId && node_id != minXId) {
                add_node(node_id, maxXId, node_ids_no_duplicate.size(), true);
                add_node(node_id, minXId, node_ids_no_duplicate.size(), true);
            } 
            if (node_id != maxYId && node_id != minYId) {
                add_node(node_id, maxYId, node_ids_no_duplicate.size(), false);
                add_node(node_id, minYId, node_ids_no_duplicate.size(), false);
            } 
        }
        add_node(maxXId, minXId, node_ids_no_duplicate.size(), true);
        add_node(maxYId, minYId, node_ids_no_duplicate.size(), false);
    }
    A_sparse_x.setFromTriplets(tripletList_x.begin(), tripletList_x.end());
    A_sparse_y.setFromTriplets(tripletList_y.begin(), tripletList_y.end());
}


void B2BSolver::populate_matrix_anchor(PartialPlacement& p_placement, unsigned iteration) {
    // Aii = Aii+w, bi = bi + wi * xi
    // TODO: verify would it be better if the initial weights are not part of the function.
    double epsilon = 1e-6;
    for (size_t i = 0; i < p_placement.num_moveable_nodes; i++){
        // double pseudo_w = coeff_pseudo_anchor; 
        double pseudo_w_x =  0.05*iteration*2.0/std::max(std::abs(p_placement.node_loc_x[i] - node_loc_x[i]), epsilon);
        double pseudo_w_y =  0.05*iteration*2.0/std::max(std::abs(p_placement.node_loc_y[i] - node_loc_y[i]), epsilon);
        A_sparse_x.coeffRef(i, i) += pseudo_w_x;
        A_sparse_y.coeffRef(i, i) += pseudo_w_y;
        b_x(i) += pseudo_w_x * p_placement.node_loc_x[i];
        b_y(i) += pseudo_w_y * p_placement.node_loc_y[i];
    }
    // double coeff_pseudo_anchor = 0.01 * std::exp((double)iteration/5.0);
    // for (size_t i = 0; i < p_placement.num_moveable_nodes; i++){
    //     double pseudo_w = coeff_pseudo_anchor; 
    //     A_sparse_x.coeffRef(i, i) += pseudo_w;
    //     A_sparse_y.coeffRef(i, i) += pseudo_w;
    //     b_x(i) += pseudo_w * p_placement.node_loc_x[i];
    //     b_y(i) += pseudo_w * p_placement.node_loc_y[i];
    // }
}