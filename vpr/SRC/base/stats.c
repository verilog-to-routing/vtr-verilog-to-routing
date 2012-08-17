#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
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

	vpr_printf(TIO_MESSAGE_INFO, 
			"Logic Area (in minimum width transistor areas, excludes I/Os and empty grid tiles):\n");

	area = 0;
	for (i = 1; i <= nx; i++) {
		for (j = 1; j <= ny; j++) {
			if (grid[i][j].offset == 0) {
				if (grid[i][j].type->area == UNDEFINED) {
					area += grid_logic_tile_area * grid[i][j].type->height;
				} else {
					area += grid[i][j].type->area;
				}
			}
		}
	}
	/* Todo: need to add pitch of routing to blocks with height > 3 */
	vpr_printf(TIO_MESSAGE_INFO, 
			"Total Logic Block Area (Warning, need to add pitch of routing to blocks with height > 3): %g \n",
			area);

	used_area = 0;
	for (i = 0; i < num_blocks; i++) {
		if (block[i].type != IO_TYPE) {
			if (block[i].type->area == UNDEFINED) {
				used_area += grid_logic_tile_area * block[i].type->height;
			} else {
				used_area += block[i].type->area;
			}
		}
	}
	vpr_printf(TIO_MESSAGE_INFO, "Total Used Logic Block Area: %g \n", used_area);

	if (route_type == DETAILED) {
		count_routing_transistors(directionality, num_switch, segment_inf,
				R_minW_nmos, R_minW_pmos);
		get_segment_usage_stats(num_segment, segment_inf);

		if (timing_analysis_enabled) {
			load_net_delay_from_routing(net_delay, clb_net, num_nets);

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

			print_net_slack(slacks->net_slack, getOutputFileName(E_NET_SLACK_FILE));
			print_net_slack_ratio(slacks->net_slack_ratio, getOutputFileName(E_NET_SLACK_RATIO_FILE));
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

	int inet, bends, total_bends, max_bends;
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

	for (inet = 0; inet < num_nets; inet++) {
		if (clb_net[inet].is_global == FALSE && clb_net[inet].num_sinks != 0) { /* Globals don't count. */
			get_num_bends_and_length(inet, &bends, &length, &segments);

			total_bends += bends;
			max_bends = max(bends, max_bends);

			total_length += length;
			max_length = max(length, max_length);

			total_segments += segments;
			max_segments = max(segments, max_segments);
		} else if (clb_net[inet].is_global) {
			num_global_nets++;
		} else {
			num_clb_opins_reserved++;
		}
	}

	av_bends = (float) total_bends / (float) (num_nets - num_global_nets);
	vpr_printf(TIO_MESSAGE_INFO, "\nAverage number of bends per net: %#g  Maximum # of bends: %d\n\n",
			av_bends, max_bends);

	av_length = (float) total_length / (float) (num_nets - num_global_nets);
	vpr_printf(TIO_MESSAGE_INFO, "\nThe number of routed nets (nonglobal): %d\n",
			num_nets - num_global_nets);
	vpr_printf(TIO_MESSAGE_INFO, "Wirelength results (all in units of 1 clb segments):\n");
	vpr_printf(TIO_MESSAGE_INFO, "\tTotal wirelength: %d   Average net length: %#g\n", total_length,
			av_length);
	vpr_printf(TIO_MESSAGE_INFO, "\tMaximum net length: %d\n\n", max_length);

	av_segments = (float) total_segments / (float) (num_nets - num_global_nets);
	vpr_printf(TIO_MESSAGE_INFO, "Wirelength results in terms of physical segments:\n");
	vpr_printf(TIO_MESSAGE_INFO, "\tTotal wiring segments used: %d   Av. wire segments per net: "
			"%#g\n", total_segments, av_segments);
	vpr_printf(TIO_MESSAGE_INFO, "\tMaximum segments used by a net: %d\n\n", max_segments);
	vpr_printf(TIO_MESSAGE_INFO, "\tTotal local nets with reserved CLB opins: %d\n\n",
			num_clb_opins_reserved);
}

static void get_channel_occupancy_stats(void) {

	/* Determines how many tracks are used in each channel.                    */

	int i, j, max_occ, total_x, total_y;
	float av_occ;
	int **chanx_occ; /* [1..nx][0..ny] */
	int **chany_occ; /* [0..nx][1..ny] */

	chanx_occ = (int **) alloc_matrix(1, nx, 0, ny, sizeof(int));
	chany_occ = (int **) alloc_matrix(0, nx, 1, ny, sizeof(int));
	load_channel_occupancies(chanx_occ, chany_occ);

	vpr_printf(TIO_MESSAGE_INFO, "\nX - Directed channels:\n\n");
	vpr_printf(TIO_MESSAGE_INFO, "j\tmax occ\tav_occ\t\tcapacity\n");

	total_x = 0;

	for (j = 0; j <= ny; j++) {
		total_x += chan_width_x[j];
		av_occ = 0.;
		max_occ = -1;

		for (i = 1; i <= nx; i++) {
			max_occ = max(chanx_occ[i][j], max_occ);
			av_occ += chanx_occ[i][j];
		}
		av_occ /= nx;
		vpr_printf(TIO_MESSAGE_INFO, "%d\t%d\t%-#9g\t%d\n", j, max_occ, av_occ, chan_width_x[j]);
	}

	vpr_printf(TIO_MESSAGE_INFO, "\nY - Directed channels:\n\n");
	vpr_printf(TIO_MESSAGE_INFO, "i\tmax occ\tav_occ\t\tcapacity\n");

	total_y = 0;

	for (i = 0; i <= nx; i++) {
		total_y += chan_width_y[i];
		av_occ = 0.;
		max_occ = -1;

		for (j = 1; j <= ny; j++) {
			max_occ = max(chany_occ[i][j], max_occ);
			av_occ += chany_occ[i][j];
		}
		av_occ /= ny;
		vpr_printf(TIO_MESSAGE_INFO, "%d\t%d\t%-#9g\t%d\n", i, max_occ, av_occ, chan_width_y[i]);
	}

	vpr_printf(TIO_MESSAGE_INFO, "\nTotal Tracks in X-direction: %d  in Y-direction: %d\n\n", total_x,
			total_y);

	free_matrix(chanx_occ, 1, nx, 0, sizeof(int));
	free_matrix(chany_occ, 0, nx, 1, sizeof(int));
}

static void load_channel_occupancies(int **chanx_occ, int **chany_occ) {

	/* Loads the two arrays passed in with the total occupancy at each of the  *
	 * channel segments in the FPGA.                                           */

	int i, j, inode, inet;
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

	for (inet = 0; inet < num_nets; inet++) {

		if (clb_net[inet].is_global && clb_net[inet].num_sinks != 0) /* Skip global and empty nets. */
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
		vpr_printf(TIO_MESSAGE_ERROR, 
				"in get_num_bends_and_length:  net #%d has no traceback.\n",
				inet);
		exit(1);
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

			if (curr_type != prev_type
					&& (prev_type == CHANX || prev_type == CHANY))
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
	int inet, bends, length, segments, index;
	float av_length;
	int prob_dist_size, i, incr;

	prob_dist_size = nx + ny + 10;
	prob_dist = (float *) my_calloc(prob_dist_size, sizeof(float));
	norm_fac = 0.;

	for (inet = 0; inet < num_nets; inet++) {
		if (clb_net[inet].is_global == FALSE && clb_net[inet].num_sinks != 0) {
			get_num_bends_and_length(inet, &bends, &length, &segments);

			/*  Assign probability to two integer lengths proportionately -- i.e.  *
			 *  if two_point_length = 1.9, add 0.9 of the pins to prob_dist[2] and *
			 *  only 0.1 to prob_dist[1].                                          */

			two_point_length = (float) length
					/ (float) (clb_net[inet].num_sinks);
			index = (int) two_point_length;
			if (index >= prob_dist_size) {

				vpr_printf(TIO_MESSAGE_WARNING, 
						"index (%d) to prob_dist exceeds its allocated size (%d)\n",
						index, prob_dist_size);
				vpr_printf(TIO_MESSAGE_INFO, 
						"Realloc'ing to increase 2-pin wirelen prob distribution array\n");
				incr = index - prob_dist_size + 2;
				prob_dist_size += incr;
				prob_dist = (float *)my_realloc(prob_dist,
						prob_dist_size * sizeof(float));
				for (i = prob_dist_size - incr; i < prob_dist_size; i++)
					prob_dist[i] = 0.0;
			}
			prob_dist[index] += (clb_net[inet].num_sinks)
					* (1 - two_point_length + index);

			index++;
			if (index >= prob_dist_size) {

				vpr_printf(TIO_MESSAGE_WARNING, 
						"Warning: index (%d) to prob_dist exceeds its allocated size (%d)\n",
						index, prob_dist_size);
				vpr_printf(TIO_MESSAGE_INFO, 
						"Realloc'ing to increase 2-pin wirelen prob distribution array\n");
				incr = index - prob_dist_size + 2;
				prob_dist_size += incr;
				prob_dist = (float *)my_realloc(prob_dist,
						prob_dist_size * sizeof(float));
				for (i = prob_dist_size - incr; i < prob_dist_size; i++)
					prob_dist[i] = 0.0;
			}
			prob_dist[index] += (clb_net[inet].num_sinks)
					* (1 - index + two_point_length);

			norm_fac += clb_net[inet].num_sinks;
		}
	}

	/* Normalize so total probability is 1 and print out. */

	vpr_printf(TIO_MESSAGE_INFO, "\nProbability distribution of 2-pin net lengths:\n\n");
	vpr_printf(TIO_MESSAGE_INFO, "Length    p(Lenth)\n");

	av_length = 0;

	for (index = 0; index < prob_dist_size; index++) {
		prob_dist[index] /= norm_fac;
		vpr_printf(TIO_MESSAGE_INFO, "%6d  %10.6f\n", index, prob_dist[index]);
		av_length += prob_dist[index] * index;
	}

	vpr_printf(TIO_MESSAGE_INFO, "\nThe number of 2-pin nets is ;%g;\n", norm_fac);
	vpr_printf(TIO_MESSAGE_INFO, "\nExpected value of 2-pin net length (R) is ;%g;\n", av_length);
	vpr_printf(TIO_MESSAGE_INFO, "\nTotal wire length is ;%g;\n", norm_fac * av_length);

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
						if (clb_net[inet].is_global == FALSE) /* Not a global clock */
							num_inputs_used++;
				}
			}
		}
	}

	lambda = (float) num_inputs_used / (float) num_blocks;
	vpr_printf(TIO_MESSAGE_INFO, "Average lambda (input pins used per clb) is: %g\n", lambda);
}

