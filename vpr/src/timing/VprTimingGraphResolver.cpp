#include "VprTimingGraphResolver.h"
#include "atom_netlist.h"
#include "atom_lookup.h"

VprTimingGraphResolver::VprTimingGraphResolver(const AtomNetlist& netlist, const AtomLookup& netlist_lookup, const tatum::TimingGraph& timing_graph, const AnalysisDelayCalculator& delay_calc)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup)
    , timing_graph_(timing_graph)
    , delay_calc_(delay_calc) {}

std::string VprTimingGraphResolver::node_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);

    return netlist_.pin_name(pin);
}

std::string VprTimingGraphResolver::node_type_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    AtomBlockId blk = netlist_.pin_block(pin);

    std::string name = netlist_.block_model(blk)->name;

    if (detail_level() == e_timing_report_detail::AGGREGATED || detail_level() == e_timing_report_detail::DETAILED_ROUTING) {
        //Detailed report consist of the aggregated reported with a breakdown of inter-block routing
        //Annotate primitive grid location, if known
        auto& atom_ctx = g_vpr_ctx.atom();
        auto& place_ctx = g_vpr_ctx.placement();
        ClusterBlockId cb = atom_ctx.lookup.atom_clb(blk);
        if (cb && place_ctx.block_locs.count(cb)) {
            int x = place_ctx.block_locs[cb].loc.x;
            int y = place_ctx.block_locs[cb].loc.y;
            name += " at (" + std::to_string(x) + "," + std::to_string(y) + ")";
        }
    }

    return name;
}

tatum::EdgeDelayBreakdown VprTimingGraphResolver::edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType tatum_delay_type) const {
    tatum::EdgeDelayBreakdown delay_breakdown;

    if (edge && (detail_level() == e_timing_report_detail::AGGREGATED || detail_level() == e_timing_report_detail::DETAILED_ROUTING)) {
        auto edge_type = timing_graph_.edge_type(edge);

        DelayType delay_type; //TODO: should unify vpr/tatum DelayType
        if (tatum_delay_type == tatum::DelayType::MAX) {
            delay_type = DelayType::MAX;
        } else {
            VTR_ASSERT(tatum_delay_type == tatum::DelayType::MIN);
            delay_type = DelayType::MIN;
        }

        if (edge_type == tatum::EdgeType::INTERCONNECT) {
            delay_breakdown.components = interconnect_delay_breakdown(edge, delay_type);
        } else {
            //Primtiive edge
            //
            tatum::DelayComponent component;

            tatum::NodeId node = timing_graph_.edge_sink_node(edge);

            AtomPinId atom_pin = netlist_lookup_.tnode_atom_pin(node);
            AtomBlockId atom_blk = netlist_.pin_block(atom_pin);

            //component.inst_name = netlist_.block_name(atom_blk);

            component.type_name = "primitive '";
            component.type_name += netlist_.block_model(atom_blk)->name;
            component.type_name += "'";

            if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
                component.type_name += " combinational delay";

                if (delay_type == DelayType::MAX) {
                    component.delay = delay_calc_.max_edge_delay(timing_graph_, edge);
                } else {
                    VTR_ASSERT(delay_type == DelayType::MIN);
                    component.delay = delay_calc_.min_edge_delay(timing_graph_, edge);
                }
            } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                if (delay_type == DelayType::MAX) {
                    component.type_name += " Tcq_max";
                    component.delay = delay_calc_.max_edge_delay(timing_graph_, edge);
                } else {
                    VTR_ASSERT(delay_type == DelayType::MIN);
                    component.type_name += " Tcq_min";
                    component.delay = delay_calc_.min_edge_delay(timing_graph_, edge);
                }

            } else {
                VTR_ASSERT(edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE);

                if (delay_type == DelayType::MAX) {
                    component.type_name += " Tsu";
                    component.delay = delay_calc_.setup_time(timing_graph_, edge);
                } else {
                    component.type_name += " Thld";
                    component.delay = delay_calc_.hold_time(timing_graph_, edge);
                }
            }

            delay_breakdown.components.push_back(component);
        }
    }

    return delay_breakdown;
}

