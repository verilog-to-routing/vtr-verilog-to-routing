#include <cmath>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph_area.h"

/* Select which area equation to use */
enum e_trans_area_eq {AREA_ORIGINAL, AREA_IMPROVED};
static const e_trans_area_eq trans_area_eq = AREA_IMPROVED;

/* Include track buffers or not */
static const boolean include_track_buffers = FALSE;

/************************ Subroutines local to this module *******************/

static void count_bidir_routing_transistors(int num_switch, float R_minW_nmos,
		float R_minW_pmos, const float trans_sram_bit);

static void count_unidir_routing_transistors(t_segment_inf * segment_inf,
		float R_minW_nmos, float R_minW_pmos, const float trans_sram_bit);

static float get_cblock_trans(int *num_inputs_to_cblock,
		int max_inputs_to_cblock, float trans_cblock_to_lblock_buf,
		float trans_sram_bit);

static float *alloc_and_load_unsharable_switch_trans(int num_switch,
		float trans_sram_bit, float R_minW_nmos);

static float *alloc_and_load_sharable_switch_trans(int num_switch,
		float trans_sram_bit, float R_minW_nmos, float R_minW_pmos);

static float trans_per_buf(float Rbuf, float R_minW_nmos, float R_minW_pmos);

static float trans_per_mux(int num_inputs, float trans_sram_bit,
		float pass_trans_area);

static float trans_per_R(float Rtrans, float R_minW_trans);

/*************************** Subroutine definitions **************************/

void count_routing_transistors(enum e_directionality directionality,
		int num_switch, t_segment_inf * segment_inf, float R_minW_nmos,
		float R_minW_pmos) {

	/* Counts how many transistors are needed to implement the FPGA routing      *
	 * resources.  Call this only when an rr_graph exists.  It does not count    *
	 * the transistors used in logic blocks, but it counts the transistors in    *
	 * the input connection block multiplexers and in the output pin drivers and *
	 * pass transistors.  NB:  this routine assumes pass transistors always      *
	 * generate two edges (one forward, one backward) between two nodes.         *
	 * Physically, this is what happens -- make sure your rr_graph does it.      *
	 *                                                                           *
	 * I assume a minimum width transistor takes 1 unit of area.  A double-width *
	 * transistor takes the twice the diffusion width, but the same spacing, so  *
	 * I assume it takes 1.5x the area of a minimum-width transitor.             */
	
	/* Area per SRAM cell (in minimum-width transistor areas) */
	const float trans_sram_bit = 4.; 
	 
	if (directionality == BI_DIRECTIONAL) {
		count_bidir_routing_transistors(num_switch, R_minW_nmos, R_minW_pmos, trans_sram_bit);
	} else {
		assert(directionality == UNI_DIRECTIONAL);
		count_unidir_routing_transistors(segment_inf, R_minW_nmos, R_minW_pmos, trans_sram_bit);
	}
}

