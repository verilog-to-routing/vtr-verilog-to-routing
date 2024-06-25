
#pragma once

#include <memory>
#include <vector>
#include "PartialPlacement.h"
#include "Eigen/Sparse"

enum class e_analytical_solver {
    QP_HYBRID
};

// TODO: How should the hint be handled?
//       Perhaps the PartialPlacement could be used as a hint. Could have a flag
//       within detailing if the placement has been populated.
class AnalyticalSolver {
public:
    virtual ~AnalyticalSolver() {}
    virtual void solve(unsigned iteration) = 0;
};

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type, PartialPlacement &p_placement);

// TODO: There is no difference between the hybrid, clique, and star since the
//       the threshold acts as a mixing factor. 0 = star, inf = clique
class QPHybridSolver : public AnalyticalSolver {
// To implement the global placement step of HeAP or SimPL efficeintly, we will
// need to store the matrices for the system of equations in the class.
// TODO: Store a standard VTR matrix and pass that into Eigen using Map
public:
    QPHybridSolver(PartialPlacement &p_placement);
    void solve(unsigned iteration) final;
    PartialPlacement &p_placement;
    // Constructor fills the following with no legalization
    Eigen::SparseMatrix<double> A_sparse;
    Eigen::VectorXd b_x;
    Eigen::VectorXd b_y;
    // Difference due to psudo anchor points from legalization
    Eigen::SparseMatrix<double> A_sparse_diff;
    Eigen::VectorXd b_x_diff;
    Eigen::VectorXd b_y_diff;
    // Indexing Sparse matrix is expensive, it records diagonal values from A_sparse, created in constructor
    std::vector<double> diagonal;

    bool isASymetric(const Eigen::SparseMatrix<double>&A);
    bool isAPosDef(const Eigen::SparseMatrix<double>&A);
};
