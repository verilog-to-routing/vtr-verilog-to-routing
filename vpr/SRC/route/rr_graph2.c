#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "read_xml_arch_file.h"

#define ALLOW_SWITCH_OFF

/* WMF: May 07 I put this feature in, but on May 09 in my testing phase
 * I found that for Wilton, this feature is bad, since Wilton is already doing
 * a reverse. */
#define ENABLE_REVERSE 0

#define SAME_TRACK -5
#define UN_SET -1
/* Variables below are the global variables shared only amongst the rr_graph *
 ************************************ routines. ******************************/

/* Used to keep my own list of free linked integers, for speed reasons.     */

t_linked_edge *free_edge_list_head = NULL;

/*************************** Variables local to this module *****************/

/* Two arrays below give the rr_node_index of the channel segment at        *
 * (i,j,track) for fast index lookup.                                       */

/* UDSD Modifications by WMF Begin */

/* The sblock_pattern_init_mux_lookup contains the assignment of incoming
 * wires to muxes. More specifically, it only contains the assignment of 
 * M-to-N cases. WMF_BETTER_COMMENTS */

/* UDSD MOdifications by WMF End */

/************************** Subroutines local to this module ****************/

static void get_switch_type(boolean is_from_sbox, boolean is_to_sbox,
		short from_node_switch, short to_node_switch, short switch_types[2]);

static void load_chan_rr_indices(INP int nodes_per_chan, INP int chan_len,
		INP int num_chans, INP t_rr_type type, INP t_seg_details * seg_details,
		INOUTP int *index, INOUTP t_ivec *** indices);

static int get_bidir_track_to_chan_seg(INP struct s_ivec conn_tracks,
		INP t_ivec *** L_rr_node_indices, INP int to_chan, INP int to_seg,
		INP int to_sb, INP t_rr_type to_type, INP t_seg_details * seg_details,
		INP boolean from_is_sbox, INP int from_switch,
		INOUTP boolean * L_rr_edge_done,
		INP enum e_directionality directionality,
		INOUTP struct s_linked_edge **edge_list);

static int get_unidir_track_to_chan_seg(INP boolean is_end_sb,
		INP int from_track, INP int to_chan, INP int to_seg, INP int to_sb,
		INP t_rr_type to_type, INP int nodes_per_chan, INP int L_nx,
		INP int L_ny, INP enum e_side from_side, INP enum e_side to_side,
		INP int Fs_per_side, INP int *opin_mux_size,
		INP short *****sblock_pattern, INP t_ivec *** L_rr_node_indices,
		INP t_seg_details * seg_details, INOUTP boolean * L_rr_edge_done,
		OUTP boolean * Fs_clipped, INOUTP struct s_linked_edge **edge_list);

static int vpr_to_phy_track(INP int itrack, INP int chan_num, INP int seg_num,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality);

static int *get_seg_track_counts(INP int num_sets, INP int num_seg_types,
		INP t_segment_inf * segment_inf, INP boolean use_full_seg_groups);

static int *label_wire_muxes(INP int chan_num, INP int seg_num,
		INP t_seg_details * seg_details, INP int max_len,
		INP enum e_direction dir, INP int nodes_per_chan,
		OUTP int *num_wire_muxes);

static int *label_wire_muxes_for_balance(INP int chan_num, INP int seg_num,
		INP t_seg_details * seg_details, INP int max_len,
		INP enum e_direction direction, INP int nodes_per_chan,
		INP int *num_wire_muxes, INP t_rr_type chan_type,
		INP int *opin_mux_size, INP t_ivec *** L_rr_node_indices);

static int *label_incoming_wires(INP int chan_num, INP int seg_num,
		INP int sb_seg, INP t_seg_details * seg_details, INP int max_len,
		INP enum e_direction dir, INP int nodes_per_chan,
		OUTP int *num_incoming_wires, OUTP int *num_ending_wires);

static int find_label_of_track(int *wire_mux_on_track, int num_wire_muxes,
		int from_track);

/******************** Subroutine definitions *******************************/

/* This assigns tracks (individually or pairs) to segment types.
 * It tries to match requested ratio. If use_full_seg_groups is
 * true, then segments are assigned only in multiples of their
 * length. This is primarily used for making a tileable unidir
 * layout. The effect of using this is that the number of tracks
 * requested will not always be met and the result will sometimes
 * be over and sometimes under.
 * The pattern when using use_full_seg_groups is to keep adding
 * one group of the track type that wants the largest number of
 * groups of tracks. Each time a group is assigned, the types
 * demand is reduced by 1 unit. The process stops when we are
 * no longer less than the requested number of tracks. As a final
 * step, if we were closer to target before last more, undo it
 * and end up with a result that uses fewer tracks than given. */
static int *
get_seg_track_counts(INP int num_sets, INP int num_seg_types,
		INP t_segment_inf * segment_inf, INP boolean use_full_seg_groups) {
	int *result;
	double *demand;
	int i, imax, freq_sum, assigned, size;
	double scale, max, reduce;

	result = (int *) my_malloc(sizeof(int) * num_seg_types);
	demand = (double *) my_malloc(sizeof(double) * num_seg_types);

	/* Scale factor so we can divide by any length
	 * and still use integers */
	scale = 1;
	freq_sum = 0;
	for (i = 0; i < num_seg_types; ++i) {
		scale *= segment_inf[i].length;
		freq_sum += segment_inf[i].frequency;
	}
	reduce = scale * freq_sum;

	/* Init assignments to 0 and set the demand values */
	for (i = 0; i < num_seg_types; ++i) {
		result[i] = 0;
		demand[i] = scale * num_sets * segment_inf[i].frequency;
		if (use_full_seg_groups) {
			demand[i] /= segment_inf[i].length;
		}
	}

	/* Keep assigning tracks until we use them up */
	assigned = 0;
	size = 0;
	imax = 0;
	while (assigned < num_sets) {
		/* Find current maximum demand */
		max = 0;
		for (i = 0; i < num_seg_types; ++i) {
			if (demand[i] > max) {
				imax = i;
				max = demand[i];
			}
		}

		/* Assign tracks to the type and reduce the types demand */
		size = (use_full_seg_groups ? segment_inf[imax].length : 1);
		demand[imax] -= reduce;
		result[imax] += size;
		assigned += size;
	}

	/* Undo last assignment if we were closer to goal without it */
	if ((assigned - num_sets) > (size / 2)) {
		result[imax] -= size;
	}

	/* Free temps */
	if (demand) {
		free(demand);
		demand = NULL;
	}

	/* This must be freed by caller */
	return result;
}

