#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "rr_graph_sbox.h"
#include "rr_graph_util.h"
#include "ReadOptions.h"

/* Switch box:                                                             *
 *                    TOP (CHANY)                                          *
 *                    | | | | | |                                          *
 *                   +-----------+                                         *
 *                 --|           |--                                       *
 *                 --|           |--                                       *
 *           LEFT  --|           |-- RIGHT                                 *
 *          (CHANX)--|           |--(CHANX)                                *
 *                 --|           |--                                       *
 *                 --|           |--                                       *
 *                   +-----------+                                         *
 *                    | | | | | |                                          *
 *                   BOTTOM (CHANY)                                        */

/* [0..3][0..3][0..nodes_per_chan-1].  Structure below is indexed as:       *
 * [from_side][to_side][from_track].  That yields an integer vector (ivec)  *
 * of the tracks to which from_track connects in the proper to_location.    *
 * For simple switch boxes this is overkill, but it will allow complicated  *
 * switch boxes with Fs > 3, etc. without trouble.                          */

int get_simple_switch_block_track(INP enum e_side from_side,
		INP enum e_side to_side, INP int from_track,
		INP enum e_switch_block_type switch_block_type, INP int nodes_per_chan);

/* Allocates and loads the switch_block_conn data structure.  This structure *
 * lists which tracks connect to which at each switch block. This is for
 * bidir. */
struct s_ivec ***
alloc_and_load_switch_block_conn(INP int nodes_per_chan,
		INP enum e_switch_block_type switch_block_type, INP int Fs) {
	enum e_side from_side, to_side;
	int from_track;
	struct s_ivec ***switch_block_conn = NULL;

	/* Currently Fs must be 3 since each track maps once to each other side */
	assert(3 == Fs);

	switch_block_conn = (struct s_ivec ***) alloc_matrix3(0, 3, 0, 3, 0,
			(nodes_per_chan - 1), sizeof(struct s_ivec));

	for (from_side = (enum e_side)0; from_side < 4; from_side = (enum e_side)(from_side + 1)) {
		for (to_side = (enum e_side)0; to_side < 4; to_side = (enum e_side)(to_side + 1)) {
			for (from_track = 0; from_track < nodes_per_chan; from_track++) {
				if (from_side != to_side) {
					switch_block_conn[from_side][to_side][from_track].nelem = 1;
					switch_block_conn[from_side][to_side][from_track].list =
							(int *) my_malloc(sizeof(int));

					switch_block_conn[from_side][to_side][from_track].list[0] =
							get_simple_switch_block_track(from_side, to_side,
									from_track, switch_block_type,
									nodes_per_chan);
				} else { /* from_side == to_side -> no connection. */
					switch_block_conn[from_side][to_side][from_track].nelem = 0;
					switch_block_conn[from_side][to_side][from_track].list =
							NULL;
				}
			}
		}
	}

	if (getEchoEnabled()) {
		int i, j, k, l;
		FILE *out;

		out = my_fopen("switch_block_conn.echo", "w", 0);
		for (l = 0; l < 4; ++l) {
			for (k = 0; k < 4; ++k) {
				fprintf(out, "Side %d to %d\n", l, k);
				for (j = 0; j < nodes_per_chan; ++j) {
					fprintf(out, "%d: ", j);
					for (i = 0; i < switch_block_conn[l][k][j].nelem; ++i) {
						fprintf(out, "%d ", switch_block_conn[l][k][j].list[i]);
					}
					fprintf(out, "\n");
				}
				fprintf(out, "\n");
			}
		}
		fclose(out);
	}
	return switch_block_conn;
}

void free_switch_block_conn(struct s_ivec ***switch_block_conn,
		int nodes_per_chan) {
	/* Frees the switch_block_conn data structure. */

	free_ivec_matrix3(switch_block_conn, 0, 3, 0, 3, 0, nodes_per_chan - 1);
}

#define SBOX_ERROR -1

/* This routine permutes the track number to connect for topologies
 * SUBSET, UNIVERSAL, and WILTON. I added FULL (for fully flexible topology)
 * but the returned value is simply a dummy, since we don't need to permute
 * what connections to make for FULL (connect to EVERYTHING) */
