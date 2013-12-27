#include <cstdio>
#include <cstring>
#include <cmath>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph_area.h"
#include "segment_stats.h"
#include "stats.h"
#include "net_delay.h"
#include "path_delay.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"

/********************** Subroutines local to this module *********************/

static void load_channel_occupancies(int **chanx_occ, int **chany_occ);

static void get_length_and_bends_stats(void);

static void get_channel_occupancy_stats(void);

/************************* Subroutine definitions ****************************/

void routing_stats(boolean full_stats, enum e_route_type route_type,
		int num_switch, t_segment_inf * segment_inf, int num_segment,
		float R_minW_nmos, float R_minW_pmos,
		enum e_directionality directionality, boolean timing_analysis_enabled,
		float **net_delay, t_slack * slacks) {

	/* Prints out various statistics about the current routing.  Both a routing *
	 * and an rr_graph must exist when you call this routine.                   */

	float area, used_area;
	int i, j;

	get_length_and_bends_stats();
	get_channel_occupancy_stats();

	vpr_printf_info("Logic area (in minimum width transistor areas, excludes I/Os and empty grid tiles)...\n");

	area = 0;
	for (i = 1; i <= nx; i++) {
		for (j = 1; j <= ny; j++) {
			if (grid[i][j].width_offset == 0 && grid[i][j].height_offset == 0) {
				if (grid[i][j].type->area == UNDEFINED) {
					area += grid_logic_tile_area * grid[i][j].type->width * grid[i][j].type->height;
				} else {
					area += grid[i][j].type->area;
				}
			}
		}
	}
	/* Todo: need to add pitch of routing to blocks with height > 3 */
	vpr_printf_info("\tTotal logic block area (Warning, need to add pitch of routing to blocks with height > 3): %g\n", area);

	used_area = 0;
	for (i = 0; i < num_blocks; i++) {
		if (block[i].type != IO_TYPE) {
			if (block[i].type->area == UNDEFINED) {
				used_area += grid_logic_tile_area * block[i].type->width * block[i].type->height;
			} else {
				used_area += block[i].type->area;
			}
		}
	}
	vpr_printf_info("\tTotal used logic block area: %g\n", used_area);

	if (route_type == DETAILED) {
		count_routing_transistors(directionality, num_switch, segment_inf,
				R_minW_nmos, R_minW_pmos);
		get_segment_usage_stats(num_segment, segment_inf);

		if (timing_analysis_enabled) {
			load_net_delay_from_routing(net_delay, g_clbs_nlist.net, g_clbs_nlist.net.size());

			load_timing_graph_net_delays(net_delay);

#ifdef HACK_LUT_PIN_SWAPPING			
			do_timing_analysis(slacks, FALSE, TRUE, TRUE);
#else
			do_timing_analysis(slacks, FALSE, FALSE, TRUE);
#endif
			if (getEchoEnabled()) {
				if(isEchoFileEnabled(E_ECHO_TIMING_GRAPH))
					print_timing_graph(getEchoFileName(E_ECHO_TIMING_GRAPH));
				if (isEchoFileEnabled(E_ECHO_NET_DELAY)) 
					print_net_delay(net_delay, getEchoFileName(E_ECHO_NET_DELAY));
				if(isEchoFileEnabled(E_ECHO_LUT_REMAPPING))
					print_lut_remapping(getEchoFileName(E_ECHO_LUT_REMAPPING));
			}

			print_slack(slacks->slack, TRUE, getOutputFileName(E_SLACK_FILE));
			print_critical_path(getOutputFileName(E_CRIT_PATH_FILE));

			print_timing_stats();
		}
	}

	if (full_stats == TRUE)
		print_wirelen_prob_dist();
}

