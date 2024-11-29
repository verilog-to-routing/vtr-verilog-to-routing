
#include "timing_place_lookup.h"

#include "rr_graph_fwd.h"
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_log.h"
#include "vtr_util.h"

#include "vtr_time.h"

#include "vpr_types.h"
#include "globals.h"
#include "place_and_route.h"
#include "route_net.h"
#include "read_xml_arch_file.h"
#include "atom_netlist.h"

#include "router_delay_profiling.h"
#include "place_delay_model.h"
#include "simple_delay_model.h"
#include "delta_delay_model.h"
#include "override_delay_model.h"


/*** Function Prototypes *****/
static t_chan_width setup_chan_width(const t_router_opts& router_opts,
                                     t_chan_width_dist chan_width_dist);

static int get_longest_segment_length(std::vector<t_segment_inf>& segment_inf);

/******* Globally Accessible Functions **********/

std::unique_ptr<PlaceDelayModel> compute_place_delay_model(const t_placer_opts& placer_opts,
                                                           const t_router_opts& router_opts,
                                                           const Netlist<>& net_list,
                                                           t_det_routing_arch* det_routing_arch,
                                                           std::vector<t_segment_inf>& segment_inf,
                                                           t_chan_width_dist chan_width_dist,
                                                           const std::vector<t_direct_inf>& directs,
                                                           bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Computing placement delta delay look-up");

    t_chan_width chan_width = setup_chan_width(router_opts, chan_width_dist);

    alloc_routing_structs(chan_width, router_opts, det_routing_arch, segment_inf, directs, is_flat);

    const RouterLookahead* router_lookahead = get_cached_router_lookahead(*det_routing_arch,
                                                                          router_opts.lookahead_type,
                                                                          router_opts.write_router_lookahead,
                                                                          router_opts.read_router_lookahead,
                                                                          segment_inf,
                                                                          is_flat);

    RouterDelayProfiler route_profiler(net_list, router_lookahead, is_flat);

    int longest_length = get_longest_segment_length(segment_inf);

    /*now setup and compute the actual arrays */
    std::unique_ptr<PlaceDelayModel> place_delay_model;
    float min_cross_layer_delay = get_min_cross_layer_delay();

    if (placer_opts.delay_model_type == PlaceDelayModelType::SIMPLE) {
        place_delay_model = std::make_unique<SimpleDelayModel>();
    } else if (placer_opts.delay_model_type == PlaceDelayModelType::DELTA) {
        place_delay_model = std::make_unique<DeltaDelayModel>(min_cross_layer_delay, is_flat);
    } else if (placer_opts.delay_model_type == PlaceDelayModelType::DELTA_OVERRIDE) {
        place_delay_model = std::make_unique<OverrideDelayModel>(min_cross_layer_delay, is_flat);
    } else {
        VTR_ASSERT_MSG(false, "Invalid placer delay model");
    }

    if (placer_opts.read_placement_delay_lookup.empty()) {
        place_delay_model->compute(route_profiler, placer_opts, router_opts, longest_length);
    } else {
        place_delay_model->read(placer_opts.read_placement_delay_lookup);
    }

    if (!placer_opts.write_placement_delay_lookup.empty()) {
        place_delay_model->write(placer_opts.write_placement_delay_lookup);
    }

    /*free all data structures that are no longer needed */
    free_routing_structs();

    return place_delay_model;
}

/******* File Accessible Functions **********/

