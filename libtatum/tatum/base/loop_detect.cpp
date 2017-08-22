#include <stack>
#include <algorithm>

#include "tatum/TimingGraph.hpp"
#include "tatum/base/loop_detect.hpp"
#include "tatum/util/tatum_linear_map.hpp"

namespace tatum {

//Internal data used in identify_strongly_connected_components() and strongconnect()
struct NodeSccInfo {
    bool on_stack = false;
    int index = -1;
    int low_link = -1;
};

void strongconnect(const tatum::TimingGraph& tg,
                   tatum::NodeId node, 
                   int& cur_index, 
                   std::stack<tatum::NodeId>& stack,
                   tatum::util::linear_map<tatum::NodeId,NodeSccInfo>& node_info,
                   std::vector<std::vector<tatum::NodeId>>& sccs,
                   size_t min_size);




//Returns sets of nodes (i.e. strongly connected componenets) which exceed the specifided min_size
std::vector<std::vector<NodeId>> identify_strongly_connected_components(const TimingGraph& tg, size_t min_size) {
    //This uses Tarjan's algorithm which identifies Strongly Connected Components (SCCs) in O(|V| + |E|) time
    int curr_index = 0;
    std::stack<NodeId> stack;
    tatum::util::linear_map<NodeId,NodeSccInfo> node_info(tg.nodes().size());
    std::vector<std::vector<NodeId>> sccs;

    for(NodeId node : tg.nodes()) {
        if(node_info[node].index == -1) {
            strongconnect(tg, node, curr_index, stack, node_info, sccs, min_size); 
        }
    }

    return sccs;
}


void strongconnect(const TimingGraph& tg,
                   NodeId node, 
                   int& cur_index, 
                   std::stack<NodeId>& stack,
                   tatum::util::linear_map<NodeId,NodeSccInfo>& node_info,
                   std::vector<std::vector<NodeId>>& sccs,
                   size_t min_size) {
    node_info[node].index = cur_index;
    node_info[node].low_link = cur_index;
    ++cur_index;

    stack.push(node);
    node_info[node].on_stack = true;

    for(EdgeId edge : tg.node_out_edges(node)) {
        if(tg.edge_disabled(edge)) continue;

        NodeId sink_node = tg.edge_sink_node(edge);

        if(node_info[sink_node].index == -1) {
            //Have not visited sink_node yet
            strongconnect(tg, sink_node, cur_index, stack, node_info, sccs, min_size);
            node_info[node].low_link = std::min(node_info[node].low_link, node_info[sink_node].low_link);
        } else if(node_info[sink_node].on_stack) {
            //sink_node is part of the SCC
            node_info[node].low_link = std::min(node_info[node].low_link, node_info[sink_node].low_link);
        }
    }

    if(node_info[node].low_link == node_info[node].index) {
        //node is the root of a SCC
        std::vector<NodeId> scc;

        NodeId scc_node;
        do {
            scc_node = stack.top();
            stack.pop();

            node_info[scc_node].on_stack = false;

            scc.push_back(scc_node);

        } while(scc_node != node);

        if(scc.size() >= min_size) {
            sccs.push_back(scc);
        }
    }
}

}
