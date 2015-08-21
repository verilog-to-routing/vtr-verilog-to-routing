#pragma once
#include "assert.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingTags.hpp"
#include "BaseAnalysisMode.hpp"
/** \file
 * The 'HoldAnalysisMode' class defines the operators needed by a timing analyzer class
 * to perform a hold (min/shortest path) analysis. It extends the BaseAnalysisMode concept class.
 *
 * \see SetupAnalysisMode
 *
 * Hold Analysis Principles
 * ==========================
 *
 * In addition to satisfying setup constraints, data arriving at a Flip-Flop (FF) must stay (i.e.
 * remain stable) for some amount of time *after* the capturing clock edge arrives. This time is
 * referred to as the 'Hold Time' of the Flip-Flop.  If the data changes during the hold window
 * (i.e. less than \f$ t_h \f$ after the capturing clock edge) then the FF may go meta-stable
 * failing to capture the data. This will put the circuit in an invalid state (this is bad).
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
 * on the previous cycle before it can be captured by the donwstream FF.
 */

/**
 * Hold Analysis Implementation
 * ===============================
 * The hold analysis implementation is generally similar to the implementation used for Setup, except
 * that the minumum arrival times (and maximum required times) are propagated through the timing graph.
 */
template<class BaseAnalysisMode = BaseAnalysisMode>
class HoldAnalysisMode : public BaseAnalysisMode {
    public:
        //External tag access
        const TimingTags& hold_data_tags(NodeId node_id) const { return hold_data_tags_[node_id]; }
        const TimingTags& hold_clock_tags(NodeId node_id) const { return hold_clock_tags_[node_id]; }
    protected:
        //Internal operations for performing hold analysis to satisfy the BaseAnalysisMode interface
        void initialize_traversal(const TimingGraph& tg);

        template<class TagPoolType>
        void pre_traverse_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class TagPoolType, class DelayCalcType>
        void forward_traverse_edge(TagPoolType& tag_pool, const TimingGraph& tg, const DelayCalcType& dc, const NodeId node_id, const EdgeId edge_id);

        template<class TagPoolType>
        void forward_traverse_finalize_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalcType>
        void backward_traverse_edge(const TimingGraph& tg, const DelayCalcType& dc, const NodeId node_id, const EdgeId edge_id);

        //Hold Tag data structure
        std::vector<TimingTags> hold_data_tags_; //Data tags for each node [0..timing_graph.num_nodes()-1]
        std::vector<TimingTags> hold_clock_tags_; //Clock tags for each node [0..timing_graph.num_nodes()-1]
};

//Implementation
#include "HoldAnalysisMode.tpp"
