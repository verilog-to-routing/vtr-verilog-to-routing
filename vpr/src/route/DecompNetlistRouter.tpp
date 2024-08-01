#pragma once

/** @file Impls for DecompNetlistRouter */

#include "DecompNetlistRouter.h"
#include "netlist_routers.h"
#include "route_net.h"
#include "sink_sampling.h"
#include "vtr_dynamic_bitset.h"
#include "vtr_time.h"

template<typename HeapType>
inline RouteIterResults DecompNetlistRouter<HeapType>::route_netlist(int itry, float pres_fac, float worst_neg_slack) {
    /* Reset results for each thread */
    for (auto& results : _results_th) {
        results = RouteIterResults();
    }

    /* Set the routing parameters: they won't change until the next call and that saves us the trouble of passing them around */
    _itry = itry;
    _pres_fac = pres_fac;
    _worst_neg_slack = worst_neg_slack;

    /* Organize netlist into a PartitionTree.
     * Nets in a given level of nodes are guaranteed to not have any overlapping bounding boxes, so they can be routed in parallel. */
    PartitionTree tree(_net_list);

    /* Put the root node on the task queue, which will add its child nodes when it's finished. Wait until the entire tree gets routed. */
    tbb::task_group g;
    route_partition_tree_node(g, tree.root());
    g.wait();

    /* Combine results from threads */
    RouteIterResults out;
    for (auto& results : _results_th) {
        out.stats.combine(results.stats);
        out.rerouted_nets.insert(out.rerouted_nets.end(), results.rerouted_nets.begin(), results.rerouted_nets.end());
        out.is_routable &= results.is_routable;
    }
    return out;
}

template<typename HeapType>
void DecompNetlistRouter<HeapType>::set_rcv_enabled(bool x) {
    if (x)
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Net decomposition with RCV is not implemented yet.\n");
}

template<typename HeapType>
void DecompNetlistRouter<HeapType>::set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info) {
    _timing_info = timing_info;
}

/** Get a sink mask for sinks inside a vnet's clipped BB. */
inline vtr::dynamic_bitset<> get_vnet_sink_mask(const VirtualNet& vnet) {
    auto& route_ctx = g_vpr_ctx.routing();
    size_t num_sinks = route_ctx.route_trees[vnet.net_id]->num_sinks();
    vtr::dynamic_bitset<> out(num_sinks + 1);

    /* 1-indexed! */
    for (size_t isink = 1; isink < num_sinks + 1; isink++) {
        RRNodeId sink_rr = route_ctx.net_rr_terminals[vnet.net_id][isink];
        if (inside_bb(sink_rr, vnet.clipped_bb))
            out.set(isink, true);
    }

    return out;
}

/** Should we decompose this net? */
template<typename HeapType>
bool DecompNetlistRouter<HeapType>::should_decompose_net(ParentNetId net_id, const PartitionTreeNode& node) {
    /* We're at a partition tree leaf: no more nodes to delegate newly created vnets to */
    if (!node.left || !node.right)
        return false;
    /* Clock net */
    if (_net_list.net_is_global(net_id) && _router_opts.two_stage_clock_routing)
        return false;
    /* Decomposition is disabled for net */
    if (_is_decomp_disabled[net_id])
        return false;
    /* We are past the iteration to try decomposition */
    if (_itry > MAX_DECOMP_ITER)
        return false;
    /* Net is too small */
    int num_sinks = _net_list.net_sinks(net_id).size();
    if (num_sinks < MIN_DECOMP_SINKS)
        return false;

    return true;
}

