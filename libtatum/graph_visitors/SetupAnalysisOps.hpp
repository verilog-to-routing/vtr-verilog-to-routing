#pragma once
#include "CommonAnalysisOps.hpp"

namespace tatum { namespace detail {

/** \class SetupAnalysisOps
 *
 * The operations for CommonAnalysisVisitor to perform setup analysis.
 * The setup analysis operations define that maximum edge delays are used, and that the 
 * maixmum arrival time (and minimum required times) are propagated through the timing graph.
 *
 * \see HoldAnalysisOps
 * \see CommonAnalysisVisitor
 */
class SetupAnalysisOps : public CommonAnalysisOps {
    public:
        SetupAnalysisOps(size_t num_tags)
            : CommonAnalysisOps(num_tags) {}

        float clock_constraint(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            return tc.setup_constraint(src_id, sink_id); 
        }

        void merge_req_tags(TimingTags& tags, const Time time, const TimingTag& ref_tag) { 
            tags.min_req(time, ref_tag); 
        }

        void merge_req_tag(TimingTag& tag, const Time time, const TimingTag& ref_tag) { 
            tag.min_req(time, ref_tag); 
        }

        void merge_arr_tags(TimingTags& tags, const Time time, const TimingTag& ref_tag) { 
            tags.max_arr(time, ref_tag); 
        }

        void merge_arr_tag(TimingTag& tag, const Time time, const TimingTag& ref_tag) { 
            tag.max_arr(time, ref_tag); 
        }

        template<class DelayCalc>
        Time data_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            return dc.max_edge_delay(tg, edge_id); 
        }

        template<class DelayCalc>
        Time launch_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            return dc.max_edge_delay(tg, edge_id);
        }

        template<class DelayCalc>
        Time capture_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            NodeId src_node = tg.edge_src_node(edge_id);
            NodeId sink_node = tg.edge_sink_node(edge_id);

            if(tg.node_type(src_node) == NodeType::CPIN && tg.node_type(sink_node) == NodeType::SINK) {
                return -dc.setup_time(tg, edge_id);
            } else {
                return dc.min_edge_delay(tg, edge_id); 
            }
        }

};

}} //namespace

