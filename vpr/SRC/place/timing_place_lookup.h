#define IMPOSSIBLE -1		/*indicator of an array location that    */
/*should never be accessed */
#define EMPTY_DELTA -2		/*indicator of an array location that  */
/*has an empty block */

void compute_delay_lookup_tables(t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		t_chan_width_dist chan_width_dist, const t_direct_inf *directs, 
		const int num_directs);
void free_place_lookup_structs(void);

float get_delta_delay(int delta_x, int delta_y);
