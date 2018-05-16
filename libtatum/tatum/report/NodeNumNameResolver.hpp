#ifndef TATUM_NODE_NUM_NAME_RESOLVER_HPP
#define TATUM_NODE_NUM_NAME_RESOLVER_HPP

#include "tatum/TimingGraphNameResolver.hpp"

namespace tatum {

//A name resolver which just resolved to node ID's and node types
class NodeNumResolver : public TimingGraphNameResolver {
    public:
        NodeNumResolver(const TimingGraph& tg, const DelayCalculator& dc, bool verbose)
            : tg_(tg)
            , dc_(dc)
            , verbose_(verbose) {}

        std::string node_name(NodeId node) const override {
            return "Node(" + std::to_string(size_t(node)) + ")";
        }

        std::string node_type_name(NodeId node) const override {
            auto type = tg_.node_type(node);
            std::stringstream ss;
            ss << type;
            return ss.str();
        }

        EdgeDelayBreakdown edge_delay_breakdown(EdgeId edge, DelayType delay_type) const override {
            EdgeDelayBreakdown delay_breakdown;

            if (edge && verbose_) {
                auto edge_type = tg_.edge_type(edge);

                DelayComponent component;
                component.inst_name = "Edge(" + std::to_string(size_t(edge)) + ")";
                std::stringstream ss;
                ss << edge_type;
                component.type_name = ss.str();

                if (delay_type == DelayType::MAX) {
                    if (edge_type == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                        component.delay = dc_.setup_time(tg_, edge);
                    } else {
                        component.delay = dc_.max_edge_delay(tg_, edge);
                    }
                } else if (delay_type == DelayType::MIN) {
                    if (edge_type == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                        component.delay = dc_.hold_time(tg_, edge);
                    } else {
                        component.delay = dc_.min_edge_delay(tg_, edge);
                    }
                }

                delay_breakdown.components.push_back(component);
            }

            return delay_breakdown;
        }
    private:
        const TimingGraph& tg_;
        const DelayCalculator& dc_;
        bool verbose_;
};

} //namespace
#endif
