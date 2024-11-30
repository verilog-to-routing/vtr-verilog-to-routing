

#include "PlacementDelayModelCreator.h"

#include "place_delay_model.h"
#include "simple_delay_model.h"
#include "delta_delay_model.h"
#include "override_delay_model.h"

#include "vtr_time.h"
#include "physical_types.h"
#include "place_and_route.h"

static int get_longest_segment_length(std::vector<t_segment_inf>& segment_inf) {
    int length = 0;

    for (const t_segment_inf& seg_info : segment_inf) {
        if (seg_info.length > length) {
            length = seg_info.length;
        }
    }

    return length;
}

std::unique_ptr<PlaceDelayModel>
PlacementDelayModelCreator::create_delay_model(const t_placer_opts& placer_opts,
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

    // now setup and compute the actual arrays
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

    // free all data structures that are no longer needed
    free_routing_structs();

    return place_delay_model;
}