void count_bidir_routing_transistors(int num_switch, float R_minW_nmos,
		float R_minW_pmos, const float trans_sram_bit) {

	/* Tri-state buffers are designed as a buffer followed by a pass transistor. *
	 * I make Rbuffer = Rpass_transitor = 1/2 Rtri-state_buffer.                 *
	 * I make the pull-up and pull-down sides of the buffer the same strength -- *
	 * i.e. I make the p transistor R_minW_pmos / R_minW_nmos wider than the n   *
	 * transistor.                                                               *
	 *                                                                           *
	 * I generate two area numbers in this routine:  ntrans_sharing and          *
	 * ntrans_no_sharing.  ntrans_sharing exactly reflects what the timing       *
	 * analyzer, etc. works with -- each switch is a completely self contained   *
	 * pass transistor or tri-state buffer.  In the case of tri-state buffers    *
	 * this is rather pessimisitic.  The inverter chain part of the buffer (as   *
	 * opposed to the pass transistor + SRAM output part) can be shared by       *
	 * several switches in the same location.  Obviously all the switches from   *
	 * an OPIN can share one buffer.  Also, CHANX and CHANY switches at the same *
	 * spot (i,j) on a single segment can share a buffer.  For a more realistic  *
	 * area number I assume all buffered switches from a node that are at the    *
	 * *same (i,j) location* can share one buffer.  Only the lowest resistance   *
	 * (largest) buffer is implemented.  In practice, you might want to build    *
	 * something that is 1.5x or 2x the largest buffer, so this may be a bit     *
	 * optimistic (but I still think it's pretty reasonable).                    */

	int *num_inputs_to_cblock; /* [0..num_rr_nodes-1], but all entries not    */

	/* corresponding to IPINs will be 0.           */

	boolean * cblock_counted; /* [0..max(nx,ny)] -- 0th element unused. */
	float *shared_buffer_trans; /* [0..max_nx,ny)] */
	float *unsharable_switch_trans, *sharable_switch_trans; /* [0..num_switch-1] */

	t_rr_type from_rr_type, to_rr_type;
	int from_node, to_node, iedge, num_edges, maxlen;
	int iswitch, i, j, iseg, max_inputs_to_cblock;
	float input_cblock_trans, shared_opin_buffer_trans;

	/* Two variables below are the accumulator variables that add up all the    *
	 * transistors in the routing.  Make doubles so that they don't stop        *
	 * incrementing once adding a switch makes a change of less than 1 part in  *
	 * 10^7 to the total.  If this still isn't good enough (adding 1 part in    *
	 * 10^15 will still be thrown away), compute the transistor count in        *
	 * "chunks", by adding up inodes 1 to 1000, 1001 to 2000 and then summing   *
	 * the partial sums together.                                               */

	double ntrans_sharing, ntrans_no_sharing;

	/* Buffers from the routing to the ipin cblock inputs, and from the ipin    *
	 * cblock outputs to the logic block, respectively.  Assume minimum size n  *
	 * transistors, and ptransistors sized to make the pull-up R = pull-down R. */

	float trans_track_to_cblock_buf;
	float trans_cblock_to_lblock_buf;

	ntrans_sharing = 0.;
	ntrans_no_sharing = 0.;
	max_inputs_to_cblock = 0;

	/* Assume the two buffers below are 4x minimum drive strength (enough to *
	 * drive a fanout of up to 16 pretty nicely -- should cover a reasonable * 
	 * wiring C plus the fanout.                                             */

	if (include_track_buffers) {
		trans_track_to_cblock_buf = trans_per_buf(R_minW_nmos / 4., R_minW_nmos,
				R_minW_pmos);
	} else {
		trans_track_to_cblock_buf = 0;
	}

	trans_cblock_to_lblock_buf = trans_per_buf(R_minW_nmos / 4., R_minW_nmos,
			R_minW_pmos);

	num_inputs_to_cblock = (int *) my_calloc(num_rr_nodes, sizeof(int));

	maxlen = max(nx, ny) + 1;
	cblock_counted = (boolean *) my_calloc(maxlen, sizeof(boolean));
	shared_buffer_trans = (float *) my_calloc(maxlen, sizeof(float));

	unsharable_switch_trans = alloc_and_load_unsharable_switch_trans(num_switch,
			trans_sram_bit, R_minW_nmos);

	sharable_switch_trans = alloc_and_load_sharable_switch_trans(num_switch,
			trans_sram_bit, R_minW_nmos, R_minW_pmos);

	for (from_node = 0; from_node < num_rr_nodes; from_node++) {

		from_rr_type = rr_node[from_node].type;

		switch (from_rr_type) {

		case CHANX:
		case CHANY:
			num_edges = rr_node[from_node].num_edges;

			for (iedge = 0; iedge < num_edges; iedge++) {

				to_node = rr_node[from_node].edges[iedge];
				to_rr_type = rr_node[to_node].type;

				/* Ignore any uninitialized rr_graph nodes */
				if ((rr_node[to_node].type == SOURCE) 
						&& (rr_node[to_node].xlow == 0) && (rr_node[to_node].ylow == 0)
						&& (rr_node[to_node].xhigh == 0) && (rr_node[to_node].yhigh == 0)) {
					continue;
				}

				switch (to_rr_type) {

				case CHANX:
				case CHANY:
					iswitch = rr_node[from_node].switches[iedge];

					if (switch_inf[iswitch].buffered) {
						iseg = seg_index_of_sblock(from_node, to_node);
						shared_buffer_trans[iseg] = max(shared_buffer_trans[iseg],
								sharable_switch_trans[iswitch]);

						ntrans_no_sharing += unsharable_switch_trans[iswitch]
								+ sharable_switch_trans[iswitch];
						ntrans_sharing += unsharable_switch_trans[iswitch];
					} else if (from_node < to_node) {

						/* Pass transistor shared by two edges -- only count once.  *
						 * Also, no part of a pass transistor is sharable.          */

						ntrans_no_sharing += unsharable_switch_trans[iswitch];
						ntrans_sharing += unsharable_switch_trans[iswitch];
					}
					break;

				case IPIN:
					num_inputs_to_cblock[to_node]++;
					max_inputs_to_cblock = max(max_inputs_to_cblock, 
							num_inputs_to_cblock[to_node]);

					iseg = seg_index_of_cblock(from_rr_type, to_node);

					if (cblock_counted[iseg] == FALSE) {
						cblock_counted[iseg] = TRUE;
						ntrans_sharing += trans_track_to_cblock_buf;
						ntrans_no_sharing += trans_track_to_cblock_buf;
					}
					break;

				default:
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
						"in count_routing_transistors:\n"
						 "\tUnexpected connection from node %d (type %d) to node %d (type %d).\n",
						from_node, from_rr_type, to_node, to_rr_type);
					break;

				} /* End switch on to_rr_type. */

			} /* End for each edge. */

			/* Now add in the shared buffer transistors, and reset some flags. */

			if (from_rr_type == CHANX) {
				for (i = rr_node[from_node].xlow - 1;
						i <= rr_node[from_node].xhigh; i++) {
					ntrans_sharing += shared_buffer_trans[i];
					shared_buffer_trans[i] = 0.;
				}

				for (i = rr_node[from_node].xlow; i <= rr_node[from_node].xhigh;
						i++)
					cblock_counted[i] = FALSE;

			} else { /* CHANY */
				for (j = rr_node[from_node].ylow - 1;
						j <= rr_node[from_node].yhigh; j++) {
					ntrans_sharing += shared_buffer_trans[j];
					shared_buffer_trans[j] = 0.;
				}

				for (j = rr_node[from_node].ylow; j <= rr_node[from_node].yhigh;
						j++)
					cblock_counted[j] = FALSE;

			}
			break;

		case OPIN:
			num_edges = rr_node[from_node].num_edges;
			shared_opin_buffer_trans = 0.;

			for (iedge = 0; iedge < num_edges; iedge++) {
				iswitch = rr_node[from_node].switches[iedge];
				ntrans_no_sharing += unsharable_switch_trans[iswitch]
						+ sharable_switch_trans[iswitch];
				ntrans_sharing += unsharable_switch_trans[iswitch];

				shared_opin_buffer_trans = max(shared_opin_buffer_trans,
						sharable_switch_trans[iswitch]);
			}

			ntrans_sharing += shared_opin_buffer_trans;
			break;

		default:
			break;

		} /* End switch on from_rr_type */
	} /* End for all nodes */

	free(cblock_counted);
	free(shared_buffer_trans);
	free(unsharable_switch_trans);
	free(sharable_switch_trans);

	/* Now add in the input connection block transistors. */

	input_cblock_trans = get_cblock_trans(num_inputs_to_cblock,
			max_inputs_to_cblock, trans_cblock_to_lblock_buf, trans_sram_bit);

	free(num_inputs_to_cblock);

	ntrans_sharing += input_cblock_trans;
	ntrans_no_sharing += input_cblock_trans;

	vpr_printf_info("\n");
	vpr_printf_info("Routing area (in minimum width transistor areas)...\n");
	vpr_printf_info("\tAssuming no buffer sharing (pessimistic). Total: %#g, per logic tile: %#g\n", 
			ntrans_no_sharing, ntrans_no_sharing / (float) (nx * ny));
	vpr_printf_info("\tAssuming buffer sharing (slightly optimistic). Total: %#g, per logic tile: %#g\n", 
			ntrans_sharing, ntrans_sharing / (float) (nx * ny));
	vpr_printf_info("\n");
}

