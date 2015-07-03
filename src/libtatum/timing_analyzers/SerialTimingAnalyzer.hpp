#pragma once
#include <chrono>
#include <string>

#include "TimingAnalyzer.hpp"
#include "memory_pool.hpp"

/* Overview
 * ==========
 * SerialTimingAnalyzer implements the TimingAnalyzer interface, providing a standard
 * serial (single-threaded) timing analyzer.
 *
 * NOTE: Any tags retrieved by reference from the analyzer will no longer be valid once
 *       it has been destroyed!
 *
 * Most parallel analyzers are derived from this class and simply re-define the
 * pre/forward/backward traversal functions as appropriate.
 *
 * Implementation
 * ================
 * This analyzer performs timing analysis by performing a levelized walk of the timing graph.
 * By performing the traversal in a levelized manner graph, we know when we process a level
 * that the dependancies for every node in the current level have been met.
 *
 * The traversals are performed by calculate_timing() which uses:
 *   pre_traversal(): to initialize arrival times on the PIs
 *   forward_traversal(): to perform the forward traversal propogating arrival times from
 *                        PIs to POs, and sets required times on POs.
 *   backward_traversal(): to perform the back traversal propogating required times from POs to PIs
 *
 * The work done at each node are encapsolated in the forward_traverse_node() and
 * backward_traverse_node() functions.  This work will vary depending upon
 * which mix-in class is specified as the AnalysisType template parameter.
 *
 * Thread-saftey
 * ==============
 * NOTE: forward_traverse_node() and backward_traverse_node() should be thread-safe,
 *       since they are often re-used by sub-classes with no modification.
 *
 *       In particular, take note that they only modify the current node they are processing
 *       (perform only read access on other nodes). This allows us to avoid using locks in
 *       any parallel analyzers if we process a node in a single thread!
 *
 * Memory Allocation
 * ===================
 * To facilitate efficient multi-clock analysis we use Tags (see class TimingTags) to record
 * arrival/required times on a per-clock basis.  For efficiency the current TimingTag
 * implementation uses a memory pool to allocate tags rather than the generic new/delete.
 *
 * This memory pool (specified by the TagPoolType template parameter) is owned
 * by the SerialTimingAnalyzer class which will destroy it is destructed.
 */

template<class AnalysisType, class DelayCalcType, class TagPoolType=MemoryPool>
class SerialTimingAnalyzer : public TimingAnalyzer<AnalysisType, DelayCalcType> {
    public:
        typedef TagPoolType tag_pool_type;

        SerialTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator);
        void calculate_timing() override;
        void reset_timing() override;
        const DelayCalcType& delay_calculator() override { return dc_; }
        std::map<std::string, double> profiling_data() override { return perf_data_; }
    protected:
        /*
         * Setup the timing graph.
         */
        virtual void pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propagate arrival times, set required times on primary outputs
         */
        virtual void forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propagate required times
         */
        virtual void backward_traversal(const TimingGraph& timing_graph);

        //Per node worker functions
        void forward_traverse_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void backward_traverse_node(const TimingGraph& tg, const NodeId node_id);

        /*
         * Data
         */
        const TimingGraph& tg_; //The timing graph to analyzer
        const TimingConstraints& tc_; //The timing constraints to evaluate
        const DelayCalcType& dc_; //The delay calculator used
        TagPoolType tag_pool_; //Memory pool for allocating tags

        std::map<std::string, double> perf_data_; //Performance profiling info
};

//Implementation
#include "SerialTimingAnalyzer.tpp"

