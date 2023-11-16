#pragma once

/** @file Utils for debugging the router */

#include <atomic>
#include "connection_router_interface.h"
#include "vpr_types.h"

/** @brief Run-time flag to control when router debug information is printed
 * Note only enables debug output if compiled with VTR_ENABLE_DEBUG_LOGGING defined
 * f_router_debug is used to stop the router when a breakpoint is reached. When a breakpoint is reached, this flag is set to true.
 *
 * In addition f_router_debug is used to print additional debug information during routing, for instance lookahead expected costs
 * information.
 *
 * d2: Made atomic as an attempt to make it work with parallel routing, but don't expect reliable results. */
extern std::atomic_bool f_router_debug;

/** Enable f_router_debug if specific sink/net debugging is set in \p router_opts */
void enable_router_debug(const t_router_opts& router_opts, ParentNetId net, RRNodeId sink_rr, int router_iteration, ConnectionRouterInterface* router);
