void build_rr_graph (enum e_route_type route_type, struct s_det_routing_arch
         det_routing_arch, t_segment_inf *segment_inf, t_timing_inf 
         timing_inf, enum e_base_cost_type base_cost_type);
void free_rr_graph_internals (enum e_route_type route_type, 
	 struct s_det_routing_arch det_routing_arch, 
         t_segment_inf *segment_inf, t_timing_inf 
         timing_inf, enum e_base_cost_type base_cost_type);
void free_rr_graph (void); 

void dump_rr_graph (char *file_name);               /* For debugging only */
void print_rr_node (FILE *fp, int inode);           /* For debugging only */
void print_rr_indexed_data (FILE *fp, int index);   /* For debugging only */ 
void load_net_rr_terminals (int **rr_node_indices, 
          int nodes_per_chan);
int **get_rr_node_indices(void);
int get_nodes_per_chan(void);