void get_length_and_bends_stats(void) {

	/* Figures out maximum, minimum and average number of bends and net length   *
	 * in the routing.                                                           */

	unsigned int inet;
	int bends, total_bends, max_bends;
	int length, total_length, max_length;
	int segments, total_segments, max_segments;
	float av_bends, av_length, av_segments;
	int num_global_nets, num_clb_opins_reserved;

	max_bends = 0;
	total_bends = 0;
	max_length = 0;
	total_length = 0;
	max_segments = 0;
	total_segments = 0;
	num_global_nets = 0;
	num_clb_opins_reserved = 0;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global == FALSE && g_clbs_nlist.net[inet].num_sinks() != 0) { /* Globals don't count. */
			get_num_bends_and_length(inet, &bends, &length, &segments);

			total_bends += bends;
			max_bends = max(bends, max_bends);

			total_length += length;
			max_length = max(length, max_length);

			total_segments += segments;
			max_segments = max(segments, max_segments);
		} else if (g_clbs_nlist.net[inet].is_global) {
			num_global_nets++;
		} else {
			num_clb_opins_reserved++;
		}
	}

	av_bends = (float) total_bends / (float) ((int) g_clbs_nlist.net.size() - num_global_nets);
	vpr_printf_info("\n");
	vpr_printf_info("Average number of bends per net: %#g  Maximum # of bends: %d\n", av_bends, max_bends);
	vpr_printf_info("\n");

	av_length = (float) total_length / (float) ((int) g_clbs_nlist.net.size() - num_global_nets);
	vpr_printf_info("Number of routed nets (nonglobal): %d\n", (int) g_clbs_nlist.net.size() - num_global_nets);
	vpr_printf_info("Wire length results (in units of 1 clb segments)...\n");
	vpr_printf_info("\tTotal wirelength: %d, average net length: %#g\n", total_length, av_length);
	vpr_printf_info("\tMaximum net length: %d\n", max_length);
	vpr_printf_info("\n");

	av_segments = (float) total_segments / (float) ((int) g_clbs_nlist.net.size() - num_global_nets);
	vpr_printf_info("Wire length results in terms of physical segments...\n");
	vpr_printf_info("\tTotal wiring segments used: %d, average wire segments per net: %#g\n", total_segments, av_segments);
	vpr_printf_info("\tMaximum segments used by a net: %d\n", max_segments);
	vpr_printf_info("\tTotal local nets with reserved CLB opins: %d\n", num_clb_opins_reserved);
}

static void get_channel_occupancy_stats(void) {

	/* Determines how many tracks are used in each channel.                    */

	int **chanx_occ; /* [1..nx][0..ny] */
	int **chany_occ; /* [0..nx][1..ny] */

	chanx_occ = (int **) alloc_matrix(1, nx, 0, ny, sizeof(int));
	chany_occ = (int **) alloc_matrix(0, nx, 1, ny, sizeof(int));
	load_channel_occupancies(chanx_occ, chany_occ);

	vpr_printf_info("\n");
	vpr_printf_info("X - Directed channels: j max occ ave occ capacity\n");
	vpr_printf_info("                      -- ------- ------- --------\n");

	int total_x = 0;
	for (int j = 0; j <= ny; ++j) {
		total_x += chan_width.x_list[j];
		float ave_occ = 0.0;
		int max_occ = -1;

		for (int i = 1; i <= nx; ++i) {
			max_occ = max(chanx_occ[i][j], max_occ);
			ave_occ += chanx_occ[i][j];
		}
		ave_occ /= nx;
		vpr_printf_info("                      %2d %7d %7.4f %8d\n", j, max_occ, ave_occ, chan_width.x_list[j]);
	}

	vpr_printf_info("Y - Directed channels: i max occ ave occ capacity\n");
	vpr_printf_info("                      -- ------- ------- --------\n");

	int total_y = 0;
	for (int i = 0; i <= nx; ++i) {
		total_y += chan_width.y_list[i];
		float ave_occ = 0.0;
		int max_occ = -1;

		for (int j = 1; j <= ny; ++j) {
			max_occ = max(chany_occ[i][j], max_occ);
			ave_occ += chany_occ[i][j];
		}
		ave_occ /= ny;
		vpr_printf_info("                      %2d %7d %7.4f %8d\n", i, max_occ, ave_occ, chan_width.y_list[i]);
	}

	vpr_printf_info("\n");
	vpr_printf_info("Total tracks in x-direction: %d, in y-direction: %d\n", total_x, total_y);
	vpr_printf_info("\n");

	free_matrix(chanx_occ, 1, nx, 0, sizeof(int));
	free_matrix(chany_occ, 0, nx, 1, sizeof(int));
}

