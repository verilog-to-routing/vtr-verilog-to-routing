#ifndef BREAKPOINT_STATE_GLOBALS
#define BREAKPOINT_STATE_GLOBALS

#include <string>
#include <vector>

//the BreakpointState struct holds all values that could possibly trigger a breakpoint
//some variables such as move_num, from_block, temp_count, blocks_affected are related to the placer and router_iter and net_id are related to the router
//there is also a string that holds the breakpoint description that are displayed in the UI and printed to the terminal
//these values are updated in place.cpp and route.cpp and expr_eval.cpp and breakpoint.cpp use these values to look for breakpoints
struct BreakpointState {
    int move_num = 0;
    int from_block = -1;
    int temp_count = 0;
    int blocks_affected = -1;
    std::vector<int> blocks_affected_vector;
    int net_id = -1;
    int router_iter = 0;
    std::string bp_description;
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