/** Should we decompose this virtual net? (see partition_tree.h) */
inline bool should_decompose_vnet(const VirtualNet& vnet, const PartitionTreeNode& node) {
    /* We're at a partition tree leaf: no more nodes to delegate newly created vnets to */
    if (!node.left || !node.right)
        return false;

    /* Vnet has been decomposed too many times */
    if (vnet.times_decomposed >= MAX_DECOMP_DEPTH)
        return false;

    /* Cutline doesn't go through vnet (a valid case: it wasn't there when partition tree was being built) */
    if (node.cutline_axis == Axis::X) {
        if (vnet.clipped_bb.xmin > node.cutline_pos || vnet.clipped_bb.xmax < node.cutline_pos)
            return false;
    } else {
        if (vnet.clipped_bb.ymin > node.cutline_pos || vnet.clipped_bb.ymax < node.cutline_pos)
            return false;
    }

    /* Vnet is too small */
    int num_sinks = get_vnet_sink_mask(vnet).count();
    if (num_sinks < MIN_DECOMP_SINKS_VNET)
        return false;

    return true;
}

template<typename HeapType>
void DecompNetlistRouter<HeapType>::route_partition_tree_node(tbb::task_group& g, PartitionTreeNode& node) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    /* Sort so that nets with the most sinks are routed first.
     * We want to interleave virtual nets with regular ones, so sort an "index vector"
     * instead where indices >= node.nets.size() refer to node.vnets.
     * Virtual nets use their parent net's #fanouts in sorting while regular
     * nets use their own #fanouts. */
    std::vector<size_t> order(node.nets.size() + node.vnets.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](size_t i, size_t j) -> bool {
        ParentNetId id1 = i < node.nets.size() ? node.nets[i] : node.vnets[i - node.nets.size()].net_id;
        ParentNetId id2 = j < node.nets.size() ? node.nets[j] : node.vnets[j - node.nets.size()].net_id;
        return _net_list.net_sinks(id1).size() > _net_list.net_sinks(id2).size();
    });

    vtr::Timer t;
    for (size_t i : order) {
        if (i < node.nets.size()) { /* Regular net (not decomposed) */
            ParentNetId net_id = node.nets[i];
            if (!should_route_net(_net_list, net_id, _connections_inf, _budgeting_inf, _worst_neg_slack, true))
                continue;
            /* Setup the net (reset or prune) only once here in the flow. Then all calls to route_net turn off auto-setup */
            setup_net(
                _itry,
                net_id,
                _net_list,
                _connections_inf,
                _router_opts,
                _worst_neg_slack);
            /* Try decomposing the net. */
            if (should_decompose_net(net_id, node)) {
                VirtualNet left_vnet, right_vnet;
                bool is_decomposed = decompose_and_route_net(net_id, node, left_vnet, right_vnet);
                if (is_decomposed) {
                    node.left->vnets.push_back(left_vnet);
                    node.right->vnets.push_back(right_vnet);
                    _results_th.local().rerouted_nets.push_back(net_id);
                    continue;
                }
            }
            /* decompose_and_route fails when we get bad flags, so we only need to handle them here */
            auto flags = route_net(
                &_routers_th.local(),
                _net_list,
                net_id,
                _itry,
                _pres_fac,
                _router_opts,
                _connections_inf,
                _results_th.local().stats,
                _net_delay,
                _netlist_pin_lookup,
                _timing_info.get(),
                _pin_timing_invalidator,
                _budgeting_inf,
                _worst_neg_slack,
                _routing_predictor,
                _choking_spots[net_id],
                _is_flat,
                route_ctx.route_bb[net_id],
                false);
            if (!flags.success && !flags.retry_with_full_bb) {
                /* Disconnected RRG and ConnectionRouter doesn't think growing the BB will work */
                _results_th.local().is_routable = false;
                return;
            }
            if (flags.retry_with_full_bb) {
                /* ConnectionRouter thinks we should grow the BB. Do that and leave this net unrouted for now */
                route_ctx.route_bb[net_id] = full_device_bb();
                /* Disable decomposition for nets like this: they're already problematic */
                _is_decomp_disabled[net_id] = true;
                continue;
            }
            if (flags.was_rerouted) {
                _results_th.local().rerouted_nets.push_back(net_id);
            }
        } else { /* Virtual net (was decomposed in the upper level) */
            VirtualNet& vnet = node.vnets[i - node.nets.size()];
            if (should_decompose_vnet(vnet, node)) {
                VirtualNet left_vnet, right_vnet;
                bool is_decomposed = decompose_and_route_vnet(vnet, node, left_vnet, right_vnet);
                if (is_decomposed) {
                    node.left->vnets.push_back(left_vnet);
                    node.right->vnets.push_back(right_vnet);
                    continue;
                }
            }
            /* Route the full vnet. Again we don't care about the flags, they should be handled by the regular path */
            auto sink_mask = get_vnet_sink_mask(vnet);
            route_net(
                &_routers_th.local(),
                _net_list,
                vnet.net_id,
                _itry,
                _pres_fac,
                _router_opts,
                _connections_inf,
                _results_th.local().stats,
                _net_delay,
                _netlist_pin_lookup,
                _timing_info.get(),
                _pin_timing_invalidator,
                _budgeting_inf,
                _worst_neg_slack,
                _routing_predictor,
                _choking_spots[vnet.net_id],
                _is_flat,
                vnet.clipped_bb,
                false,
                sink_mask);
        }
    }

    PartitionTreeDebug::log("Node with " + std::to_string(node.nets.size())
                            + " nets and " + std::to_string(node.vnets.size())
                            + " virtual nets routed in " + std::to_string(t.elapsed_sec())
                            + " s");

    /* This node is finished: add left & right branches to the task queue */
    if (node.left && node.right) {
        g.run([&]() {
            route_partition_tree_node(g, *node.left);
        });
        g.run([&]() {
            route_partition_tree_node(g, *node.right);
        });
    } else {
        VTR_ASSERT(!node.left && !node.right); // there shouldn't be a node with a single branch
    }
}

