/**
 * @file
 * @author  Alex Singer and Robert Luo
 * @date    October 2024
 * @brief   The declarations of the Analytical Solver base class which is used
 *          to define the functionality of all solvers used in the AP flow.
 */

#pragma once

#include <memory>
#include "ap_netlist_fwd.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

#ifdef EIGEN_INSTALLED
#include "Eigen/Sparse"
#endif  // EIGEN_INSTALLED

// Forward declarations
class PartialPlacement;
class APNetlist;

/**
 * @brief Enumeration of all of the solvers currently implemented in VPR.
 *
 * NOTE: More are coming.
 */
enum class e_analytical_solver {
    QP_HYBRID       // A solver which optimizes the quadratic HPWL of the design.
};

/**
 * @brief A strong ID for the rows in a matrix used during solving.
 *
 * This gives a linearized ID for each of the moveable blocks from 0 to the
 * number of moveable blocks.
 */
struct ap_row_id_tag {};
typedef vtr::StrongId<ap_row_id_tag, size_t> APRowId;

/**
 * @brief The Analytical Solver base class
 *
 * This provides functionality that all Analytical Solvers will use.
 *
 * It provides a standard interface that all Analytical Solvers must implement
 * so they can be used interchangably. This makes it very easy to test and
 * compare different solvers.
 */
class AnalyticalSolver {
public:
    virtual ~AnalyticalSolver() {}

    /**
     * @brief Constructor of the base AnalyticalSolver class
     *
     * Initializes the internal data members of the base class which are useful
     * for all solvers.
     */
    AnalyticalSolver(const APNetlist &netlist);

    /**
     * @brief Run an iteration of the solver using the given partial placement
     *        as a hint for what a "legal" solution would look like.
     *
     * Each solver is trying to optimize its own objective function. The solver
     * is also, at the same time, trying to approach a solution that is "similar"
     * to the hint provided. The larger the iteration, the more influenced the
     * solver is by the hint provided.
     *
     * It is implied that the hint partial placement is a placement which has
     * gone through some form of legalization. This allows the solver to
     * optimize its objective function, while also trying to approach a more
     * legal solution.
     *
     *  @param iteration    The current iteration number of the Global Placer.
     *  @param p_placement  A "hint" to a legal solution that the solver should
     *                      try and be like.
     */
    virtual void solve(unsigned iteration, PartialPlacement &p_placement) = 0;

protected:

    /// @brief The APNetlist the solver is optimizing over. It is implied that
    ///        the netlist is not being modified during global placement.
    const APNetlist& netlist_;

    /// @brief The number of moveable blocks in the netlist. This is helpful
    ///        when allocating matrices.
    size_t num_moveable_blocks_ = 0;

    /// @brief A lookup between a moveable APBlock and its linear ID from
    ///        [0, num_moveable_blocks). Fixed blocks will return an invalid row
    ///        ID. This is useful when knowing which row in the matrix
    ///        corresponds with the given APBlock.
    vtr::vector<APBlockId, APRowId> blk_id_to_row_id_;

    /// @brief A reverse lookup between the linear moveable block ID and the
    ///        APBlock it represents. useful when getting the results from the
    ///        solver.
    vtr::vector<APRowId, APBlockId> row_id_to_blk_id_;
};

/**
 * @brief A factory method which creates an Analytical Solver of the given type.
 */
std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_analytical_solver solver_type,
                                                         const APNetlist &netlist);

// The Eigen library is used to solve matrix equations in the following solvers.
// The solver cannot be built if Eigen is not installed.
#ifdef EIGEN_INSTALLED

/**
 * @brief An Analytical Solver which tries to minimize the quadratic HPWL
 *        objective:
 *              SUM((xmax - xmin)^2 + (ymax - ymin)^2) over all nets.
 *
 * This is implemented using a hybrid approach, which uses both the Clique and
 * Star net models. Since circuits are hypernets, a single net may connect more
 * than just two blocks. The Clique model creates a clique connection between
 * all blocks in the net (each block connects to each other block exactly once).
 * The Star model creates a "fake". moveable star node which all blocks in the
 * net connect to (but the blocks in the net do not connect to each other).
 *
 * The Star net model creates an extra variable for the solver to solve; however
 * it allows the matrix to become more sparse (compared to the Clique net model).
 * This solver uses a net pin threshold, where if the the number of pins in a
 * net is larger than that threshold, that net will use the Star net model, and
 * if the number of pins is smaller the Clique model is used.
 *
 * This technique was proposed in FastPlace, where they proved that, if the
 * weights of the Star and Clique connections are set to certain values, they
 * will both achieve the same answer (minimizing the quadratic HPWL objective).
 *          https://doi.org/10.1109/TCAD.2005.846365
 */
class QPHybridSolver : public AnalyticalSolver {
private:
    /// @brief The threshold for the number of pins a net will have to use the
    ///        Star or Clique net models. If the number of pins is larger
    ///        than this number, a star node will be created.
    /// This number will not change the solution, but may improve performance if
    /// tuned properly. If too low, then more star nodes will be created which
    /// adds more variables to the linear system; if too high, then more clique
    /// connections will be created, which will make the coefficient matrix less
    /// sparse.
    static constexpr size_t star_num_pins_threshold = 3;

    /**
     * @brief Initializes the linear system of Ax = b_x and Ay = b_y based on
     *        the APNetlist and the fixed APBlock locations.
     *
     * This is the "ideal" quadratic linear system where no anchor-points are
     * used. This system will be solved in the first iteration of the solver,
     * then used to generate the next systems.
     */
    void init_linear_system();

    // The following variables represent the linear system without any anchor
    // points. These are filled in the constructor and never modified.
    // When the anchor-points are taken into consideration, the diagonal of the
    // coefficient matrix and the elements of the constant vectors are updated
    // and used in the solver. These are stored to prevent re-computing each
    // iteration.

    /// @brief The coefficient matrix for the un-anchored linear system
    /// This is expected to be sparse. This is shared between the x and y
    /// dimensions.
    Eigen::SparseMatrix<double> A_sparse;
    /// @brief The constant vector in the x dimension for the linear system.
    Eigen::VectorXd b_x;
    /// @brief The constant vector in the y dimension for the linear system.
    Eigen::VectorXd b_y;

public:

    /**
     * @brief Constructor of the QPHybridSolver
     *
     * Initializes internal data and constructs the initial linear system.
     */
    QPHybridSolver(const APNetlist& netlist) : AnalyticalSolver(netlist) {
        // Initializing the linear system only depends on the netlist and fixed
        // block locations. Both are provided by the netlist, allowing this to
        // be initialized in the constructor.
        init_linear_system();
    }

    /**
     * @brief Perform an iteration of the QP solver, storing the result into
     *        the partial placement object passed in.
     *
     * In the first iteration (iteration = 0), the partial placement object will
     * not be used and the result would be the solution to the quadratic hpwl
     * objective.
     *
     * In the following iterations (iteration > 0), the partial placement object
     * will be used as anchor-points to guide the solver to a "more legal"
     * solution (assuming the partial placement contains a legal solution).
     * Higher iterations use stronger attraction forces between the moveable
     * blocks and their anchor-points.
     *
     * See the base class for more information.
     *
     *  @param iteration    The current iteration of the Global Placer.
     *  @param p_placement  A "guess" solution. The result will be written into
     *                      this object.
     */
    void solve(unsigned iteration, PartialPlacement &p_placement) final;
};

#endif // EIGEN_INSTALLED

