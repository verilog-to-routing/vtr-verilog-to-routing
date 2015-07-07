#pragma once
#include "assert.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingTags.hpp"
#include "BaseAnalysisMode.hpp"

/*
 * The 'SetupAnalysisMode' class defines the operators needed by a timing analyzer class
 * to perform a setup (max/longest path) analysis.
 *
 * Setup Analysis Principles
 * ==========================
 * To operate correctly data arriving at a Flip-Flop (FF) must arrive (i.e. be stable) some
 * amount of time BEFORE the capturing clock edge.  This time is referred to as the
 * 'Setup Time' of the Flip-Flop.  If the data arrives during the setup window
 * (i.e. less than t_s before the capturing clock edge) then the FF may go meta-stable
 * failing to capture the data. This will put the circuit in an invalid state (this is bad).
 *
 * More formally, for correct operation at every cycle we require the following to be satisfied
 * for every path in the circuit:
 *
 *      t_{clock}^{(launch)} + t_{cq}^{(max)} + t_{comb}^{(max)} \leq t_{clock}^{(capture)} - t_s   (1)
 *
 * where t_{clock}^{(launch)} is the clock arrival time at the upstream FF, t_{cq}^{(max)} is the
 * maximum clock-to-q delay of the upstream FF, and t_{comb}^{(max)} is the maximum combinational
 * path delay from the upstream to downstream FFs, t_s is the setup constraint of the downstream
 * FF, and t_{clock}^{(capture)} is the clock arrival time at the downstream FF.
 *
 * Typically t_{clock}^{(launch)} and t_{clock}^{(capture)} have a periodic relationship. To ensure
 * a non-optimistic analysis we need to consider the minimum possible time difference between
 * t_{clock}^{(capture)} and t_{clock}^{(launch)}.  In the case where the launch and capture clocks
 * are the same this 'constraint (T_{cstr})' value is simply the clock period (T_{clk}); however, in multi-clock
 * scenarios the closest alignment of clock edges is used, which may be smaller than the clock period
 * of either the launch or capture clock (depending on their period and phase relationship). It is
 * typically assumed that the launch clock arrives at time zero (even if this is not strictly true
 * in an absolute sense, such as if the clock has a rise time > 0, we can achieve this by adjusting
 * the value of T_{cstr}).
 *
 * Additionally, the arrival times of the launch and capture edges are unlikely to be perfectly
 * aligned in practise, due to clock skew.
 *
 * Formally, we can re-write our condition for correct operation as:
 *
 *      t_{clk_insrt}^{(launch)} + t_{cq}^{(max)} + t_{comb}^{(max)} + t_s \leq + t_{clk_insrt}^{(capture)} + T_{cstr}    (2)
 *
 * where t_{clk_insrt}^{(launch)} and t_{clk_insrt}^{(capture)} represent the clock insertion delays
 * to the launch/capture nodes, and T_{cstr} the ideal constraint (excluding skew).
 *
 * We refer to the left hand side of (2) as the 'arrival time' (when the data actually arrives at a FF capture node),
 * and the right hand side as the 'required time' (when the data is required to arrive for correct operation), so
 * (2) becomes:
 *      t_{arr}^{(max)} \leq t_{req}^{(min)} (3)
 *
 * Setup Analysis Implementation
 * ===============================
 * When we perform setup analysis we follow the formulation of (2), by performing two key operations: traversing
 * the clock network, and traversing the data paths.
 *
 * Clock Propogation
 * -------------------
 * We traverse the clock network to determine the clock delays (t_{clk_arr}^{(launch)}, t_{clk_arr}^{(capture)})
 * at each FF clock pin (FF_CLOCK node in the timing graph).  Clock related delay information is stored and
 * propogated as sets of 'clock tags'.
 *
 * Data Propogation
 * ------------------
 * We traverse the data paths in the circuit to determine t_{arr}^{(max)} in (2).
 * In particular, at each node in the circuit we track the maximum arrival time to it as a set
 * of 'data_tags'.
 *
 * The timing graph uses separte nodes to represent FF Pins (FF_IPIN, FF_OPIN) and FF Sources/Sinks
 * (FF_SOURCE/FF_SINK). As a result t_{cq} delays are actually placed on the edges between FF_SOURCEs
 * and FF_OPINs, t_s values are similarily placed as edge delays between FF_IPINs and FF_SINKs.
 *
 * The data launch nodes (e.g. FF_SOURCES) have their, arrival times initialized to the clock insertion
 * delay (t_{clk_insrt}^{(launch)}). Then at each downstream node we store the maximum of the upstream
 * arrival time plus the incoming edge delay as the arrival time at each node.  As a result the final
 * arrival time at a capture node (e.g. FF_SINK) is the maximum arival time.
 *
 *
 * The required times at sink nodes (POs, e.g. FF_SINK) can be calculated directly after clock propogation,
 * since the value of T_{cstr} is determined ahead of time.
 *
 * To facilitate the calculation of slack at each node we also propogate required times back through
 * the timing graph.  This follows a similar procedure to arrival propogation but happens in reverse
 * order (from POs to PIs), with each node taking the minumum of the downstream required time minus
 * the edge delay.
 *
 * Combined Clock & Data Propogation
 * -----------------------------------
 * In practice the clock and data propogation, although sometimes logically useful to think of as separate,
 * are combined into a single traversal for efficiency (minimizing graph walks).  This is enabled by
 * building the timing graph with edges FF_CLOCK and FF_SINK/FF_SOUCE nodes.  On the forward traversal
 * we propogate clock tags from known clock sources, which are converted to data tags (with appropriate
 * arrival time) at FF_SOURCE node, and data tags (with appropriate required time) at FF_SINK nodes.
 *
 */
template<class Base = BaseAnalysisMode>
class SetupAnalysisMode : public Base {
    public:
        //External tag access
        const TimingTags& setup_data_tags(NodeId node_id) const { return setup_data_tags_[node_id]; }
        const TimingTags& setup_clock_tags(NodeId node_id) const { return setup_clock_tags_[node_id]; }
    protected:
        //Internal operations for performing setup analysis to satisfy the BaseAnalysisMode interface
        void initialize_traversal(const TimingGraph& tg);

        template<class TagPoolType>
        void pre_traverse_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class TagPoolType, class DelayCalc>
        void forward_traverse_edge(TagPoolType& tag_pool, const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class TagPoolType>
        void forward_traverse_finalize_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void backward_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        //Tag data storage
        std::vector<TimingTags> setup_data_tags_; //Data tags for each node [0..timing_graph.num_nodes()-1]
        std::vector<TimingTags> setup_clock_tags_; //Clock tags for each node [0..timing_graph.num_nodes()-1]
};



//Implementation
#include "SetupAnalysisMode.tpp"