/** Clip bb to one side of the cutline given the axis and position of the cutline.
 * Note that cutlines are assumed to be at axis = cutline_pos + 0.5. */
inline t_bb clip_to_side(const t_bb& bb, Axis axis, int cutline_pos, Side side) {
    t_bb out = bb;
    if (axis == Axis::X && side == Side::LEFT)
        out.xmax = cutline_pos;
    else if (axis == Axis::X && side == Side::RIGHT)
        out.xmin = cutline_pos + 1;
    else if (axis == Axis::Y && side == Side::LEFT)
        out.ymax = cutline_pos;
    else if (axis == Axis::Y && side == Side::RIGHT)
        out.ymin = cutline_pos + 1;
    else
        VTR_ASSERT_MSG(false, "Unreachable");
    return out;
}

/** Break a net/vnet into two. Output into references */
inline void make_vnet_pair(ParentNetId net_id, const t_bb& bb, Axis cutline_axis, int cutline_pos, VirtualNet& left, VirtualNet& right) {
    left.net_id = net_id;
    left.clipped_bb = clip_to_side(bb, cutline_axis, cutline_pos, Side::LEFT);
    right.net_id = net_id;
    right.clipped_bb = clip_to_side(bb, cutline_axis, cutline_pos, Side::RIGHT);
}

template<typename HeapType>
bool DecompNetlistRouter<HeapType>::decompose_and_route_net(ParentNetId net_id, const PartitionTreeNode& node, VirtualNet& left, VirtualNet& right) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& net_bb = route_ctx.route_bb[net_id];

    /* Sample enough sinks to provide branch-off points to the virtual nets we create */
    auto sink_mask = get_decomposition_mask(net_id, node);

    /* Route the net with the given mask: only the sinks we ask for will be routed */
    auto flags = route_net(
        &_routers_th.local(),
        _net_list,
        net_id,
        _itry,
        _pres_fac,
        _router_opts,
        _connections_inf,
        _results_th.local().stats,
        _net_delay,
        _netlist_pin_lookup,
        _timing_info.get(),
        _pin_timing_invalidator,
        _budgeting_inf,
        _worst_neg_slack,
        _routing_predictor,
        _choking_spots[net_id],
        _is_flat,
        net_bb,
        false,
        sink_mask);

    if (!flags.success) { /* Even if flags.retry_with_full_bb is set, better to bail out here */
        return false;
    }

    /* Divide the net into two halves */
    make_vnet_pair(net_id, net_bb, node.cutline_axis, node.cutline_pos, left, right);
    left.times_decomposed = 1;
    right.times_decomposed = 1;
    return true;
}

