#ifndef VPR_ANALYTIC_PLACEMENT_H
#define VPR_ANALYTIC_PLACEMENT_H

#ifdef ENABLE_ANALYTIC_PLACE
/**
 * @file
 * @brief This file implements the analytic placer, described as lower-bound placement in SimPL. It formulates
 * the placement problem into a set of linear equations, in the form of a matrix equation. Solving the matrix
 * equation gives the minimum of the objective function, in this case wirelength. The result placement, although
 * most optimal in terms of optimization, thus the name lower-bound placement, almost always is not legal. This
 * lower-bound solution is then legalized using Cut-Spreading (@see cut_spreader.h).
 *
 **************************************************************************************************************
 * 											Algorithm Overview												  *
 **************************************************************************************************************
 *
 * The most common objective function for placement is the sum of half-perimeter wirelengths (HPWL) over all nets.
 * Efficient AP techniques approximate this objective function with a function that can be minimized efficiently.
 *
 * First, all multi-pin nets are converted into a set of 2-pin connections. In SimPL/HeAP, the Bound2bound net
 * model is used. For each multi-pin net, the blocks with the minimum and maximum locations (in either x or y directon
 * as build-solve operates on only 1 direction at a time) on a net (so-called bound-blocks) are connected to each
 * other and to each internal block on the net. In other words, for a p-terminal net, each internal block has 2 connections,
 * one to each bound block, and each bound block has p-1 connections, one to every block other than itself.
 *
 * Then, the weighted sum of the squared lengths of these 2-pin connections are minimized. This objective function
 * can be separated into x and y components and cast in matrix form. To minimize this degree-2 polynomial, partial
 * derivative is taken with respect to each variable. Setting the resulting system of linear equations to 0 gives
 * the following equation (only x direction shown):
 * 											Qx = -c
 * where Q is a matrix capturing all connection between movable objects (objects to solve), x is a vector of all
 * movable block locations (free variables), and c is a vector representing connections between movable and fixed objects.
 * *** for detailed derivation and an example, refer to comments for add_pin_to_pin_connection() in analytic_placer.cpp.
 *
 * After formulating Q and c, a standard off-the-shelf solver (Eigen package) is used to solve for x. This completes
 * the lower-bound placement.
 *
 * However, since the objective function does not take placement constraints into consideration, the generated
 * solution is not legal. It generally has many blocks overlapping with one another, and the blocks may be on
 * incompatible physical tiles. To legalize this solution, a geometric partitioning and spreading technique, introduced
 * in SimPL, is used (@see cut_sreader.h). This completes the upper-bound placement.
 *
 * After the completion of 1 iteration of lower-bound & upper-bound placement, artificial pseudo connections are created
 * between each block and its target location in the legalized overlap-free placement. When the mathematical system is
 * again formulated and solved, the pseudo connections pull blocks towards their target locations, which tends to reduce
 * overlaps in the placement. The strength of pseudo-connections increase with iterations, making lower-bound and
 * upper-bound solutions converge.
 *
 * This process of formulating the system, solving, and legalizing is repeated until sufficiently good placement is
 * acquired. Currently the stopping criterion is HEAP_STALLED_ITERATIONS_STOP iterations without improvement in total_hpwl.
 *
 *
 * Parameters to tweak & things to try out
 * =======================================
 * Currently the QoR  of AP+quench combination is slightly worse than SA. See PR #1504 for comparison.
 * The following parameters/things can be tweaked to find the best configuration:
 *
 * * Stopping criteria			when to stop AP iterations, see (AnalyticPlacer::ap_place())
 * * PlacerHeapCfg.alpha		anchoring strength of pseudo-connection
 * * PlacerHeapCfg.beta			overutilization factor (@see CutSpreader::SpreaderRegion.overused())
 * * PlacerHeapCfg.timingWeight	implement timing in AP (@see AnalyticPlacer::build_equations())
 * * PlacerHeapCfg.criticality	same as above
 * * Interaction with SA:
 * 	 * init_t					Initial temperature of annealer after AP (currently init_t = 0)
 * 	 * quench inner_num			how much swapping in quenching to attemp
 * 	 * quench_recompute_limit	frequency of criticality update in quenching to improve quench results
 *
 * @cite SimPL
 * Original analytic placer with cut-spreading legalizing was intended for ASIC design, proposed in SimPL.
 * SimPL: An Effective Placement Algorithm, Myung-Chul Kim, Dong-Jin Lee and Igor L. Markov
 * http://www.ece.umich.edu/cse/awards/pdfs/iccad10-simpl.pdf
 *
 * @cite HeAP
 * FPGA adaptation of SimPL, targeting FPGAs with heterogeneous blocks located at discrete locations.
 * Analytical Placement for Heterogeneous FPGAs, Marcel Gort and Jason H. Anderson
 * https://janders.eecg.utoronto.ca/pdfs/marcelfpl12.pdf
 *
 * @cite nextpnr
 * An implementation of HeAP, which the cut-spreader and legalizer here is based off of. Implementation details
 * have been modified for the architecture and netlist specification of VTR, and better performance.
 * nextpnr -- Next Generation Place and Route,  placer_heap, David Shah <david@symbioticeda.com>
 * https://github.com/YosysHQ/nextpnr
 */

