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

        Time clock_constraint(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            return tc.hold_constraint(src_id, sink_id); 
        }

        Time clock_uncertainty(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            //Hold analysis, so late capture clock arrival is pessimistic
            return +tc.hold_clock_uncertainty(src_id, sink_id); 
        }

        TimingTag const_gen_tag() { return TimingTag::CONST_GEN_TAG_HOLD(); }

        void merge_req_tags(const NodeId node, const Time time, const NodeId origin, const TimingTag& ref_tag, bool arrival_must_be_valid=false) { 
            node_tags_[node].max(time, origin, ref_tag, arrival_must_be_valid); 
        }

        void merge_arr_tags(const NodeId node, const Time time, const NodeId origin, const TimingTag& ref_tag) { 
            node_tags_[node].min(time, origin, ref_tag); 
        }

        template<class DelayCalc>
        Time data_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            Time delay = dc.min_edge_delay(tg, edge_id);
            TATUM_ASSERT_MSG(delay.value() >= 0., "Data edge delay expected to be positive");

            return delay; 
        }

        template<class DelayCalc>
        Time launch_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            Time delay = dc.min_edge_delay(tg, edge_id);
            TATUM_ASSERT_MSG(delay.value() >= 0., "Launch clock edge delay expected to be positive");

            return delay;
        }

        template<class DelayCalc>
        Time capture_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            NodeId src_node = tg.edge_src_node(edge_id);
            NodeId sink_node = tg.edge_sink_node(edge_id);

            if(tg.node_type(src_node) == NodeType::CPIN && tg.node_type(sink_node) == NodeType::SINK) {
                Time thld = dc.hold_time(tg, edge_id);
                TATUM_ASSERT_MSG(!std::isnan(thld.value()), "Hold Time (Thld) must be numeric value (not NaN)");

                //The hold time is returned as a positive value, since it is placed on the clock path
                //(instead of the data path).
                return thld;
            } else {
                Time tcq = dc.max_edge_delay(tg, edge_id);
                TATUM_ASSERT_MSG(tcq.value() >= 0., "Clock-to-q delay (Tcq) expected to be positive");

                return tcq; 
            }
        }
};

}} //namespace

