/* 
  Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.

  Author: Jason Luu
  Date: July 22, 2013
 */

/* Constructors/Destructors */
t_lb_router_data *alloc_and_load_router_data(INP vector<t_lb_type_rr_node> *lb_type_graph);
void free_router_data(INOUTP t_lb_router_data *router_data);


/* Routing Functions */
void add_atom_as_target(INOUTP t_lb_router_data *router_data, INP int iatom_block);
void remove_atom_from_target(INOUTP t_lb_router_data *router_data, INP int iatom_block);
void save_current_route(INOUTP t_lb_router_data *router_data);
void set_reset_pb_modes(INOUTP t_lb_router_data *router_data, INP t_pb *pb, INP boolean set);
void restore_previous_route(INOUTP t_lb_router_data *router_data);
boolean try_intra_lb_route(INOUTP t_lb_router_data *router_data);

/* Accessor Functions */
t_lb_traceback *get_final_route(INOUTP t_lb_router_data *router_data);