#    include "vpr_context.h"
#    include "timing_place.h"
#    include "PlacementDelayCalculator.h"

/*
 * @brief Templated struct for constructing and solving matrix equations in analytic placer
 * Eigen library is used in EquationSystem::solve()
 */
template<typename T>
struct EquationSystem;

// sentinel for blks not solved in current iteration
extern int DONT_SOLVE;

// sentinel for blks not part of a placement macro
extern int NO_MACRO;

/*
 * @brief helper function to find the index of macro that contains blk
 * returns index in placementCtx.pl_macros, NO_MACRO if blk not in any macros
 */
int imacro(ClusterBlockId blk);

/*
 * @brief helper function to find the head block of the macro that contains blk
 * placement macro head is the base of the macro, where the locations of the other macro members can be
 * calculated using base.loc + member.offset.
 * Only the placement of macro head is calculated directly from AP, the position of other macro members need
 * to be calculated later using above formula.
 *
 * returns the ID of the head block
 */
ClusterBlockId macro_head(ClusterBlockId blk);

class AnalyticPlacer {
  public:
    /*
     * @brief Constructor of AnalyticPlacer, currently initializes AnalyticPlacerCfg for the analytic placer
     * To tune these parameters, change directly in constructor
     */
    AnalyticPlacer();

    /*
     * @brief main function of analytic placement
     * Takes the random initial placement from place.cpp through g_vpr_ctx
     * Repeat the following until stopping criteria is met:
     * 	* Formulate and solve equations in x, y directions for 1 type of logial block
     * 	* Instantiate CutSpreader to spread and strict_legalize() to strictly legalize
     *
     * The final legal placement is passed back to annealer in g_vpr_ctx.mutable_placement()
     */
    void ap_place();

  private:
    // for CutSpreader to access placement info from solver (legal_pos, block_locs, etc).
    friend class CutSpreader;

    // AP parameters that can influence it's behavior
    struct AnalyticPlacerCfg {
        float alpha;                        // anchoring strength of pseudo-connections
        float beta;                         // over-utilization factor
        int criticalityExponent;            // not currently used, @see build_equations()
        int timingWeight;                   // not currently used, @see build_equations()
        float solverTolerance;              // parameter of the solver
        int buildSolveIter;                 // build_solve iterations for iterative solver
        int spread_scale_x, spread_scale_y; // see CutSpreader::expand_regions()
    };

    AnalyticPlacerCfg ap_cfg; // TODO: PlacerHeapCfg should be externally configured & supplied

    // Lokup of all sub_tiles by sub_tile type
    // legal_pos[0..device_ctx.num_block_types-1][0..num_sub_tiles - 1][0..num_legal - 1] = t_pl_loc for a single
    // placement location of the proper tile type and sub_tile type.
    std::vector<std::vector<std::vector<t_pl_loc>>> legal_pos;

    // row number in the system of linear equations for each block
    // which corresponds to the equation produced by differentiating objective function w.r.t that block location
    vtr::vector_map<ClusterBlockId, int32_t> row_num;

    // Encapsulates 3 types of locations for each logic block
    struct BlockLocation {
        t_pl_loc loc; // real, up-to-date location of the logic block in the AP process
                      // first initiated with initial random placement from g_vpr_ctx
                      // then, eath time after solving equations, it's updated with rounded
                      // raw solutions from solver
                      // finally, it is accessed and modified by legalizer to store legal placement
                      // at the end of each AP iteration

        t_pl_loc legal_loc; // legalized location, used to create psudo connections in the next AP iteration
                            // updated in AP main loop in ap_place() at the end of each iteration

        double rawx, rawy; // raw location storing float result from matrix solver
                           // used by cut_speader to spread out logic blocks using linear interpolation
    };

    // Lookup from blockID to block location
    vtr::vector_map<ClusterBlockId, BlockLocation> blk_locs;

    /*
     * The set of blks of different types to be placed by AnalyticPlacement process,
     * i.e. the free variable blocks.
     * Excludes non-head macro blocks (blocks part of placement macros but not the head), fixed blocks, and blocks
     * with no connections.
     */
    std::vector<ClusterBlockId> place_blks;

    // blocks of the same type to be solved in the current formulation of matrix equation
    // which are a subset of place_blks
    std::vector<ClusterBlockId> solve_blks;

    /*
     * Prints the location of each block, and a simple drawing of FPGA fabric, showing num of blocks on each tile
     * Very useful for debugging
     * See implementation for usage
     */
    void print_place(const char* place_file);

    //build fast lookup of compatible tiles/subtiles by tile, x, y, subtiles
    void build_fast_tiles();

