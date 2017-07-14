#include <cstdio>
#include <cstring>
#include <cmath>
#include <set>
using namespace std;

#include "vtr_assert.h"
#include "vtr_matrix.h"
#include "vtr_log.h"
#include "vtr_math.h"
#include "vtr_ndmatrix.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "rr_graph_area.h"
#include "segment_stats.h"
#include "stats.h"
#include "net_delay.h"
#include "path_delay.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "endpoint_timing.h"

#include "timing_info.h"
#include "RoutingDelayCalculator.h"

#include "timing_util.h"
#include "VprTimingGraphNameResolver.h"
#include "tatum/TimingReporter.hpp"

/********************** Subroutines local to this module *********************/

static void load_channel_occupancies(vtr::Matrix<int>& chanx_occ, vtr::Matrix<int>& chany_occ);

static void length_and_bends_stats(void);

static void get_channel_occupancy_stats(void);

/************************* Subroutine definitions ****************************/

void routing_stats(bool full_stats, enum e_route_type route_type,
		int num_rr_switch, t_segment_inf * segment_inf, int num_segment,
		float R_minW_nmos, float R_minW_pmos,
        float grid_logic_tile_area,
		enum e_directionality directionality, int wire_to_ipin_switch,
		bool timing_analysis_enabled,
		float **net_delay 
#ifdef ENABLE_CLASSIC_VPR_STA
        , t_slack * slacks, const t_timing_inf &timing_inf
#endif
        ) {

	/* Prints out various statistics about the current routing.  Both a routing *
	 * and an rr_graph must exist when you call this routine.                   */

	float area, used_area;
	int i, j;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

	length_and_bends_stats();
	get_channel_occupancy_stats();

	vtr::printf_info("Logic area (in minimum width transistor areas, excludes I/Os and empty grid tiles)...\n");

	area = 0;
	for (i = 1; i <= device_ctx.nx; i++) {
		for (j = 1; j <= device_ctx.ny; j++) {
			if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
				if (device_ctx.grid[i][j].type->area == UNDEFINED) {
					area += grid_logic_tile_area * device_ctx.grid[i][j].type->width * device_ctx.grid[i][j].type->height;
				} else {
					area += device_ctx.grid[i][j].type->area;
				}
			}
		}
	}
	/* Todo: need to add pitch of routing to blocks with height > 3 */
	vtr::printf_info("\tTotal logic block area (Warning, need to add pitch of routing to blocks with height > 3): %g\n", area);

	used_area = 0;
	for (i = 0; i < (int) cluster_ctx.clb_nlist.blocks().size(); i++) {
		if (cluster_ctx.clb_nlist.block_type((BlockId) i) != device_ctx.IO_TYPE) {
			if (cluster_ctx.clb_nlist.block_type((BlockId)i)->area == UNDEFINED) {
				used_area += grid_logic_tile_area * cluster_ctx.clb_nlist.block_type((BlockId)i)->width * cluster_ctx.clb_nlist.block_type((BlockId)i)->height;
			} else {
				used_area += cluster_ctx.clb_nlist.block_type((BlockId)i)->area;
			}
		}
	}
	vtr::printf_info("\tTotal used logic block area: %g\n", used_area);

	if (route_type == DETAILED) {
		count_routing_transistors(directionality, num_rr_switch, wire_to_ipin_switch, 
				segment_inf, R_minW_nmos, R_minW_pmos);
		get_segment_usage_stats(num_segment, segment_inf);

	}

    if (timing_analysis_enabled) {
        load_net_delay_from_routing(net_delay, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());

        auto routing_delay_calc = std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay);

        std::shared_ptr<SetupTimingInfo> timing_info = make_setup_timing_info(routing_delay_calc);
        timing_info->update();

#ifdef ENABLE_CLASSIC_VPR_STA
        load_timing_graph_net_delays(net_delay);

        do_timing_analysis(slacks, timing_inf, false, true);

        if (getEchoEnabled()) {
            print_timing_graph("routing_stats.timing_graph.classic.echo");
            if (isEchoFileEnabled(E_ECHO_NET_DELAY)) 
                print_net_delay(net_delay, getEchoFileName(E_ECHO_NET_DELAY));
        }

        print_slack(slacks->slack, true, getOutputFileName(E_SLACK_FILE));
        print_critical_path(getOutputFileName(E_CRIT_PATH_FILE), timing_inf);

        if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ENDPOINT_TIMING)) {
            print_endpoint_timing(getEchoFileName(E_ECHO_ENDPOINT_TIMING));
        }

        vtr::printf("Classic Timing Stats\n");
        vtr::printf("====================\n");
        print_timing_stats();