static void load_channel_occupancies(int **chanx_occ, int **chany_occ) {

	/* Loads the two arrays passed in with the total occupancy at each of the  *
	 * channel segments in the FPGA.                                           */

	int i, j, inode;
	unsigned inet;
	struct s_trace *tptr;
	t_rr_type rr_type;

	/* First set the occupancy of everything to zero. */

	for (i = 1; i <= nx; i++)
		for (j = 0; j <= ny; j++)
			chanx_occ[i][j] = 0;

	for (i = 0; i <= nx; i++)
		for (j = 1; j <= ny; j++)
			chany_occ[i][j] = 0;

	/* Now go through each net and count the tracks and pins used everywhere */

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {

		if (g_clbs_nlist.net[inet].is_global && g_clbs_nlist.net[inet].num_sinks() != 0) /* Skip global and empty nets. */
			continue;

		tptr = trace_head[inet];
		while (tptr != NULL) {
			inode = tptr->index;
			rr_type = rr_node[inode].type;

			if (rr_type == SINK) {
				tptr = tptr->next; /* Skip next segment. */
				if (tptr == NULL)
					break;
			}

			else if (rr_type == CHANX) {
				j = rr_node[inode].ylow;
				for (i = rr_node[inode].xlow; i <= rr_node[inode].xhigh; i++)
					chanx_occ[i][j]++;
			}

			else if (rr_type == CHANY) {
				i = rr_node[inode].xlow;
				for (j = rr_node[inode].ylow; j <= rr_node[inode].yhigh; j++)
					chany_occ[i][j]++;
			}

			tptr = tptr->next;
		}
	}
}

void get_num_bends_and_length(int inet, int *bends_ptr, int *len_ptr,
		int *segments_ptr) {

	/* Counts and returns the number of bends, wirelength, and number of routing *
	 * resource segments in net inet's routing.                                  */

	struct s_trace *tptr, *prevptr;
	int inode;
	t_rr_type curr_type, prev_type;
	int bends, length, segments;

	bends = 0;
	length = 0;
	segments = 0;

	prevptr = trace_head[inet]; /* Should always be SOURCE. */
	if (prevptr == NULL) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"in get_num_bends_and_length: net #%d has no traceback.\n", inet);
	}
	inode = prevptr->index;
	prev_type = rr_node[inode].type;

	tptr = prevptr->next;

	while (tptr != NULL) {
		inode = tptr->index;
		curr_type = rr_node[inode].type;

		if (curr_type == SINK) { /* Starting a new segment */
			tptr = tptr->next; /* Link to existing path - don't add to len. */
			if (tptr == NULL)
				break;

			curr_type = rr_node[tptr->index].type;
		}

		else if (curr_type == CHANX || curr_type == CHANY) {
			segments++;
			length += 1 + rr_node[inode].xhigh - rr_node[inode].xlow
					+ rr_node[inode].yhigh - rr_node[inode].ylow;

			if (curr_type != prev_type && (prev_type == CHANX || prev_type == CHANY))
				bends++;
		}

		prev_type = curr_type;
		tptr = tptr->next;
	}

	*bends_ptr = bends;
	*len_ptr = length;
	*segments_ptr = segments;
}

