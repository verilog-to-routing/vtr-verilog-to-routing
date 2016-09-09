float **alloc_net_delay(vtr::t_chunk *chunk_list_ptr,
		vector<t_vnet> & nets, unsigned int n_nets);

void free_net_delay(float **net_delay,
		vtr::t_chunk *chunk_list_ptr);

void load_net_delay_from_routing(float **net_delay, vector<t_vnet> &nets,
		unsigned int n_nets);

void load_constant_net_delay(float **net_delay, float delay_value,
		vector<t_vnet> &nets, unsigned int n_nets);