#endif

    }

	if (full_stats == true)
		print_wirelen_prob_dist();
}

void length_and_bends_stats(void) {

	/* Figures out maximum, minimum and average number of bends and net length   *
	 * in the routing.                                                           */

	unsigned int inet, l;
	int bends, total_bends, max_bends;
	int length, total_length, max_length;
	int segments, total_segments, max_segments;
	float av_bends, av_length, av_segments;
	int num_global_nets, num_clb_opins_reserved;

    auto& cluster_ctx = g_vpr_ctx.clustering();

	max_bends = 0;
	total_bends = 0;
	max_length = 0;
	total_length = 0;
	max_segments = 0;
	total_segments = 0;
	num_global_nets = 0;
	num_clb_opins_reserved = 0;

	for (inet = 0, l = cluster_ctx.clbs_nlist.net.size(); inet < l; inet++) {
		if (cluster_ctx.clbs_nlist.net[inet].is_global == false && cluster_ctx.clbs_nlist.net[inet].num_sinks() != 0) { /* Globals don't count. */
			get_num_bends_and_length(inet, &bends, &length, &segments);

			total_bends += bends;
			max_bends = max(bends, max_bends);

			total_length += length;
			max_length = max(length, max_length);

			total_segments += segments;
			max_segments = max(segments, max_segments);
		} else if (cluster_ctx.clbs_nlist.net[inet].is_global) {
			num_global_nets++;
		} else {
			num_clb_opins_reserved++;
		}
	}

	av_bends = (float) total_bends / (float) ((int) cluster_ctx.clbs_nlist.net.size() - num_global_nets);
	vtr::printf_info("\n");
	vtr::printf_info("Average number of bends per net: %#g  Maximum # of bends: %d\n", av_bends, max_bends);
	vtr::printf_info("\n");

	av_length = (float) total_length / (float) ((int) cluster_ctx.clbs_nlist.net.size() - num_global_nets);
	vtr::printf_info("Number of routed nets (nonglobal): %d\n", (int) cluster_ctx.clbs_nlist.net.size() - num_global_nets);
	vtr::printf_info("Wire length results (in units of 1 clb segments)...\n");
	vtr::printf_info("\tTotal wirelength: %d, average net length: %#g\n", total_length, av_length);
	vtr::printf_info("\tMaximum net length: %d\n", max_length);
	vtr::printf_info("\n");

	av_segments = (float) total_segments / (float) ((int) cluster_ctx.clbs_nlist.net.size() - num_global_nets);
	vtr::printf_info("Wire length results in terms of physical segments...\n");
	vtr::printf_info("\tTotal wiring segments used: %d, average wire segments per net: %#g\n", total_segments, av_segments);
	vtr::printf_info("\tMaximum segments used by a net: %d\n", max_segments);
	vtr::printf_info("\tTotal local nets with reserved CLB opins: %d\n", num_clb_opins_reserved);
}

static void get_channel_occupancy_stats(void) {

	/* Determines how many tracks are used in each channel.                    */
    auto& device_ctx = g_vpr_ctx.device();

	auto chanx_occ = vtr::Matrix<int>({{
                                        {1, size_t(device_ctx.nx+1)}, //[1..device_ctx.nx]
                                        {0, size_t(device_ctx.ny+1)}  //[0..device_ctx.ny]
                                      }}, 
                                      0);

	auto chany_occ = vtr::Matrix<int>({{
                                        {0, size_t(device_ctx.nx+1)}, //[0..device_ctx.nx]
                                        {1, size_t(device_ctx.ny+1)}  //[1..device_ctx.ny]
                                      }},
                                      0);
	load_channel_occupancies(chanx_occ, chany_occ);

	vtr::printf_info("\n");
	vtr::printf_info("X - Directed channels:   j max occ ave occ capacity\n");
	vtr::printf_info("                      ---- ------- ------- --------\n");

	int total_x = 0;
	for (int j = 0; j <= device_ctx.ny; ++j) {
		total_x += device_ctx.chan_width.x_list[j];
		float ave_occ = 0.0;
		int max_occ = -1;

		for (int i = 1; i <= device_ctx.nx; ++i) {
			max_occ = max(chanx_occ[i][j], max_occ);
			ave_occ += chanx_occ[i][j];
		}
		ave_occ /= device_ctx.nx;
		vtr::printf_info("                      %4d %7d %7.3f %8d\n", j, max_occ, ave_occ, device_ctx.chan_width.x_list[j]);
	}

	vtr::printf_info("Y - Directed channels:   i max occ ave occ capacity\n");
	vtr::printf_info("                      ---- ------- ------- --------\n");

	int total_y = 0;
	for (int i = 0; i <= device_ctx.nx; ++i) {
		total_y += device_ctx.chan_width.y_list[i];
		float ave_occ = 0.0;
		int max_occ = -1;

		for (int j = 1; j <= device_ctx.ny; ++j) {
			max_occ = max(chany_occ[i][j], max_occ);
			ave_occ += chany_occ[i][j];
		}
		ave_occ /= device_ctx.ny;
		vtr::printf_info("                      %4d %7d %7.3f %8d\n", i, max_occ, ave_occ, device_ctx.chan_width.y_list[i]);
	}

	vtr::printf_info("\n");
	vtr::printf_info("Total tracks in x-direction: %d, in y-direction: %d\n", total_x, total_y);
	vtr::printf_info("\n");
}

