#pragma once

#include <vector>

#include "timing_graph_fwd.hpp"
#include "TimingGraph.hpp"
#include "Time.hpp"

class TimingNode {
    public:
        TimingNode()
            : type_(TN_Type::UNKOWN)
            , clock_domain_(INVALID_CLOCK_DOMAIN)
            , is_clock_source_(false) {}
        TimingNode(TN_Type new_type, DomainId clk_domain, BlockId blk_id, bool is_clk_src)
            : type_(new_type)
            , clock_domain_(clk_domain) 
            , logical_block_(blk_id)
            , is_clock_source_(is_clk_src) {}

        int num_out_edges() const { return out_edge_ids_.size(); }
        int num_in_edges() const { return in_edge_ids_.size(); }

        EdgeId out_edge_id(int idx) const { return out_edge_ids_[idx]; }
        void add_out_edge_id(EdgeId edge_id) { out_edge_ids_.push_back(edge_id); }

        EdgeId in_edge_id(int idx) const { return in_edge_ids_[idx]; }
        void add_in_edge_id(EdgeId edge_id) { in_edge_ids_.push_back(edge_id); }

        TN_Type type() const { return type_; }
        DomainId clock_domain() const { return clock_domain_; }
        BlockId logical_block() const { return logical_block_; }
        bool is_clock_source() const { return is_clock_source_; }

        Time arrival_time() const { return T_arr_; }
        void set_arrival_time(Time new_arrival_time) { T_arr_ = new_arrival_time; }

        Time required_time() const { return T_req_; }
        void set_required_time(Time new_required_time) { T_req_ = new_required_time; }

    private:
        Time T_arr_; //Data arrival time at this node
        Time T_req_; //Data required arrival time at this node
        Time T_clock_delay_; //Clock arrival time at this node, only valid for FF types

        TN_Type type_;
        DomainId clock_domain_;
        BlockId logical_block_;
        bool is_clock_source_;

        std::vector<EdgeId> in_edge_ids_; //Timing edges driving this node
        std::vector<EdgeId> out_edge_ids_; //Timing edges driven by this node

};