void count_unidir_routing_transistors(t_segment_inf * segment_inf,
		float R_minW_nmos, float R_minW_pmos, const float trans_sram_bit) {
	boolean * cblock_counted; /* [0..max(nx,ny)] -- 0th element unused. */
	int *num_inputs_to_cblock; /* [0..num_rr_nodes-1], but all entries not    */

	/* corresponding to IPINs will be 0.           */

	t_rr_type from_rr_type, to_rr_type;
	int i, j, iseg, from_node, to_node, iedge, num_edges, maxlen;
	int max_inputs_to_cblock, cost_index, seg_type, switch_type;
	float input_cblock_trans;

	/* Two variables below are the accumulator variables that add up all the    *
	 * transistors in the routing.  Make doubles so that they don't stop        *
	 * incrementing once adding a switch makes a change of less than 1 part in  *
	 * 10^7 to the total.  If this still isn't good enough (adding 1 part in    *
	 * 10^15 will still be thrown away), compute the transistor count in        *
	 * "chunks", by adding up inodes 1 to 1000, 1001 to 2000 and then summing   *
	 * the partial sums together.                                               */

	double ntrans;


	/* Buffers from the routing to the ipin cblock inputs, and from the ipin    *
	 * cblock outputs to the logic block, respectively.  Assume minimum size n  *
	 * transistors, and ptransistors sized to make the pull-up R = pull-down R. */

	float trans_track_to_cblock_buf;
	float trans_cblock_to_lblock_buf;

	max_inputs_to_cblock = 0;

	/* Assume the two buffers below are 4x minimum drive strength (enough to *
	 * drive a fanout of up to 16 pretty nicely -- should cover a reasonable * 
	 * wiring C plus the fanout.                                             */

	if (include_track_buffers) {
		trans_track_to_cblock_buf = trans_per_buf(R_minW_nmos / 4., R_minW_nmos,
				R_minW_pmos);
	} else {
		trans_track_to_cblock_buf = 0;
	}

	trans_cblock_to_lblock_buf = trans_per_buf(R_minW_nmos / 4., R_minW_nmos,
			R_minW_pmos);

	num_inputs_to_cblock = (int *) my_calloc(num_rr_nodes, sizeof(int));
	maxlen = max(nx, ny) + 1;
	cblock_counted = (boolean *) my_calloc(maxlen, sizeof(boolean));

	ntrans = 0;
	for (from_node = 0; from_node < num_rr_nodes; from_node++) {

		from_rr_type = rr_node[from_node].type;

		switch (from_rr_type) {

		case CHANX:
		case CHANY:
			num_edges = rr_node[from_node].num_edges;
			cost_index = rr_node[from_node].cost_index;
			seg_type = rr_indexed_data[cost_index].seg_index;
			switch_type = segment_inf[seg_type].wire_switch;
			assert(
					segment_inf[seg_type].wire_switch == segment_inf[seg_type].opin_switch);
			/* With unidir, routing wires and opins both connect through routing mux */
			assert(switch_inf[switch_type].mux_trans_size >= 1);
			/* can't be smaller than min sized transistor */

			assert(rr_node[from_node].num_opin_drivers == 0);
			/* undir has no opin or wire switches */
			assert(rr_node[from_node].num_wire_drivers == 0);
			/* undir has no opin or wire switches */

			/* Each wire segment begins with a multipexer followed by a driver for unidirectional */
			/* Each multiplexer contains all the fan-in to that routing node */
			/* Add up area of multiplexer */
			ntrans += trans_per_mux(rr_node[from_node].fan_in, trans_sram_bit,
					switch_inf[switch_type].mux_trans_size);			

			/* Add up area of buffer */
			/* If buffer size not specified, compute using R otherwise just add to ntrans */
			if (switch_inf[switch_type].buf_size == 0) {
				ntrans += trans_per_buf(switch_inf[switch_type].R, R_minW_nmos,
						R_minW_pmos);
			} else {
				ntrans += switch_inf[switch_type].buf_size;
			}

			/* Increment number of inputs per cblock if IPIN */
			for (iedge = 0; iedge < num_edges; iedge++) {

				to_node = rr_node[from_node].edges[iedge];
				to_rr_type = rr_node[to_node].type;

				/* Ignore any uninitialized rr_graph nodes */
				if ((rr_node[to_node].type == SOURCE) 
						&& (rr_node[to_node].xlow == 0) && (rr_node[to_node].ylow == 0)
						&& (rr_node[to_node].xhigh == 0) && (rr_node[to_node].yhigh == 0)) {
					continue;
				}

				switch (to_rr_type) {

				case CHANX:
				case CHANY:
					break;

				case IPIN:
					num_inputs_to_cblock[to_node]++;
					max_inputs_to_cblock = max(max_inputs_to_cblock,
							num_inputs_to_cblock[to_node]);
					iseg = seg_index_of_cblock(from_rr_type, to_node);

					if (cblock_counted[iseg] == FALSE) {
						cblock_counted[iseg] = TRUE;
						ntrans += trans_track_to_cblock_buf;
					}
					break;

				default:
					vpr_throw(VPR_ERROR_ROUTE,  __FILE__, __LINE__,
						"in count_routing_transistors:\n"
						"\tUnexpected connection from node %d (type %d) to node %d (type %d).\n", 
						from_node, from_rr_type, to_node, to_rr_type);
					break;

				} /* End switch on to_rr_type. */

			} /* End for each edge. */

			/* Reset some flags */
			if (from_rr_type == CHANX) {
				for (i = rr_node[from_node].xlow; i <= rr_node[from_node].xhigh;
						i++)
					cblock_counted[i] = FALSE;

			} else { /* CHANY */
				for (j = rr_node[from_node].ylow; j <= rr_node[from_node].yhigh;
						j++)
					cblock_counted[j] = FALSE;

			}
			break;
		case OPIN:
			break;

		default:
			break;

		} /* End switch on from_rr_type */
	} /* End for all nodes */

	/* Now add in the input connection block transistors. */

	input_cblock_trans = get_cblock_trans(num_inputs_to_cblock,
			max_inputs_to_cblock, trans_cblock_to_lblock_buf, trans_sram_bit);

	free(cblock_counted);
	free(num_inputs_to_cblock);

	ntrans += input_cblock_trans;

	vpr_printf_info("\n");
	vpr_printf_info("Routing area (in minimum width transistor areas)...\n");
	vpr_printf_info("\tTotal routing area: %#g, per logic tile: %#g\n", ntrans, ntrans / (float) (nx * ny));
}