    // build legal_pos
    void build_legal_locations();

    // build blk_locs based on initial placement from place_ctx.
    // put blocks that needs to be placed in place_blks;
    void init();

    // get hpwl for a net
    int get_net_hpwl(ClusterNetId net_id);

    // get hpwl for all nets
    int total_hpwl();

    // build matrix equations and solve for block type "run" in both x and y directions
    // macro member positions are updated after solving
    // iter is used to determine pseudo-connection strength
    void build_solve_type(t_logical_block_type_ptr run, int iter);

    /*
     * Setup the blocks of type blkTypes (ex. clb, io) to be solved. These blocks are put into
     * solve_blks vector. Each of them is a free variable in the matrix equation (thus excluding
     * macro members, as they are formulated into the equation for the macro's head)
     * A row number is assigned to each of these blocks, which corresponds to its equation in
     * the matrix (the equation acquired from differentiating the objective function w.r.t its
     * x or y location).
     */
    void setup_solve_blks(t_logical_block_type_ptr blkTypes);

    /*
     * Update the location of all members of all macros based on location of macro_head
     * since only macro_head is solved (connections to macro members are also taken into account
     * when formulating the matrix equations), a location update for members is necessary
     */
    void update_macros();

    /*
     * Build and solve in one direction
     * yaxis chooses x or y location of each block from blk_locs to formulate the matrix equation
     * Solved solutions are written back to block_locs[blk].rawx/rawy for double float raw solution,
     * rounded int solutions are written back to block_locs[blk].loc, for each blk in solve_blks
     *
     * iter is the number of AnalyticPlacement iterations (solving and legalizing all types of logic
     * blocks once). When iter != -1, at least one iteration has completed. It signals build_equations()
     * to create pseudo-connections between each block and its prior legal position.
     *
     * build_solve_iter determines number of iterations of building and solving for the iterative solver
     * (i.e. more build_solve_iter means better result, with runtime tradeoff. This parameter can be
     * tuned for better performance)
     * the solution from the previous build-solve iteration is used as a guess for the iterative solver
     */
    void build_solve_direction(bool yaxis, int iter, int build_solve_iter);

    /*
     * Stamp 1 weight for 1 connection on matrix or rhs vector
     * if var is movable objects, weight is added on matrix
     * if var is immovable objects, weight*-var_pos is added on rhs
     * if var is a macro member (not macro head), weight*-offset_from_macro_head is added on rhs
     *
     * for detailed derivation and examples, see comments for add_pin_to_pin_connection() in analytic_placer.cpp
     */
    void stamp_weight_on_matrix(EquationSystem<double>& es,
                                bool dir,
                                ClusterBlockId var,
                                ClusterBlockId eqn,
                                double weight);

    /*
     * Add weights for connection between bound_pin and this_pin into matrix
     * Calculate weight for connection and stamp them into appropriate position in matrix by invoking
     * stamp_weight_on_matrix() multiple times. For more detail, see comments in implementation.
     */
    void add_pin_to_pin_connection(EquationSystem<double>& es,
                                   bool dir,
                                   int num_pins,
                                   ClusterPinId bound_pin,
                                   ClusterPinId this_pin);

    /*
     * Build the system of equations for either X or Y
     * When iter != -1, for each block, psudo-conenction to its prior legal location is formed,
     * the strength is determined by ap_cfg.alpha and iter
     */
    void build_equations(EquationSystem<double>& es, bool yaxis, int iter = -1);

    /*
     * Solve the system of equations passed in by es, for the set of blocks in data member solve_blks
     * yaxis is used to select current x or y location of these blocks from blk_locs
     * this current location is provided to iterative solver as a guess
     * the solved location is written back to blk_locs, and is used as guess for the next
     * iteration of solving (@see build_solve_direct())
     */
    void solve_equations(EquationSystem<double>& es, bool yaxis);

    /*
     * Debug use
     * finds # of blocks on each tile location, returned in overlap matrix
     */
    void find_overlap(vtr::Matrix<int>& overlap);

    /*
     * Debug use
     * prints a simple figure of FPGA fabric, with numbers on each tile showing usage.
     * called in AnalyticPlacer::print_place()
     */
    std::string print_overlap(vtr::Matrix<int>& overlap, FILE* fp);

    // header of VTR_LOG for AP
    void print_AP_status_header();

    void print_run_stats(const int iter,
                         const float time,
                         const float runTime,
                         const char* blockType,
                         const int blockNum,
                         const float solveTime,
                         const float spreadTime,
                         const float legalTime,
                         const int solvedHPWL,
                         const int spreadHPWL,
                         const int legalHPWL);

    void print_iter_stats(const int iter,
                          const float iterTime,
                          const float time,
                          const int bestHPWL,
                          const int stall);
};

#endif /* ENABLE_ANALYTIC_PLACE */

#endif /* VPR_ANALYTIC_PLACEMENT_H */