t_seg_details *
alloc_and_load_seg_details(INOUTP int *nodes_per_chan, INP int max_len,
		INP int num_seg_types, INP t_segment_inf * segment_inf,
		INP boolean use_full_seg_groups, INP boolean is_global_graph,
		INP enum e_directionality directionality) {

	/* Allocates and loads the seg_details data structure.  Max_len gives the   *
	 * maximum length of a segment (dimension of array).  The code below tries  *
	 * to:                                                                      *
	 * (1) stagger the start points of segments of the same type evenly;        *
	 * (2) spread out the limited number of connection boxes or switch boxes    *
	 *     evenly along the length of a segment, starting at the segment ends;  *
	 * (3) stagger the connection and switch boxes on different long lines,     *
	 *     as they will not be staggered by different segment start points.     */

	int i, cur_track, ntracks, itrack, length, j, index;
	int wire_switch, opin_switch, fac, num_sets, tmp;
	int group_start, first_track;
	int *sets_per_seg_type = NULL;
	t_seg_details *seg_details = NULL;
	boolean longline;

	/* Unidir tracks are assigned in pairs, and bidir tracks individually */
	if (directionality == BI_DIRECTIONAL) {
		fac = 1;
	} else {
		assert(directionality == UNI_DIRECTIONAL);
		fac = 2;
	}
	assert(*nodes_per_chan % fac == 0);

	/* Map segment type fractions and groupings to counts of tracks */
	sets_per_seg_type = get_seg_track_counts((*nodes_per_chan / fac),
			num_seg_types, segment_inf, use_full_seg_groups);

	/* Count the number tracks actually assigned. */
	tmp = 0;
	for (i = 0; i < num_seg_types; ++i) {
		tmp += sets_per_seg_type[i] * fac;
	}
	assert(use_full_seg_groups || (tmp == *nodes_per_chan));
	*nodes_per_chan = tmp;

	seg_details = (t_seg_details *) my_malloc(
			*nodes_per_chan * sizeof(t_seg_details));

	/* Setup the seg_details data */
	cur_track = 0;
	for (i = 0; i < num_seg_types; ++i) {
		first_track = cur_track;

		num_sets = sets_per_seg_type[i];
		ntracks = fac * num_sets;
		if (ntracks < 1) {
			continue;
		}
		/* Avoid divide by 0 if ntracks */
		longline = segment_inf[i].longline;
		length = segment_inf[i].length;
		if (longline) {
			length = max_len;
		}

		wire_switch = segment_inf[i].wire_switch;
		opin_switch = segment_inf[i].opin_switch;
		assert(
				(wire_switch == opin_switch) || (directionality != UNI_DIRECTIONAL));

		/* Set up the tracks of same type */
		group_start = 0;
		for (itrack = 0; itrack < ntracks; itrack++) {

			/* Remember the start track of the current wire group */
			if ((itrack / fac) % length == 0 && (itrack % fac) == 0) {
				group_start = cur_track;
			}

			seg_details[cur_track].length = length;
			seg_details[cur_track].longline = longline;

			/* Stagger the start points in for each track set. The 
			 * pin mappings should be aware of this when chosing an
			 * intelligent way of connecting pins and tracks.
			 * cur_track is used as an offset so that extra tracks
			 * from different segment types are hopefully better
			 * balanced. */
			seg_details[cur_track].start = (cur_track / fac) % length + 1;

			/* These properties are used for vpr_to_phy_track to determine
			 * * twisting of wires. */
			seg_details[cur_track].group_start = group_start;
			seg_details[cur_track].group_size =
					std::min(ntracks + first_track - group_start,
							length * fac);
			assert(0 == seg_details[cur_track].group_size % fac);
			if (0 == seg_details[cur_track].group_size) {
				seg_details[cur_track].group_size = length * fac;
			}

			/* Setup the cb and sb patterns. Global route graphs can't depopulate cb and sb
			 * since this is a property of a detailed route. */
			seg_details[cur_track].cb = (boolean *) my_malloc(
					length * sizeof(boolean));
			seg_details[cur_track].sb = (boolean *) my_malloc(
					(length + 1) * sizeof(boolean));
			for (j = 0; j < length; ++j) {
				if (is_global_graph) {
					seg_details[cur_track].cb[j] = TRUE;
				} else {
					index = j;

					/* Rotate longline's so they vary across the FPGA */
					if (longline) {
						index = (index + itrack) % length;
					}

					/* Reverse the order for tracks going in DEC_DIRECTION */
					if (itrack % fac == 1) {
						index = (length - 1) - j;
					}

					/* Use the segment's pattern. */
					index = j % segment_inf[i].cb_len;
					seg_details[cur_track].cb[j] = segment_inf[i].cb[index];
				}
			}
			for (j = 0; j < (length + 1); ++j) {
				if (is_global_graph) {
					seg_details[cur_track].sb[j] = TRUE;
				} else {
					index = j;

					/* Rotate longline's so they vary across the FPGA */
					if (longline) {
						index = (index + itrack) % (length + 1);
					}

					/* Reverse the order for tracks going in DEC_DIRECTION */
					if (itrack % fac == 1) {
						index = ((length + 1) - 1) - j;
					}

					/* Use the segment's pattern. */
					index = j % segment_inf[i].sb_len;
					seg_details[cur_track].sb[j] = segment_inf[i].sb[index];
				}
			}

			seg_details[cur_track].Rmetal = segment_inf[i].Rmetal;
			seg_details[cur_track].Cmetal = segment_inf[i].Cmetal;
			//seg_details[cur_track].Cmetal_per_m = segment_inf[i].Cmetal_per_m;

			seg_details[cur_track].wire_switch = wire_switch;
			seg_details[cur_track].opin_switch = opin_switch;

			if (BI_DIRECTIONAL == directionality) {
				seg_details[cur_track].direction = BI_DIRECTION;
			} else {
				assert(UNI_DIRECTIONAL == directionality);
				seg_details[cur_track].direction =
						(itrack % 2) ? DEC_DIRECTION : INC_DIRECTION;
			}

			switch (segment_inf[i].directionality) {
			case UNI_DIRECTIONAL:
				seg_details[cur_track].drivers = SINGLE;
				break;
			case BI_DIRECTIONAL:
				seg_details[cur_track].drivers = MULTI_BUFFERED;
				break;
			}

			seg_details[cur_track].index = i;

			++cur_track;
		}
	} /* End for each segment type. */

	/* free variables */
	free(sets_per_seg_type);

	return seg_details;
}

void free_seg_details(t_seg_details * seg_details, int nodes_per_chan) {

	/* Frees all the memory allocated to an array of seg_details structures. */

	int i;

	for (i = 0; i < nodes_per_chan; i++) {
		free(seg_details[i].cb);
		free(seg_details[i].sb);
	}
	free(seg_details);
}

/* Dumps out an array of seg_details structures to file fname.  Used only   *
 * for debugging.                                                           */
void dump_seg_details(t_seg_details * seg_details, int nodes_per_chan,
		const char *fname) {

	FILE *fp;
	int i, j;
	const char *drivers_names[] = { "multi_buffered", "single" };
	const char *direction_names[] = { "inc_direction", "dec_direction",
			"bi_direction" };

	fp = my_fopen(fname, "w", 0);

	for (i = 0; i < nodes_per_chan; i++) {
		fprintf(fp, "Track: %d.\n", i);
		fprintf(fp, "Length: %d,  Start: %d,  Long line: %d  "
				"wire_switch: %d  opin_switch: %d.\n", seg_details[i].length,
				seg_details[i].start, seg_details[i].longline,
				seg_details[i].wire_switch, seg_details[i].opin_switch);

		fprintf(fp, "Rmetal: %g  Cmetal: %g\n", seg_details[i].Rmetal,
				seg_details[i].Cmetal);

		fprintf(fp, "Direction: %s  Drivers: %s\n",
				direction_names[seg_details[i].direction],
				drivers_names[seg_details[i].drivers]);

		fprintf(fp, "cb list:  ");
		for (j = 0; j < seg_details[i].length; j++)
			fprintf(fp, "%d ", seg_details[i].cb[j]);
		fprintf(fp, "\n");

		fprintf(fp, "sb list: ");
		for (j = 0; j <= seg_details[i].length; j++)
			fprintf(fp, "%d ", seg_details[i].sb[j]);
		fprintf(fp, "\n");

		fprintf(fp, "\n");
	}

	fclose(fp);
}

/* Returns the segment number at which the segment this track lies on        *
 * started.                                                                  */
int get_seg_start(INP t_seg_details * seg_details, INP int itrack,
		INP int chan_num, INP int seg_num) {

	int seg_start, length, start;

	seg_start = 1;
	if (FALSE == seg_details[itrack].longline) {

		length = seg_details[itrack].length;
		start = seg_details[itrack].start;

		/* Start is guaranteed to be between 1 and length.  Hence adding length to *
		 * the quantity in brackets below guarantees it will be nonnegative.       */

		assert(start > 0);
		assert(start <= length);

		/* NOTE: Start points are staggered between different channels.
		 * The start point must stagger backwards as chan_num increases.
		 * Unidirectional routing expects this to allow the N-to-N 
		 * assumption to be made with respect to ending wires in the core. */
		seg_start = seg_num - (seg_num + length + chan_num - start) % length;
		if (seg_start < 1) {
			seg_start = 1;
		}
	}

	return seg_start;
}

int get_seg_end(INP t_seg_details * seg_details, INP int itrack, INP int istart,
		INP int chan_num, INP int seg_max) {
	int len, ofs, end, first_full;

	len = seg_details[itrack].length;
	ofs = seg_details[itrack].start;

	/* Normal endpoint */
	end = istart + len - 1;

	/* If start is against edge it may have been clipped */
	if (1 == istart) {
		/* If the (staggered) startpoint of first full wire wasn't
		 * also 1, we must be the clipped wire */
		first_full = (len - (chan_num % len) + ofs - 1) % len + 1;
		if (first_full > 1) {
			/* then we stop just before the first full seg */
			end = first_full - 1;
		}
	}

	/* Clip against far edge */
	if (end > seg_max) {
		end = seg_max;
	}

	return end;
}

/* Returns the number of tracks to which clb opin #ipin at (i,j) connects.   *
 * Also stores the nodes to which this pin connects in the linked list       *
 * pointed to by *edge_list_ptr.                                             */
int get_bidir_opin_connections(INP int i, INP int j, INP int ipin,
		INP struct s_linked_edge **edge_list, INP int *****opin_to_track_map,
		INP int Fc, INP boolean * L_rr_edge_done,
		INP t_ivec *** L_rr_node_indices, INP t_seg_details * seg_details) {

	int iside, num_conn, ofs, tr_i, tr_j, chan, seg;
	int to_track, to_switch, to_node, iconn;
	int is_connected_track;
	t_type_ptr type;
	t_rr_type to_type;

	type = grid[i][j].type;
	ofs = grid[i][j].offset;

	num_conn = 0;

	/* [0..num_types-1][0..num_pins-1][0..height][0..3][0..Fc-1] */
	for (iside = 0; iside < 4; iside++) {

		/* Figure out coords of channel segment based on side */
		tr_i = ((iside == LEFT) ? (i - 1) : i);
		tr_j = ((iside == BOTTOM) ? (j - 1) : j);

		to_type = ((iside == LEFT) || (iside == RIGHT)) ? CHANY : CHANX;

		chan = ((to_type == CHANX) ? tr_j : tr_i);
		seg = ((to_type == CHANX) ? tr_i : tr_j);

		/* Don't connect where no tracks on fringes */
		if ((tr_i < 0) || (tr_i > nx)) {
			continue;
		}
		if ((tr_j < 0) || (tr_j > ny)) {
			continue;
		}
		if ((CHANX == to_type) && (tr_i < 1)) {
			continue;
		}
		if ((CHANY == to_type) && (tr_j < 1)) {
			continue;
		}

		is_connected_track = FALSE;

		/* Itterate of the opin to track connections */
		for (iconn = 0; iconn < Fc; ++iconn) {
			to_track = opin_to_track_map[type->index][ipin][ofs][iside][iconn];

			/* Skip unconnected connections */
			if (OPEN == to_track || is_connected_track) {
				is_connected_track = TRUE;
				assert(
						OPEN == opin_to_track_map[type-> index][ipin][ofs][iside] [0]);
				continue;
			}

			/* Only connect to wire if there is a CB */
			if (is_cbox(chan, seg, to_track, seg_details, BI_DIRECTIONAL)) {
				to_switch = seg_details[to_track].wire_switch;
				to_node = get_rr_node_index(tr_i, tr_j, to_type, to_track,
						L_rr_node_indices);

				*edge_list = insert_in_edge_list(*edge_list, to_node,
						to_switch);
				L_rr_edge_done[to_node] = TRUE;
				++num_conn;
			}
		}
	}

	return num_conn;
}

