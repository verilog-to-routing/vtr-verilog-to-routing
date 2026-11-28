#pragma once
/**
 * @file
 * @brief Functions to read/write a .route file, which contains a serialized routing state.
 *
 * This is used to perform --analysis only
 */

#include "routing_netlist_fwd.h"
#include "vpr_types.h"

bool read_route(const RoutingNetlist& net_list,
                const char* route_file,
                const t_router_opts& RouterOpts,
                bool verify_file_digests);

void print_route(const RoutingNetlist& net_list,
                 const char* placement_file,
                 const char* route_file);