static void load_channel_occupancies(vtr::Matrix<int>& chanx_occ, vtr::Matrix<int>& chany_occ) {

	/* Loads the two arrays passed in with the total occupancy at each of the  *
	 * channel segments in the FPGA.                                           */

	int i, j, inode;
	unsigned int inet, l;
	t_trace *tptr;
	t_rr_type rr_type;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

	/* First set the occupancy of everything to zero. */

	for (i = 1; i <= device_ctx.nx; i++)
		for (j = 0; j <= device_ctx.ny; j++)
			chanx_occ[i][j] = 0;

	for (i = 0; i <= device_ctx.nx; i++)
		for (j = 1; j <= device_ctx.ny; j++)
			chany_occ[i][j] = 0;

	/* Now go through each net and count the tracks and pins used everywhere */

	for (inet = 0, l = cluster_ctx.clbs_nlist.net.size(); inet < l; inet++) {
		/* Skip global and empty nets. */
		if (cluster_ctx.clbs_nlist.net[inet].is_global && cluster_ctx.clbs_nlist.net[inet].num_sinks() != 0) 
			continue;

		tptr = route_ctx.trace_head[inet];
		while (tptr != NULL) {
			inode = tptr->index;
			rr_type = device_ctx.rr_nodes[inode].type();

			if (rr_type == SINK) {
				tptr = tptr->next; /* Skip next segment. */
				if (tptr == NULL)
					break;
			}

			else if (rr_type == CHANX) {
				j = device_ctx.rr_nodes[inode].ylow();
				for (i = device_ctx.rr_nodes[inode].xlow(); i <= device_ctx.rr_nodes[inode].xhigh(); i++)
					chanx_occ[i][j]++;
			}

			else if (rr_type == CHANY) {
				i = device_ctx.rr_nodes[inode].xlow();
				for (j = device_ctx.rr_nodes[inode].ylow(); j <= device_ctx.rr_nodes[inode].yhigh(); j++)
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
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();

	t_trace *tptr, *prevptr;
	int inode;
	t_rr_type curr_type, prev_type;
	int bends, length, segments;

	bends = 0;
	length = 0;
	segments = 0;

	prevptr = route_ctx.trace_head[inet]; /* Should always be SOURCE. */
	if (prevptr == NULL) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"in get_num_bends_and_length: net #%d has no traceback.\n", inet);
	}
	inode = prevptr->index;
	prev_type = device_ctx.rr_nodes[inode].type();

	tptr = prevptr->next;

	while (tptr != NULL) {
		inode = tptr->index;
		curr_type = device_ctx.rr_nodes[inode].type();

		if (curr_type == SINK) { /* Starting a new segment */
			tptr = tptr->next; /* Link to existing path - don't add to len. */
			if (tptr == NULL)
				break;

			curr_type = device_ctx.rr_nodes[tptr->index].type();
		}

		else if (curr_type == CHANX || curr_type == CHANY) {
			segments++;
			length += 1 + device_ctx.rr_nodes[inode].xhigh() - device_ctx.rr_nodes[inode].xlow()
					+ device_ctx.rr_nodes[inode].yhigh() - device_ctx.rr_nodes[inode].ylow();

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
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

	float *prob_dist;
	float norm_fac, two_point_length;
	unsigned int inet; 
	int bends, length, segments, index;
	float av_length;
	int prob_dist_size, i, incr;

	prob_dist_size = device_ctx.nx + device_ctx.ny + 10;
	prob_dist = (float *) vtr::calloc(prob_dist_size, sizeof(float));
	norm_fac = 0.;

	for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
		if (cluster_ctx.clbs_nlist.net[inet].is_global == false && cluster_ctx.clbs_nlist.net[inet].num_sinks() != 0) {
			get_num_bends_and_length(inet, &bends, &length, &segments);

			/*  Assign probability to two integer lengths proportionately -- i.e.  *
			 *  if two_point_length = 1.9, add 0.9 of the pins to prob_dist[2] and *
			 *  only 0.1 to prob_dist[1].                                          */

            int num_sinks = cluster_ctx.clbs_nlist.net[inet].num_sinks();
            VTR_ASSERT(num_sinks > 0);

			two_point_length = (float) length / (float) (num_sinks);
			index = (int) two_point_length;
			if (index >= prob_dist_size) {

				vtr::printf_warning(__FILE__, __LINE__,
						"Index (%d) to prob_dist exceeds its allocated size (%d).\n",
						index, prob_dist_size);
				vtr::printf_info("Realloc'ing to increase 2-pin wirelen prob distribution array.\n");
				incr = index - prob_dist_size + 2;
				prob_dist_size += incr;
				prob_dist = (float *)vtr::realloc(prob_dist, prob_dist_size * sizeof(float));
				for (i = prob_dist_size - incr; i < prob_dist_size; i++)
					prob_dist[i] = 0.0;
			}
			prob_dist[index] += (num_sinks) * (1 - two_point_length + index);

			index++;
			if (index >= prob_dist_size) {

				vtr::printf_warning(__FILE__, __LINE__,
						"Index (%d) to prob_dist exceeds its allocated size (%d).\n",
						index, prob_dist_size);
				vtr::printf_info("Realloc'ing to increase 2-pin wirelen prob distribution array.\n");
				incr = index - prob_dist_size + 2;
				prob_dist_size += incr;
				prob_dist = (float *)vtr::realloc(prob_dist,
						prob_dist_size * sizeof(float));
				for (i = prob_dist_size - incr; i < prob_dist_size; i++)
					prob_dist[i] = 0.0;
			}
			prob_dist[index] += (num_sinks) * (1 - index + two_point_length);

			norm_fac += num_sinks;
		}
	}

	/* Normalize so total probability is 1 and print out. */

	vtr::printf_info("\n");
	vtr::printf_info("Probability distribution of 2-pin net lengths:\n");
	vtr::printf_info("\n");
	vtr::printf_info("Length    p(Lenth)\n");

	av_length = 0;

	for (index = 0; index < prob_dist_size; index++) {
		prob_dist[index] /= norm_fac;
		vtr::printf_info("%6d  %10.6f\n", index, prob_dist[index]);
		av_length += prob_dist[index] * index;
	}

	vtr::printf_info("\n");
	vtr::printf_info("Number of 2-pin nets: ;%g;\n", norm_fac);
	vtr::printf_info("Expected value of 2-pin net length (R): ;%g;\n", av_length);
	vtr::printf_info("Total wirelength: ;%g;\n", norm_fac * av_length);

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

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

	for (bnum = 0; bnum < (int) cluster_ctx.clb_nlist.blocks().size(); bnum++) {
		type = cluster_ctx.clb_nlist.block_type((BlockId) bnum);
		VTR_ASSERT(type != NULL);
		if (type != device_ctx.IO_TYPE) {
			for (ipin = 0; ipin < type->num_pins; ipin++) {
				iclass = type->pin_class[ipin];
				if (type->class_inf[iclass].type == RECEIVER) {
					inet = cluster_ctx.blocks[bnum].nets[ipin];
					if (inet != OPEN) /* Pin is connected? */
						if (cluster_ctx.clbs_nlist.net[inet].is_global == false) /* Not a global clock */
							num_inputs_used++;
				}
			}
		}
	}

	lambda = (float) num_inputs_used / (float) cluster_ctx.clb_nlist.blocks().size();
	vtr::printf_info("Average lambda (input pins used per clb) is: %g\n", lambda);
}

int count_netlist_clocks(void) {
	/* Count how many clocks are in the netlist. */

    auto& atom_ctx = g_vpr_ctx.atom();

    std::set<std::string> clock_names;

    //Loop through each clock pin and record the names in clock_names
    for(auto blk_id : atom_ctx.nlist.blocks()) {
        for(auto pin_id : atom_ctx.nlist.block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            clock_names.insert(atom_ctx.nlist.net_name(net_id));
        }
    }

    //Since std::set does not include duplicates, the number of clocks is the size of the set
	return static_cast<int>(clock_names.size());
}