int get_unidir_opin_connections(INP int chan, INP int seg, INP int Fc,
		INP t_rr_type chan_type, INP t_seg_details * seg_details,
		INOUTP t_linked_edge ** edge_list_ptr, INOUTP int **Fc_ofs,
		INOUTP boolean * L_rr_edge_done, INP int max_len,
		INP int nodes_per_chan, INP t_ivec *** L_rr_node_indices,
		OUTP boolean * Fc_clipped) {
	/* Gets a linked list of Fc nodes to connect to in given
	 * chan seg.  Fc_ofs is used for the for the opin staggering
	 * pattern. */

	int *inc_muxes = NULL;
	int *dec_muxes = NULL;
	int num_inc_muxes, num_dec_muxes, iconn;
	int inc_inode, dec_inode;
	int inc_mux, dec_mux;
	int inc_track, dec_track;
	int x, y;
	int num_edges;

	*Fc_clipped = FALSE;

	/* Fc is assigned in pairs so check it is even. */
	assert(Fc % 2 == 0);

	/* get_rr_node_indices needs x and y coords. */
	x = ((CHANX == chan_type) ? seg : chan);
	y = ((CHANX == chan_type) ? chan : seg);

	/* Get the lists of possible muxes. */
	inc_muxes = label_wire_muxes(chan, seg, seg_details, max_len, INC_DIRECTION,
			nodes_per_chan, &num_inc_muxes);
	dec_muxes = label_wire_muxes(chan, seg, seg_details, max_len, DEC_DIRECTION,
			nodes_per_chan, &num_dec_muxes);

	/* Clip Fc to the number of muxes. */
	if (((Fc / 2) > num_inc_muxes) || ((Fc / 2) > num_dec_muxes)) {
		*Fc_clipped = TRUE;
		Fc = 2 * std::min(num_inc_muxes, num_dec_muxes);
	}

	/* Assign tracks to meet Fc demand */
	num_edges = 0;
	for (iconn = 0; iconn < (Fc / 2); ++iconn) {
		/* Figure of the next mux to use */
		inc_mux = Fc_ofs[chan][seg] % num_inc_muxes;
		dec_mux = Fc_ofs[chan][seg] % num_dec_muxes;
		++Fc_ofs[chan][seg];

		/* Figure out the track it corresponds to. */
		inc_track = inc_muxes[inc_mux];
		dec_track = dec_muxes[dec_mux];

		/* Figure the inodes of those muxes */
		inc_inode = get_rr_node_index(x, y, chan_type, inc_track,
				L_rr_node_indices);
		dec_inode = get_rr_node_index(x, y, chan_type, dec_track,
				L_rr_node_indices);

		/* Add to the list. */
		if (FALSE == L_rr_edge_done[inc_inode]) {
			L_rr_edge_done[inc_inode] = TRUE;
			*edge_list_ptr = insert_in_edge_list(*edge_list_ptr, inc_inode,
					seg_details[inc_track].opin_switch);
			++num_edges;
		}
		if (FALSE == L_rr_edge_done[dec_inode]) {
			L_rr_edge_done[dec_inode] = TRUE;
			*edge_list_ptr = insert_in_edge_list(*edge_list_ptr, dec_inode,
					seg_details[dec_track].opin_switch);
			++num_edges;
		}
	}

	if (inc_muxes) {
		free(inc_muxes);
		inc_muxes = NULL;
	}
	if (dec_muxes) {
		free(dec_muxes);
		dec_muxes = NULL;
	}

	return num_edges;
}

boolean is_cbox(INP int chan, INP int seg, INP int track,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality) {

	int length, ofs, start_seg;

	length = seg_details[track].length;

	/* Make sure they gave us correct start */
	start_seg = get_seg_start(seg_details, track, chan, seg);

	ofs = seg - start_seg;

	assert(ofs >= 0);
	assert(ofs < length);

	/* If unidir segment that is going backwards, we need to flip the ofs */
	if (DEC_DIRECTION == seg_details[track].direction) {
		ofs = (length - 1) - ofs;
	}

	return seg_details[track].cb[ofs];
}

static void load_chan_rr_indices(INP int nodes_per_chan, INP int chan_len,
		INP int num_chans, INP t_rr_type type, INP t_seg_details * seg_details,
		INOUTP int *index, INOUTP t_ivec *** indices) {
	int chan, seg, track, start, inode;

	indices[type] = (t_ivec **) my_malloc(sizeof(t_ivec *) * num_chans);
	for (chan = 0; chan < num_chans; ++chan) {
		indices[type][chan] = (t_ivec *) my_malloc(sizeof(t_ivec) * chan_len);

		indices[type][chan][0].nelem = 0;
		indices[type][chan][0].list = NULL;

		for (seg = 1; seg < chan_len; ++seg) {
			/* Alloc the track inode lookup list */
			indices[type][chan][seg].nelem = nodes_per_chan;
			indices[type][chan][seg].list = (int *) my_malloc(
					sizeof(int) * nodes_per_chan);
			for (track = 0; track < nodes_per_chan; ++track) {
				indices[type][chan][seg].list[track] = OPEN;
			}
		}
	}

	for (chan = 0; chan < num_chans; ++chan) {
		for (seg = 1; seg < chan_len; ++seg) {
			/* Assign an inode to the starts of tracks */
			for (track = 0; track < indices[type][chan][seg].nelem; ++track) {
				start = get_seg_start(seg_details, track, chan, seg);

				/* If the start of the wire doesn't have a inode, 
				 * assign one to it. */
				inode = indices[type][chan][start].list[track];
				if (OPEN == inode) {
					inode = *index;
					++(*index);

					indices[type][chan][start].list[track] = inode;
				}

				/* Assign inode of start of wire to current position */
				indices[type][chan][seg].list[track] = inode;
			}
		}
	}
}

struct s_ivec ***
alloc_and_load_rr_node_indices(INP int nodes_per_chan, INP int L_nx,
		INP int L_ny, INOUTP int *index, INP t_seg_details * seg_details) {

	/* Allocates and loads all the structures needed for fast lookups of the   *
	 * index of an rr_node.  rr_node_indices is a matrix containing the index  *
	 * of the *first* rr_node at a given (i,j) location.                       */

	int i, j, k, ofs;
	t_ivec ***indices;
	t_ivec tmp;
	t_type_ptr type;

	/* Alloc the lookup table */
	indices = (t_ivec ***) my_malloc(sizeof(t_ivec **) * NUM_RR_TYPES);
	indices[IPIN] = (t_ivec **) my_malloc(sizeof(t_ivec *) * (L_nx + 2));
	indices[SINK] = (t_ivec **) my_malloc(sizeof(t_ivec *) * (L_nx + 2));
	for (i = 0; i <= (L_nx + 1); ++i) {
		indices[IPIN][i] = (t_ivec *) my_malloc(sizeof(t_ivec) * (L_ny + 2));
		indices[SINK][i] = (t_ivec *) my_malloc(sizeof(t_ivec) * (L_ny + 2));
		for (j = 0; j <= (L_ny + 1); ++j) {
			indices[IPIN][i][j].nelem = 0;
			indices[IPIN][i][j].list = NULL;

			indices[SINK][i][j].nelem = 0;
			indices[SINK][i][j].list = NULL;
		}
	}

	/* Count indices for block nodes */
	for (i = 0; i <= (L_nx + 1); i++) {
		for (j = 0; j <= (L_ny + 1); j++) {
			ofs = grid[i][j].offset;
			if (0 == ofs) {
				type = grid[i][j].type;

				/* Load the pin class lookups. The ptc nums for SINK and SOURCE
				 * are disjoint so they can share the list. */
				tmp.nelem = type->num_class;
				tmp.list = NULL;
				if (tmp.nelem > 0) {
					tmp.list = (int *) my_malloc(sizeof(int) * tmp.nelem);
					for (k = 0; k < tmp.nelem; ++k) {
						tmp.list[k] = *index;
						++(*index);
					}
				}
				indices[SINK][i][j] = tmp;

				/* Load the pin lookups. The ptc nums for IPIN and OPIN
				 * are disjoint so they can share the list. */
				tmp.nelem = type->num_pins;
				tmp.list = NULL;
				if (tmp.nelem > 0) {
					tmp.list = (int *) my_malloc(sizeof(int) * tmp.nelem);
					for (k = 0; k < tmp.nelem; ++k) {
						tmp.list[k] = *index;
						++(*index);
					}
				}
				indices[IPIN][i][j] = tmp;
			}
		}
	}

	/* Point offset blocks of a large block to base block */
	for (i = 0; i <= (L_nx + 1); i++) {
		for (j = 0; j <= (L_ny + 1); j++) {
			ofs = grid[i][j].offset;
			if (ofs > 0) {
				/* NOTE: this only supports vertical large blocks */
				indices[SINK][i][j] = indices[SINK][i][j - ofs];
				indices[IPIN][i][j] = indices[IPIN][i][j - ofs];
			}
		}
	}

	/* SOURCE and SINK have unique ptc values so their data can be shared.
	 * IPIN and OPIN have unique ptc values so their data can be shared. */
	indices[SOURCE] = indices[SINK];
	indices[OPIN] = indices[IPIN];

	/* Load the data for x and y channels */
	load_chan_rr_indices(nodes_per_chan, L_nx + 1, L_ny + 1, CHANX, seg_details,
			index, indices);
	load_chan_rr_indices(nodes_per_chan, L_ny + 1, L_nx + 1, CHANY, seg_details,
			index, indices);

	return indices;
}

