#pragma once
#include "tatum/graph_visitors/CommonAnalysisOps.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"

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

        Time clock_constraint(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            return tc.setup_constraint(src_id, sink_id); 
        }

        Time clock_uncertainty(const TimingConstraints& tc, const DomainId src_id, const DomainId sink_id) { 
            //Setup analysis, so early capture clock arrival is pessimistic
            return -tc.setup_clock_uncertainty(src_id, sink_id); 
        }

        Time launch_source_latency(const TimingConstraints& tc, const DomainId domain) {
            //For pessimistic setup analysis launch occurs late
            return tc.source_latency(domain, ArrivalType::LATE);
        }

        Time capture_source_latency(const TimingConstraints& tc, const DomainId domain) {
            //For pessimistic setup analysis capture occurs early
            return tc.source_latency(domain, ArrivalType::EARLY);
        }

        Time input_constraint(const TimingConstraints& tc, const NodeId node, const DomainId domain) {
            return tc.input_constraint(node, domain, DelayType::MAX);
        }

        auto input_constraints(const TimingConstraints& tc, const NodeId node) {
            return tc.input_constraints(node, DelayType::MAX);
        }

        Time output_constraint(const TimingConstraints& tc, const NodeId node, const DomainId domain) {
            return tc.output_constraint(node, domain, DelayType::MAX);
        }

        auto output_constraints(const TimingConstraints& tc, const NodeId node) {
            return tc.output_constraints(node, DelayType::MAX);
        }

        TimingTag const_gen_tag() { return TimingTag::CONST_GEN_TAG_SETUP(); }

        void merge_req_tags(const NodeId node, const Time time, const NodeId origin, const TimingTag& ref_tag, bool arrival_must_be_valid=false) { 
            node_tags_[node].min(time, origin, ref_tag, arrival_must_be_valid); 
        }

        void merge_arr_tags(const NodeId node, const Time time, const NodeId origin, const TimingTag& ref_tag) { 
            node_tags_[node].max(time, origin, ref_tag); 
        }

        Time data_edge_delay(const DelayCalculator& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            Time delay = dc.max_edge_delay(tg, edge_id); 

            TATUM_ASSERT_MSG(delay.value() >= 0., "Data edge delay expected to be positive");

            return delay;
        }

        Time launch_clock_edge_delay(const DelayCalculator& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            Time delay = dc.max_edge_delay(tg, edge_id);

            TATUM_ASSERT_MSG(delay.value() >= 0., "Launch clock edge delay expected to be positive");

            return delay;
        }

        Time capture_clock_edge_delay(const DelayCalculator& dc, const TimingGraph& tg, const EdgeId edge_id) { 
            NodeId src_node = tg.edge_src_node(edge_id);
            NodeId sink_node = tg.edge_sink_node(edge_id);

            if(tg.node_type(src_node) == NodeType::CPIN && tg.node_type(sink_node) == NodeType::SINK) {
                Time tsu = dc.setup_time(tg, edge_id);
                TATUM_ASSERT_MSG(!std::isnan(tsu.value()), "Setup Time (Tsu) expected to be numeric value (not NaN)");

                //The setup time is returned as a negative value, since it is placed on the clock path
                //(instead of the data path).
                return -tsu;
            } else {
                Time tcq = dc.max_edge_delay(tg, edge_id);
                TATUM_ASSERT_MSG(tcq.value() >= 0., "Clock-to-q delay (Tcq) expected to be positive");

                return tcq; 
            }
        }

        Time calculate_slack(const Time required_time, const Time arrival_time) {
            //Setup requires the arrival to occur *before* the required time, so
            //slack is the amount of required time left after the arrival time; meaning
            //we we subtract the arrival time from the required time to get the setup slack
            return required_time - arrival_time;
        }

};

}} //namespace

