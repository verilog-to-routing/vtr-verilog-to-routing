#ifndef TATUM_LOOP_DETECT
#define TATUM_LOOP_DETECT

#include <set>
#include <vector>

#include "tatum/TimingGraphFwd.hpp"

namespace tatum {

//Returns the set of nodes (Strongly Connected Components) that form loops in the timing graph
std::vector<std::vector<NodeId>> identify_combinational_loops(const TimingGraph& tg);

}

#endif
