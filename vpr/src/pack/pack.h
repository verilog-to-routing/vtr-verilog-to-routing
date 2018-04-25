#ifndef PACK_H
#define PACK_H

void try_pack(t_packer_opts *packer_opts,
        const t_arch * arch,
		const t_model *user_models,
        const t_model *library_models,
        float interc_delay,
        vector<t_lb_type_rr_node> *lb_type_rr_graphs
#ifdef ENABLE_CLASSIC_VPR_STA
        , t_timing_inf timing_inf
#endif
        );

float get_arch_switch_info(short switch_index, int switch_fanin, float &Tdel_switch, float &R_switch, float &Cout_switch);

#endif
