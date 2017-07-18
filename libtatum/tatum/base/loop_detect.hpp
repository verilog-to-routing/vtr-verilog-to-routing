#ifndef TATUM_LOOP_DETECT
#define TATUM_LOOP_DETECT

#include <set>
#include <vector>

#include "tatum/TimingGraphFwd.hpp"

namespace tatum {

//Returns the set of Strongly Connected Components with 
//size >= min_size found in the timing graph
std::vector<std::vector<NodeId>> identify_strongly_connected_components(const TimingGraph& tg, size_t min_size);

}

#endif
