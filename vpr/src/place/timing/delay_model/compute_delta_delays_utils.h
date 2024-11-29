
#pragma once

#include "vtr_ndmatrix.h"
#include "physical_types.h"
#include "rr_graph_fwd.h"

struct t_placer_opts;
struct t_router_opts;
class RouterDelayProfiler;

vtr::NdMatrix<float, 4> compute_delta_delay_model(RouterDelayProfiler& route_profiler,
                                                  const t_placer_opts& placer_opts,
                                                  const t_router_opts& router_opts,
                                                  bool measure_directconnect,
                                                  int longest_length,
                                                  bool is_flat);

bool find_direct_connect_sample_locations(const t_direct_inf* direct,
                                          t_physical_tile_type_ptr from_type,
                                          int from_pin,
                                          int from_pin_class,
                                          t_physical_tile_type_ptr to_type,
                                          int to_pin,
                                          int to_pin_class,
                                          RRNodeId& out_src_node,
                                          RRNodeId& out_sink_node);