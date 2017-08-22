#ifndef TATUM_TIMING_GRAPH_NAME_RESOLVER_HPP
#define TATUM_TIMING_GRAPH_NAME_RESOLVER_HPP
#include <string>
#include "TimingGraphFwd.hpp"

namespace tatum {

//Abstract interface for resolving timing graph nodes to human-readable strings
class TimingGraphNameResolver {
    public:
        virtual ~TimingGraphNameResolver() = default;

        virtual std::string node_name(tatum::NodeId node) const = 0;
        virtual std::string node_block_type_name(tatum::NodeId node) const = 0;
};

} //namespace

#endif