void print_timing_stats(void) {

	/* Prints critical path delay/fmax, least slack in design, and, for multiple-clock designs, 
	minimum required clock period to meet each constraint, least slack per constraint,
	geometric average clock frequency, and fanout-weighted geometric average clock frequency. */

	int netlist_clock_index = 0, source_clock_domain, sink_clock_domain, 
		clock_domain, fanout, total_fanout = 0, num_netlist_clocks_with_intra_domain_paths = 0;
	float geomean_period = 1, fanout_weighted_geomean_period = 1, critical_path_delay = UNDEFINED, 
		least_slack_in_design = HUGE_POSITIVE_FLOAT;
	boolean found;

	/* Find the "Critical path", which is the minimum clock period required to meet the constraint
	 corresponding to the pair of source and sink clock domains with the least slack in the design.  
	 If the pb_max_internal_delay is greater than this, it becomes the limiting factor on critical
	 path delay, so print that instead, with a special message. */

	for (source_clock_domain = 0; source_clock_domain < num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < num_constrained_clocks; sink_clock_domain++) {
			if (least_slack_in_design > timing_stats->least_slack_per_constraint[source_clock_domain][sink_clock_domain]) {
				least_slack_in_design = timing_stats->least_slack_per_constraint[source_clock_domain][sink_clock_domain];
				critical_path_delay = timing_stats->critical_path_delay[source_clock_domain][sink_clock_domain];
			}
		}
	}
	
	if (pb_max_internal_delay != UNDEFINED && pb_max_internal_delay > critical_path_delay) {
		critical_path_delay = pb_max_internal_delay;
		vpr_printf(TIO_MESSAGE_INFO, "\nCritical path: %g ns\n\t(capped by fmax of block type %s)\n", 
		critical_path_delay * 1e9, pbtype_max_internal_delay->name);
		
	} else {
		vpr_printf(TIO_MESSAGE_INFO, "\nCritical path: %g ns\n", critical_path_delay * 1e9);
	}

	if (num_constrained_clocks <= 1) {
		/* Although critical path delay is always well-defined, it doesn't make sense to talk about fmax for multi-clock circuits */
		vpr_printf(TIO_MESSAGE_INFO, "f_max: %g MHz\n\n", 1e-6 / timing_stats->critical_path_delay[netlist_clock_index][netlist_clock_index]);
	}
	
	/* Also print the least slack in the design */
	vpr_printf(TIO_MESSAGE_INFO, "Least slack in design: %g ns\n\n", least_slack_in_design * 1e9);

	if (num_constrained_clocks > 1) { /* Multiple-clock design */

		/* Print minimum possible clock period to meet each constraint */

		vpr_printf(TIO_MESSAGE_INFO, "\nMinimum possible clock period to meet each constraint (including skew effects):\n");
		for (source_clock_domain = 0; source_clock_domain < num_constrained_clocks; source_clock_domain++) {
			/* Print the clock name and intra-domain constraint. */
			vpr_printf(TIO_MESSAGE_INFO, "%s:\t", constrained_clocks[source_clock_domain].name);
			if (timing_constraint[source_clock_domain][source_clock_domain] > -1e-15) { /* If this domain pair was analysed */
				if (timing_stats->critical_path_delay[source_clock_domain][source_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
					vpr_printf(TIO_MESSAGE_INFO, "to %s: %g ns (%g MHz)", constrained_clocks[source_clock_domain].name, 
						1e9 * timing_stats->critical_path_delay[source_clock_domain][source_clock_domain],
						1e-6 / timing_stats->critical_path_delay[source_clock_domain][source_clock_domain]);
				} else {
					vpr_printf(TIO_MESSAGE_INFO, "to %s: --", constrained_clocks[source_clock_domain].name);
				}
			}
			vpr_printf(TIO_MESSAGE_INFO, "\n");
			/* Then, print all other constraints on separate lines. */
			for (sink_clock_domain = 0; sink_clock_domain < num_constrained_clocks; sink_clock_domain++) {
				if (source_clock_domain == sink_clock_domain) continue; /* already done that */
				if (timing_constraint[source_clock_domain][sink_clock_domain] > -1e-15) { /* If this domain pair was analysed */
					if (timing_stats->critical_path_delay[source_clock_domain][source_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
						vpr_printf(TIO_MESSAGE_INFO, "\tto %s: %g ns (%g MHz)\n", constrained_clocks[sink_clock_domain].name, 
							1e9 * timing_stats->critical_path_delay[source_clock_domain][sink_clock_domain],
							1e-6 / timing_stats->critical_path_delay[source_clock_domain][sink_clock_domain]);
					} else {
						vpr_printf(TIO_MESSAGE_INFO, "to %s: --", constrained_clocks[sink_clock_domain].name);
					}
				}
			}
		}

		/* Print least slack per constraint */

		vpr_printf(TIO_MESSAGE_INFO, "\nLeast slack per constraint (including skew effects):\n");
		for (source_clock_domain = 0; source_clock_domain < num_constrained_clocks; source_clock_domain++) {
			/* Print the clock name and intra-domain least slack. */
			vpr_printf(TIO_MESSAGE_INFO, "%s:\t", constrained_clocks[source_clock_domain].name);
			if (timing_constraint[source_clock_domain][source_clock_domain] > -1e-15) { /* If this domain pair was analysed */
				if (timing_stats->least_slack_per_constraint[source_clock_domain][source_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
					vpr_printf(TIO_MESSAGE_INFO, "\tto %s: %g ns\n", constrained_clocks[source_clock_domain].name, 
						1e9 * timing_stats->least_slack_per_constraint[source_clock_domain][source_clock_domain]);
				} else {
					vpr_printf(TIO_MESSAGE_INFO, "to %s: --", constrained_clocks[source_clock_domain].name);
				}
			}
			vpr_printf(TIO_MESSAGE_INFO, "\n");
			/* Then, print all other slacks on separate lines. */
			for (sink_clock_domain = 0; sink_clock_domain < num_constrained_clocks; sink_clock_domain++) {
				if (source_clock_domain == sink_clock_domain) continue; /* already done that */
				if (timing_constraint[source_clock_domain][sink_clock_domain] > -1e-15) { /* If this domain pair was analysed */
					if (timing_stats->least_slack_per_constraint[source_clock_domain][source_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
						vpr_printf(TIO_MESSAGE_INFO, "\tto %s: %g ns\n", constrained_clocks[sink_clock_domain].name, 
							1e9 * timing_stats->least_slack_per_constraint[source_clock_domain][sink_clock_domain]);
					} else {
						vpr_printf(TIO_MESSAGE_INFO, "to %s: --", constrained_clocks[sink_clock_domain].name);
					}
				}
			}
		}
	
		/* Calculate geometric mean f_max (fanout-weighted and unweighted) from the diagonal (intra-domain) entries of critical_path_delay, 
		excluding domains without intra-domain paths (for which the timing constraint is DO_NOT_ANALYSE). */
		found = FALSE;
		for (clock_domain = 0; clock_domain < num_constrained_clocks; clock_domain++) {
			if (timing_constraint[clock_domain][clock_domain] > -1e-15) {
				geomean_period *= timing_stats->critical_path_delay[clock_domain][clock_domain];
				fanout = constrained_clocks[clock_domain].fanout;
				fanout_weighted_geomean_period *= pow(timing_stats->critical_path_delay[clock_domain][clock_domain], fanout);
				total_fanout += fanout;
				num_netlist_clocks_with_intra_domain_paths++;
				found = TRUE;
			}
		}
		if (found) { /* Only print these if we found at least one clock domain with intra-domain paths. */
			geomean_period = pow(geomean_period, (float) 1/num_netlist_clocks_with_intra_domain_paths);
			fanout_weighted_geomean_period = pow(fanout_weighted_geomean_period, (float) 1/total_fanout);
			/* Convert to MHz */
			vpr_printf(TIO_MESSAGE_INFO, "\nGeometric mean intra-domain period: %g ns (%g MHz)\n", 
				1e9 * geomean_period, 1e-6 / geomean_period);
			vpr_printf(TIO_MESSAGE_INFO, "Fanout-weighted geomean intra-domain period: %g ns (%g MHz)\n", 
				1e9 * fanout_weighted_geomean_period, 1e-6 / fanout_weighted_geomean_period);
		}

		vpr_printf(TIO_MESSAGE_INFO, "\n");
	}
}