int get_simple_switch_block_track(INP enum e_side from_side,
		INP enum e_side to_side, INP int from_track,
		INP enum e_switch_block_type switch_block_type, INP int nodes_per_chan) {

	/* This routine returns the track number to which the from_track should     *
	 * connect.  It supports three simple, Fs = 3, switch blocks.               */

	int to_track;

	to_track = SBOX_ERROR; /* Can check to see if it's not set later. */

	if (switch_block_type == SUBSET) { /* NB:  Global routing uses SUBSET too */
		to_track = from_track;
	}

	/* See S. Wilton Phd thesis, U of T, 1996 p. 103 for details on following. */

	else if (switch_block_type == WILTON) {

		if (from_side == LEFT) {

			if (to_side == RIGHT) { /* CHANX to CHANX */
				to_track = from_track;
			} else if (to_side == TOP) { /* from CHANX to CHANY */
				to_track = (nodes_per_chan - from_track) % nodes_per_chan;
			} else if (to_side == BOTTOM) {
				to_track = (nodes_per_chan + from_track - 1) % nodes_per_chan;
			}
		}

		else if (from_side == RIGHT) {
			if (to_side == LEFT) { /* CHANX to CHANX */
				to_track = from_track;
			} else if (to_side == TOP) { /* from CHANX to CHANY */
				to_track = (nodes_per_chan + from_track - 1) % nodes_per_chan;
			} else if (to_side == BOTTOM) {
				to_track = (2 * nodes_per_chan - 2 - from_track)
						% nodes_per_chan;
			}
		}

		else if (from_side == BOTTOM) {
			if (to_side == TOP) { /* CHANY to CHANY */
				to_track = from_track;
			} else if (to_side == LEFT) { /* from CHANY to CHANX */
				to_track = (from_track + 1) % nodes_per_chan;
			} else if (to_side == RIGHT) {
				to_track = (2 * nodes_per_chan - 2 - from_track)
						% nodes_per_chan;
			}
		}

		else if (from_side == TOP) {
			if (to_side == BOTTOM) { /* CHANY to CHANY */
				to_track = from_track;
			} else if (to_side == LEFT) { /* from CHANY to CHANX */
				to_track = (nodes_per_chan - from_track) % nodes_per_chan;
			} else if (to_side == RIGHT) {
				to_track = (from_track + 1) % nodes_per_chan;
			}
		}

	}
	/* End switch_block_type == WILTON case. */
	else if (switch_block_type == UNIVERSAL) {

		if (from_side == LEFT) {

			if (to_side == RIGHT) { /* CHANX to CHANX */
				to_track = from_track;
			} else if (to_side == TOP) { /* from CHANX to CHANY */
				to_track = nodes_per_chan - 1 - from_track;
			} else if (to_side == BOTTOM) {
				to_track = from_track;
			}
		}

		else if (from_side == RIGHT) {
			if (to_side == LEFT) { /* CHANX to CHANX */
				to_track = from_track;
			} else if (to_side == TOP) { /* from CHANX to CHANY */
				to_track = from_track;
			} else if (to_side == BOTTOM) {
				to_track = nodes_per_chan - 1 - from_track;
			}
		}

		else if (from_side == BOTTOM) {
			if (to_side == TOP) { /* CHANY to CHANY */
				to_track = from_track;
			} else if (to_side == LEFT) { /* from CHANY to CHANX */
				to_track = from_track;
			} else if (to_side == RIGHT) {
				to_track = nodes_per_chan - 1 - from_track;
			}
		}

		else if (from_side == TOP) {
			if (to_side == BOTTOM) { /* CHANY to CHANY */
				to_track = from_track;
			} else if (to_side == LEFT) { /* from CHANY to CHANX */
				to_track = nodes_per_chan - 1 - from_track;
			} else if (to_side == RIGHT) {
				to_track = from_track;
			}
		}
	}

	/* End switch_block_type == UNIVERSAL case. */
	/* UDSD Modification by WMF Begin */
	if (switch_block_type == FULL) { /* Just a placeholder. No meaning in reality */
		to_track = from_track;
	}
	/* UDSD Modification by WMF End */

	if (to_track == SBOX_ERROR) {
		vpr_printf(TIO_MESSAGE_ERROR, "in get_simple_switch_block_track.\n");
		vpr_printf(TIO_MESSAGE_ERROR, "\tUnexpected connection from_side: %d to_side: %d switch_block_type: %d.\n",
				from_side, to_side, switch_block_type);
		exit(1);
	}

	return (to_track);
}
