#pragma once
#include "TimingTags.hpp"
#include "timing_graph_fwd.hpp"
#include "tatum_linear_map.hpp"

namespace tatum { namespace detail {

/** \class HoldAnalysisOps
 *
 * The operations for CommonAnalysisVisitor to perform hold analysis.
 * The operations are similar to those used for setup analysis, except that minum edge delays 
 * are used, and the minumum arrival times (and maximum required times) are propagated through 
 * the timing graph.
 *
 * \see SetupAnalysisOps
 * \see CommonAnalysisVisitor
 */
class HoldAnalysisOps {
    public:
        HoldAnalysisOps(size_t num_tags)
            : data_tags_(num_tags)
            , clock_tags_(num_tags) {}

        TimingTags& get_data_tags(const NodeId node_id) { return data_tags_[node_id]; }
        TimingTags& get_clock_tags(const NodeId node_id) { return clock_tags_[node_id]; }
        const TimingTags& get_data_tags(const NodeId node_id) const { return data_tags_[node_id]; }
        const TimingTags& get_clock_tags(const NodeId node_id) const { return clock_tags_[node_id]; }

        void reset() { data_tags_.clear(); clock_tags_.clear(); }

        float clock_constraint(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            return tc.hold_constraint(src_id, sink_id); 
        }

        void merge_req_tags(TimingTags& tags, const Time time, const TimingTag& ref_tag) { 
            tags.max_req(time, ref_tag); 
        }

        void merge_req_tag(TimingTag& tag, const Time time, const TimingTag& ref_tag) { 
            tag.max_req(time, ref_tag); 
        }

        void merge_arr_tags(TimingTags& tags, const Time time, const TimingTag& ref_tag) { 
            tags.min_arr(time, ref_tag); 
        }

        void merge_arr_tag(TimingTag& tag, const Time time, const TimingTag& ref_tag) { 
            tag.min_arr(time, ref_tag); 
        };

        template<class DelayCalc>
        const Time edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            NodeId src_node = tg.edge_src_node(edge_id);
            NodeId sink_node = tg.edge_sink_node(edge_id);

            if(tg.node_type(src_node) == NodeType::CPIN && tg.node_type(sink_node) == NodeType::SINK) {
                return dc.hold_time(tg, edge_id);
            } else {
                return dc.max_edge_delay(tg, edge_id); 
            }
        }

    private:
        tatum::util::linear_map<NodeId,TimingTags> data_tags_;
        tatum::util::linear_map<NodeId,TimingTags> clock_tags_;
};

}} //namespace

