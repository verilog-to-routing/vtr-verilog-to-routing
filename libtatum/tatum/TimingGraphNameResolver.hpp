#ifndef TATUM_TIMING_GRAPH_NAME_RESOLVER_HPP
#define TATUM_TIMING_GRAPH_NAME_RESOLVER_HPP
#include <string>
#include <vector>
#include "TimingGraphFwd.hpp"
#include "tatum/base/DelayType.hpp"
#include "tatum/Time.hpp"

namespace tatum {

struct DelayComponent {
    std::string type_name; //Type of element
    std::string inst_name; //Unique identifier of element
    Time delay; //Associated delay of element
};

struct EdgeDelayBreakdown {
    std::vector<DelayComponent> components;
};

//Abstract interface for resolving timing graph nodes to human-readable strings
class TimingGraphNameResolver {
    public:
        virtual ~TimingGraphNameResolver() = default;

        virtual std::string node_name(tatum::NodeId node) const = 0;
        virtual std::string node_type_name(tatum::NodeId node) const = 0;
        virtual EdgeDelayBreakdown edge_delay_breakdown(tatum::EdgeId /*edge*/, DelayType /*delay_type*/) const {
            //Default no edge delay breakdown
            return EdgeDelayBreakdown();
        }
};

} //namespace

#endif
