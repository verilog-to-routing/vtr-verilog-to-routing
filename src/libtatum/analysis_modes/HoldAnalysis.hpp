#pragma once
#include <memory>
#include <vector>

#include "tatum_assert.hpp"

#include "CommonAnalysisVisitor.hpp"
#include "TimingTags.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

/** \file
 * The 'HoldAnalysis' class defines the operations needed by a timing analyzer class
 * to perform a hold (min/shortest path) analysis.
 *
 * \see SetupAnalysis
 * \see TimingAnalyzer
 *
 * Hold Analysis Principles
 * ==========================
 *
 * In addition to satisfying setup constraints, data arriving at a Flip-Flop (FF) must stay (i.e.
 * remain stable) for some amount of time *after* the capturing clock edge arrives. This time is
 * referred to as the 'Hold Time' of the Flip-Flop, \f$ t_h \f$.  If the data changes during the 
 * hold window (i.e. less than \f$ t_h \f$ after the capturing clock edge) then the FF may go 
 * meta-stable failing to capture the data. This will put the circuit in an invalid state (this 
 * is bad).
 *
 * More formally, for correct operation at every cycle we require the following to be satisfied
 * for every path in the circuit:
 *
 * \f[
 *      t_{clk\_insrt}^{(launch)} + t_{cq}^{(min)} + t_{comb}^{(min)} \geq t_{clk\_insrt}^{(capture)} + t_h   (1)
 * \f]
 *
 * where \f$ t_{clk\_insrt}^{(launch)}, t_{clk\_insrt}^{(capture)} \f$ are the up/downstream FF clock insertion
 * delays,  \f$ t_{cq}^{(min)} \f$ is the minimum clock-to-q delay of the upstream FF, \f$ t_{comb}^{(min)} \f$ is
 * the minimum combinational path delay from the upstream to downstream FFs, and \f$ t_h \f$ is the hold
 * constraint of the downstream FF.
 *
 * Note that unlike in setup analysis this behaviour is indepenant of clock period.
 * Intuitively, hold analysis can be viewed as data from the upstream FF trampling the data launched
 * on the previous cycle before it can be captured by the downstream FF.
 */

/** \class HoldAnalysisOps
 *
 * The hold analysis operations are similar to those used for setup analysis, except that minum edge delays 
 * are used, and the minumum arrival times (and maximum required times) are propagated through the timing graph.
 *
 * \see SetupAnalysisOps
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
            return tc.hold_clock_constraint(src_id, sink_id); 
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
            return dc.min_edge_delay(tg, edge_id); 
        }

    private:
        tatum::linear_map<NodeId,TimingTags> data_tags_;
        tatum::linear_map<NodeId,TimingTags> clock_tags_;
};

/** \class HoldAnalysis
 *
 * The 'HoldAnalysis' class defines the operations needed by a timing analyzer
 * to perform a hold (min/shortest path) analysis.
 *
 * \see SetupAnalysis
 * \see TimingAnalyzer
 * \see CommonAnalysisVisitor
 */
class HoldAnalysis : public CommonAnalysisVisitor<HoldAnalysisOps> {

    public:
        HoldAnalysis(size_t num_tags)
            : CommonAnalysisVisitor<HoldAnalysisOps>(num_tags) {}

        const TimingTags& get_hold_data_tags(const NodeId node_id) const { return ops_.get_data_tags(node_id); }
        const TimingTags& get_hold_clock_tags(const NodeId node_id) const { return ops_.get_clock_tags(node_id); }
};