std::vector<tatum::DelayComponent> VprTimingGraphResolver::interconnect_delay_breakdown(tatum::EdgeId edge, DelayType delay_type) const {
    VTR_ASSERT(timing_graph_.edge_type(edge) == tatum::EdgeType::INTERCONNECT);
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    std::vector<tatum::DelayComponent> components;

    //We assume that the delay calculator has already cached all of the relevant delays,
    //we just retrieve the cached values. This assumption greatly simplifies the calculation
    //process and avoids us duplicating the complex delay calculation logic from the delay
    //calcualtor.
    //
    //However note that this does couple this code tightly with the delay calculator implementation.

    //Force delay calculation to ensure results are cached (redundant if already up-to-date)
    delay_calc_.atom_net_delay(timing_graph_, edge, delay_type);

    ClusterPinId src_pin = ClusterPinId::INVALID();
    ClusterPinId sink_pin = ClusterPinId::INVALID();

    std::tie(src_pin, sink_pin) = delay_calc_.get_cached_pins(edge, delay_type);

    if (!src_pin && !sink_pin) {
        //Cluster internal
        tatum::NodeId node = timing_graph_.edge_sink_node(edge);

        AtomPinId atom_pin = netlist_lookup_.tnode_atom_pin(node);
        AtomBlockId atom_blk = netlist_.pin_block(atom_pin);

        ClusterBlockId clb_blk = netlist_lookup_.atom_clb(atom_blk);

        tatum::DelayComponent internal_component;
        //internal_component.inst_name = cluster_ctx.clb_nlist.block_name(clb_blk);
        internal_component.type_name = "intra '";
        internal_component.type_name += cluster_ctx.clb_nlist.block_type(clb_blk)->name;
        internal_component.type_name += "' routing";
        internal_component.delay = delay_calc_.get_cached_delay(edge, delay_type);
        components.push_back(internal_component);

    } else {
        //Cluster external
        VTR_ASSERT(src_pin);
        VTR_ASSERT(sink_pin);

        ClusterBlockId src_blk = cluster_ctx.clb_nlist.pin_block(src_pin);
        ClusterBlockId sink_blk = cluster_ctx.clb_nlist.pin_block(sink_pin);

        tatum::Time driver_clb_delay = delay_calc_.get_driver_clb_cached_delay(edge, delay_type);
        tatum::Time sink_clb_delay = delay_calc_.get_sink_clb_cached_delay(edge, delay_type);

        ClusterNetId src_net = cluster_ctx.clb_nlist.pin_net(src_pin);
        VTR_ASSERT(src_net == cluster_ctx.clb_nlist.pin_net(sink_pin));
        tatum::Time net_delay = tatum::Time(delay_calc_.inter_cluster_delay(src_net,
                                                                            0,
                                                                            cluster_ctx.clb_nlist.pin_net_index(sink_pin)));

        VTR_ASSERT(driver_clb_delay.valid());
        VTR_ASSERT(net_delay.valid());
        VTR_ASSERT(sink_clb_delay.valid());

        tatum::DelayComponent driver_component;
        //driver_component.inst_name = cluster_ctx.clb_nlist.block_name(src_blk);
        driver_component.type_name = "intra '";
        driver_component.type_name += cluster_ctx.clb_nlist.block_type(src_blk)->name;
        driver_component.type_name += "' routing";
        driver_component.delay = driver_clb_delay;
        components.push_back(driver_component);

        // For detailed timing report, we would like to breakdown the inter-block routing into
        // its constituent net components. As a pre-requisite, we would like to ensure these conditions:
        // 1. detailed timing report is selected
        // 2. the vector of tracebacks are not empty (important because this module is also called during
        // placement, which occurs before routing and implies and empty vector of tracebacks).
        // 3. the traceback is not a nullptr, for global routing, there are null tracebacks representing
        // unrouted nets.

        tatum::DelayComponent interblock_component;
        interblock_component.type_name = "inter-block routing";
        interblock_component.delay = net_delay;

        if (detail_level() == e_timing_report_detail::DETAILED_ROUTING && !route_ctx.trace.empty()) {
            //check if detailed timing report has been selected and that the vector of tracebacks
            //is not empty.
            if (route_ctx.trace[src_net].head != nullptr) {
                //the traceback is not a nullptr, so we find the path of the net from source to sink.
                //Note that the previously declared interblock_component will not be added to the
                //vector of net components.
                get_detailed_interconnect_components(components, src_net, sink_pin);
            } else {
                //the traceback is a nullptr which means this is an unrouted net as part of global routing.
                //We add the tag global net, and push the interblock into the vector of net components.
                interblock_component.type_name += ":global net";
                components.push_back(interblock_component);
            }
        } else {
            //for aggregated and netlist modes, we simply add the previously declared interblock_component
            //to the vector.
            components.push_back(interblock_component);
        }
        tatum::DelayComponent sink_component;
        //sink_component.inst_name = cluster_ctx.clb_nlist.block_name(sink_blk);
        sink_component.type_name = "intra '";
        sink_component.type_name += cluster_ctx.clb_nlist.block_type(sink_blk)->name;
        sink_component.type_name += "' routing";
        sink_component.delay = sink_clb_delay;
        components.push_back(sink_component);
    }
    return components;
}