std::vector<int> get_best_classes(enum e_pin_type pintype, t_physical_tile_type_ptr type) {
    std::vector<int> best_classes;

    //Record any non-zero Fc pins
    //
    //Note that we track non-zero Fc pins, since certain Fc overrides
    //may apply to only a subset of wire types. This ensures we record
    //which pins can potentially connect to global routing.
    std::unordered_set<int> non_zero_fc_pins;
    for (const t_fc_specification& fc_spec : type->fc_specs) {
        if (fc_spec.fc_value == 0) continue;

        non_zero_fc_pins.insert(fc_spec.pins.begin(), fc_spec.pins.end());
    }

    //Collect all classes of matching type which connect to general routing
    for (int i = 0; i < (int)type->class_inf.size(); i++) {
        if (type->class_inf[i].type == pintype) {
            //Check whether all pins in this class are ignored or have zero fc
            bool any_pins_connect_to_general_routing = false;
            for (int ipin = 0; ipin < type->class_inf[i].num_pins; ++ipin) {
                int pin = type->class_inf[i].pinlist[ipin];
                //If the pin isn't ignored, and has a non-zero Fc to some general
                //routing the class is suitable for delay profiling
                if (!type->is_ignored_pin[pin] && non_zero_fc_pins.count(pin)) {
                    any_pins_connect_to_general_routing = true;
                    break;
                }
            }

            //Skip if the pin class doesn't connect to general routing
            if (!any_pins_connect_to_general_routing) continue;

            //Record candidate class
            best_classes.push_back(i);
        }
    }

    //Sort classes so the largest pin class is first
    auto cmp_class = [&](int lhs, int rhs) {
        return type->class_inf[lhs].num_pins > type->class_inf[rhs].num_pins;
    };

    std::stable_sort(best_classes.begin(), best_classes.end(), cmp_class);

    return best_classes;
}

static int get_longest_segment_length(std::vector<t_segment_inf>& segment_inf) {
    int length = 0;

    for (const t_segment_inf &seg_info : segment_inf) {
        if (seg_info.length > length) {
            length = seg_info.length;
        }
    }

    return length;
}

static t_chan_width setup_chan_width(const t_router_opts& router_opts,
                                     t_chan_width_dist chan_width_dist) {
    /*we give plenty of tracks, this increases routability for the */
    /*lookup table generation */

    t_graph_type graph_directionality;
    int width_fac;

    if (router_opts.fixed_channel_width == NO_FIXED_CHANNEL_WIDTH) {
        auto& device_ctx = g_vpr_ctx.device();

        auto type = find_most_common_tile_type(device_ctx.grid);

        width_fac = 4 * type->num_pins;
        /*this is 2x the value that binary search starts */
        /*this should be enough to allow most pins to   */
        /*connect to tracks in the architecture */
    } else {
        width_fac = router_opts.fixed_channel_width;
    }

    if (router_opts.route_type == GLOBAL) {
        graph_directionality = GRAPH_BIDIR;
    } else {
        graph_directionality = GRAPH_UNIDIR;
    }

    return init_chan(width_fac, chan_width_dist, graph_directionality);
}

bool directconnect_exists(RRNodeId src_rr_node, RRNodeId sink_rr_node) {
    //Returns true if there is a directconnect between the two RR nodes
    //
    //This is checked by looking for a SOURCE -> OPIN -> IPIN -> SINK path
    //which starts at src_rr_node and ends at sink_rr_node
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    VTR_ASSERT(rr_graph.node_type(src_rr_node) == SOURCE && rr_graph.node_type(sink_rr_node) == SINK);

    //TODO: This is a constant depth search, but still may be too slow
    for (t_edge_size i_src_edge = 0; i_src_edge < rr_graph.num_edges(src_rr_node); ++i_src_edge) {
        RRNodeId opin_rr_node = rr_graph.edge_sink_node(src_rr_node, i_src_edge);

        if (rr_graph.node_type(opin_rr_node) != OPIN) continue;

        for (t_edge_size i_opin_edge = 0; i_opin_edge < rr_graph.num_edges(opin_rr_node); ++i_opin_edge) {
            RRNodeId ipin_rr_node = rr_graph.edge_sink_node(opin_rr_node, i_opin_edge);
            if (rr_graph.node_type(ipin_rr_node) != IPIN) continue;

            for (t_edge_size i_ipin_edge = 0; i_ipin_edge < rr_graph.num_edges(ipin_rr_node); ++i_ipin_edge) {
                if (sink_rr_node == rr_graph.edge_sink_node(ipin_rr_node, i_ipin_edge)) {
                    return true;
                }
            }
        }
    }
    return false;
}