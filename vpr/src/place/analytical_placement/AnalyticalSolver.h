
#pragma once

#include <memory>
#include <vector>
#include "PartialPlacement.h"
#include "Eigen/Sparse"

enum class e_analytical_solver {
    QP_HYBRID,
    B2B
};

// TODO: How should the hint be handled?
//       Perhaps the PartialPlacement could be used as a hint. Could have a flag
//       within detailing if the placement has been populated.
class AnalyticalSolver {
public:
    virtual ~AnalyticalSolver() {}
    virtual void solve(unsigned iteration, PartialPlacement &p_placement) = 0;
};

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type);

// TODO: There is no difference between the hybrid, clique, and star since the
//       the threshold acts as a mixing factor. 0 = star, inf = clique
class QPHybridSolver : public AnalyticalSolver {
// To implement the global placement step of HeAP or SimPL efficeintly, we will
// need to store the matrices for the system of equations in the class.
// TODO: Store a standard VTR matrix and pass that into Eigen using Map
public:
    void solve(unsigned iteration, PartialPlacement &p_placement) final;
    // Constructor fills the following with no legalization
    Eigen::SparseMatrix<double> A_sparse;
    Eigen::VectorXd b_x;
    Eigen::VectorXd b_y;
};

class B2BSolver : public AnalyticalSolver {
    public:
        void solve(unsigned iteration, PartialPlacement &p_placement) final;
        void b2b_solve_loop(unsigned iteration, PartialPlacement &p_placement);
        void initialize_placement_random_normal(PartialPlacement &p_placement);
        void initialize_placement_random_uniform(PartialPlacement &p_placement);
        void initialize_placement_least_dense(PartialPlacement &p_placement);
        void populate_matrix(PartialPlacement &p_placement);
        void populate_matrix_anchor(PartialPlacement& p_placement, unsigned iteration);
        std::pair<size_t, size_t> boundNode(std::vector<size_t> &node_id, std::vector<double> &node_loc);

        Eigen::SparseMatrix<double> A_sparse_x;
        Eigen::SparseMatrix<double> A_sparse_y;
        Eigen::VectorXd b_x;
        Eigen::VectorXd b_y;
        std::vector<double> node_loc_x_solved;
        std::vector<double> node_loc_y_solved;
        std::vector<double> node_loc_x_legalized;
        std::vector<double> node_loc_y_legalized;
};