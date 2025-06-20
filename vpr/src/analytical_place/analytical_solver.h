#pragma once
/**
 * @file
 * @author  Alex Singer and Robert Luo
 * @date    October 2024
 * @brief   The declarations of the Analytical Solver base class which is used
 *          to define the functionality of all solvers used in the AP flow.
 */

#include <memory>
#include "ap_flow_enums.h"
#include "ap_netlist.h"
#include "device_grid.h"
#include "place_delay_model.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

#ifdef EIGEN_INSTALLED
// The eigen library contains a warning in GCC13 for a null dereference. This
// causes the CI build to fail due to the warning. Ignoring the warning for
// these include files. Using push to return to the state of GCC diagnostics.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"

#include "Eigen/Sparse"

// Pop the GCC diagnostics state back to what it was before.
#pragma GCC diagnostic pop
#endif // EIGEN_INSTALLED

// Forward declarations
struct PartialPlacement;
class APNetlist;
class AtomNetlist;
class PreClusterTimingManager;

/**
 * @brief A strong ID for the rows in a matrix used during solving.
 *
 * This gives a linearized ID for each of the moveable blocks from 0 to the
 * number of moveable blocks.
 */
typedef vtr::StrongId<struct ap_row_id_tag, size_t> APRowId;

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
    AnalyticalSolver(const APNetlist& netlist,
                     const AtomNetlist& atom_netlist,
                     const DeviceGrid& device_grid,
                     float ap_timing_tradeoff,
                     int log_verbosity);

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
    virtual void solve(unsigned iteration, PartialPlacement& p_placement) = 0;

    /**
     * @brief Print statistics on the analytical solver.
     *
     * This is expected to be called after global placement to collect cummulative
     * information on how the solver performed.
     */
    virtual void print_statistics() = 0;

    /**
     * @brief Update the net weights according to the criticality of the nets.
     *
     *  @param pre_cluster_timing_manager
     *      The timing manager which manages the criticalities of the nets.
     */
    virtual void update_net_weights(const PreClusterTimingManager& pre_cluster_timing_manager) = 0;

  protected:
    /// @brief The APNetlist the solver is optimizing over. It is implied that
    ///        the netlist is not being modified during global placement.
    const APNetlist& netlist_;

    /// @brief The Atom netlist the solver is optimizing over. It is implied
    ///        that the atom netlist is not being modified during global
    ///        placement.
    const AtomNetlist& atom_netlist_;

    /// @brief The number of moveable blocks in the netlist. This is helpful
    ///        when allocating matrices.
    size_t num_moveable_blocks_ = 0;

    /// @brief The number of fixed blocks in the netlist.
    size_t num_fixed_blocks_ = 0;

    /// @brief A lookup between a moveable APBlock and its linear ID from
    ///        [0, num_moveable_blocks). Fixed blocks will return an invalid row
    ///        ID. This is useful when knowing which row in the matrix
    ///        corresponds with the given APBlock.
    vtr::vector<APBlockId, APRowId> blk_id_to_row_id_;

    /// @brief A reverse lookup between the linear moveable block ID and the
    ///        APBlock it represents. useful when getting the results from the
    ///        solver.
    vtr::vector<APRowId, APBlockId> row_id_to_blk_id_;

    /// @brief The base weight of each net in the AP netlist. This weight can
    ///        be used to make the solver more interested in some nets over
    ///        others. These weights can be any positive value, but are often
    ///        between 0 and 1.
    vtr::vector<APNetId, float> net_weights_;

    /// @brief A vector of blocks in the netlist which are not connected to any
    ///        non-ignored nets. Since the blocks have no connection to any
    ///        other blocks in the netlist, the are not considered movable or
    ///        fixed. The solver will just leave these blocks wherever they were
    ///        legalized.
    std::vector<APBlockId> disconnected_blocks_;

    /// @brief The width of the device grid. Used for randomly generating points
    ///        on the grid.
    size_t device_grid_width_;

    /// @brief The height of the device grid. Used for randomly generating points
    ///        on the grid.
    size_t device_grid_height_;

    /// @brief The AP timing tradeoff term used during global placement. Decides
    ///        how much the solver cares about timing vs wirelength.
    float ap_timing_tradeoff_;

    /// @brief The verbosity of log messages in the Analytical Solver.
    int log_verbosity_;
};

/**
 * @brief A factory method which creates an Analytical Solver of the given type.
 */