static float get_cblock_trans(int *num_inputs_to_cblock,
		int max_inputs_to_cblock, float trans_cblock_to_lblock_buf,
		float trans_sram_bit) {

	/* Computes the transistors in the input connection block multiplexers and   *
	 * the buffers from connection block outputs to the logic block input pins.  *
	 * For speed, I precompute the number of transistors in the multiplexers of  *
	 * interest.                                                                 */

	float *trans_per_cblock; /* [0..max_inputs_to_cblock] */
	float trans_count;
	int i, num_inputs;
	trans_per_cblock = (float *) my_malloc(
			(max_inputs_to_cblock + 1) * sizeof(float));

	trans_per_cblock[0] = 0.; /* i.e., not an IPIN or no inputs */

	/* With one or more inputs, add the mux and output buffer.  I add the output *
	 * buffer even when the number of inputs = 1 (i.e. no mux) because I assume  *
	 * I need the drivability just for metal capacitance.                        */

	for (i = 1; i <= max_inputs_to_cblock; i++)
		trans_per_cblock[i] = trans_per_mux(i, trans_sram_bit,
				ipin_mux_trans_size) + trans_cblock_to_lblock_buf;

	trans_count = 0.;
	
	for (i = 0; i < num_rr_nodes; i++) {
		num_inputs = num_inputs_to_cblock[i];
		trans_count += trans_per_cblock[num_inputs];
	}
	
	free(trans_per_cblock);
	return (trans_count);
}

