#pragma once
#include <chrono>
#include <string>

#include "TimingAnalyzer.hpp"
#include "memory_pool.hpp"

/**
 * Overview
 * ==========
 * SerialTimingAnalyzer implements the TimingAnalyzer interface, providing a standard
 * serial (single-threaded) timing analyzer.
 *
 * \warning Any tags retrieved by reference from the analyzer will no longer be valid once
 *       it has been destroyed!
 *
 * Most parallel analyzers are derived from this class and simply re-define the
 * pre/forward/backward traversal functions as appropriate.
 *
 * Implementation
 * ================
 * This analyzer performs timing analysis by performing a levelized walk of the timing graph.
 * By performing the traversal in this manner graph, we know that the current level's
 * dependancies have already been met.
 *
 * The traversals are performed by calculate_timing() which uses:
 *   pre_traversal(): to initialize arrival times on the PIs
 *   forward_traversal(): to perform the forward traversal propogating arrival times from
 *                        PIs to POs, and sets required times on POs.
 *   backward_traversal(): to perform the back traversal propogating required times from POs to PIs
 *
 * The work done at each node is encapsolated in the forward_traverse_node() and
 * backward_traverse_node() functions.  The actual work performed will vary depending upon
 * which mix-in class is specified as the AnalysisType template parameter.
 *
 * Thread-saftey
 * ==============
 * NOTE: forward_traverse_node() and backward_traverse_node() should be thread-safe,
 *       since they are often re-used by sub-classes with no modification.
 *
 *       In particular, take note that they only modify the current node they are processing;
 *       they only read from their upstream nodes (this can as each node 'pulling' the results
 *       of its dependancies. . This allows us to avoid using locks in any parallel analyzers
 *       provided we process a node in only a single thread!
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
        ///The type of the pooled memory allocator
        typedef TagPoolType tag_pool_type;

        ///Initializes the analyzer
        /// \param timing_graph The timing graph to analyze
        /// \param timing_constraints The timing constraints to apply during analysis
        /// \param delay_calculator The delay calculator to use to determine edge delays
        SerialTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator);
        void calculate_timing() override;
        void reset_timing() override;
        const DelayCalcType& delay_calculator() override { return dc_; }
        std::map<std::string, double> profiling_data() override { return perf_data_; }
    protected:
        ///Setups the timing graph in preparation for main traversals.
        /// Initializes arrival times on primary inputs
        virtual void pre_traversal();

        ///Propagate arrival times, set required times on primary outputs
        virtual void forward_traversal();

        ///Propagate required times from primary outputs to primary inputs
        virtual void backward_traversal();

        ///Per node worker function for forward traversal
        /// \param tag_pool The tag pool used to allocate new TimingTag objects
        /// \param node_id The node to process
        void forward_traverse_node(TagPoolType& tag_pool, const NodeId node_id);

        ///Per node worker function for backward traversal
        /// \param tag_pool The tag pool used to allocate new TimingTag objects
        /// \param node_id The node to process
        void backward_traverse_node(TagPoolType& tag_pool, const NodeId node_id);

        /*
         * Data
         */
        const TimingGraph& tg_; //The timing graph to analyzer
        const TimingConstraints& tc_; //The timing constraints to evaluate
        const DelayCalcType& dc_; //The delay calculator to use
        TagPoolType tag_pool_; //Memory pool for allocating tags

        std::map<std::string, double> perf_data_; //Performance profiling info, assumes each data point has a unique string identifier
};

//Implementation
#include "SerialTimingAnalyzer.tpp"