/* Debug code for PartitionTreeDebug (describes existing routing) */

inline std::string describe_bbox(const t_bb& bb) {
    return std::to_string(bb.xmin) + "," + std::to_string(bb.ymin)
           + "x" + std::to_string(bb.xmax) + "," + std::to_string(bb.ymax);
}

inline std::string describe_rr_coords(RRNodeId inode) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    return std::to_string(rr_graph.node_xlow(inode))
           + "," + std::to_string(rr_graph.node_ylow(inode))
           + " -> " + std::to_string(rr_graph.node_xhigh(inode))
           + "," + std::to_string(rr_graph.node_yhigh(inode));
}

/** Build a string describing \p vnet and its existing routing */
inline std::string describe_vnet(const VirtualNet& vnet) {
    const auto& route_ctx = g_vpr_ctx.routing();

    std::string out = "";
    out += "Virtual net with bbox " + describe_bbox(vnet.clipped_bb)
           + " parent net: " + std::to_string(size_t(vnet.net_id))
           + " parent bbox: " + describe_bbox(route_ctx.route_bb[vnet.net_id]) + "\n";

    RRNodeId source_rr = route_ctx.net_rr_terminals[vnet.net_id][0];
    out += "source: " + describe_rr_coords(source_rr) + ", sinks:";
    for (size_t i = 1; i < route_ctx.net_rr_terminals[vnet.net_id].size(); i++) {
        RRNodeId sink_rr = route_ctx.net_rr_terminals[vnet.net_id][i];
        out += " " + describe_rr_coords(sink_rr);
    }
    out += "\n";

    const auto& vnet_isinks = get_vnet_sink_mask(vnet);
    auto my_isinks = sink_mask_to_vector(vnet_isinks, route_ctx.route_trees[vnet.net_id]->num_sinks());
    out += "my sinks:";
    for (int isink : my_isinks)
        out += " " + std::to_string(isink);
    out += "\n";

    out += "current routing:";
    auto all_nodes = route_ctx.route_trees[vnet.net_id]->all_nodes();
    for (auto it = all_nodes.begin(); it != all_nodes.end(); ++it) {
        if ((*it).is_leaf()) {
            out += describe_rr_coords((*it).inode) + " END ";
            ++it;
            if (it == all_nodes.end())
                break;
            out += describe_rr_coords((*it).parent()->inode) + " -> ";
            out += describe_rr_coords((*it).inode) + " -> ";
        } else {
            out += describe_rr_coords((*it).inode) + " -> ";
        }
    }
    out += "\n";

    return out;
}

/* Debug code for PartitionTreeDebug ends */

template<typename HeapType>
bool DecompNetlistRouter<HeapType>::decompose_and_route_vnet(VirtualNet& vnet, const PartitionTreeNode& node, VirtualNet& left, VirtualNet& right) {
    /* Sample enough sinks to provide branch-off points to the virtual nets we create */
    auto sink_mask = get_vnet_decomposition_mask(vnet, node);

    /* Route the *parent* net with the given mask: only the sinks we ask for will be routed */
    auto flags = route_net(
        &_routers_th.local(),
        _net_list,
        vnet.net_id,
        _itry,
        _pres_fac,
        _router_opts,
        _connections_inf,
        _results_th.local().stats,
        _net_delay,
        _netlist_pin_lookup,
        _timing_info.get(),
        _pin_timing_invalidator,
        _budgeting_inf,
        _worst_neg_slack,
        _routing_predictor,
        _choking_spots[vnet.net_id],
        _is_flat,
        vnet.clipped_bb,
        false,
        sink_mask);

    if (!flags.success) { /* Even if flags.retry_with_full_bb is set, better to bail out here */
        PartitionTreeDebug::log("Failed to route decomposed net:\n" + describe_vnet(vnet));
        return false;
    }

    /* Divide the net into two halves */
    make_vnet_pair(vnet.net_id, vnet.clipped_bb, node.cutline_axis, node.cutline_pos, left, right);
    left.times_decomposed = vnet.times_decomposed + 1;
    right.times_decomposed = vnet.times_decomposed + 1;
    return true;
}

