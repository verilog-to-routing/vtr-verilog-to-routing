#include "route_debug.h"

std::atomic_bool f_router_debug = false;

void enable_router_debug(
    const t_router_opts& router_opts,
    ParentNetId net,
    RRNodeId sink_rr,
    int router_iteration,
    ConnectionRouterInterface* router) {
    bool active_net_debug = (router_opts.router_debug_net >= -1);
    bool active_sink_debug = (router_opts.router_debug_sink_rr >= 0);
    bool active_iteration_debug = (router_opts.router_debug_iteration >= 0);

    bool match_net = (ParentNetId(router_opts.router_debug_net) == net || router_opts.router_debug_net == -1);
    bool match_sink = (router_opts.router_debug_sink_rr == int(sink_rr) || router_opts.router_debug_sink_rr < 0);
    bool match_iteration = (router_opts.router_debug_iteration == router_iteration || router_opts.router_debug_iteration < 0);

    f_router_debug = active_net_debug || active_sink_debug || active_iteration_debug;

    if (active_net_debug) f_router_debug = f_router_debug && match_net;
    if (active_sink_debug) f_router_debug = f_router_debug && match_sink;
    if (active_iteration_debug) f_router_debug = f_router_debug && match_iteration;

    router->set_router_debug(f_router_debug);

#ifndef VTR_ENABLE_DEBUG_LOGGING
    VTR_LOGV_WARN(f_router_debug, "Limited router debug output provided since compiled without VTR_ENABLE_DEBUG_LOGGING defined\n");
#endif
}
