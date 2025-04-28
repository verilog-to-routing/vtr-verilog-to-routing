/*
 * Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.
 *
 * Author: Jason Luu
 * Date: July 22, 2013
 */
#ifndef CLUSTER_ROUTER_H
#define CLUSTER_ROUTER_H
#include <vector>
#include "atom_netlist_fwd.h"
#include "pack_types.h"

/* Constructors/Destructors */
t_lb_router_data* alloc_and_load_router_data(std::vector<t_lb_type_rr_node>* lb_type_graph, t_logical_block_type_ptr type);
void free_router_data(t_lb_router_data* router_data);
void free_intra_lb_nets(std::vector<t_intra_lb_net>* intra_lb_nets);

/* Routing Functions */
void add_atom_as_target(t_lb_router_data* router_data, const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb);
void remove_atom_from_target(t_lb_router_data* router_data, const AtomBlockId blk_id, const AtomPBBimap& atom_to_pb);
void set_reset_pb_modes(t_lb_router_data* router_data, const t_pb* pb, const bool set);
bool try_intra_lb_route(t_lb_router_data* router_data, int verbosity, t_mode_selection_status* mode_status);
void reset_intra_lb_route(t_lb_router_data* router_data);

/* Accessor Functions */
/**
 * @brief Creates an array [0..num_pb_graph_pins-1] for intra-logic block routing lookup. 
 * Given a pb_graph_pin ID for a CLB, this lookup returns t_pb_route corresponding to that
 * pin.
 *
 * @param intra_lb_nets Vector of intra-logic block nets.
 * @param logic_block_type Logic block type of the current cluster.
 * @param intra_lb_pb_pin_lookup Intra-logic block pin lookup to get t_pb_graph_pin from a pin ID.
 * @return t_pb_routes An array [0..num_pb_graph_pins-1] for intra-logic block routing lookup.
 */
t_pb_routes alloc_and_load_pb_route(const std::vector<t_intra_lb_net>* intra_lb_nets,
                                    t_logical_block_type_ptr logic_block_type,
                                    const IntraLbPbPinLookup& intra_lb_pb_pin_lookup);
void free_pb_route(t_pb_route* free_pb_route);

#endif
