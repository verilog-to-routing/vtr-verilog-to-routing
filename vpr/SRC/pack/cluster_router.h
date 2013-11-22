/* 
  Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.

  Author: Jason Luu
  Date: July 22, 2013
 */

/* Constructors/Destructors */
t_lb_router_data *alloc_and_load_router_data(INP vector<t_lb_type_rr_node> *lb_type_graph, t_type_ptr type);
void free_router_data(INOUTP t_lb_router_data *router_data);
void free_intra_lb_nets(vector <t_intra_lb_net> *intra_lb_nets);

/* Routing Functions */
void add_atom_as_target(INOUTP t_lb_router_data *router_data, INP int iatom);
void remove_atom_from_target(INOUTP t_lb_router_data *router_data, INP int iatom);
void set_reset_pb_modes(INOUTP t_lb_router_data *router_data, INP t_pb *pb, INP boolean set);
boolean try_intra_lb_route(INOUTP t_lb_router_data *router_data);

/* Accessor Functions */
t_pb_pin_route_stats *alloc_and_load_pb_pin_route_stats(const vector <t_intra_lb_net> *intra_lb_nets, t_pb_graph_node *pb_graph_head); 
void free_pb_pin_route_stats(t_pb_pin_route_stats *pb_pin_route_stats);