static float *
alloc_and_load_unsharable_switch_trans(int num_switch, float trans_sram_bit,
		float R_minW_nmos) {

	/* Loads up an array that says how many transistors are needed to implement  *
	 * the unsharable portion of each switch type.  The SRAM bit of a switch and *
	 * the pass transistor (forming either the entire switch or the output part  *
	 * of a tri-state buffer) are both unsharable.                               */

	float *unsharable_switch_trans, Rpass;
	int i;

	unsharable_switch_trans = (float *) my_malloc(num_switch * sizeof(float));

	for (i = 0; i < num_switch; i++) {

		if (switch_inf[i].buffered == FALSE) {
			Rpass = switch_inf[i].R;
		} else { /* Buffer.  Set Rpass = Rbuf = 1/2 Rtotal. */
			Rpass = switch_inf[i].R / 2.;
		}

		unsharable_switch_trans[i] = trans_per_R(Rpass, R_minW_nmos)
				+ trans_sram_bit;
	}

	return (unsharable_switch_trans);
}

static float *
alloc_and_load_sharable_switch_trans(int num_switch, float trans_sram_bit,
		float R_minW_nmos, float R_minW_pmos) {

	/* Loads up an array that says how many transistor are needed to implement   *
	 * the sharable portion of each switch type.  The SRAM bit of a switch and   *
	 * the pass transistor (forming either the entire switch or the output part  *
	 * of a tri-state buffer) are both unsharable.  Only the buffer part of a    *
	 * buffer switch is sharable.                                                */

	float *sharable_switch_trans, Rbuf;
	int i;

	sharable_switch_trans = (float *) my_malloc(num_switch * sizeof(float));

	for (i = 0; i < num_switch; i++) {

		if (switch_inf[i].buffered == FALSE) {
			sharable_switch_trans[i] = 0.;
		} else { /* Buffer.  Set Rbuf = Rpass = 1/2 Rtotal. */
			Rbuf = switch_inf[i].R / 2.;
			sharable_switch_trans[i] = trans_per_buf(Rbuf, R_minW_nmos,
					R_minW_pmos);
		}
	}

	return (sharable_switch_trans);
}

