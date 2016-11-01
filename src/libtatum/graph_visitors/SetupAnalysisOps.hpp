#pragma once
#include "TimingTags.hpp"
#include "timing_graph_fwd.hpp"
#include "tatum_linear_map.hpp"

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
class SetupAnalysisOps {
    public:
        SetupAnalysisOps(size_t num_tags)
            : data_tags_(num_tags)
            , clock_tags_(num_tags) {}

        TimingTags& get_data_tags(const NodeId node_id) { return data_tags_[node_id]; }
        TimingTags& get_clock_tags(const NodeId node_id) { return clock_tags_[node_id]; }
        const TimingTags& get_data_tags(const NodeId node_id) const { return data_tags_[node_id]; }
        const TimingTags& get_clock_tags(const NodeId node_id) const { return clock_tags_[node_id]; }

        void reset() { data_tags_.clear(); clock_tags_.clear(); }

        float clock_constraint(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            return tc.setup_clock_constraint(src_id, sink_id); 
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
        };

        template<class DelayCalc>
        const Time edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            return dc.max_edge_delay(tg, edge_id); 
        }

    private:
        tatum::util::linear_map<NodeId,TimingTags> data_tags_;
        tatum::util::linear_map<NodeId,TimingTags> clock_tags_;
};

}} //namespace