void free_rr_node_indices(INP t_ivec *** L_rr_node_indices) {
	int i, j, ofs;
	/* This function must unallocate the structure allocated in 
	 * alloc_and_load_rr_node_indices. */
	for (i = 0; i <= (nx + 1); ++i) {
		for (j = 0; j <= (ny + 1); ++j) {
			ofs = grid[i][j].offset;
			if (ofs > 0) {
				/* Vertical large blocks reference is same as offset 0 */
				L_rr_node_indices[SINK][i][j].list = NULL;
				L_rr_node_indices[IPIN][i][j].list = NULL;
				continue;
			}
			if (L_rr_node_indices[SINK][i][j].list != NULL) {
				free(L_rr_node_indices[SINK][i][j].list);
			}
			if (L_rr_node_indices[IPIN][i][j].list != NULL) {
				free(L_rr_node_indices[IPIN][i][j].list);
			}
		}
		free(L_rr_node_indices[SINK][i]);
		free(L_rr_node_indices[IPIN][i]);
	}
	free(L_rr_node_indices[SINK]);
	free(L_rr_node_indices[IPIN]);

	for (i = 0; i < (nx + 1); ++i) {
		for (j = 0; j < (ny + 1); ++j) {
			if (L_rr_node_indices[CHANY][i][j].list != NULL) {
				free(L_rr_node_indices[CHANY][i][j].list);
			}
		}
		free(L_rr_node_indices[CHANY][i]);
	}
	free(L_rr_node_indices[CHANY]);

	for (i = 0; i < (ny + 1); ++i) {
		for (j = 0; j < (nx + 1); ++j) {
			if (L_rr_node_indices[CHANX][i][j].list != NULL) {
				free(L_rr_node_indices[CHANX][i][j].list);
			}
		}
		free(L_rr_node_indices[CHANX][i]);
	}
	free(L_rr_node_indices[CHANX]);

	free(L_rr_node_indices);
}

int get_rr_node_index(int x, int y, t_rr_type rr_type, int ptc,
		t_ivec *** L_rr_node_indices) {
	/* Returns the index of the specified routing resource node.  (x,y) are     *
	 * the location within the FPGA, rr_type specifies the type of resource,    *
	 * and ptc gives the number of this resource.  ptc is the class number,   *
	 * pin number or track number, depending on what type of resource this      *
	 * is.  All ptcs start at 0 and go up to pins_per_clb-1 or the equivalent. *
	 * The order within a clb is:  SOURCEs + SINKs (type->num_class of them); IPINs,  *
	 * and OPINs (pins_per_clb of them); CHANX; and CHANY (nodes_per_chan of    *
	 * each).  For (x,y) locations that point at pads the order is:  type->capacity     *
	 * occurances of SOURCE, SINK, OPIN, IPIN (one for each pad), then one      *
	 * associated channel (if there is a channel at (x,y)).  All IO pads are    *
	 * bidirectional, so while each will be used only as an INPAD or as an      *
	 * OUTPAD, all the switches necessary to do both must be in each pad.       *
	 *                                                                          *
	 * Note that for segments (CHANX and CHANY) of length > 1, the segment is   *
	 * given an rr_index based on the (x,y) location at which it starts (i.e.   *
	 * lowest (x,y) location at which this segment exists).                     *
	 * This routine also performs error checking to make sure the node in       *
	 * question exists.                                                         */

	int iclass, tmp;
	t_type_ptr type;
	t_ivec lookup;

	assert(ptc >= 0);
	assert(x >= 0 && x <= (nx + 1));
	assert(y >= 0 && y <= (ny + 1));

	type = grid[x][y].type;

	/* Currently need to swap x and y for CHANX because of chan, seg convention */
	if (CHANX == rr_type) {
		tmp = x;
		x = y;
		y = tmp;
	}

	/* Start of that block.  */
	lookup = L_rr_node_indices[rr_type][x][y];

	/* Check valid ptc num */
	assert(ptc >= 0);
	assert(ptc < lookup.nelem);

#ifdef DEBUG
	switch (rr_type) {
	case SOURCE:
		assert(ptc < type->num_class);
		assert(type->class_inf[ptc].type == DRIVER);
		break;

	case SINK:
		assert(ptc < type->num_class);
		assert(type->class_inf[ptc].type == RECEIVER);
		break;

	case OPIN:
		assert(ptc < type->num_pins);
		iclass = type->pin_class[ptc];
		assert(type->class_inf[iclass].type == DRIVER);
		break;

	case IPIN:
		assert(ptc < type->num_pins);
		iclass = type->pin_class[ptc];
		assert(type->class_inf[iclass].type == RECEIVER);
		break;

	case CHANX:
	case CHANY:
		break;

	default:
		vpr_printf(TIO_MESSAGE_ERROR, "Bad rr_node passed to get_rr_node_index.\n");
		vpr_printf(TIO_MESSAGE_ERROR, "Request for type=%d ptc=%d at (%d, %d).\n", rr_type, ptc, x, y);
		exit(1);
	}
#endif

	return lookup.list[ptc];
}

int get_track_to_ipins(int seg, int chan, int track,
		t_linked_edge ** edge_list_ptr, t_ivec *** L_rr_node_indices,
		struct s_ivec ****track_to_ipin_lookup, t_seg_details * seg_details,
		enum e_rr_type chan_type, int chan_length, int wire_to_ipin_switch,
		enum e_directionality directionality) {

	/* This counts the fan-out from wire segment (chan, seg, track) to blocks on either side */

	t_linked_edge *edge_list_head;
	int j, pass, iconn, phy_track, end, to_node, max_conn, ipin, side, x, y,
			num_conn;
	t_type_ptr type;
	int off;

	/* End of this wire */
	end = get_seg_end(seg_details, track, seg, chan, chan_length);

	edge_list_head = *edge_list_ptr;
	num_conn = 0;

	for (j = seg; j <= end; j++) {
		if (is_cbox(chan, j, track, seg_details, directionality)) {
			for (pass = 0; pass < 2; ++pass) {
				if (CHANX == chan_type) {
					x = j;
					y = chan + pass;
					side = (0 == pass ? TOP : BOTTOM);
				} else {
					assert(CHANY == chan_type);
					x = chan + pass;
					y = j;
					side = (0 == pass ? RIGHT : LEFT);
				}

				/* PAJ - if the pointed to is an EMPTY then shouldn't look for ipins */
				if (grid[x][y].type == EMPTY_TYPE)
					continue;

				/* Move from logical (straight) to physical (twisted) track index 
				 * - algorithm assigns ipin connections to same physical track index
				 * so that the logical track gets distributed uniformly */
				phy_track = vpr_to_phy_track(track, chan, j, seg_details,
						directionality);

				/* We need the type to find the ipin map for this type */
				type = grid[x][y].type;
				off = grid[x][y].offset;

				max_conn =
						track_to_ipin_lookup[type->index][phy_track][off][side].nelem;
				for (iconn = 0; iconn < max_conn; iconn++) {
					ipin =
							track_to_ipin_lookup[type->index][phy_track][off][side].list[iconn];

					/* Check there is a connection and Fc map isn't wrong */
					assert(type->pinloc[off][side][ipin]);
					assert(type->is_global_pin[ipin] == FALSE);

					to_node = get_rr_node_index(x, y, IPIN, ipin,
							L_rr_node_indices);
					edge_list_head = insert_in_edge_list(edge_list_head,
							to_node, wire_to_ipin_switch);
				}
				num_conn += max_conn;
			}
		}
	}

	*edge_list_ptr = edge_list_head;
	return (num_conn);
}

/* Counts how many connections should be made from this segment to the y-   *
 * segments in the adjacent channels at to_j.  It returns the number of     *
 * connections, and updates edge_list_ptr to point at the head of the       *
 * (extended) linked list giving the nodes to which this segment connects   *
 * and the switch type used to connect to each.                             *
 *                                                                          *
 * An edge is added from this segment to a y-segment if:                    *
 * (1) this segment should have a switch box at that location, or           *
 * (2) the y-segment to which it would connect has a switch box, and the    *
 *     switch type of that y-segment is unbuffered (bidirectional pass      *
 *     transistor).                                                         *
 *                                                                          *
 * For bidirectional:                                                       *
 * If the switch in each direction is a pass transistor (unbuffered), both  *
 * switches are marked as being of the types of the larger (lower R) pass   *
 * transistor.                                                              */