static float trans_per_buf(float Rbuf, float R_minW_nmos, float R_minW_pmos) {

	/* Returns the number of minimum width transistor area equivalents needed to *
	 * implement this buffer.  Assumes a stage ratio of 4, and equal strength    *
	 * pull-up and pull-down paths.                                              */

	int num_stage, istage;
	float trans_count, stage_ratio, Rstage;

	if (Rbuf > 0.6 * R_minW_nmos || Rbuf <= 0.) { /* Use a single-stage buffer */
		trans_count = trans_per_R(Rbuf, R_minW_nmos)
				+ trans_per_R(Rbuf, R_minW_pmos);
	} else { /* Use a multi-stage buffer */

		/* Target stage ratio = 4.  1 minimum width buffer, then num_stage bigger *
		 * ones.                                                                  */

		num_stage = nint(log10(R_minW_nmos / Rbuf) / log10(4.));
		num_stage = max(num_stage, 1);
		stage_ratio = pow((float)(R_minW_nmos / Rbuf), (float)( 1. / (float) num_stage));

		Rstage = R_minW_nmos;
		trans_count = 0.;

		for (istage = 0; istage <= num_stage; istage++) {
			trans_count += trans_per_R(Rstage, R_minW_nmos)
					+ trans_per_R(Rstage, R_minW_pmos);
			Rstage /= stage_ratio;
		}
	}

	return (trans_count);
}