/** Is \p inode less than \p thickness away from the cutline? */
inline bool is_close_to_cutline(RRNodeId inode, Axis cutline_axis, int cutline_pos, int thickness) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    /* Cutlines are considered to be at x + 0.5, set a thickness of +1 here by checking for equality */
    if (cutline_axis == Axis::X) {
        return rr_graph.node_xlow(inode) - thickness <= cutline_pos && rr_graph.node_xhigh(inode) + thickness >= cutline_pos;
    } else {
        return rr_graph.node_ylow(inode) - thickness <= cutline_pos && rr_graph.node_yhigh(inode) + thickness >= cutline_pos;
    }
}

/** Is \p inode less than \p thickness away from the \p bb perimeter? */
inline bool is_close_to_bb(RRNodeId inode, const t_bb& bb, int thickness) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int xlow = rr_graph.node_xlow(inode) - thickness;
    int ylow = rr_graph.node_ylow(inode) - thickness;
    int xhigh = rr_graph.node_xhigh(inode) + thickness;
    int yhigh = rr_graph.node_yhigh(inode) + thickness;

    return (xlow <= bb.xmin && xhigh >= bb.xmin)
           || (ylow <= bb.ymin && yhigh >= bb.ymin)
           || (xlow <= bb.xmax && xhigh >= bb.xmax)
           || (ylow <= bb.ymax && yhigh >= bb.ymax);
}

/** Does this net either:
 * * have a very narrow sink side or
 * * have less than MIN_SINKS sinks in the sink side?
 * If so, put all sinks in the sink side into \p out and return true */
inline bool get_reduction_mask(ParentNetId net_id, Axis cutline_axis, int cutline_pos, vtr::dynamic_bitset<>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();

    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    int num_sinks = tree.num_sinks();
    vtr::dynamic_bitset<> sink_side_mask(num_sinks + 1);
    int all_sinks = 0;

    Side source_side = which_side(tree.root().inode, cutline_axis, cutline_pos);
    const t_bb& net_bb = route_ctx.route_bb[net_id];
    t_bb sink_side_bb = clip_to_side(net_bb, cutline_axis, cutline_pos, !source_side);
    auto& is_isink_reached = tree.get_is_isink_reached();

    /* Get sinks on the sink side */
    for (int isink = 1; isink < num_sinks + 1; isink++) {
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][isink];
        if (inside_bb(rr_sink, sink_side_bb)) {
            if (!is_isink_reached.get(isink))
                sink_side_mask.set(isink, true);
            if (is_close_to_cutline(rr_sink, cutline_axis, cutline_pos, 1)) /* Don't count sinks close to cutline */
                continue;
            all_sinks++;
        }
    }

    /* Are there too few sinks on the sink side? In that case, just route to all of them */
    const int MIN_SINKS = 4;
    if (all_sinks <= MIN_SINKS) {
        out |= sink_side_mask;
        return true;
    }

    /* Is the sink side narrow? In that case, it may not contain enough wires to route */
    const int MIN_WIDTH = 10;
    int W = sink_side_bb.xmax - sink_side_bb.xmin + 1;
    int H = sink_side_bb.ymax - sink_side_bb.ymin + 1;
    if (W < MIN_WIDTH || H < MIN_WIDTH) {
        out |= sink_side_mask;
        return true;
    }

    return false;
}

