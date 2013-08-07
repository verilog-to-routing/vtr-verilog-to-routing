/* 
  Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.

  Author: Jason Luu
  Date: July 22, 2013
 */

/* Constructors/Destructors */
t_lb_router_data *alloc_and_load_router_data(vector<t_lb_type_rr_node> *lb_type_graph);
void free_router_data(t_lb_router_data *router_data);

