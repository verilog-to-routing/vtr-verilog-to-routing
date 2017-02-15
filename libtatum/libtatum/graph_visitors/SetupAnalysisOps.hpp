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
        SetupAnalysisOps(size_t num_tags, size_t num_slacks)
            : CommonAnalysisOps(num_tags, num_slacks) {}

        float clock_constraint(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            return tc.setup_constraint(src_id, sink_id); 
        }

        void merge_req_tags(const NodeId node, const Time time, const TimingTag& ref_tag, bool arrival_must_be_valid=false) { 
            node_tags_[node].min(time, ref_tag, arrival_must_be_valid); 
        }

        void merge_arr_tags(const NodeId node, const Time time, const TimingTag& ref_tag) { 
            node_tags_[node].max(time, ref_tag); 
        }

        template<class DelayCalc>
        Time data_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            Time delay = dc.max_edge_delay(tg, edge_id); 

            TATUM_ASSERT_MSG(delay.value() >= 0., "Data edge delay expected to be positive");

            return delay;
        }

        template<class DelayCalc>
        Time launch_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            Time delay = dc.max_edge_delay(tg, edge_id);

            TATUM_ASSERT_MSG(delay.value() >= 0., "Launch clock edge delay expected to be positive");

            return delay;
        }

        template<class DelayCalc>
        Time capture_clock_edge_delay(const DelayCalc& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            NodeId src_node = tg.edge_src_node(edge_id);
            NodeId sink_node = tg.edge_sink_node(edge_id);

            if(tg.node_type(src_node) == NodeType::CPIN && tg.node_type(sink_node) == NodeType::SINK) {
                Time tsu = dc.setup_time(tg, edge_id);
                TATUM_ASSERT_MSG(tsu.value() >= 0., "Setup Time (Tsu) expected to be positive");

                return -tsu;
            } else {
                Time tcq = dc.min_edge_delay(tg, edge_id);
                TATUM_ASSERT_MSG(tcq.value() >= 0., "Clock-to-q delay (Tcq) expected to be positive");

                return tcq; 
            }
        }

};

}} //namespace

