void build_rr_graph (enum e_route_type route_type, struct s_det_routing_arch
         det_routing_arch, t_segment_inf *segment_inf, t_timing_inf 
         timing_inf, enum e_base_cost_type base_cost_type);

void free_rr_graph (void); 

void dump_rr_graph (char *file_name);               /* For debugging only */
void print_rr_node (FILE *fp, int inode);           /* For debugging only */
void print_rr_indexed_data (FILE *fp, int index);   /* For debugging only */ 
