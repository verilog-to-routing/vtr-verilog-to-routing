
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

    bool isSymmetric(const Eigen::SparseMatrix<double>&A);
    bool isSemiPosDef(const Eigen::SparseMatrix<double>&A);
};