std::unique_ptr<AnalyticalSolver> make_analytical_solver(e_ap_analytical_solver solver_type,
                                                         const APNetlist& netlist,
                                                         const DeviceGrid& device_grid,
                                                         const AtomNetlist& atom_netlist,
                                                         const PreClusterTimingManager& pre_cluster_timing_manager,
                                                         std::shared_ptr<PlaceDelayModel> place_delay_model,
                                                         float ap_timing_tradeoff,
                                                         unsigned num_threads,
                                                         int log_verbosity);

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

    // The following constants are used to configure the anchor weighting.
    // The weights of anchors grow exponentially each iteration by the following
    // function:
    //      anchor_w = anchor_weight_mult_ * e^(iter / anchor_weight_exp_fac_)
    // The numbers below were empircally found to work well.

    /// @brief Multiplier for the anchorweight. The smaller this number is, the
    ///        weaker the anchors will be at the start.
    static constexpr double anchor_weight_mult_ = 0.001;

    /// @brief Factor for controlling the growth of the exponential term in the
    ///        weight factor function. Larger numbers will cause the anchor
    ///        weights to grow slower.
    static constexpr double anchor_weight_exp_fac_ = 5.0;

    /// @brief Due to the iterative nature of Conjugate Gradient method, the
    ///        solver may overstep 0 to give a slightly negative solution. This
    ///        is ok, and we can just clamp the position to 0. However, negative
    ///        values that are too large may be indicative of an issue in the
    ///        formulation. This value is how negative we tolerate the positions
    ///        to be.
    static constexpr double negative_soln_tolerance_ = 1e-9;

    /**
     * @brief Initializes the linear system of Ax = b_x and Ay = b_y based on
     *        the APNetlist and the fixed APBlock locations.
     *
     * This is the "ideal" quadratic linear system where no anchor-points are
     * used. This system will be solved in the first iteration of the solver,
     * then used to generate the next systems.
     */
    void init_linear_system();

    /**
     * @brief Intializes the guesses which will be used in the solver.
     *
     * The guesses will be used as starting points for the CG solver. The better
     * these guesses are, the faster the solver will converge.
     */
    void init_guesses(const DeviceGrid& device_grid);

    /**
     * @brief Helper method to update the linear system with anchors to the
     *        current partial placement.
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
     *  @param p_placement      The location the moveable blocks should be
     *                          anchored to.
     *  @param num_moveable_blocks  The number of moveable blocks in the netlist.
     *  @param row_id_to_blk_id     Lookup for the row id from the APBlock Id.
     *  @param iteration        The current iteration of the Global Placer.
     */
    void update_linear_system_with_anchors(Eigen::SparseMatrix<double>& A_sparse_diff,
                                           Eigen::VectorXd& b_x_diff,
                                           Eigen::VectorXd& b_y_diff,
                                           PartialPlacement& p_placement,
                                           unsigned iteration);

    /**
     * @brief Store the x and y solutions in Eigen's vectors into the partial
     *        placement object.
     */
    void store_solution_into_placement(const Eigen::VectorXd& x_soln,
                                       const Eigen::VectorXd& y_soln,
                                       PartialPlacement& p_placement);

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
    /// @brief The number of variables in the solver. This is the sum of the
    ///        number of moveable blocks in the netlist and the number of star
    ///        nodes that exist.
    size_t num_variables_ = 0;

    /// @brief The current guess for the x positions of the blocks.
    Eigen::VectorXd guess_x;
    /// @brief The current guess for the y positions of the blocks.
    Eigen::VectorXd guess_y;

    /// @brief The total number of CG iterations this solver has performed so far.
    unsigned total_num_cg_iters_ = 0;

  public:
    /**
     * @brief Constructor of the QPHybridSolver
     *
     * Initializes internal data and constructs the initial linear system.
     */
    QPHybridSolver(const APNetlist& netlist,
                   const DeviceGrid& device_grid,
                   const AtomNetlist& atom_netlist,
                   const PreClusterTimingManager& pre_cluster_timing_manager,
                   float ap_timing_tradeoff,
                   int log_verbosity)
        : AnalyticalSolver(netlist,
                           atom_netlist,
                           device_grid,
                           ap_timing_tradeoff,
                           log_verbosity) {
        // Update the net weights. These net weights are used when the linear
        // system is initialized.
        update_net_weights(pre_cluster_timing_manager);

        // Initializing the linear system only depends on the netlist and fixed
        // block locations. Both are provided by the netlist, allowing this to
        // be initialized in the constructor.
        // TODO: Investigate re-initializing the linear system every so often
        //       given changes in the net weights / timing.
        init_linear_system();

        // Initialize the guesses for the first iteration.
        init_guesses(device_grid);
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
    void solve(unsigned iteration, PartialPlacement& p_placement) final;

    /**
     * @brief Update the net weights according to the criticality of the nets.
     *
     *  @param pre_cluster_timing_manager
     *      The timing manager which manages the criticalities of the nets.
     */
    void update_net_weights(const PreClusterTimingManager& pre_cluster_timing_manager) final;

    /**
     * @brief Print statistics of the solver.
     */
    void print_statistics() final;
};

