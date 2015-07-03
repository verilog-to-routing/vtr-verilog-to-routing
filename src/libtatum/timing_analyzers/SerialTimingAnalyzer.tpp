template<class AnalysisType, class DelayCalcType, class TagPoolType>
SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::SerialTimingAnalyzer(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalcType& dc)
    : tg_(tg)
    , tc_(tc)
    , dc_(dc)
    //Need to give the size of the object to allocate
    , tag_pool_(sizeof(TimingTag)) {
    AnalysisType::initialize_traversal(tg_);
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::calculate_timing() {
    using namespace std::chrono;

    auto analysis_start = high_resolution_clock::now();

    auto pre_traversal_start = high_resolution_clock::now();
    pre_traversal(tg_, tc_);
    auto pre_traversal_end = high_resolution_clock::now();

    auto fwd_traversal_start = high_resolution_clock::now();
    forward_traversal(tg_, tc_);
    auto fwd_traversal_end = high_resolution_clock::now();

    auto bck_traversal_start = high_resolution_clock::now();
    backward_traversal(tg_);
    auto bck_traversal_end = high_resolution_clock::now();

    auto analysis_end = high_resolution_clock::now();

    //Convert time points to durations and store
    perf_data_["analysis"] = duration_cast<duration<double>>(analysis_end - analysis_start).count();
    perf_data_["pre_traversal"] = duration_cast<duration<double>>(pre_traversal_end - pre_traversal_start).count();
    perf_data_["fwd_traversal"] = duration_cast<duration<double>>(fwd_traversal_end - fwd_traversal_start).count();
    perf_data_["bck_traversal"] = duration_cast<duration<double>>(bck_traversal_end - bck_traversal_start).count();
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::reset_timing() {
    //Release the memory allocated to tags
    tag_pool_.purge_memory();

    AnalysisType::initialize_traversal(tg_);
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     */
    for(NodeId node_id : timing_graph.primary_inputs()) {
        AnalysisType::pre_traverse_node(tag_pool_, timing_graph, timing_constraints, node_id);
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    using namespace std::chrono;

    //Forward traversal (arrival times)
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
        auto fwd_level_start = high_resolution_clock::now();

        for(NodeId node_id : timing_graph.level(level_idx)) {
            forward_traverse_node(tag_pool_, timing_graph, timing_constraints, node_id);
        }

        auto fwd_level_end = high_resolution_clock::now();
        std::string key = std::string("fwd_level_") + std::to_string(level_idx);
        perf_data_[key] = duration_cast<duration<double>>(fwd_level_end - fwd_level_start).count();
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::backward_traversal(const TimingGraph& timing_graph) {
    using namespace std::chrono;

    //Backward traversal (required times)
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        auto bck_level_start = high_resolution_clock::now();

        for(NodeId node_id : timing_graph.level(level_idx)) {
            backward_traverse_node(timing_graph, node_id);
        }

        auto bck_level_end = high_resolution_clock::now();
        std::string key = std::string("bck_level_") + std::to_string(level_idx);
        perf_data_[key] = duration_cast<duration<double>>(bck_level_end - bck_level_start).count();
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::forward_traverse_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Pull from upstream sources to current node
    for(int edge_idx = 0; edge_idx < tg.num_node_in_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_in_edge(node_id, edge_idx);

        AnalysisType::forward_traverse_edge(tag_pool, tg, dc_, node_id, edge_id);
    }

    AnalysisType::forward_traverse_finalize_node(tag_pool, tg, tc, node_id);
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>::backward_traverse_node(const TimingGraph& tg, const NodeId node_id) {
    //Pull from downstream sinks to current node

    TN_Type node_type = tg.node_type(node_id);

    //We don't propagate required times past FF_CLOCK nodes,
    //since anything upstream is part of the clock network
    //
    //TODO: if performing optimization on a clock network this may actually be useful
    if(node_type == TN_Type::FF_CLOCK) {
        return;
    }

    //Each back-edge from down stream node
    for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

        AnalysisType::backward_traverse_edge(tg, dc_, node_id, edge_id);
    }
}