int get_track_to_tracks(INP int from_chan, INP int from_seg, INP int from_track,
		INP t_rr_type from_type, INP int to_seg, INP t_rr_type to_type,
		INP int chan_len, INP int nodes_per_chan, INP int *opin_mux_size,
		INP int Fs_per_side, INP short *****sblock_pattern,
		INOUTP struct s_linked_edge **edge_list,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality,
		INP t_ivec *** L_rr_node_indices, INOUTP boolean * L_rr_edge_done,
		INP struct s_ivec ***switch_block_conn) {
	int num_conn;
	int from_switch, from_end, from_sb, from_first;
	int to_chan, to_sb;
	int start, end;
	struct s_ivec conn_tracks;
	boolean from_is_sbox, is_behind, Fs_clipped;
	enum e_side from_side_a, from_side_b, to_side;

	assert(
			from_seg == get_seg_start(seg_details, from_track, from_chan, from_seg));

	from_switch = seg_details[from_track].wire_switch;
	from_end = get_seg_end(seg_details, from_track, from_seg, from_chan,
			chan_len);
	from_first = from_seg - 1;

	/* Figure out the sides of SB the from_wire will use */
	if (CHANX == from_type) {
		from_side_a = RIGHT;
		from_side_b = LEFT;
	} else {
		assert(CHANY == from_type);
		from_side_a = TOP;
		from_side_b = BOTTOM;
	}

	/* Figure out if the to_wire is connecting to a SB 
	 * that is behind it. */
	is_behind = FALSE;
	if (to_type == from_type) {
		/* If inline, check that they only are trying
		 * to connect at endpoints. */
		assert((to_seg == (from_end + 1)) || (to_seg == (from_seg - 1)));
		if (to_seg > from_end) {
			is_behind = TRUE;
		}
	} else {
		/* If bending, check that they are adjacent to
		 * our channel. */
		assert((to_seg == from_chan) || (to_seg == (from_chan + 1)));
		if (to_seg > from_chan) {
			is_behind = TRUE;
		}
	}

	/* Figure out the side of SB the to_wires will use.
	 * The to_seg and from_chan are in same direction. */
	if (CHANX == to_type) {
		to_side = (is_behind ? RIGHT : LEFT);
	} else {
		assert(CHANY == to_type);
		to_side = (is_behind ? TOP : BOTTOM);
	}

	/* Set the loop bounds */
	start = from_first;
	end = from_end;

	/* If we are connecting in same direction the connection is 
	 * on one of the two sides so clip the bounds to the SB of
	 * interest and proceed normally. */
	if (to_type == from_type) {
		start = (is_behind ? end : start);
		end = start;
	}

	/* Iterate over the SBs */
	num_conn = 0;
	for (from_sb = start; from_sb <= end; ++from_sb) {
		/* Figure out if we are at a sbox */
		from_is_sbox = is_sbox(from_chan, from_seg, from_sb, from_track,
				seg_details, directionality);
		/* end of wire must be an sbox */
		if (from_sb == from_end || from_sb == from_first) {
			from_is_sbox = TRUE; /* Endpoints always default to true */
		}

		/* to_chan is the current segment if different directions,
		 * otherwise to_chan is the from_chan */
		to_chan = from_sb;
		to_sb = from_chan;
		if (from_type == to_type) {
			to_chan = from_chan;
			to_sb = from_sb;
		}

		/* Do the edges going to the left or down */
		if (from_sb < from_end) {
			if (BI_DIRECTIONAL == directionality) {
				conn_tracks =
						switch_block_conn[from_side_a][to_side][from_track];
				num_conn += get_bidir_track_to_chan_seg(conn_tracks,
						L_rr_node_indices, to_chan, to_seg, to_sb, to_type,
						seg_details, from_is_sbox, from_switch, L_rr_edge_done,
						directionality, edge_list);
			}
			if (UNI_DIRECTIONAL == directionality) {
				/* No fanout if no SB. */
				/* We are connecting from the top or right of SB so it
				 * makes the most sense to only there from DEC_DIRECTION wires. */
				if ((from_is_sbox)
						&& (DEC_DIRECTION == seg_details[from_track].direction)) {
					num_conn += get_unidir_track_to_chan_seg(
							(boolean)(from_sb == from_first), from_track, to_chan,
							to_seg, to_sb, to_type, nodes_per_chan, nx, ny,
							from_side_a, to_side, Fs_per_side, opin_mux_size,
							sblock_pattern, L_rr_node_indices, seg_details,
							L_rr_edge_done, &Fs_clipped, edge_list);
				}
			}
		}

		/* Do the edges going to the right or up */
		if (from_sb > from_first) {
			if (BI_DIRECTIONAL == directionality) {
				conn_tracks =
						switch_block_conn[from_side_b][to_side][from_track];
				num_conn += get_bidir_track_to_chan_seg(conn_tracks,
						L_rr_node_indices, to_chan, to_seg, to_sb, to_type,
						seg_details, from_is_sbox, from_switch, L_rr_edge_done,
						directionality, edge_list);
			}
			if (UNI_DIRECTIONAL == directionality) {
				/* No fanout if no SB. */
				/* We are connecting from the bottom or left of SB so it
				 * makes the most sense to only there from INC_DIRECTION wires. */
				if ((from_is_sbox)
						&& (INC_DIRECTION == seg_details[from_track].direction)) {
					num_conn += get_unidir_track_to_chan_seg(
							(boolean)(from_sb == from_end), from_track, to_chan, to_seg,
							to_sb, to_type, nodes_per_chan, nx, ny, from_side_b,
							to_side, Fs_per_side, opin_mux_size, sblock_pattern,
							L_rr_node_indices, seg_details, L_rr_edge_done,
							&Fs_clipped, edge_list);
				}
			}
		}
	}

	return num_conn;
}

static int get_bidir_track_to_chan_seg(INP struct s_ivec conn_tracks,
		INP t_ivec *** L_rr_node_indices, INP int to_chan, INP int to_seg,
		INP int to_sb, INP t_rr_type to_type, INP t_seg_details * seg_details,
		INP boolean from_is_sbox, INP int from_switch,
		INOUTP boolean * L_rr_edge_done,
		INP enum e_directionality directionality,
		INOUTP struct s_linked_edge **edge_list) {
	int iconn, to_track, to_node, to_switch, num_conn, to_x, to_y, i;
	boolean to_is_sbox;
	short switch_types[2];

	/* x, y coords for get_rr_node lookups */
	if (CHANX == to_type) {
		to_x = to_seg;
		to_y = to_chan;
	} else {
		assert(CHANY == to_type);
		to_x = to_chan;
		to_y = to_seg;
	}

	/* Go through the list of tracks we can connect to */
	num_conn = 0;
	for (iconn = 0; iconn < conn_tracks.nelem; ++iconn) {
		to_track = conn_tracks.list[iconn];
		to_node = get_rr_node_index(to_x, to_y, to_type, to_track,
				L_rr_node_indices);

		/* Skip edge if already done */
		if (L_rr_edge_done[to_node]) {
			continue;
		}

		/* Get the switches for any edges between the two tracks */
		to_switch = seg_details[to_track].wire_switch;

		to_is_sbox = is_sbox(to_chan, to_seg, to_sb, to_track, seg_details,
				directionality);
		get_switch_type(from_is_sbox, to_is_sbox, from_switch, to_switch,
				switch_types);

		/* There are up to two switch edges allowed from track to track */
		for (i = 0; i < 2; ++i) {
			/* If the switch_type entry is empty, skip it */
			if (OPEN == switch_types[i]) {
				continue;
			}

			/* Add the edge to the list */
			*edge_list = insert_in_edge_list(*edge_list, to_node,
					switch_types[i]);
			/* Mark the edge as now done */
			L_rr_edge_done[to_node] = TRUE;
			++num_conn;
		}
	}

	return num_conn;
}

static int get_unidir_track_to_chan_seg(INP boolean is_end_sb,
		INP int from_track, INP int to_chan, INP int to_seg, INP int to_sb,
		INP t_rr_type to_type, INP int nodes_per_chan, INP int L_nx,
		INP int L_ny, INP enum e_side from_side, INP enum e_side to_side,
		INP int Fs_per_side, INP int *opin_mux_size,
		INP short *****sblock_pattern, INP t_ivec *** L_rr_node_indices,
		INP t_seg_details * seg_details, INOUTP boolean * L_rr_edge_done,
		OUTP boolean * Fs_clipped, INOUTP struct s_linked_edge **edge_list) {
	int to_track, to_mux, to_node, to_x, to_y, i, max_len, num_labels;
	int sb_x, sb_y, count;
	int *mux_labels = NULL;
	enum e_direction to_dir;
	boolean is_fringe, is_core, is_corner, is_straight;

	/* x, y coords for get_rr_node lookups */
	if (CHANX == to_type) {
		to_x = to_seg;
		to_y = to_chan;
		sb_x = to_sb;
		sb_y = to_chan;
		max_len = L_nx;
	} else {
		assert(CHANY == to_type);
		to_x = to_chan;
		to_y = to_seg;
		sb_x = to_chan;
		sb_y = to_sb;
		max_len = L_ny;
	}

	to_dir = DEC_DIRECTION;
	if (to_sb < to_seg) {
		to_dir = INC_DIRECTION;
	}

	*Fs_clipped = FALSE;

	/* SBs go from (0, 0) to (nx, ny) */
	is_corner = (boolean)(((sb_x < 1) || (sb_x >= L_nx))
			&& ((sb_y < 1) || (sb_y >= L_ny)));
	is_fringe = (boolean)((FALSE == is_corner)
			&& ((sb_x < 1) || (sb_y < 1) || (sb_x >= L_nx) || (sb_y >= L_ny)));
	is_core = (boolean)((FALSE == is_corner) && (FALSE == is_fringe));
	is_straight = (boolean)((from_side == RIGHT && to_side == LEFT)
			|| (from_side == LEFT && to_side == RIGHT)
			|| (from_side == TOP && to_side == BOTTOM)
			|| (from_side == BOTTOM && to_side == TOP));

	/* Ending wires use N-to-N mapping if not fringe or if goes straight */
	if (is_end_sb && (is_core || is_corner || is_straight)) {
		/* Get the list of possible muxes for the N-to-N mapping. */
		mux_labels = label_wire_muxes(to_chan, to_seg, seg_details, max_len,
				to_dir, nodes_per_chan, &num_labels);
	} else {
		assert(is_fringe || !is_end_sb);

		mux_labels = label_wire_muxes_for_balance(to_chan, to_seg, seg_details,
				max_len, to_dir, nodes_per_chan, &num_labels, to_type,
				opin_mux_size, L_rr_node_indices);
	}

	/* Can't connect if no muxes. */
	if (num_labels < 1) {
		if (mux_labels) {
			free(mux_labels);
			mux_labels = NULL;
		}
		return 0;
	}

	/* Check if Fs demand was too high. */
	if (Fs_per_side > num_labels) {
		*Fs_clipped = TRUE;
	}

	/* Get the target label */
	to_mux = sblock_pattern[sb_x][sb_y][from_side][to_side][from_track];
	assert(to_mux != UN_SET);

	/* Handle Fs > 3 but assigning consecutive muxes. */
	count = 0;
	for (i = 0; i < Fs_per_side; ++i) {
		/* Use the balanced labeling for passing and fringe wires */
		to_track = mux_labels[(to_mux + i) % num_labels];
		to_node = get_rr_node_index(to_x, to_y, to_type, to_track,
				L_rr_node_indices);

		/* Add edge to list. */
		if (FALSE == L_rr_edge_done[to_node]) {
			L_rr_edge_done[to_node] = TRUE;
			*edge_list = insert_in_edge_list(*edge_list, to_node,
					seg_details[to_track].wire_switch);
			++count;
		}
	}

	if (mux_labels) {
		free(mux_labels);
		mux_labels = NULL;
	}

	return count;
}

