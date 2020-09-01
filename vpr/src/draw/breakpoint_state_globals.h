#ifndef BREAKPOINT_STATE_GLOBALS
#define BREAKPOINT_STATE_GLOBALS

#include <string>
#include <vector>

//the BreakpointState struct holds all values that could possibly trigger a breakpoint
//some variables such as move_num, from_block, temp_count, blocks_affected are related to the placer and router_iter and net_id are related to the router
//there is also a string that holds the breakpoint description that are displayed in the UI and printed to the terminal
//these values are updated in place.cpp and route.cpp and expr_eval.cpp and breakpoint.cpp use these values to look for breakpoints
struct BreakpointState {
    int move_num = 0;                         //current number of completed placer moves
    int from_block = -1;                      //first block moved in the current placement swap
    int temp_count = 0;                       //number of temperature changes thus far
    int block_affected = -1;                  //the block_id that was requested to be stopped at if in blocks_affected
    std::vector<int> blocks_affected_by_move; //vector giving the clb netlist block ids of all blocks moving in the current perturbation
    int route_net_id = -1;                    //clb netlist id of net that was just routed
    int router_iter = 0;                      //current rip-up and re-route iteration count of router
    std::string bp_description;               //the breakpoint description to appear in the breakpoint list in the GUI
};

class BreakpointStateGlobals {
    //holds one global BreakpointState variable to be accessed and modified by the placer and router
    BreakpointState glob_breakpoint_state;

  public:
    //accessor for glob_breakpoint_state
    BreakpointState* get_glob_breakpoint_state() {
        return &glob_breakpoint_state;
    }
};

#endif
