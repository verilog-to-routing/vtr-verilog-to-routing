#ifndef VPR_ANALYTIC_PLACEMENT_H
#define VPR_ANALYTIC_PLACEMENT_H

// #ifdef ENABLE_ANALYTIC_PLACE
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
 * model is used. For each multi-pin net, the blocks with the minimum and maximum locations on a net (so-called
 * bound-blocks) are connected to each other and to each internal block on the net. In other words, for a p-terminal
 * net, each internal block has 2 connections, one to each bound block, and each bound block has p-1 connections,
 * one to every block other than itself.
 *
 * Then, the weighted sum of the squared lengths of these 2-pin connections are minimized. This objective function
 * can be separated into x and y components and cast in matrix form. To minimize this degree-2 polynomial, partial
 * derivative is taken with respect to each variable. Setting the resulting system of linear equations to 0 gives
 * the following equation (only x direction shown):
 * 											Qx = -c
 * where Q is a matrix capturing all connection between movable objects (objects to solve), x is a vector of all
 * block locations, and c is a vector representing connections between movable and fixed objects.
 * *** for detailed derivation and an example, refer to HeAP paper cited below.
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
 * overlaps in the placement.
 *
 * This process of formulating the system, solving, and legalizing is repeated until sufficiently good placement is
 * acquired. Currently the stopping criterion is 15 iterations without improvement in total_hpwl.
 *
 *
 * Parameters to tweak & things to try out
 * =======================================
 * Currently the QoR and runtime of AP+quench combination is slightly worse than SA. See PR #1504 for comparison.
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

typedef signed long int wirelen_t;

/*
 * @brief Templated struct for constructing and solving matrix equations in analytic placer
 * Eigen library is used in EquationSystem::solve()
 */
template<typename T>
struct EquationSystem;

/*
 * @brief helper function to find the index of macro that contains blk
 * returns index in placementCtx.pl_macros, -1 if blk not in any macros
 */
int imacro(ClusterBlockId blk);

/*
 * @brief helper fucntion to find the head of macro containing blk
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
     * 	* Instantiate CutSpreader to spread and strict_legalize
     *
     * The final legal placement is passed back to annealer in g_vpr_ctx.mutable_placement()
     */
    void ap_place();

  private:
    // for CutSpreader to access legal_pos, block_locs, etc.
    friend class CutSpreader;

    // AP parameters that can influence it's behavior
    struct AnalyticPlacerCfg {
        float alpha;                        // anchoring strength of pseudo-connections
        float beta;                         // over-utilization factor
        int criticalityExponent;            // not currently used, @see build_equations()
        int timingWeight;                   // not currently used, @see build_equations()
        float solverTolerance;              // parameter of the solver
        int spread_scale_x, spread_scale_y; // see CutSpreader::expand_regions()
    };

    AnalyticPlacerCfg tmpCfg; // TODO: PlacerHeapCfg should be externally configured & supplied

    // @see initial_placement.cpp load_legal_locations()
    std::vector<std::vector<int>> num_legal_pos;
    std::vector<std::vector<std::vector<t_pl_loc>>> legal_pos;

    // marker for blks not solved in current iteration
    int32_t dont_solve = std::numeric_limits<int32_t>::max();

    // row number in the system of linear equations for each block
    // which corresponds to the equation produced by differentiating objective function w.r.t that block location
    std::unordered_map<ClusterBlockId, int32_t> row_num;

    // Encapsulates 3 types of locations for each logic block
    struct BlockLocation {
        t_pl_loc loc;       // real, up-to-date location of the logic block
        t_pl_loc legal_loc; // legalized location, used to create psudo connections
        double rawx, rawy;  // raw location storing float result from matrix solver
    };

    // Lookup from blockID to block location
    std::unordered_map<ClusterBlockId, BlockLocation> blk_locs;

    // The set of blks actually placed.
    // Excludes children of macros.
    std::vector<ClusterBlockId> place_blks;

    // blocks being solved in the current equation
    // which are a subset of all blocks being placed
    std::vector<ClusterBlockId> solve_blks;

    /*
     * number of blocks on each tile
     * used to print a simple placement graph in print_place
     */
    std::vector<std::vector<int>> overlap;

    /*
     * Prints the location of each block, and a simple drawing of FPGA fabric, showing num of blocks on each tile
     * Very useful for debugging
     * See implementation for usage
     */
    void print_place(const char* place_file);

    //build fast lookup of tiles/subtiles by tile, x, y, subtiles
    void build_fast_tiles();

    // build num_legal_pos, legal_pos like in initial_placement.cpp
    void build_legal_locations();

    // build blk_locs based on initial placement;
    // build and place_blks for blocks that needs to be placed;
    void init();

    wirelen_t get_net_hpwl(ClusterNetId net_id);

    wirelen_t total_hpwl();

    // Setup the blocks of type blkTypes to be solved
    // Returns the number of rows (number of blks to solve)
    int setup_solve_blks(t_logical_block_type_ptr blkTypes);

    // Update the location of all members of all macros based on location of macro_head
    // Since only macro_head is solved (connections to macro members are also taken into account
    // when formulating the matrix equations)
    void update_macros();

    // Build and solve in one direction
    // Solved solutions are written back to block_locs in rawx, rawy
    void build_solve_direction(bool yaxis, int iter, int build_solve_iter);

    // Build the system of equations for either X or Y
    // When iter > 1, psudo-conenctions are formed, the strength is determined by alpha and iter
    void build_equations(EquationSystem<double>& es, bool yaxis, int iter = -1);

    // Solve the system of equations
    void solve_equations(EquationSystem<double>& es, bool yaxis);

    // Debug use
    float find_overlap();

    // Debug use
    std::string print_overlap();

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
                         const wirelen_t solvedHPWL,
                         const wirelen_t spreadHPWL,
                         const wirelen_t legalHPWL);

    void print_iter_stats(const int iter,
                          const float iterTime,
                          const float time,
                          const wirelen_t bestHPWL,
                          const int stall);
};

// #endif /* ENABLE_ANALYTIC_PLACE */

#endif /* VPR_ANALYTIC_PLACEMENT_H */
