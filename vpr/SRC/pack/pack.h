void try_pack(INP struct s_packer_opts *packer_opts, INP const t_arch * arch, 
	INP t_model *user_models, INP t_model *library_models, t_timing_inf timing_inf, 
	float interc_delay, vector<t_lb_type_rr_node> *lb_type_rr_graphs);

float get_arch_switch_info(short switch_index, int switch_fanin, float &Tdel_switch, float &R_switch, float &Cout_switch);
