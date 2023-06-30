/**
 * @file
 * @brief Functions to read/write a .route file, which contains a serialized routing state.
 *
 * This is used to perform --analysis only
 */

#ifndef READ_ROUTE_H
#define READ_ROUTE_H

#include "netlist.h"
#include "vpr_types.h"

bool read_route(const char* route_file, const t_router_opts& RouterOpts, bool verify_file_digests, bool is_flat);
void print_route(const Netlist<>& net_list, const char* placement_file, const char* route_file, bool is_flat);

#endif /* READ_ROUTE_H */
