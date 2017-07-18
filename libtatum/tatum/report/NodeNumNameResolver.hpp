#ifndef TATUM_NODE_NUM_NAME_RESOLVER_HPP
#define TATUM_NODE_NUM_NAME_RESOLVER_HPP

#include "tatum/TimingGraphNameResolver.hpp"

namespace tatum {

//A name resolver which just resolved to node ID's and node types
class NodeNumResolver : public tatum::TimingGraphNameResolver {
    public:
        NodeNumResolver(const tatum::TimingGraph& tg): tg_(tg) {}

        std::string node_name(tatum::NodeId node) const override {
            return "Node(" + std::to_string(size_t(node)) + ")";
        }

        std::string node_block_type_name(tatum::NodeId node) const override {
            auto type = tg_.node_type(node);
            std::stringstream ss;
            ss << type;
            return ss.str();
        }
    private:
        const tatum::TimingGraph& tg_;
};

} //namespace
#endif