boolean is_sbox(INP int chan, INP int wire_seg, INP int sb_seg, INP int track,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality) {

	int length, ofs, fac;

	fac = 1;
	if (UNI_DIRECTIONAL == directionality) {
		fac = 2;
	}

	length = seg_details[track].length;

	/* Make sure they gave us correct start */
	wire_seg = get_seg_start(seg_details, track, chan, wire_seg);

	ofs = sb_seg - wire_seg + 1; /* Ofset 0 is behind us, so add 1 */

	assert(ofs >= 0);
	assert(ofs < (length + 1));

	/* If unidir segment that is going backwards, we need to flip the ofs */
	if ((ofs % fac) > 0) {
		ofs = length - ofs;
	}

	return seg_details[track].sb[ofs];
}

static void get_switch_type(boolean is_from_sbox, boolean is_to_sbox,
		short from_node_switch, short to_node_switch, short switch_types[2]) {
	/* This routine looks at whether the from_node and to_node want a switch,  *
	 * and what type of switch is used to connect *to* each type of node       *
	 * (from_node_switch and to_node_switch).  It decides what type of switch, *
	 * if any, should be used to go from from_node to to_node.  If no switch   *
	 * should be inserted (i.e. no connection), it returns OPEN.  Its returned *
	 * values are in the switch_types array.  It needs to return an array      *
	 * because one topology (a buffer in the forward direction and a pass      *
	 * transistor in the backward direction) results in *two* switches.        */

	boolean forward_pass_trans;
	boolean backward_pass_trans;
	int used, min_switch, max_switch;

	switch_types[0] = OPEN; /* No switch */
	switch_types[1] = OPEN;
	used = 0;
	forward_pass_trans = FALSE;
	backward_pass_trans = FALSE;

	/* Connect forward if we are a sbox */
	if (is_from_sbox) {
		switch_types[used] = to_node_switch;
		if (FALSE == switch_inf[to_node_switch].buffered) {
			forward_pass_trans = TRUE;
		}
		++used;
	}

	/* Check for pass_trans coming backwards */
	if (is_to_sbox) {
		if (FALSE == switch_inf[from_node_switch].buffered) {
			switch_types[used] = from_node_switch;
			backward_pass_trans = TRUE;
			++used;
		}
	}

	/* Take the larger pass trans if there are two */
	if (forward_pass_trans && backward_pass_trans) {
		min_switch = std::min(to_node_switch, from_node_switch);
		max_switch = std::max(to_node_switch, from_node_switch);

		/* Take the smaller index unless the other 
		 * pass_trans is bigger (smaller R). */
		switch_types[used] = min_switch;
		if (switch_inf[max_switch].R < switch_inf[min_switch].R) {
			switch_types[used] = max_switch;
		}
		++used;
	}
}

static int vpr_to_phy_track(INP int itrack, INP int chan_num, INP int seg_num,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality) {
	int group_start, group_size;
	int vpr_offset_for_first_phy_track;
	int vpr_offset, phy_offset;
	int phy_track;
	int fac;

	/* Assign in pairs if unidir. */
	fac = 1;
	if (UNI_DIRECTIONAL == directionality) {
		fac = 2;
	}

	group_start = seg_details[itrack].group_start;
	group_size = seg_details[itrack].group_size;

	vpr_offset_for_first_phy_track = (chan_num + seg_num - 1)
			% (group_size / fac);
	vpr_offset = (itrack - group_start) / fac;
	phy_offset = (vpr_offset_for_first_phy_track + vpr_offset)
			% (group_size / fac);
	phy_track = group_start + (fac * phy_offset) + (itrack - group_start) % fac;

	return phy_track;
}

short *****
alloc_sblock_pattern_lookup(INP int L_nx, INP int L_ny, INP int nodes_per_chan) {
	int i, j, from_side, to_side, itrack, items;
	short *****result;
	short *****i_list;
	short ****j_list;
	short ***from_list;
	short **to_list;
	short *track_list;

	/* loading up the sblock connection pattern matrix. It's a huge matrix because
	 * for nonquantized W, it's impossible to make simple permutations to figure out
	 * where muxes are and how to connect to them such that their sizes are balanced */

	/* Do chunked allocations to make freeing easier, speed up malloc and free, and
	 * reduce some of the memory overhead. Could use fewer malloc's but this way
	 * avoids all considerations of pointer sizes and allignment. */

	/* Alloc each list of pointers in one go. items is a running product that increases
	 * with each new dimension of the matrix. */
	items = 1;
	items *= (L_nx + 1);
	i_list = (short *****) my_malloc(sizeof(short ****) * items);
	items *= (L_ny + 1);
	j_list = (short ****) my_malloc(sizeof(short ***) * items);
	items *= (4);
	from_list = (short ***) my_malloc(sizeof(short **) * items);
	items *= (4);
	to_list = (short **) my_malloc(sizeof(short *) * items);
	items *= (nodes_per_chan);
	track_list = (short *) my_malloc(sizeof(short) * items);

	/* Build the pointer lists to form the multidimensional array */
	result = i_list;
	i_list += (L_nx + 1); /* Skip forward nx+1 items */
	for (i = 0; i < (L_nx + 1); ++i) {

		result[i] = j_list;
		j_list += (L_ny + 1); /* Skip forward ny+1 items */
		for (j = 0; j < (L_ny + 1); ++j) {

			result[i][j] = from_list;
			from_list += (4); /* Skip forward 4 items */
			for (from_side = 0; from_side < 4; ++from_side) {

				result[i][j][from_side] = to_list;
				to_list += (4); /* Skip forward 4 items */
				for (to_side = 0; to_side < 4; ++to_side) {

					result[i][j][from_side][to_side] = track_list;
					track_list += (nodes_per_chan); /* Skip forward nodes_per_chan items */
					for (itrack = 0; itrack < nodes_per_chan; itrack++) {

						/* Set initial value to be unset */
						result[i][j][from_side][to_side][itrack] = UN_SET;
					}
				}
			}
		}
	}

	/* This is the outer pointer to the full matrix */
	return result;
}

void free_sblock_pattern_lookup(INOUTP short *****sblock_pattern) {
	/* This free function corresponds to the chunked matrix 
	 * allocation above and there should only be one free
	 * call for each dimension. */

	/* Free dimensions from the inner one, outwards so
	 * we can still access them. The comments beside
	 * each one indicate the corresponding name used when
	 * allocating them. */
	free(****sblock_pattern); /* track_list */
	free(***sblock_pattern); /* to_list */
	free(**sblock_pattern); /* from_list */
	free(*sblock_pattern); /* j_list */
	free(sblock_pattern); /* i_list */
}

