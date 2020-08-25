#ifndef VPR_ANALYTIC_PLACEMENT_H
#define VPR_ANALYTIC_PLACEMENT_H

#include "vpr_context.h"
#include "timing_place.h"
#include "PlacementDelayCalculator.h"

typedef signed long int wirelen_t;

template<typename T>
struct EquationSystem;

class AnalyticPlacer {
  public:
    AnalyticPlacer(VprContext& ctx);

    void ap_place();

  private:
    // for CutSpreader to access legal_pos, block_locs, etc.
    friend class CutSpreader;

    // TODO: PlacerHeapCfg should be externally configured & supplied
    struct PlacerHeapCfg {
        float alpha, beta;
        int criticalityExponent;
        int timingWeight;
        float solverTolerance;
        bool placeAllAtOnce;
        int spread_scale_x, spread_scale_y;
    };

    PlacerHeapCfg tmpCfg;

    int max_x = 0, max_y = 0; //size of the grid
    VprContext& vpr_ctx;
    const DeviceContext& device_ctx;
    const ClusteredNetlist& clb_nlist;
    PlacementContext& place_ctx;

    std::vector<std::vector<int>> num_legal_pos; /* [0..num_sub_tiles - 1][0..num_legal_pos-1] */
    std::vector<std::vector<std::vector<t_pl_loc>>> legal_pos;
    std::vector<std::vector<std::vector<std::vector<std::vector<t_pl_loc>>>>> fast_tiles;

    // marker for blks not solved in current equation
    int32_t dont_solve = std::numeric_limits<int32_t>::max();
    // marker for blks not in any macros
    size_t not_in_macros = std::numeric_limits<size_t>::max();

    struct MacroInfo;

    struct BlockInfo {
        ClusterBlockId blkId;
        int32_t udata = std::numeric_limits<int32_t>::max(); //dont_solve
        t_logical_block_type_ptr type = nullptr;

        // info for macros
        MacroInfo* macroInfo = nullptr; // nullptr if not in macro
    };
    std::unordered_map<ClusterBlockId, BlockInfo> blk_info;

    struct BlockLocation {
        t_pl_loc loc;
        t_pl_loc legal_loc;
        int rawz;
        double rawx, rawy;
        bool locked;
    };

    std::unordered_map<ClusterBlockId, BlockLocation> blk_locs;

    struct MacroInfo {
        const BlockInfo* macro_head = nullptr;
        const t_pl_macro* pl_macro = nullptr;
        size_t imember = std::numeric_limits<size_t>::max(); // 0 if head of the macro
        t_pl_offset offset;
    };

    // keep track the set of blocks that are part of a macro
    std::unordered_map<ClusterBlockId, MacroInfo> macro_blks;

    // The set of blks actually placed.
    // Excludes children of macros.
    std::vector<ClusterBlockId> place_blks;

    // blocks being solved in the current equation
    // subset of all blocks being placed
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

    // build blk_info and blk_locs based on initial placement;
    // build macro_blks and place_blks;
    void init_blk_info();

    wirelen_t get_net_hpwl(ClusterNetId net_id);

    wirelen_t total_hpwl();

    // Setup the cells to be solved, returns the number of rows (number of blks to solve)
    int setup_solve_blks(t_logical_block_type_ptr blkTypes);

    // Update the location of all members of all macros
    void update_macros();

    // Build and solve in one direction
    void build_solve_direction(bool yaxis, int iter, int build_solve_iter);

    // Build the system of equations for either X or Y
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

#endif