template<typename HeapType>
vtr::dynamic_bitset<> DecompNetlistRouter<HeapType>::get_decomposition_mask(ParentNetId net_id, const PartitionTreeNode& node) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    size_t num_sinks = tree.num_sinks();

    /** Note that sink masks are 1-indexed */
    auto& is_isink_reached = tree.get_is_isink_reached();
    vtr::dynamic_bitset<> out(num_sinks + 1);

    /* Sometimes cutlines divide a net very unevenly. In that case, just route to all
     * sinks in the small side and unblock. Stick with convex hull sampling if source
     * is close to cutline. */
    bool is_reduced = get_reduction_mask(net_id, node.cutline_axis, node.cutline_pos, out);

    bool source_on_cutline = is_close_to_cutline(tree.root().inode, node.cutline_axis, node.cutline_pos, 1);
    if (!is_reduced || source_on_cutline)
        convex_hull_downsample(net_id, route_ctx.route_bb[net_id], out);

    /* Always sample "known samples": sinks known to fail to route.
     * We don't lock it here, because it's written to during the routing step of decomposition,
     * which happens after this fn. */
    out |= _net_known_samples[net_id];

    /* Sample if a sink is too close to the cutline (and unreached).
     * Those sinks are likely to fail routing */
    for (size_t isink = 1; isink < num_sinks + 1; isink++) {
        if (is_isink_reached.get(isink))
            continue;

        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][isink];
        if (is_close_to_cutline(rr_sink, node.cutline_axis, node.cutline_pos, 1))
            out.set(isink, true);
    }

    return out;
}

/** Does this net either:
 * * have a very narrow side or
 * * have less than MIN_SINKS sinks in at least one side?
 * If so, put all sinks in the sides matching the above condition into \p out and return true */
inline int get_reduction_mask_vnet_no_source(const VirtualNet& vnet, Axis cutline_axis, int cutline_pos, vtr::dynamic_bitset<>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();

    const RouteTree& tree = route_ctx.route_trees[vnet.net_id].value();
    int num_sinks = tree.num_sinks();
    const t_bb& net_bb = vnet.clipped_bb;

    t_bb left_side = clip_to_side(net_bb, cutline_axis, cutline_pos, Side::LEFT);
    t_bb right_side = clip_to_side(net_bb, cutline_axis, cutline_pos, Side::RIGHT);
    auto& is_isink_reached = tree.get_is_isink_reached();

    int reduced_sides = 0;

    for (const t_bb& side_bb : {left_side, right_side}) {
        vtr::dynamic_bitset<> side_mask(num_sinks + 1);
        int all_sinks = 0;

        const int MIN_WIDTH = 10;
        int W = side_bb.xmax - side_bb.xmin + 1;
        int H = side_bb.ymax - side_bb.ymin + 1;
        bool is_narrow = (W < MIN_WIDTH || H < MIN_WIDTH);
        bool should_reduce = true;

        const int MIN_SINKS = 4;

        for (int isink = 1; isink < num_sinks + 1; isink++) {
            RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][isink];
            if (!inside_bb(rr_sink, side_bb))
                continue;
            if (!is_isink_reached.get(isink))
                side_mask.set(isink, true);
            if (is_narrow) /* If the box is narrow, don't check for all_sinks -- we are going to reduce it anyway */
                continue;
            if (is_close_to_bb(rr_sink, side_bb, 1))
                continue;
            all_sinks++;
            if (all_sinks > MIN_SINKS) {
                should_reduce = false;
                break;
            }
        }

        if (!should_reduce) /* We found enough sinks and the box is not narrow */
            continue;

        /* Either we have a narrow box, or too few unique sink locations. Just route to every sink on this side */
        out |= side_mask;
        reduced_sides++;
    }

    return reduced_sides;
}

/** Similar fn to \see get_reduction_mask, but works with virtual nets
 * and checks against the clipped bounding box instead of the cutline when counting sink-side sinks. */