void load_sblock_pattern_lookup(INP int i, INP int j, INP int nodes_per_chan,
		INP t_seg_details * seg_details, INP int Fs,
		INP enum e_switch_block_type switch_block_type,
		INOUTP short *****sblock_pattern) {

	/* This routine loads a lookup table for sblock topology. The lookup table is huge
	 * because the sblock varies from location to location. The i, j means the owning
	 * location of the sblock under investigation. */

	int side_cw_incoming_wire_count, side_ccw_incoming_wire_count,
			opp_incoming_wire_count;
	int to_side, side, side_cw, side_ccw, side_opp, itrack;
	int Fs_per_side, chan, seg, chan_len, sb_seg;
	boolean is_core_sblock, is_corner_sblock, x_edge, y_edge;
	int *incoming_wire_label[4];
	int *wire_mux_on_track[4];
	int num_incoming_wires[4];
	int num_ending_wires[4];
	int num_wire_muxes[4];
	boolean skip, vert, pos_dir;
	enum e_direction dir;

	Fs_per_side = 1;
	if (Fs != -1) {
		Fs_per_side = Fs / 3;
	}

	/* SB's have coords from (0, 0) to (nx, ny) */
	assert(i >= 0);
	assert(i <= nx);
	assert(j >= 0);
	assert(j <= ny);

	/* May 12 - 15, 2007
	 *
	 * I identify three types of sblocks in the chip: 1) The core sblock, whose special
	 * property is that the number of muxes (and ending wires) on each side is the same (very useful
	 * property, since it leads to a N-to-N assignment problem with ending wires). 2) The corner sblock
	 * which is same as a L=1 core sblock with 2 sides only (again N-to-N assignment problem). 3) The
	 * fringe / chip edge sblock which is most troublesome, as balance in each side of muxes is 
	 * attainable but balance in the entire sblock is not. The following code first identifies the
	 * incoming wires, which can be classified into incoming passing wires with sbox and incoming
	 * ending wires (the word "incoming" is sometimes dropped for ease of discussion). It appropriately 
	 * labels all the wires on each side by the following order: By the call to label_incoming_wires, 
	 * which labels for one side, the order is such that the incoming ending wires (always with sbox) 
	 * are labelled first 0,1,2,... p-1, then the incoming passing wires with sbox are labelled 
	 * p,p+1,p+2,... k-1 (for total of k). By this convention, one can easily distinguish the ending
	 * wires from the passing wires by checking a label against num_ending_wires variable. 
	 *
	 * After labelling all the incoming wires, this routine labels the muxes on the side we're currently
	 * connecting to (iterated for four sides of the sblock), called the to_side. The label scheme is
	 * the natural order of the muxes by their track #. Also we find the number of muxes.
	 *
	 * For each to_side, the total incoming wires that connect to the muxes on to_side 
	 * come from three sides: side_1 (to_side's right), side_2 (to_side's left) and opp_side.
	 * The problem of balancing mux size is then: considering all incoming passing wires
	 * with sbox on side_1, side_2 and opp_side, how to assign them to the muxes on to_side
	 * (with specified Fs) in a way that mux size is imbalanced by at most 1. I solve this
	 * problem by this approach: the first incoming passing wire will connect to 0, 1, 2, 
	 * ..., Fs_per_side - 1, then the next incoming passing wire will connect to 
	 * Fs_per_side, Fs_per_side+1, ..., Fs_per_side*2-1, and so on. This consistent STAGGERING
	 * ensures N-to-N assignment is perfectly balanced and M-to-N assignment is imbalanced by no
	 * more than 1.
	 *
	 * For the sblock_pattern_init_mux_lookup lookup table, I will only need the lookup
	 * table to remember the first/init mux to connect, since the convention is Fs_per_side consecutive
	 * muxes to connect. Then how do I determine the order of the incoming wires? I use the labels
	 * on side_1, then labels on side_2, then labels on opp_side. Effectively I listed all
	 * incoming passing wires from the three sides, and order them to each make Fs_per_side
	 * consecutive connections to muxes, and use % to rotate to keep imbalance at most 1. 
	 */

	/* SB's range from (0, 0) to (nx, ny) */
	/* First find all four sides' incoming wires */
	x_edge = (boolean)((i < 1) || (i >= nx));
	y_edge = (boolean)((j < 1) || (j >= ny));

	is_corner_sblock = (boolean)(x_edge && y_edge);
	is_core_sblock = (boolean)(!x_edge && !y_edge);

	/* "Label" the wires around the switch block by connectivity. */
	for (side = 0; side < 4; ++side) {
		/* Assume the channel segment doesn't exist. */
		wire_mux_on_track[side] = NULL;
		incoming_wire_label[side] = NULL;
		num_incoming_wires[side] = 0;
		num_ending_wires[side] = 0;
		num_wire_muxes[side] = 0;

		/* Skip the side and leave the zero'd value if the
		 * channel segment doesn't exist. */
		skip = TRUE;
		switch (side) {
		case TOP:
			if (j < ny) {
				skip = FALSE;
			}
			;
			break;
		case RIGHT:
			if (i < nx) {
				skip = FALSE;
			}
			break;
		case BOTTOM:
			if (j > 0) {
				skip = FALSE;
			}
			break;
		case LEFT:
			if (i > 0) {
				skip = FALSE;
			}
			break;
		}
		if (skip) {
			continue;
		}

		/* Figure out the channel and segment for a certain direction */
		vert = (boolean) ((side == TOP) || (side == BOTTOM));
		pos_dir = (boolean) ((side == TOP) || (side == RIGHT));
		chan = (vert ? i : j);
		sb_seg = (vert ? j : i);
		seg = (pos_dir ? (sb_seg + 1) : sb_seg);
		chan_len = (vert ? ny : nx);

		/* Figure out all the tracks on a side that are ending and the
		 * ones that are passing through and have a SB. */
		dir = (pos_dir ? DEC_DIRECTION : INC_DIRECTION);
		incoming_wire_label[side] = label_incoming_wires(chan, seg, sb_seg,
				seg_details, chan_len, dir, nodes_per_chan,
				&num_incoming_wires[side], &num_ending_wires[side]);

		/* Figure out all the tracks on a side that are starting. */
		dir = (pos_dir ? INC_DIRECTION : DEC_DIRECTION);
		wire_mux_on_track[side] = label_wire_muxes(chan, seg, seg_details,
				chan_len, dir, nodes_per_chan, &num_wire_muxes[side]);
	}

	for (to_side = 0; to_side < 4; to_side++) {
		/* Can't do anything if no muxes on this side. */
		if (0 == num_wire_muxes[to_side]) {
			continue;
		}

		/* Figure out side rotations */
		assert((TOP == 0) && (RIGHT == 1) && (BOTTOM == 2) && (LEFT == 3));
		side_cw = (to_side + 1) % 4;
		side_opp = (to_side + 2) % 4;
		side_ccw = (to_side + 3) % 4;

		/* For the core sblock:
		 * The new order for passing wires should appear as
		 * 0,1,2..,scw-1, for passing wires with sbox on side_cw 
		 * scw,scw+1,...,sccw-1, for passing wires with sbox on side_ccw
		 * sccw,sccw+1,... for passing wires with sbox on side_opp.
		 * This way, I can keep the imbalance to at most 1.
		 *
		 * For the fringe sblocks, I don't distinguish between
		 * passing and ending wires so the above statement still holds
		 * if you replace "passing" by "incoming" */

		side_cw_incoming_wire_count = 0;
		if (incoming_wire_label[side_cw]) {
			for (itrack = 0; itrack < nodes_per_chan; itrack++) {
				/* Ending wire, or passing wire with sbox. */
				if (incoming_wire_label[side_cw][itrack] != UN_SET) {

					if ((is_corner_sblock || is_core_sblock)
							&& (incoming_wire_label[side_cw][itrack]
									< num_ending_wires[side_cw])) {
						/* The ending wires in core sblocks form N-to-N assignment 
						 * problem, so can use any pattern such as Wilton. This N-to-N 
						 * mapping depends on the fact that start points stagger across
						 * channels. */
						assert(
								num_ending_wires[side_cw] == num_wire_muxes[to_side]);
						sblock_pattern[i][j][side_cw][to_side][itrack] =
								get_simple_switch_block_track((enum e_side)side_cw, (enum e_side)to_side,
										incoming_wire_label[side_cw][itrack],
										switch_block_type,
										num_wire_muxes[to_side]);

					} else {

						/* These are passing wires with sbox only for core sblocks
						 * or passing and ending wires (for fringe cases). */
						sblock_pattern[i][j][side_cw][to_side][itrack] =
								(side_cw_incoming_wire_count * Fs_per_side)
										% num_wire_muxes[to_side];
						side_cw_incoming_wire_count++;
					}
				}
			}
		}

		side_ccw_incoming_wire_count = 0;
		for (itrack = 0; itrack < nodes_per_chan; itrack++) {

			/* if that side has no channel segment skip it */
			if (incoming_wire_label[side_ccw] == NULL)
				break;

			/* not ending wire nor passing wire with sbox */
			if (incoming_wire_label[side_ccw][itrack] != UN_SET) {

				if ((is_corner_sblock || is_core_sblock)
						&& (incoming_wire_label[side_ccw][itrack]
								< num_ending_wires[side_ccw])) {
					/* The ending wires in core sblocks form N-to-N assignment problem, so can
					 * use any pattern such as Wilton */
					assert(
							incoming_wire_label[side_ccw] [itrack] < num_wire_muxes[to_side]);
					sblock_pattern[i][j][side_ccw][to_side][itrack] =
							get_simple_switch_block_track((enum e_side)side_ccw, (enum e_side)to_side,
									incoming_wire_label[side_ccw][itrack],
									switch_block_type, num_wire_muxes[to_side]);
				} else {

					/* These are passing wires with sbox only for core sblocks
					 * or passing and ending wires (for fringe cases). */
					sblock_pattern[i][j][side_ccw][to_side][itrack] =
							((side_ccw_incoming_wire_count
									+ side_cw_incoming_wire_count) * Fs_per_side)
									% num_wire_muxes[to_side];
					side_ccw_incoming_wire_count++;
				}
			}
		}

		opp_incoming_wire_count = 0;
		if (incoming_wire_label[side_opp]) {
			for (itrack = 0; itrack < nodes_per_chan; itrack++) {
				/* not ending wire nor passing wire with sbox */
				if (incoming_wire_label[side_opp][itrack] != UN_SET) {

					/* corner sblocks for sure have no opposite channel segments so don't care about them */
					if (is_core_sblock) {
						if (incoming_wire_label[side_opp][itrack]
								< num_ending_wires[side_opp]) {
							/* The ending wires in core sblocks form N-to-N assignment problem, so can
							 * use any pattern such as Wilton */
							/* In the direct connect case, I know for sure the init mux is at the same track #
							 * as this ending wire, but still need to find the init mux label for Fs > 3 */
							sblock_pattern[i][j][side_opp][to_side][itrack] =
									find_label_of_track(
											wire_mux_on_track[to_side],
											num_wire_muxes[to_side], itrack);
						} else {
							/* These are passing wires with sbox for core sblocks */
							sblock_pattern[i][j][side_opp][to_side][itrack] =
									((side_ccw_incoming_wire_count
											+ side_cw_incoming_wire_count)
											* Fs_per_side
											+ opp_incoming_wire_count
													* (Fs_per_side - 1))
											% num_wire_muxes[to_side];
							opp_incoming_wire_count++;
						}
					} else {
						if (incoming_wire_label[side_opp][itrack]
								< num_ending_wires[side_opp]) {
							sblock_pattern[i][j][side_opp][to_side][itrack] =
									find_label_of_track(
											wire_mux_on_track[to_side],
											num_wire_muxes[to_side], itrack);
						} else {
							/* These are passing wires with sbox for fringe sblocks */
							sblock_pattern[i][j][side_opp][to_side][itrack] =
									((side_ccw_incoming_wire_count
											+ side_cw_incoming_wire_count)
											* Fs_per_side
											+ opp_incoming_wire_count
													* (Fs_per_side - 1))
											% num_wire_muxes[to_side];
							opp_incoming_wire_count++;
						}
					}
				}
			}
		}
	}

	for (side = 0; side < 4; ++side) {
		if (incoming_wire_label[side]) {
			free(incoming_wire_label[side]);
		}
		if (wire_mux_on_track[side]) {
			free(wire_mux_on_track[side]);
		}
	}
}