static float trans_per_mux(int num_inputs, float trans_sram_bit,
		float pass_trans_area) {

	/* Returns the number of transistors needed to build a pass transistor mux. *
	 * DOES NOT include input buffers or any output buffer.                     *
	 * Attempts to select smart multiplexer size depending on number of inputs  *
	 * For multiplexers with inputs 4 or less, one level is used, more has two  *
	 * levels.                                                                  */
	float ntrans, sram_trans, pass_trans;
	int num_second_stage_trans;
	
	if (num_inputs <= 1) {
		return (0);
	} else if (num_inputs == 2) {
		pass_trans = 2 * pass_trans_area;
		sram_trans = 1 * trans_sram_bit;
	} else if (num_inputs <= 4) {
		/* One-hot encoding */
		pass_trans = num_inputs * pass_trans_area;
		sram_trans = num_inputs * trans_sram_bit;
	} else {
		/* This is a large multiplexer so design it using a two-level multiplexer   *
		 * + 0.00001 is to make sure exact square roots two don't get rounded down  *
		 * to one lower level.                                                      */
		num_second_stage_trans = (int)floor((float)sqrt((float)num_inputs) + 0.00001);
		pass_trans = (num_inputs + num_second_stage_trans) * pass_trans_area;
		sram_trans = (ceil(
				(float) num_inputs / num_second_stage_trans - 0.00001)
				+ num_second_stage_trans) * trans_sram_bit;
		if (num_second_stage_trans == 2) {
			/* Can use one-bit instead of a two-bit one-hot encoding for the second stage */
			/* Eliminates one sram bit counted earlier */
			sram_trans -= 1 * trans_sram_bit;
		}
	}

	ntrans = pass_trans + sram_trans;

	return (ntrans);
}

static float trans_per_R(float Rtrans, float R_minW_trans) {

	/* Returns the number of minimum width transistor area equivalents needed    *
	 * to make a transistor with Rtrans, given that the resistance of a minimum  *
	 * width transistor of this type is R_minW_trans.                            */

	float trans_area;

	if (Rtrans <= 0.) /* Assume resistances are nonsense -- use min. width */
		return (1.);

	if (Rtrans >= R_minW_trans)
		return (1.);

	/* Old area model (developed with 0.35um process rules) */
	/* Area = minimum width area (1) + 0.5 for each additional unit of width.  *
	 * The 50% factor takes into account the "overlapping" that occurs in      *
	 * horizontally-paralleled transistors, and the need for only one spacing, *
	 * not two (i.e. two min W transistors need two spaces; a 2W transistor    *
	 * needs only 1).                                                          */

	/* New area model (developed with 65nm process rules) */
	/* These more advanced process rules change how much area we need to add   *
	 * for each additional unit of width vs. the old area model.               */

	if (trans_area_eq == AREA_ORIGINAL) {
		/* Old transistor area estimation equation */
		trans_area = 0.5 * R_minW_trans / Rtrans + 0.5;
	} else {
		/* New transistor area estimation equation */
		float drive_strength;
		drive_strength = R_minW_trans / Rtrans;
		trans_area = 0.447 + 0.128*drive_strength + 0.391*sqrt(drive_strength);
	}

	return (trans_area);
}
