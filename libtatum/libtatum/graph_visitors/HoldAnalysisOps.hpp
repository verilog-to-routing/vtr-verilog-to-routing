#pragma once
#include "CommonAnalysisOps.hpp"

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
class HoldAnalysisOps : public CommonAnalysisOps {
    public:
        HoldAnalysisOps(size_t num_tags, size_t num_slacks)
            : CommonAnalysisOps(num_tags, num_slacks) {}

        float clock_constraint(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            return tc.hold_constraint(src_id, sink_id); 
        }

        void merge_req_tags(const NodeId node, const Time time, const TimingTag& ref_tag, bool arrival_must_be_valid=false) { 
            node_tags_[node].max(time, ref_tag, arrival_must_be_valid); 
        }

        void merge_arr_tags(const NodeId node, const Time time, const TimingTag& ref_tag) { 
            node_tags_[node].min(time, ref_tag); 
        }

        void merge_slack_tags(const EdgeId edge, const Time time, TimingTag ref_tag) { 
            ref_tag.set_type(TagType::SLACK);
            edge_slacks_[edge].min(time, ref_tag); 
        }

        template<class DelayCalc>
        Time data_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            return dc.min_edge_delay(tg, edge_id); 
        }

        template<class DelayCalc>
        Time launch_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            return dc.min_edge_delay(tg, edge_id);
        }

        template<class DelayCalc>
        Time capture_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            NodeId src_node = tg.edge_src_node(edge_id);
            NodeId sink_node = tg.edge_sink_node(edge_id);

            if(tg.node_type(src_node) == NodeType::CPIN && tg.node_type(sink_node) == NodeType::SINK) {
                return dc.hold_time(tg, edge_id);
            } else {
                return dc.max_edge_delay(tg, edge_id); 
            }
        }
};

}} //namespace