inline bool get_reduction_mask_vnet_with_source(const VirtualNet& vnet, Axis cutline_axis, int cutline_pos, vtr::dynamic_bitset<>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();

    const RouteTree& tree = route_ctx.route_trees[vnet.net_id].value();
    int num_sinks = tree.num_sinks();
    vtr::dynamic_bitset<> sink_side_mask(num_sinks + 1);
    int all_sinks = 0;

    Side source_side = which_side(tree.root().inode, cutline_axis, cutline_pos);
    const t_bb& net_bb = vnet.clipped_bb;
    t_bb sink_side_bb = clip_to_side(net_bb, cutline_axis, cutline_pos, !source_side);
    auto& is_isink_reached = tree.get_is_isink_reached();

    /* Get sinks on the sink side */
    for (int isink = 1; isink < num_sinks + 1; isink++) {
        RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][isink];
        if (inside_bb(rr_sink, sink_side_bb)) {
            if (!is_isink_reached.get(isink))
                sink_side_mask.set(isink, true);
            if (is_close_to_bb(rr_sink, sink_side_bb, 1)) /* Don't count sinks close to BB */
                continue;
            all_sinks++;
        }
    }

    /* Are there too few sinks on the sink side? In that case, just route to all of them */
    const int MIN_SINKS = 4;
    if (all_sinks <= MIN_SINKS) {
        out |= sink_side_mask;
        return true;
    }

    /* Is the sink side narrow? In that case, it may not contain enough wires to route */
    const int MIN_WIDTH = 10;
    int W = sink_side_bb.xmax - sink_side_bb.xmin + 1;
    int H = sink_side_bb.ymax - sink_side_bb.ymin + 1;
    if (W < MIN_WIDTH || H < MIN_WIDTH) {
        out |= sink_side_mask;
        return true;
    }

    return false;
}

template<typename HeapType>
vtr::dynamic_bitset<> DecompNetlistRouter<HeapType>::get_vnet_decomposition_mask(const VirtualNet& vnet, const PartitionTreeNode& node) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const RouteTree& tree = route_ctx.route_trees[vnet.net_id].value();
    int num_sinks = tree.num_sinks();

    auto& is_isink_reached = tree.get_is_isink_reached();
    vtr::dynamic_bitset<> out(num_sinks + 1);

    /* Sometimes cutlines divide a net very unevenly. In that case, just route to all
     * sinks in the small side and unblock. Add convex hull since we are in a vnet which
     * may not have a source at all */
    if (inside_bb(tree.root().inode, vnet.clipped_bb)) { /* We have source, no need to sample after reduction in most cases */
        bool is_reduced = get_reduction_mask_vnet_with_source(vnet, node.cutline_axis, node.cutline_pos, out);
        bool source_on_cutline = is_close_to_cutline(tree.root().inode, node.cutline_axis, node.cutline_pos, 1);
        if (!is_reduced || source_on_cutline)
            convex_hull_downsample(vnet.net_id, vnet.clipped_bb, out);
    } else {
        int reduced_sides = get_reduction_mask_vnet_no_source(vnet, node.cutline_axis, node.cutline_pos, out);
        if (reduced_sides < 2) {
            convex_hull_downsample(vnet.net_id, vnet.clipped_bb, out);
        }
    }

    std::vector<size_t> isinks = sink_mask_to_vector(get_vnet_sink_mask(vnet), tree.num_sinks());

    /* Sample if a sink is too close to the cutline (and unreached).
     * Those sinks are likely to fail routing */
    for (size_t isink : isinks) {
        if (is_isink_reached.get(isink))
            continue;
        RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][isink];
        if (is_close_to_cutline(rr_sink, node.cutline_axis, node.cutline_pos, 1)) {
            out.set(isink, true);
            continue;
        }
        if (is_close_to_bb(rr_sink, vnet.clipped_bb, 1))
            out.set(isink, true);
    }

    return out;
}