e_timing_report_detail VprTimingGraphResolver::detail_level() const {
    return detail_level_;
}

void VprTimingGraphResolver::set_detail_level(e_timing_report_detail report_detail) {
    detail_level_ = report_detail;
}

void VprTimingGraphResolver::get_detailed_interconnect_components(std::vector<tatum::DelayComponent>& components, ClusterNetId net_id, ClusterPinId sink_pin) const {
    /* This routine obtains the interconnect components such as: OPIN, CHANX, CHANY, IPIN which join 
     * two intra-block clusters in two parts. In part one, we construct the route tree 
     * from the traceback and computes its value for R, C, and Tdel. Next, we find the pointer to
     * the route tree sink which corresponds to the sink_pin. In part two, we call the helper function,
     * which walks the route tree from the sink to the source. Along the way, we process each node 
     * and construct net_components that are added to the vector of components. */

    t_rt_node* rt_root = traceback_to_route_tree(net_id);              //obtain the route tree from the traceback
    load_new_subtree_R_upstream(rt_root);                              //load in the resistance values for the route
    load_new_subtree_C_downstream(rt_root);                            //load in the capacitance values for the route tree
    load_route_tree_Tdel(rt_root, 0.);                                 //load the time delay values for the route tree
    t_rt_node* rt_sink = find_sink_rt_node(rt_root, net_id, sink_pin); //find the sink matching sink_pin
    get_detailed_interconnect_components_helper(components, rt_sink);  //from sink, walk up to source and add net components
    free_route_tree(rt_root);
}