/**
 * @brief An Analytical Solver which tries to minimize the linear HPWL objective:
 *          SUM((xmax - xmin) + (ymax - ymin)) over all nets.
 *
 * This is implemented using the Bound2Bound method, which iteratively sets up a
 * linear system of equations (similar to the QP Hybrid approach above) which
 * solves a quadratic objective function. For a net model, each block connects
 * to the current bounding blocks in the given dimension and the weight of this
 * connection is inversly proportional to the distance of the block to the bound.
 * After minimizing this system, the bounds are likely to change; so the system
 * needs to be reconstructed and solved iteratively.
 *
 * This technique was proposed in Kraftwerk2, where they proved that the B2B Net
 * Model will, in theory, converge on the linear HPWL solution.
 *          https://doi.org/10.1109/TCAD.2008.925783
 */
class B2BSolver : public AnalyticalSolver {
  private:
    /**
     * @brief Enumeration for different initial placements that this class can
     *        perform in the first iteration.
     *
     * TODO: Investigate other initial placement techniques, the first iteration
     *       can be very expensive.
     */
    enum class e_initial_placement_type {
        LeastDense //< Randomly place blocks as a uniform grid over the device.
    };

    /// @brief Which initial placement algorithm to use in the first iteration.
    ///        In the first iteration, we need some solution to initialize the
    ///        bounds. Some papers have found that setting it to a random
    ///        initial placement is the best approach.
    static constexpr e_initial_placement_type initial_placement_ty_ = e_initial_placement_type::LeastDense;

    /// @brief Since the weights in the B2B model divide by the distance between
    ///        blocks and their bounds, that distance may get very very close to
    ///        0. This causes the weight matrix to become numerically unstable.
    ///        We can gaurd against this by clamping the distance to not be smaller
    ///        than some epsilon.
    ///        Decreasing this number may lead to more instability, but can yield
    ///        a higher quality solution.
    static constexpr double distance_epsilon_ = 0.01;

    /// @brief The gap between the HPWL of the current solved solution in the
    ///        B2B loop and the previous solved solution that is considered to
    ///        be close-enough to be converged (as a fraction of the current
    ///        solved solution HPWL).
    /// Decreasing this number toward zero would cause the B2B solver to run
    /// more iterations to try and reduce the HPWL further.
    static constexpr double b2b_convergence_gap_fac_ = 0.001;

    /// @brief The number of times the B2B loop should "converge" before stopping
    ///        the loop. Due to numerical inaccuracies, it is possible for the
    ///        HPWL to bounce up and down as it converges. Increasing this number
    ///        will allow more bounces which may get better quality; however
    ///        more iterations will need to be run.
    static constexpr unsigned target_num_b2b_convergences_ = 2;

    /// @brief Max number of bound update / solve iterations. Increasing this
    ///        number will yield better quality at the expense of runtime.
    static constexpr unsigned max_num_bound_updates_ = 24;

    /// @brief Max number of iterations the Conjugate Gradient solver can perform.
    ///        Due to the weights getting very large in the early iterations of
    ///        Global Placement, the CG solver may take a very long time to
    ///        converge; but the solution quality will not change much. By
    ///        default the max iteration is set to 2 * num_moveable_blocks;
    ///        which causes the first iteration of B2B to become quadratic in the
    ///        number of moveable blocks if it cannot converge. Found through
    ///        experimentation that this can be clamped to a much smaller number
    ///        to prevent this behaviour and get good runtime.
    // TODO: Need to investigate this more to find a good number for this.
    // TODO: Should this be a proportion of the design size?
    static constexpr unsigned max_cg_iterations_ = 150;

    // The following constants are used to configure the anchor weighting.
    // The weights of anchors grow exponentially each iteration by the following
    // function:
    //      anchor_w = anchor_weight_mult_ * e^(iter / anchor_weight_exp_fac_)
    // The numbers below were empircally found to work well.

    /// @brief Multiplier for the anchorweight. The smaller this number is, the
    ///        weaker the anchors will be at the start.
    static constexpr double anchor_weight_mult_ = 0.01;

