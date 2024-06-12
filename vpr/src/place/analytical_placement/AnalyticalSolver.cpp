#include "AnalyticalSolver.h"
#include <Eigen/src/Core/util/Constants.h>
#include <Eigen/src/SparseCore/SparseMatrix.h>
#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/Eigenvalues>
#include <cstdio>
#include <memory>
#include "PartialPlacement.h"
#include "atom_netlist.h"
#include "globals.h"
#include "partition_region.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vtr_assert.h"

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type, PartialPlacement &p_placement) {
    if (solver_type == e_analytical_solver::QP_HYBRID)
        return std::make_unique<QPHybridSolver>(p_placement);
    VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Unrecognized analytical solver type");
    return nullptr;
}

static inline void populate_hybrid_matrix(Eigen::SparseMatrix<double> &A_sparse,
                                          Eigen::VectorXd &b_x,
                                          Eigen::VectorXd &b_y,
                                          PartialPlacement& p_placement,
                                          unsigned iteration) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    const AtomNetlist& netlist = p_placement.atom_netlist; 
    std::vector<Eigen::Triplet<double>> tripletList;
    size_t num_moveable_nodes = p_placement.num_moveable_nodes;
    tripletList.reserve(num_moveable_nodes * netlist.nets().size());

    double pseudo_w = 0.01;
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
            if(iteration != 0) {
                tripletList.emplace_back(star_node_id, star_node_id, pseudo_w * iteration * w);
                b_x(star_node_id) += pseudo_w * iteration * w * p_placement.node_loc_x[star_node_id];
                b_y(star_node_id) += pseudo_w * iteration * w * p_placement.node_loc_y[star_node_id];
            }
            for (AtomPinId pin_id : netlist.net_pins(net_id)) {
                AtomBlockId blk_id = netlist.pin_block(pin_id);
                size_t node_id = p_placement.get_node_id_from_blk(blk_id, atom_ctx.atom_molecules);
                // Note: the star node is always moveable
                if (p_placement.is_moveable_node(node_id)) {
                    tripletList.emplace_back(node_id, node_id, pseudo_w * iteration * w);
                    b_x(node_id) += pseudo_w * iteration * w * p_placement.node_loc_x[node_id];
                    b_y(node_id) += pseudo_w * iteration * w * p_placement.node_loc_y[node_id];
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
                if(iteration != 0 && p_placement.is_moveable_node(first_node_id)) {
                    b_x(first_node_id) += pseudo_w * iteration * w * p_placement.node_loc_x[first_node_id];
                    b_y(first_node_id) += pseudo_w * iteration * w * p_placement.node_loc_y[first_node_id];
                    tripletList.emplace_back(first_node_id, first_node_id, pseudo_w * iteration * w);
                }
            }
        }
    }

    A_sparse.setFromTriplets(tripletList.begin(), tripletList.end());
}

void QPHybridSolver::solve(unsigned iteration) {
    VTR_LOG("Running Quadratic Solver\n");
    if (iteration != 0) {
        long num_total_nodes = A_sparse.innerSize();
        A_sparse_diff = Eigen::SparseMatrix<double>(num_total_nodes, num_total_nodes);
        b_x_diff = Eigen::VectorXd::Zero(num_total_nodes);
        b_y_diff = Eigen::VectorXd::Zero(num_total_nodes);
        populate_hybrid_matrix(A_sparse_diff, b_x_diff, b_y_diff, p_placement, iteration);
        A_sparse += A_sparse_diff;
        b_x += b_x_diff;
        b_y += b_y_diff;
    }

    Eigen::VectorXd x, y;
    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower|Eigen::Upper> cg;
    VTR_LOG("cg max iteration %d\n", cg.maxIterations());
    cg.compute(A_sparse);
    cg.setMaxIterations(A_sparse.innerSize() * 32);
    cg.setTolerance(cg.tolerance()*10);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at compute!");
    x = cg.solve(b_x);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_x!");
    y = cg.solve(b_y);
    VTR_ASSERT(cg.info() == Eigen::Success && "Conjugate Gradient failed at solving b_y!");
    for (size_t node_id = 0; node_id < p_placement.num_moveable_nodes; node_id++) {
        p_placement.node_loc_x[node_id] = x[node_id];
        p_placement.node_loc_y[node_id] = y[node_id];
    }
    if(iteration != 0) {
        A_sparse -= A_sparse_diff;
        b_x -= b_x_diff;
        b_y -= b_y_diff;
    }
}

QPHybridSolver::QPHybridSolver(PartialPlacement &p):p_placement(p){
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

bool QPHybridSolver::isASymetric(){
    if (A_sparse.innerSize() != A_sparse.outerSize()) {
        return false;
    }
    Eigen::SparseMatrix<double> T = A_sparse.transpose();
    return ((A_sparse - T).norm() == 0);
}

bool QPHybridSolver::isAPosSemiDef(){
    if(!isASymetric()){
        return false;
    }
    Eigen::SparseMatrix<double> A = A_sparse.selfadjointView<Eigen::Lower>();
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(A);

    VTR_ASSERT(eigensolver.info() == Eigen::Success && "Eigenvalue computation failed!");

    Eigen::VectorXd eigenvalues = eigensolver.eigenvalues();
    for (int i = 0; i < eigenvalues.size(); ++i) {
        if (eigenvalues[i] < 0) {
            return false;
        }
    }
    return true;
}