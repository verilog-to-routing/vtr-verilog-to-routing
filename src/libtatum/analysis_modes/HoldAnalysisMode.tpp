template<class Base>
void HoldAnalysisMode<Base>::initialize_traversal(const TimingGraph& tg) {
    Base::initialize_traversal(tg);

    hold_data_tags_ = std::vector<TimingTags>(tg.num_nodes());
    hold_clock_tags_ = std::vector<TimingTags>(tg.num_nodes());
}

template<class Base>
template<class TagPoolType>
void HoldAnalysisMode<Base>::pre_traverse_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    Base::pre_traverse_node(tag_pool, tg, tc, node_id);

    //Primary Input
    ASSERT(tg.num_node_in_edges(node_id) == 0);

    TN_Type node_type = tg.node_type(node_id);

    //Note that we assume that edge counting has set the effective period constraint assuming a
    //launch edge at time zero.
    if(node_type == TN_Type::CONSTANT_GEN_SOURCE) {
        //Pass, we don't propagate any tags from constant generators,
        //since they do not effect they dynamic timing behaviour of the
        //system

    } else if(node_type == TN_Type::CLOCK_SOURCE) {
        ASSERT_MSG(hold_clock_tags_[node_id].num_tags() == 0, "Clock source already has clock tags");
        hold_clock_tags_[node_id].add_tag(tag_pool,
                TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id));

    } else {
        ASSERT(node_type == TN_Type::INPAD_SOURCE);

        //A standard primary input

        //VPR applies input delays to the arc from INPAD_SOURCE to INPAD_OPIN
        //so we do not need to account for it directly in the arrival time of INPAD_SOURCES

        //Figure out if we are an input which defines a clock
        if(tg.node_is_clock_source(node_id)) {
            ASSERT_MSG(hold_clock_tags_[node_id].num_tags() == 0, "Primary input already has clock tags");
            hold_clock_tags_[node_id].add_tag(tag_pool,
                    TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id));
        } else {
            ASSERT_MSG(hold_clock_tags_[node_id].num_tags() == 0, "Primary input already has data tags");
            hold_data_tags_[node_id].add_tag(tag_pool,
                    TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id));

        }
    }
}

template<class Base>
template<class TagPoolType, class DelayCalcType>
void HoldAnalysisMode<Base>::forward_traverse_edge(TagPoolType& tag_pool, const TimingGraph& tg, const DelayCalcType& dc, const NodeId node_id, const EdgeId edge_id) {
    Base::forward_traverse_edge(tag_pool, tg, dc, node_id, edge_id);

    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = hold_data_tags_[node_id];
    TimingTags& node_clock_tags = hold_clock_tags_[node_id];

    //Pulling values from upstream source node
    NodeId src_node_id = tg.edge_src_node(edge_id);

    const Time& edge_delay = dc.min_edge_delay(tg, edge_id);

    /*
     * Clock tags
     */
    if(tg.node_type(src_node_id) != TN_Type::FF_SOURCE) {
        //Do not propagate clock tags from an FF Source,
        //the clock arrival there will have already been converted to a
        //data tag
        const TimingTags& src_clk_tags = hold_clock_tags_[src_node_id];
        for(const TimingTag& src_clk_tag : src_clk_tags) {
            //Standard propagation through the clock network
            node_clock_tags.min_arr(tag_pool, src_clk_tag.arr_time() + edge_delay, src_clk_tag);

            if(tg.node_type(node_id) == TN_Type::FF_SOURCE) {
                //This is a clock to data launch edge
                //Mark the data arrival (launch) time at this FF

                //FF Launch edge, begin data propagation
                //
                //We convert the clock arrival time to a
                //data arrival time at this node (since the clock arrival
                //launches the data)
                TimingTag launch_tag = src_clk_tag;
                launch_tag.set_launch_node(src_node_id);
                ASSERT(launch_tag.next() == nullptr);

                //Mark propagated launch time as a DATA tag
                node_data_tags.min_arr(tag_pool, launch_tag.arr_time() + edge_delay, launch_tag);
            }
        }
    }

    /*
     * Data tags
     */

    const TimingTags& src_data_tags = hold_data_tags_[src_node_id];

    for(const TimingTag& src_data_tag : src_data_tags) {
        //Standard data-path propagation
        node_data_tags.min_arr(tag_pool, src_data_tag.arr_time() + edge_delay, src_data_tag);
    }
}

template<class Base>
template<class TagPoolType>
void HoldAnalysisMode<Base>::forward_traverse_finalize_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    Base::forward_traverse_finalize_node(tag_pool, tg, tc, node_id);

    TimingTags& node_data_tags = hold_data_tags_[node_id];
    TimingTags& node_clock_tags = hold_clock_tags_[node_id];
    /*
     * Calculate required times
     */
    if(tg.node_type(node_id) == TN_Type::OUTPAD_SINK) {
        //Determine the required time for outputs
        DomainId node_domain = tg.node_clock_domain(node_id);
        for(const TimingTag& data_tag : node_data_tags) {
            //Should we be analyzing paths between these two domains?
            if(tc.should_analyze(data_tag.clock_domain(), node_domain)) {
                //Only some clock domain paths should be analyzed

                float clock_constraint = tc.hold_clock_constraint(data_tag.clock_domain(), node_domain);

                //The output delay is assumed to be on the edge from the OUTPAD_IPIN to OUTPAD_SINK
                //so we do not need to account for it here
                node_data_tags.max_req(tag_pool, Time(clock_constraint), data_tag);
            }
        }
    } else if (tg.node_type(node_id) == TN_Type::FF_SINK) {
        //Determine the required time at this FF
        //
        //We need to generate a required time for each clock domain for which there is a data
        //arrival time at this node, while considering all possible clocks that could drive
        //this node (i.e. take the most restrictive constraint accross all clock tags at this
        //node)

        for(TimingTag& node_data_tag : node_data_tags) {
            for(const TimingTag& node_clock_tag : node_clock_tags) {
                //Should we be analyzing paths between these two domains?
                if(tc.should_analyze(node_data_tag.clock_domain(), node_clock_tag.clock_domain())) {

                    //We only set a required time if the source domain actually reaches this sink
                    //domain.  This is indicated by having a valid arrival time.
                    if(node_data_tag.arr_time().valid()) {
                        float clock_constraint = tc.hold_clock_constraint(node_data_tag.clock_domain(),
                                                                          node_clock_tag.clock_domain());

                        node_data_tag.max_req(node_clock_tag.arr_time() + Time(clock_constraint), node_data_tag);
                    }
                }
            }
        }
    }
}

template<class Base>
template<class DelayCalcType>
void HoldAnalysisMode<Base>::backward_traverse_edge(const TimingGraph& tg, const DelayCalcType& dc, const NodeId node_id, const EdgeId edge_id) {
    Base::backward_traverse_edge(tg, dc, node_id, edge_id);

    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = hold_data_tags_[node_id];

    //Pulling values from downstream sink node
    int sink_node_id = tg.edge_sink_node(edge_id);

    const Time& edge_delay = dc.min_edge_delay(tg, edge_id);

    const TimingTags& sink_data_tags = hold_data_tags_[sink_node_id];

    for(const TimingTag& sink_tag : sink_data_tags) {
        //We only take the min if we have a valid arrival time
        TimingTagIterator matched_tag_iter = node_data_tags.find_tag_by_clock_domain(sink_tag.clock_domain());
        if(matched_tag_iter != node_data_tags.end() && matched_tag_iter->arr_time().valid()) {
            matched_tag_iter->max_req(sink_tag.req_time() - edge_delay, sink_tag);
        }
    }
}