    /// @brief Factor for controlling the growth of the exponential term in the
    ///        weight factor function. Larger numbers will cause the anchor
    ///        weights to grow slower.
    static constexpr double anchor_weight_exp_fac_ = 5.0;

    /// @brief Factor for controlling the strength of the timing term in the
    ///        objective relative to the wirelength term. By increasing this
    ///        number, the solver will focus more on timing and less on wirelength.
    static constexpr double timing_slope_fac_ = 0.75;

  public:
    B2BSolver(const APNetlist& ap_netlist,
              const DeviceGrid& device_grid,
              const AtomNetlist& atom_netlist,
              const PreClusterTimingManager& pre_cluster_timing_manager,
              std::shared_ptr<PlaceDelayModel> place_delay_model,
              float ap_timing_tradeoff,
              int log_verbosity)
        : AnalyticalSolver(ap_netlist,
                           atom_netlist,
                           device_grid,
                           ap_timing_tradeoff,
                           log_verbosity)
        , pre_cluster_timing_manager_(pre_cluster_timing_manager)
        , place_delay_model_(place_delay_model) {}

    /**
     * @brief Perform an iteration of the B2B solver, storing the result into
     *        the partial placement object passed in.
     *
     * In the first iteration (iteration = 0), the partial placement object will
     * be ignored, and a random initial placement will be used to initially
     * construct the system of equations. In all other iterations, the previous
     * solved solution will be used.
     *
     * The B2B solver will then iteratively solve the system of equations and
     * update the system to achieve a good HPWL solution which is close to the
     * linear HPWL solution. Due to numerical issues with this algorithm, we will
     * likely not converge on the true minimum HPWL solution, but it should be
     * close.
     *
     * See the base class for more information.
     *
     *  @param iteration
     *      The current iteration of the Global Placer
     *  @param p_placement
     *      A "guess" solution. The result will be written into this object.
     *      In all iterations other than the first, this solution will be used
     *      as anchor-points in the system.
     */
    void solve(unsigned iteration, PartialPlacement& p_placement) final;

    /**
     * @brief Update the net weights. Currently unused by the B2B solver.
     *
     * TODO: Investigate weighting by some factor of fanout.
     */
    void update_net_weights(const PreClusterTimingManager& pre_cluster_timing_manager);

    /**
     * @brief Print overall statistics on this solver.
     *
     * This is expected to be called after all iterations of Global Placement
     * has been complete.
     */
    void print_statistics() final;

  private:
    /**
     * @brief Run the B2B outer solving loop.
     *
     * The placement in p_placement should be initialized with the initial
     * positions of the blocks that the B2B algorithm should use to build the
     * first system of equations. This placement will be iteratively updated
     * with better and better solutions as B2B iterates.
     *
     * If iteration is 0, no anchor-blocks will be added to the system, otherwise
     * the solution in block_locs_legalized will be used as anchor-blocks.
     */
    void b2b_solve_loop(unsigned iteration, PartialPlacement& p_placement);

    /**
     * @brief Randomly distributes AP blocks using a normal distribution.
     */
    void initialize_placement_random_normal(PartialPlacement& p_placement);

    /**
     * @brief Randomly distributes AP blocks using a uniform distribution.
     */
    void initialize_placement_random_uniform(PartialPlacement& p_placement);

    /**
     * @brief Randomly distributes AP blocks using as a uniform grid.
     */
    void initialize_placement_least_dense(PartialPlacement& p_placement);

    /**
     * @brief Add a weighted connection to the linear system between the first
     *        and second blocks for a single dimension.
     *
     * This method is used to construct different linear systems for different
     * dimensions (x and y). Since the act of adding weighted connections is the
     * same regardless of dimension, this method passes in dimension-specific
     * information to be updated.
     *
     *  @param first_blk_id
     *  @param second_blk_id
     *  @param num_pins
     *      The number of pins in the hypernet connecting the two blocks.
     *  @param blk_locs
     *      The location of all blocks in a given dimension.
     *  @param triplet_list
     *      The triplet list which will be used to construct the connectivity
     *      matrix for this dimension.
     *  @param b
     *      The constant vector for this dimension.
     */
    void add_connection_to_system(APBlockId first_blk_id,
                                  APBlockId second_blk_id,
                                  size_t num_pins,
                                  double net_w,
                                  const vtr::vector<APBlockId, double>& blk_locs,
                                  std::vector<Eigen::Triplet<double>>& triplet_list,
                                  Eigen::VectorXd& b);

