
#pragma once

#include <memory>
#include "Eigen/Sparse"
#include "ap_netlist_fwd.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

class PartialPlacement;
class APNetlist;

enum class e_analytical_solver {
    QP_HYBRID,
    B2B
};

// A new strong ID for the AP rows.
// This gives a linearized ID for each of the moveable blocks from 0 to the
// number of moveable blocks.
struct ap_row_id_tag {};
typedef vtr::StrongId<ap_row_id_tag, size_t> APRowId;

// TODO: How should the hint be handled?
//       Perhaps the PartialPlacement could be used as a hint. Could have a flag
//       within detailing if the placement has been populated.
class AnalyticalSolver {
public:
    virtual ~AnalyticalSolver() {}
    AnalyticalSolver(const APNetlist &inetlist);
    virtual void solve(unsigned iteration, PartialPlacement &p_placement) = 0;

protected:
    const APNetlist& netlist;
    size_t num_moveable_blocks = 0;
    vtr::vector<APBlockId, APRowId> blk_id_to_row_id;
    vtr::vector<APRowId, APBlockId> row_id_to_blk_id;
};

std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type, const APNetlist &netlist);

// TODO: There is no difference between the hybrid, clique, and star since the
//       the threshold acts as a mixing factor. 0 = star, inf = clique
class QPHybridSolver : public AnalyticalSolver {
    using AnalyticalSolver::AnalyticalSolver;

    static constexpr size_t star_num_pins_threshold = 3;
public:
    void solve(unsigned iteration, PartialPlacement &p_placement) final;
    // Constructor fills the following with no legalization
    Eigen::SparseMatrix<double> A_sparse;
    Eigen::VectorXd b_x;
    Eigen::VectorXd b_y;
};

class B2BSolver : public AnalyticalSolver {
    using AnalyticalSolver::AnalyticalSolver;
    public:
        void solve(unsigned iteration, PartialPlacement &p_placement) final;
        void b2b_solve_loop(unsigned iteration, PartialPlacement &p_placement);
        void initialize_placement_random_normal(PartialPlacement &p_placement);
        void initialize_placement_random_uniform(PartialPlacement &p_placement);
        void initialize_placement_least_dense(PartialPlacement &p_placement);
        void populate_matrix(PartialPlacement &p_placement);
        void populate_matrix_anchor(PartialPlacement& p_placement, unsigned iteration);
        
        static constexpr double epsilon = 1e-6;
        static constexpr unsigned inner_iterations = 30;

        // These are stored because there might be potential oppurtunities of reuse. 
        // Also, it could be more efficient to allocate heap once and reusing it instead
        // of freeing and reallocating. 
        Eigen::SparseMatrix<double> A_sparse_x;
        Eigen::SparseMatrix<double> A_sparse_y;
        Eigen::VectorXd b_x;
        Eigen::VectorXd b_y;
        // They are being stored because legalizer will modified the placement passed in. While building the b2b model with anchors,
        // both the previously solved and legalized placement are needed, so we store them as a member.
        vtr::vector<APBlockId, double> block_x_locs_solved;
        vtr::vector<APBlockId, double> block_y_locs_solved;
        vtr::vector<APBlockId, double> block_x_locs_legalized;
        vtr::vector<APBlockId, double> block_y_locs_legalized;
};