void VprTimingGraphResolver::get_detailed_interconnect_components_helper(std::vector<tatum::DelayComponent>& components, t_rt_node* node) const {
    /* This routine comprieses the second part of obtaining the interconnect components.
     * We begin at the sink node and travel up the tree towards the source. For each node, we would 
     * like to retreive information such as the routing resource type, index, and incremental delay.
     * If the type is a channel, then we retrieve the name of the segment as well as the coordinates
     * of its start and endpoint. All of this information is stored in the object DelayComponent,
     * which belong to the vector "components".
     */

    auto& device_ctx = g_vpr_ctx.device();

    //Declare a separate vector "interconnect_components" to hold the interconnect components. We need
    //this because we walk from the sink to the source, we will need to add elements to the front of
    //interconnect_components so that we maintain the correct order of net components.
    //Illustration:
    //    INTRA
    //      |
    //     IPIN <-end
    //      |
    //    CHANX
    //      |
    //     OPIN <-start
    //      |
    //    INTRA - not seen yet
    //Pushing a stack steps:
    //1. OPIN, 2. CHANX, OPIN 3. IPIN, CHANX, OPIN (order is preserved)
    //At this point of the code, the vector "components" contains intrablock components.
    //At the end of the module, we will append "interconnect_components" to the end of vector "components".

    std::vector<tatum::DelayComponent> interconnect_components;

    std::string start_x; //start x-coordinate
    std::string start_y; //start y-coordinate
    std::string end_x;   //end x-coordinate
    std::string end_y;   //end y-coordinate
    std::string arrow;   //direction arrow

    while (node != nullptr) {
        //Process the current interconnect component if it is of type OPIN, CHANX, CHANY, IPIN
        if (device_ctx.rr_nodes[node->inode].type() == OPIN
            || device_ctx.rr_nodes[node->inode].type() == IPIN
            || device_ctx.rr_nodes[node->inode].type() == CHANX
            || device_ctx.rr_nodes[node->inode].type() == CHANY) {
            tatum::DelayComponent net_component; //declare a new instance of DelayComponent

            net_component.type_name = device_ctx.rr_nodes[node->inode].type_string(); //write the component's type as a routing resource node
            net_component.type_name += ":" + std::to_string(node->inode) + " ";       //add the index of the routing resource node
            if (device_ctx.rr_nodes[node->inode].type() == OPIN || device_ctx.rr_nodes[node->inode].type() == IPIN) {
                net_component.type_name += "side:";                                        //add the side of the routing resource node
                net_component.type_name += device_ctx.rr_nodes[node->inode].side_string(); //add the side of the routing resource node
                // For OPINs and IPINs the starting and ending coordinate are identical, so we can just arbitrarily assign the start to larger values
                // and the end to the lower coordinate
                start_x = " (" + std::to_string(device_ctx.rr_nodes[node->inode].xhigh()) + ","; //start and end coordinates are the same for OPINs and IPINs
                start_y = std::to_string(device_ctx.rr_nodes[node->inode].yhigh()) + ")";
                end_x = "";
                end_y = "";
                arrow = "";
            }
            if (device_ctx.rr_nodes[node->inode].type() == CHANX || device_ctx.rr_nodes[node->inode].type() == CHANY) {                                         //for channels, we would like to describe the component with segment specific information
                net_component.type_name += device_ctx.arch->Segments[device_ctx.rr_indexed_data[device_ctx.rr_nodes[node->inode].cost_index()].seg_index].name; //Write the segment name
                net_component.type_name += " length:" + std::to_string(device_ctx.rr_nodes[node->inode].length());                                              //add the length of the segment
                //Figure out the starting and ending coordinate of the segment depending on the direction

                arrow = "->"; //we will point the coordinates from start to finish, left to right

                if (device_ctx.rr_nodes[node->inode].direction() == DEC_DIRECTION) {                 //signal travels along decreasing direction
                    start_x = " (" + std::to_string(device_ctx.rr_nodes[node->inode].xhigh()) + ","; //start coordinates have large value
                    start_y = std::to_string(device_ctx.rr_nodes[node->inode].yhigh()) + ")";
                    end_x = "(" + std::to_string(device_ctx.rr_nodes[node->inode].xlow()) + ","; //end coordinates have smaller value
                    end_y = std::to_string(device_ctx.rr_nodes[node->inode].ylow()) + ")";
                }

                else {                                                                              // signal travels in increasing direction, stays at same point, or can travel both directions
                    start_x = " (" + std::to_string(device_ctx.rr_nodes[node->inode].xlow()) + ","; //start coordinates have smaller value
                    start_y = std::to_string(device_ctx.rr_nodes[node->inode].ylow()) + ")";
                    end_x = "(" + std::to_string(device_ctx.rr_nodes[node->inode].xhigh()) + ","; //end coordinates have larger value
                    end_y = std::to_string(device_ctx.rr_nodes[node->inode].yhigh()) + ")";
                    if (device_ctx.rr_nodes[node->inode].direction() == BI_DIRECTION) {
                        arrow = "<->"; //indicate that signal can travel both direction
                    }
                }
            }

            net_component.type_name += start_x + start_y;                            //Write the starting coordinates
            net_component.type_name += arrow;                                        //Indicate the direction
            net_component.type_name += end_x + end_y;                                //Write the end coordiates
            net_component.delay = tatum::Time(node->Tdel - node->parent_node->Tdel); // add the incremental delay
            interconnect_components.push_back(net_component);                        //insert net_component into the front of vector interconnect_component
        }
        node = node->parent_node; //travel up the tree through the parent
    }
    components.insert(components.end(), interconnect_components.rbegin(), interconnect_components.rend()); //append the completed "interconnect_component" to "component_vector"
}