    /**
     * @brief Get the instantaneous derivative of delay for the given driver
     *        and sink pair.
     *
     * The instantaneous derivative gives the amount delay would increase or
     * decrease for a change in distance. This is passed into the objective
     * function to help guide the solver to trading off timing and wirelength.
     *
     *  @param driver_blk
     *      The driver block for the edge to get the derivative of.
     *  @param sink_blk
     *      The sink block for the edge to get the derivative of.
     *  @param p_placement
     *      The current placement of the AP blocks. Used to get the current
     *      distance from the driver to the sink.
     *
     *  @return The instantaneous derivative of delay with respect to distance
     *          in the x and y dimensions respectively.
     */
    std::pair<double, double> get_delay_derivative(APBlockId driver_blk,
                                                   APBlockId sink_blk,
                                                   const PartialPlacement& p_placement);

    /**
     * @brief Get normalization factors to normalize away time units out of the
     *        objective.
     *
     *  @param driver_blk
     *      The driver block of the edge to normalize the objecive for.
     */
    std::pair<double, double> get_delay_normalization_facs(APBlockId driver_blk);

    /**
     * @brief Initializes the linear system with the given partial placement.
     *
     * Blocks will be connected to the bounding blocks of their nets using
     * weighted connections, with weight inversly proportional to the distance
     * between blocks and the bounds. When solved in a quadratic equation this
     * approximates a linear equation.
     *
     * This will set the connectivity matrices (A) and constant vectors (b) to
     * be solved by B2B.
     */
    void init_linear_system(PartialPlacement& p_placement, unsigned iteration);

    /**
     * @brief Updates the linear system with anchor-blocks from the legalized
     *        solution.
     */
    void update_linear_system_with_anchors(unsigned iteration);

    /**
     * @brief Store the x and y solutions in Eigen's vectors into the partial
     *        placement object.
     *
     * Note: The x_soln and y_soln may be modified if it is found that the
     *       solution is imposible (i.e. has negative positions).
     */
    void store_solution_into_placement(Eigen::VectorXd& x_soln,
                                       Eigen::VectorXd& y_soln,
                                       PartialPlacement& p_placement);

    // The following are variables used to store the system of equations to be
    // solved in the x and y dimensions. The equations are of the form:
    //          Ax = b
    // There are two sets of matrices and vectors since the x and y dimensions
    // of the objective are independent and can be solved separately.
    // These are updated each iteration of the B2B loop.

    /// @brief The coefficient / connectivity matrix for the x dimension.
    Eigen::SparseMatrix<double> A_sparse_x;
    /// @brief The coefficient / connectivity matrix for the y dimension.
    Eigen::SparseMatrix<double> A_sparse_y;
    /// @brief The constant vector in the x dimension.
    Eigen::VectorXd b_x;
    /// @brief The constant vector in the y dimension.
    Eigen::VectorXd b_y;

    // The following is the solution of the previous iteration of this solver.
    // They are updated at the end of solve() and are used as the starting point
    // for the next call to solve.
    vtr::vector<APBlockId, double> block_x_locs_solved;
    vtr::vector<APBlockId, double> block_y_locs_solved;

    // The following are the legalized solution coming into the analytical solver
    // (other than the first iteration). These are stored to be used as anchor
    // blocks during the solver.
    vtr::vector<APBlockId, double> block_x_locs_legalized;
    vtr::vector<APBlockId, double> block_y_locs_legalized;

    /// @brief The total number of CG iterations that this solver has performed
    ///        so far. This can be a useful metric for the amount of work the
    ///        solver performs.
    unsigned total_num_cg_iters_ = 0;

    /// @brief The total time spent building the linear systems in the B2B solve
    ///        loop so far. This includes creating connections between blocks
    ///        in the connectivity matrix and constant vector as well as adding
    ///        anchor connections.
    float total_time_spent_building_linear_system_ = 0.0f;

    /// @brief The total time spent solving the linear systems in the B2B solve
    ///        loop so far. This includes creating the CG solver object and
    ///        actually solving for a solution.
    float total_time_spent_solving_linear_system_ = 0.0f;

    /// @brief Timing manager object used for calculating the criticality of
    ///        edges in the graph.
    const PreClusterTimingManager& pre_cluster_timing_manager_;

    /// @breif The place delay model used for calculating the delay between
    ///        to tiles on the FPGA. Used for computing the timing terms.
    std::shared_ptr<PlaceDelayModel> place_delay_model_;
};

#endif // EIGEN_INSTALLED