void print_wirelen_prob_dist(void) {

	/* Prints out the probability distribution of the wirelength / number   *
	 * input pins on a net -- i.e. simulates 2-point net length probability *
	 * distribution.                                                        */

	float *prob_dist;
	float norm_fac, two_point_length;
	unsigned int inet; 
	int bends, length, segments, index;
	float av_length;
	int prob_dist_size, i, incr;

	prob_dist_size = nx + ny + 10;
	prob_dist = (float *) my_calloc(prob_dist_size, sizeof(float));
	norm_fac = 0.;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global == FALSE && g_clbs_nlist.net[inet].num_sinks() != 0) {
			get_num_bends_and_length(inet, &bends, &length, &segments);

			/*  Assign probability to two integer lengths proportionately -- i.e.  *
			 *  if two_point_length = 1.9, add 0.9 of the pins to prob_dist[2] and *
			 *  only 0.1 to prob_dist[1].                                          */

			two_point_length = (float) length
					/ (float) (g_clbs_nlist.net[inet].num_sinks());
			index = (int) two_point_length;
			if (index >= prob_dist_size) {

				vpr_printf_warning(__FILE__, __LINE__,
						"Index (%d) to prob_dist exceeds its allocated size (%d).\n",
						index, prob_dist_size);
				vpr_printf_info("Realloc'ing to increase 2-pin wirelen prob distribution array.\n");
				incr = index - prob_dist_size + 2;
				prob_dist_size += incr;
				prob_dist = (float *)my_realloc(prob_dist,
						prob_dist_size * sizeof(float));
				for (i = prob_dist_size - incr; i < prob_dist_size; i++)
					prob_dist[i] = 0.0;
			}
			prob_dist[index] += (g_clbs_nlist.net[inet].num_sinks())
					* (1 - two_point_length + index);

			index++;
			if (index >= prob_dist_size) {

				vpr_printf_warning(__FILE__, __LINE__,
						"Index (%d) to prob_dist exceeds its allocated size (%d).\n",
						index, prob_dist_size);
				vpr_printf_info("Realloc'ing to increase 2-pin wirelen prob distribution array.\n");
				incr = index - prob_dist_size + 2;
				prob_dist_size += incr;
				prob_dist = (float *)my_realloc(prob_dist,
						prob_dist_size * sizeof(float));
				for (i = prob_dist_size - incr; i < prob_dist_size; i++)
					prob_dist[i] = 0.0;
			}
			prob_dist[index] += (g_clbs_nlist.net[inet].num_sinks())
					* (1 - index + two_point_length);

			norm_fac += g_clbs_nlist.net[inet].num_sinks();
		}
	}

	/* Normalize so total probability is 1 and print out. */

	vpr_printf_info("\n");
	vpr_printf_info("Probability distribution of 2-pin net lengths:\n");
	vpr_printf_info("\n");
	vpr_printf_info("Length    p(Lenth)\n");

	av_length = 0;

	for (index = 0; index < prob_dist_size; index++) {
		prob_dist[index] /= norm_fac;
		vpr_printf_info("%6d  %10.6f\n", index, prob_dist[index]);
		av_length += prob_dist[index] * index;
	}

	vpr_printf_info("\n");
	vpr_printf_info("Number of 2-pin nets: ;%g;\n", norm_fac);
	vpr_printf_info("Expected value of 2-pin net length (R): ;%g;\n", av_length);
	vpr_printf_info("Total wirelength: ;%g;\n", norm_fac * av_length);

	free(prob_dist);
}

void print_lambda(void) {

	/* Finds the average number of input pins used per clb.  Does not    *
	 * count inputs which are hooked to global nets (i.e. the clock     *
	 * when it is marked global).                                       */

	int bnum, ipin;
	int num_inputs_used = 0;
	int iclass, inet;
	float lambda;
	t_type_ptr type;

	for (bnum = 0; bnum < num_blocks; bnum++) {
		type = block[bnum].type;
		assert(type != NULL);
		if (type != IO_TYPE) {
			for (ipin = 0; ipin < type->num_pins; ipin++) {
				iclass = type->pin_class[ipin];
				if (type->class_inf[iclass].type == RECEIVER) {
					inet = block[bnum].nets[ipin];
					if (inet != OPEN) /* Pin is connected? */
						if (g_clbs_nlist.net[inet].is_global == FALSE) /* Not a global clock */
							num_inputs_used++;
				}
			}
		}
	}

	lambda = (float) num_inputs_used / (float) num_blocks;
	vpr_printf_info("Average lambda (input pins used per clb) is: %g\n", lambda);
}

int count_netlist_clocks(void) {

	/* Count how many clocks are in the netlist. */

	int iblock, i, clock_net;
	char * name;
	boolean found;
	int num_clocks = 0;
	char ** clock_names = NULL;

	for (iblock = 0; iblock < num_logical_blocks; iblock++) {
		if (logical_block[iblock].clock_net != OPEN) {
			clock_net = logical_block[iblock].clock_net;
			assert(clock_net != OPEN);
			name = logical_block[clock_net].name;
			/* Now that we've found a clock, let's see if we've counted it already */
			found = FALSE;
			for (i = 0; !found && i < num_clocks; i++) {
				if (strcmp(clock_names[i], name) == 0) {
					found = TRUE;
				}
			}
			if (!found) {
				/* If we get here, the clock is new and so we dynamically grow the array netlist_clocks by one. */
				clock_names = (char **) my_realloc (clock_names, ++num_clocks * sizeof(char *));
				clock_names[num_clocks - 1] = name;
			}
		}
	}
	free (clock_names);
	return num_clocks;
}



