float **alloc_net_delay(struct s_linked_vptr **chunk_list_head_ptr, struct s_net *nets, int n_nets);

void free_net_delay(float **net_delay,
		    struct s_linked_vptr **chunk_list_head_ptr);

void load_net_delay_from_routing(float **net_delay, struct s_net *nets, int n_nets);

void load_constant_net_delay(float **net_delay,
			     float delay_value, struct s_net *nets, int n_nets);

void print_net_delay(float **net_delay,
		     char *fname, struct s_net *nets, int n_nets);