static int *
label_wire_muxes_for_balance(INP int chan_num, INP int seg_num,
		INP t_seg_details * seg_details, INP int max_len,
		INP enum e_direction direction, INP int nodes_per_chan,
		INP int *num_wire_muxes, INP t_rr_type chan_type,
		INP int *opin_mux_size, INP t_ivec *** L_rr_node_indices) {

	/* Labels the muxes on that side (seg_num, chan_num, direction). The returned array
	 * maps a label to the actual track #: array[0] = <the track number of the first mux> */

	/* Sblock (aka wire2mux) pattern generation occurs after opin2mux connections have been 
	 * made. Since opin2muxes are done with a pattern with which I guarantee imbalance of at most 1 due 
	 * to them, we will observe that, for each side of an sblock some muxes have one fewer size 
	 * than the others, considering only the contribution from opins. I refer to these muxes as "holes"
	 * as they have one fewer opin connection going to them than the rest (like missing one electron)*/

	/* Before May 14, I was labelling wire muxes in the natural order of their track # (lowest first). 
	 * Now I want to label wire muxes like this: first label the holes in order of their track #,
	 * then label the non-holes in order of their track #. This way the wire2mux generation will 
	 * not overlap its own "holes" with the opin "holes", thus creating imbalance greater than 1. */

	/* The best approach in sblock generation is do one assignment of all incoming wires from 3 other
	 * sides to the muxes on the fourth side, connecting the "opin hole" muxes first (i.e. filling
	 * the holes) then the rest -> this means after all opin2mux and wire2mux connections the 
	 * mux size imbalance on one side is at most 1. The mux size imbalance in one sblock is thus
	 * also one, since the number of muxes per side is identical for all four sides, and they number
	 * of incoming wires per side is identical for full pop, and almost the same for depop (due to 
	 * staggering) within +1 or -1. For different tiles (different sblocks) the imbalance is irrelevant,
	 * since if the tiles are different in mux count then they have to be designed with a different
	 * physical tile. */

	int num_labels, max_opin_mux_size, min_opin_mux_size;
	int inode, i, j, x, y;
	int *pre_labels, *final_labels;

	if (chan_type == CHANX) {
		x = seg_num;
		y = chan_num;
	} else if (chan_type == CHANY) {
		x = chan_num;
		y = seg_num;
	} else {
		vpr_printf(TIO_MESSAGE_ERROR, "Bad channel type (%d).\n", chan_type);
		exit(1);
	}

	/* Generate the normal labels list as the baseline. */
	pre_labels = label_wire_muxes(chan_num, seg_num, seg_details, max_len,
			direction, nodes_per_chan, &num_labels);

	/* Find the min and max mux size. */
	min_opin_mux_size = MAX_SHORT;
	max_opin_mux_size = 0;
	for (i = 0; i < num_labels; ++i) {
		inode = get_rr_node_index(x, y, chan_type, pre_labels[i],
				L_rr_node_indices);
		if (opin_mux_size[inode] < min_opin_mux_size) {
			min_opin_mux_size = opin_mux_size[inode];
		}
		if (opin_mux_size[inode] > max_opin_mux_size) {
			max_opin_mux_size = opin_mux_size[inode];
		}
	}
	if (max_opin_mux_size > (min_opin_mux_size + 1)) {
		vpr_printf(TIO_MESSAGE_ERROR, "opin muxes are not balanced!\n");
		vpr_printf(TIO_MESSAGE_ERROR, "max_opin_mux_size %d min_opin_mux_size %d chan_type %d x %d y %d\n",
				max_opin_mux_size, min_opin_mux_size, chan_type, x, y);
		exit(1);
	}

	/* Create a new list that we will move the muxes with 'holes' to the start of list. */
	final_labels = (int *) my_malloc(sizeof(int) * num_labels);
	j = 0;
	for (i = 0; i < num_labels; ++i) {
		inode = pre_labels[i];
		if (opin_mux_size[inode] < max_opin_mux_size) {
			final_labels[j] = inode;
			++j;
		}
	}
	for (i = 0; i < num_labels; ++i) {
		inode = pre_labels[i];
		if (opin_mux_size[inode] >= max_opin_mux_size) {
			final_labels[j] = inode;
			++j;
		}
	}

	/* Free the baseline labelling. */
	if (pre_labels) {
		free(pre_labels);
		pre_labels = NULL;
	}

	*num_wire_muxes = num_labels;
	return final_labels;
}

static int *
label_wire_muxes(INP int chan_num, INP int seg_num,
		INP t_seg_details * seg_details, INP int max_len,
		INP enum e_direction dir, INP int nodes_per_chan,
		OUTP int *num_wire_muxes) {

	/* Labels the muxes on that side (seg_num, chan_num, direction). The returned array
	 * maps a label to the actual track #: array[0] = <the track number of the first/lowest mux> 
	 * This routine orders wire muxes by their natural order, i.e. track # */

	int itrack, start, end, num_labels, pass;
	int *labels = NULL;
	boolean is_endpoint;

	/* COUNT pass then a LOAD pass */
	num_labels = 0;
	for (pass = 0; pass < 2; ++pass) {
		/* Alloc the list on LOAD pass */
		if (pass > 0) {
			labels = (int *) my_malloc(sizeof(int) * num_labels);
			num_labels = 0;
		}

		/* Find the tracks that are starting. */
		for (itrack = 0; itrack < nodes_per_chan; ++itrack) {
			start = get_seg_start(seg_details, itrack, chan_num, seg_num);
			end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

			/* Skip tracks going the wrong way */
			if (seg_details[itrack].direction != dir) {
				continue;
			}

			/* Determine if we are a wire startpoint */
			is_endpoint = (boolean)(seg_num == start);
			if (DEC_DIRECTION == seg_details[itrack].direction) {
				is_endpoint = (boolean)(seg_num == end);
			}

			/* Count the labels and load if LOAD pass */
			if (is_endpoint) {
				if (pass > 0) {
					labels[num_labels] = itrack;
				}
				++num_labels;
			}
		}
	}

	*num_wire_muxes = num_labels;
	return labels;
}

static int *
label_incoming_wires(INP int chan_num, INP int seg_num, INP int sb_seg,
		INP t_seg_details * seg_details, INP int max_len,
		INP enum e_direction dir, INP int nodes_per_chan,
		OUTP int *num_incoming_wires, OUTP int *num_ending_wires) {

	/* Labels the incoming wires on that side (seg_num, chan_num, direction). 
	 * The returned array maps a track # to a label: array[0] = <the new hash value/label for track 0>,
	 * the labels 0,1,2,.. identify consecutive incoming wires that have sbox (passing wires with sbox and ending wires) */

	int itrack, start, end, i, num_passing, num_ending, pass;
	int *labels;
	boolean sbox_exists, is_endpoint;

	/* Alloc the list of labels for the tracks */
	labels = (int *) my_malloc(nodes_per_chan * sizeof(int));
	for (i = 0; i < nodes_per_chan; ++i) {
		labels[i] = UN_SET; /* crash hard if unset */
	}

	num_ending = 0;
	num_passing = 0;
	for (pass = 0; pass < 2; ++pass) {
		for (itrack = 0; itrack < nodes_per_chan; ++itrack) {
			if (seg_details[itrack].direction == dir) {
				start = get_seg_start(seg_details, itrack, chan_num, seg_num);
				end = get_seg_end(seg_details, itrack, start, chan_num,
						max_len);

				/* Determine if we are a wire endpoint */
				is_endpoint = (boolean)(seg_num == end);
				if (DEC_DIRECTION == seg_details[itrack].direction) {
					is_endpoint = (boolean)(seg_num == start);
				}

				/* Determine if we have a sbox on the wire */
				sbox_exists = is_sbox(chan_num, seg_num, sb_seg, itrack,
						seg_details, UNI_DIRECTIONAL);

				switch (pass) {
				/* On first pass, only load ending wire labels. */
				case 0:
					if (is_endpoint) {
						labels[itrack] = num_ending;
						++num_ending;
					}
					break;

					/* On second pass, load the passing wire labels. They
					 * will follow after the ending wire labels. */
				case 1:
					if ((FALSE == is_endpoint) && sbox_exists) {
						labels[itrack] = num_ending + num_passing;
						++num_passing;
					}
					break;
				}
			}
		}
	}

	*num_incoming_wires = num_passing + num_ending;
	*num_ending_wires = num_ending;
	return labels;
}

static int find_label_of_track(int *wire_mux_on_track, int num_wire_muxes,
		int from_track) {
	int i;

	/* Returns the index/label in array wire_mux_on_track whose entry equals from_track. If none are
	 * found, then returns the index of the entry whose value is the largest */

	for (i = 0; i < num_wire_muxes; i++) {
		if (wire_mux_on_track[i] == from_track) {
			return i; /* matched, return now */
		}
	}

	vpr_printf(TIO_MESSAGE_ERROR, "Expected mux not found.\n");
	exit(1);
}
