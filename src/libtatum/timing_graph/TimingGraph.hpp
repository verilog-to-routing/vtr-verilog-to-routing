#pragma once

#include <vector>
#include <map>
#include <forward_list>
#include <iostream>
#include <iomanip>

#include "Time.hpp"

#include "timing_graph_fwd.hpp"

#include "TimingNode.hpp"
#include "TimingEdge.hpp"
#include "TimingTag.hpp"

#include "aligned_allocator.hpp"

class TimingGraph {
    public:
        //Node accessors
        TN_Type node_type(NodeId id) const { return node_types_[id]; }
        DomainId node_clock_domain(NodeId id) const { return node_clock_domains_[id]; }
        int num_node_out_edges(NodeId id) const { return node_out_edges_[id].size(); }
        int num_node_in_edges(NodeId id) const { return node_in_edges_[id].size(); }
        EdgeId node_out_edge(NodeId node_id, int edge_idx) const { return node_out_edges_[node_id][edge_idx]; }
        EdgeId node_in_edge(NodeId node_id, int edge_idx) const { return node_in_edges_[node_id][edge_idx]; }
        const std::vector<TimingTag>& node_arr_times(NodeId id) const { return node_arr_times_[id]; }
        const std::vector<TimingTag>& node_req_times(NodeId id) const { return node_req_times_[id]; }

        //Node modifiers
        void set_node_arr_times(NodeId id, const std::vector<TimingTag>& tags) { node_arr_times_[id] = tags; }
        void set_node_req_times(NodeId id, const std::vector<TimingTag>& tags) { node_req_times_[id] = tags; }
        void add_node_arr_time(NodeId id, const TimingTag& tags) { node_arr_times_[id].push_back(tags); }
        void add_node_req_time(NodeId id, const TimingTag& tags) { node_req_times_[id].push_back(tags); }
        void reset_node_arr_times(NodeId id) { node_arr_times_[id].clear(); }
        void reset_node_req_times(NodeId id) { node_req_times_[id].clear(); }

        //Edge accessors
        NodeId edge_sink_node(EdgeId id) const { return edge_sink_nodes_[id]; }
        NodeId edge_src_node(EdgeId id) const { return edge_src_nodes_[id]; }
        Time edge_delay(EdgeId id) const { return edge_delays_[id]; }



        //Graph accessors
        NodeId num_nodes() const { return node_arr_times_.size(); }
        EdgeId num_edges() const { return edge_delays_.size(); }
        int num_levels() const { return node_levels_.size(); }

        const std::vector<NodeId>& level(NodeId level_id) const { return node_levels_[level_id]; }

        const std::vector<NodeId>& primary_inputs() const { return node_levels_[0]; }
        const std::vector<NodeId>& primary_outputs() const { return primary_outputs_; }

        //Graph modifiers
        NodeId add_node(const TimingNode& new_node);
        EdgeId add_edge(const TimingEdge& new_edge);
        void set_num_levels(const NodeId nlevels) { node_levels_ = std::vector<std::vector<NodeId>>(nlevels); }
        void add_level(const NodeId level_id, const std::vector<NodeId>& level_node_ids) {node_levels_[level_id] = level_node_ids;}
        void fill_back_edges();
        void contiguize_level_edges();
        std::map<NodeId,NodeId> contiguize_level_nodes();

    private:
        /*
         * For improved memory locality, we use a Struct of Arrays (SoA)
         * data layout, rather than Array of Structs (AoS)
         */
        //Node data
        std::vector<TN_Type> node_types_;
        std::vector<DomainId> node_clock_domains_;
        std::vector<std::vector<EdgeId>> node_out_edges_;
        std::vector<std::vector<EdgeId>> node_in_edges_;

#ifdef TIME_MEM_ALIGN
        std::vector<Time, aligned_allocator<Time, TIME_MEM_ALIGN>> node_arr_times_;
        std::vector<Time, aligned_allocator<Time, TIME_MEM_ALIGN>> node_req_times_;
#else
        std::vector<std::vector<TimingTag>> node_arr_times_;
        std::vector<std::vector<TimingTag>> node_req_times_;
#endif

        //Edge data
        std::vector<NodeId> edge_sink_nodes_;
        std::vector<NodeId> edge_src_nodes_;
#ifdef TIME_MEM_ALIGN
        std::vector<Time, aligned_allocator<Time, TIME_MEM_ALIGN>> edge_delays_;
#else
        std::vector<Time> edge_delays_;
#endif


        std::vector<std::vector<NodeId>> node_levels_;
        std::vector<NodeId> primary_outputs_;
};
