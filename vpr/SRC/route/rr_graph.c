#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "check_rr_graph.h"
#include "rr_graph_timing_params.h"
#include "rr_graph_indexed_data.h"
#include "vpr_utils.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"

/* #define ENABLE_DUMP */
/* #define MUX_SIZE_DIST_DISPLAY */

/* mux size statistic data structures */

typedef struct s_mux {
	int size;
	struct s_mux *next;
} t_mux;

typedef struct s_mux_size_distribution {
	int mux_count;
	int max_index;
	int *distr;
	struct s_mux_size_distribution *next;
} t_mux_size_distribution;

typedef struct s_clb_to_clb_directs {
	t_type_descriptor *from_clb_type;
	int from_clb_pin_start_index;
	int from_clb_pin_end_index;
	t_type_descriptor *to_clb_type;
	int to_clb_pin_start_index;
	int to_clb_pin_end_index;
} t_clb_to_clb_directs;

/* UDSD Modifications by WMF End */

/******************* Variables local to this module. ***********************/

/* Used to free "chunked" memory.  If NULL, no rr_graph exists right now.  */
static t_chunk rr_mem_ch = {NULL, 0, NULL};

/* Status of current chunk being dished out by calls to my_chunk_malloc.   */

/********************* Subroutines local to this module. *******************/
static int ****alloc_and_load_pin_to_track_map(INP enum e_pin_type pin_type,
		INP int nodes_per_chan, INP int *Fc, INP t_type_ptr Type,
		INP boolean perturb_switch_pattern,
		INP enum e_directionality directionality);

static struct s_ivec ***alloc_and_load_track_to_pin_lookup(
		INP int ****pin_to_track_map, INP int *Fc, INP int height,
		INP int num_pins, INP int nodes_per_chan);

static void build_bidir_rr_opins(INP int i, INP int j,
		INOUTP t_rr_node * L_rr_node, INP t_ivec *** L_rr_node_indices,
		INP int *****opin_to_track_map, INP int **Fc_out,
		INP boolean * L_rr_edge_done, INP t_seg_details * seg_details,
		INP struct s_grid_tile **L_grid, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs);

static void build_unidir_rr_opins(INP int i, INP int j,
		INP struct s_grid_tile **L_grid, INP int **Fc_out,
		INP int nodes_per_chan, INP t_seg_details * seg_details,
		INOUTP int **Fc_xofs, INOUTP int **Fc_yofs,
		INOUTP t_rr_node * L_rr_node, INOUTP boolean * L_rr_edge_done,
		OUTP boolean * Fc_clipped, INP t_ivec *** L_rr_node_indices, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs);

static int get_opin_direct_connecions(int x, int y, int opin, INOUTP t_linked_edge ** edge_list_ptr, INP t_ivec *** L_rr_node_indices, 
	INP int delayless_switch, INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs);

static void alloc_and_load_rr_graph(INP int num_nodes,
		INP t_rr_node * L_rr_node, INP int num_seg_types,
		INP t_seg_details * seg_details, INP boolean * L_rr_edge_done,
		INP struct s_ivec ****track_to_ipin_lookup,
		INP int *****opin_to_track_map, INP struct s_ivec ***switch_block_conn,
		INP struct s_grid_tile **L_grid, INP int L_nx, INP int L_ny, INP int Fs,
		INP short *****sblock_pattern, INP int **Fc_out, INP int **Fc_xofs,
		INP int **Fc_yofs, INP t_ivec *** L_rr_node_indices,
		INP int nodes_per_chan, INP enum e_switch_block_type sb_type,
		INP int delayless_switch, INP enum e_directionality directionality,
		INP int wire_to_ipin_switch, OUTP boolean * Fc_clipped, INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs);

static void load_uniform_switch_pattern(INP t_type_ptr type,
		INOUTP int ****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *offset_ordering, INP int nodes_per_chan, INP int Fc,
		INP enum e_directionality directionality);

static void load_perturbed_switch_pattern(INP t_type_ptr type,
		INOUTP int ****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *offset_ordering, INP int nodes_per_chan, INP int Fc,
		INP enum e_directionality directionality);

static void check_all_tracks_reach_pins(t_type_ptr type,
		int ****tracks_connected_to_pin, int nodes_per_chan, int Fc,
		enum e_pin_type ipin_or_opin);

static boolean *alloc_and_load_perturb_ipins(INP int nodes_per_chan,
		INP int L_num_types, INP int **Fc_in, INP int **Fc_out,
		INP enum e_directionality directionality);

static void build_rr_sinks_sources(INP int i, INP int j,
		INP t_rr_node * L_rr_node, INP t_ivec *** L_rr_node_indices,
		INP int delayless_switch, INP struct s_grid_tile **L_grid);

static void build_rr_xchan(INP int i, INP int j,
		INP struct s_ivec ****track_to_ipin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int nodes_per_chan, INP int *opin_mux_size,
		INP short *****sblock_pattern, INP int Fs_per_side,
		INP t_seg_details * seg_details, INP t_ivec *** L_rr_node_indices,
		INP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality);

static void build_rr_ychan(INP int i, INP int j,
		INP struct s_ivec ****track_to_ipin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int nodes_per_chan, INP int *opin_mux_size,
		INP short *****sblock_pattern, INP int Fs_per_side,
		INP t_seg_details * seg_details, INP t_ivec *** L_rr_node_indices,
		INP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality);

static void rr_graph_externals(t_timing_inf timing_inf,
		t_segment_inf * segment_inf, int num_seg_types, int nodes_per_chan,
		int wire_to_ipin_switch, enum e_base_cost_type base_cost_type);

void alloc_and_load_edges_and_switches(INP t_rr_node * L_rr_node, INP int inode,
		INP int num_edges, INP boolean * L_rr_edge_done,
		INP t_linked_edge * edge_list_head);

static void alloc_net_rr_terminals(void);

static void alloc_and_load_rr_clb_source(t_ivec *** L_rr_node_indices);

static t_clb_to_clb_directs *alloc_and_load_clb_to_clb_directs(INP t_direct_inf *directs, INP int num_directs);

#if 0
static void load_uniform_opin_switch_pattern_paired(INP int *Fc_out,
		INP int num_pins,
		INP int *pins_in_chan_seg,
		INP int num_wire_inc_muxes,
		INP int num_wire_dec_muxes,
		INP int *wire_inc_muxes,
		INP int *wire_dec_muxes,
		INOUTP t_rr_node * L_rr_node,
		INOUTP boolean *
		L_rr_edge_done,
		INP t_seg_details *
		seg_details,
		OUTP boolean * Fc_clipped);
#endif
void watch_edges(int inode, t_linked_edge * edge_list_head);
#if MUX_SIZE_DIST_DISPLAY
static void view_mux_size_distribution(t_ivec *** L_rr_node_indices,
		int nodes_per_chan,
		t_seg_details * seg_details_x,
		t_seg_details * seg_details_y);
static void print_distribution(FILE * fptr,
		t_mux_size_distribution * distr_struct);
#endif

static void free_type_pin_to_track_map(int***** ipin_to_track_map,
		t_type_ptr types);

static void free_type_track_to_ipin_map(struct s_ivec**** track_to_pin_map,
		t_type_ptr types, int nodes_per_chan);

static t_seg_details *alloc_and_load_global_route_seg_details(
		INP int nodes_per_chan, INP int global_route_switch);

/* UDSD Modifications by WMF End */

static int **alloc_and_load_actual_fc(INP int L_num_types, INP t_type_ptr types,
		INP int nodes_per_chan, INP boolean is_Fc_out,
		INP enum e_directionality directionality, OUTP boolean * Fc_clipped, INP boolean ignore_Fc_0);

/******************* Subroutine definitions *******************************/

void build_rr_graph(INP t_graph_type graph_type, INP int L_num_types,
		INP t_type_ptr types, INP int L_nx, INP int L_ny,
		INP struct s_grid_tile **L_grid, INP int chan_width,
		INP struct s_chan_width_dist *chan_capacity_inf,
		INP enum e_switch_block_type sb_type, INP int Fs, INP int num_seg_types,
		INP int num_switches, INP t_segment_inf * segment_inf,
		INP int global_route_switch, INP int delayless_switch,
		INP t_timing_inf timing_inf, INP int wire_to_ipin_switch,
		INP enum e_base_cost_type base_cost_type, INP t_direct_inf *directs, 
		INP int num_directs, INP boolean ignore_Fc_0, OUTP int *Warnings) {
	/* Temp structures used to build graph */
	int nodes_per_chan, i, j;
	t_seg_details *seg_details = NULL;
	int **Fc_in = NULL; /* [0..num_types-1][0..num_pins-1] */
	int **Fc_out = NULL; /* [0..num_types-1][0..num_pins-1] */

	int *****opin_to_track_map = NULL; /* [0..num_types-1][0..num_pins-1][0..height][0..3][0..Fc-1] */
	int *****ipin_to_track_map = NULL; /* [0..num_types-1][0..num_pins-1][0..height][0..3][0..Fc-1] */
	t_ivec ****track_to_ipin_lookup = NULL; /* [0..num_types-1][0..nodes_per_chan-1][0..height][0..3] */
	t_ivec ***switch_block_conn = NULL;
	short *****unidir_sb_pattern = NULL;
	boolean *L_rr_edge_done = NULL;
	boolean is_global_graph;
	boolean Fc_clipped;
	boolean use_full_seg_groups;
	boolean *perturb_ipins = NULL;
	enum e_directionality directionality;
	int **Fc_xofs = NULL; /* [0..ny-1][0..nx-1] */
	int **Fc_yofs = NULL; /* [0..nx-1][0..ny-1] */
	t_clb_to_clb_directs *clb_to_clb_directs;

	rr_node_indices = NULL;
	rr_node = NULL;
	num_rr_nodes = 0;

	/* Reset warning flag */
	*Warnings = RR_GRAPH_NO_WARN;

	/* Decode the graph_type */
	is_global_graph = FALSE;
	if (GRAPH_GLOBAL == graph_type) {
		is_global_graph = TRUE;
	}
	use_full_seg_groups = FALSE;
	if (GRAPH_UNIDIR_TILEABLE == graph_type) {
		use_full_seg_groups = TRUE;
	}
	directionality = UNI_DIRECTIONAL;
	if (GRAPH_BIDIR == graph_type) {
		directionality = BI_DIRECTIONAL;
	}
	if (is_global_graph) {
		directionality = BI_DIRECTIONAL;
	}

	/* Global routing uses a single longwire track */
	nodes_per_chan = (is_global_graph ? 1 : chan_width);
	assert(nodes_per_chan > 0);

	clb_to_clb_directs = NULL;
	if(num_directs > 0) {
		clb_to_clb_directs = alloc_and_load_clb_to_clb_directs(directs, num_directs);
	}

	/* START SEG_DETAILS */
	if (is_global_graph) {
		/* Sets up a single unit length segment type for global routing. */
		seg_details = alloc_and_load_global_route_seg_details(nodes_per_chan,
				global_route_switch);
	} else {
		/* Setup segments including distrubuting tracks and staggering.
		 * If use_full_seg_groups is specified, nodes_per_chan may be 
		 * changed. Warning should be singled to caller if this happens. */
		seg_details = alloc_and_load_seg_details(&nodes_per_chan,
				std::max(L_nx, L_ny), num_seg_types, segment_inf,
				use_full_seg_groups, is_global_graph, directionality);
		if ((is_global_graph ? 1 : chan_width) != nodes_per_chan) {
			*Warnings |= RR_GRAPH_WARN_CHAN_WIDTH_CHANGED;
		}
		if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_SEG_DETAILS)) {
			dump_seg_details(seg_details, nodes_per_chan, 
				getEchoFileName(E_ECHO_SEG_DETAILS));
		} else
			;
	}
	/* END SEG_DETAILS */

	/* START FC */
	/* Determine the actual value of Fc */
	if (is_global_graph) {
		Fc_in = (int **) my_malloc(sizeof(int) * L_num_types);
		Fc_out = (int **) my_malloc(sizeof(int) * L_num_types);
		for (i = 0; i < L_num_types; ++i) {
			for (j = 0; j < types[i].num_pins; ++j) {
				Fc_in[i][j] = 1;
				Fc_out[i][j] = 1;
			}
		}
	} else {
		Fc_clipped = FALSE;
		Fc_in = alloc_and_load_actual_fc(L_num_types, types, nodes_per_chan,
				FALSE, directionality, &Fc_clipped, ignore_Fc_0);
		if (Fc_clipped) {
			*Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
		}
		Fc_clipped = FALSE;
		Fc_out = alloc_and_load_actual_fc(L_num_types, types, nodes_per_chan,
				TRUE, directionality, &Fc_clipped, ignore_Fc_0);
		if (Fc_clipped) {
			*Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
		}

#ifdef VERBOSE
		for (i = 1; i < L_num_types; ++i) { /* Skip "<EMPTY>" */
			for (j = 0; j < type_descriptors[i].num_pins; ++j) { 
				if (type_descriptors[i].is_Fc_full_flex[j]) {
					vpr_printf(TIO_MESSAGE_INFO, "Fc Actual Values: type = %s, Fc_out = full, Fc_in = %d.\n",
							type_descriptors[i].name, Fc_in[i][j]);
				}
				else {
					vpr_printf(TIO_MESSAGE_INFO, "Fc Actual Values: type = %s, Fc_out = %d, Fc_in = %d.\n",
							type_descriptors[i].name, Fc_out[i][j], Fc_in[i][j]);
				}
			}
		}
#endif /* VERBOSE */
	}

	perturb_ipins = alloc_and_load_perturb_ipins(nodes_per_chan, L_num_types,
			Fc_in, Fc_out, directionality);
	/* END FC */
	
	/* Alloc node lookups, count nodes, alloc rr nodes */
	num_rr_nodes = 0;
	rr_node_indices = alloc_and_load_rr_node_indices(nodes_per_chan, L_nx, L_ny,
			&num_rr_nodes, seg_details);
	rr_node = (t_rr_node *) my_malloc(sizeof(t_rr_node) * num_rr_nodes);
	memset(rr_node, 0, sizeof(t_rr_node) * num_rr_nodes);
	L_rr_edge_done = (boolean *) my_malloc(sizeof(boolean) * num_rr_nodes);
	memset(L_rr_edge_done, 0, sizeof(boolean) * num_rr_nodes);

	/* These are data structures used by the the unidir opin mapping. */
	if (UNI_DIRECTIONAL == directionality) {
		Fc_xofs = (int **) alloc_matrix(0, L_ny, 0, L_nx, sizeof(int));
		Fc_yofs = (int **) alloc_matrix(0, L_nx, 0, L_ny, sizeof(int));
		for (i = 0; i <= L_nx; ++i) {
			for (j = 0; j <= L_ny; ++j) {
				Fc_xofs[j][i] = 0;
				Fc_yofs[i][j] = 0;
			}
		}
	}

	/* START SB LOOKUP */
	/* Alloc and load the switch block lookup */
	if (is_global_graph) {
		assert(nodes_per_chan == 1);
		switch_block_conn = alloc_and_load_switch_block_conn(1, SUBSET, 3);
	} else if (BI_DIRECTIONAL == directionality) {
		switch_block_conn = alloc_and_load_switch_block_conn(nodes_per_chan,
				sb_type, Fs);
	} else {
		assert(UNI_DIRECTIONAL == directionality);

		unidir_sb_pattern = alloc_sblock_pattern_lookup(L_nx, L_ny,
				nodes_per_chan);
		for (i = 0; i <= L_nx; i++) {
			for (j = 0; j <= L_ny; j++) {
				load_sblock_pattern_lookup(i, j, nodes_per_chan, seg_details,
						Fs, sb_type, unidir_sb_pattern);
			}
		}
	}
	/* END SB LOOKUP */

	/* START IPINP MAP */
	/* Create ipin map lookups */
	ipin_to_track_map = (int *****) my_malloc(sizeof(int ****) * L_num_types);
	track_to_ipin_lookup = (struct s_ivec ****) my_malloc(
			sizeof(struct s_ivec ***) * L_num_types);
	for (i = 0; i < L_num_types; ++i) {
		ipin_to_track_map[i] = alloc_and_load_pin_to_track_map(RECEIVER,
			nodes_per_chan, Fc_in[i], &types[i], perturb_ipins[i],
			directionality);
		track_to_ipin_lookup[i] = alloc_and_load_track_to_pin_lookup(
			ipin_to_track_map[i], Fc_in[i], types[i].height,
			types[i].num_pins, nodes_per_chan);
	}
	/* END IPINP MAP */

	/* START OPINP MAP */
	/* Create opin map lookups */
	if (BI_DIRECTIONAL == directionality) {
		opin_to_track_map = (int *****) my_malloc(
				sizeof(int ****) * L_num_types);
		for (i = 0; i < L_num_types; ++i) {
			opin_to_track_map[i] = alloc_and_load_pin_to_track_map(DRIVER,
				nodes_per_chan, Fc_out[i], &types[i], FALSE, directionality);
		}
	}
	/* END OPINP MAP */

	/* UDSD Modifications by WMF begin */
	/* I'm adding 2 new fields to t_rr_node, and I want them initialized to 0. */
	for (i = 0; i < num_rr_nodes; i++) {
		rr_node[i].num_wire_drivers = 0;
		rr_node[i].num_opin_drivers = 0;
	}

	alloc_and_load_rr_graph(num_rr_nodes, rr_node, num_seg_types, seg_details,
			L_rr_edge_done, track_to_ipin_lookup, opin_to_track_map,
			switch_block_conn, L_grid, L_nx, L_ny, Fs, unidir_sb_pattern,
			Fc_out, Fc_xofs, Fc_yofs, rr_node_indices, nodes_per_chan, sb_type,
			delayless_switch, directionality, wire_to_ipin_switch, &Fc_clipped, directs, num_directs, clb_to_clb_directs);

#ifdef MUX_SIZE_DIST_DISPLAY
	if (UNI_DIRECTIONAL == directionality)
	{
		view_mux_size_distribution(rr_node_indices, nodes_per_chan,
				seg_details, seg_details);
	}
#endif

	/* Update rr_nodes capacities if global routing */
	if (graph_type == GRAPH_GLOBAL) {
		for (i = 0; i < num_rr_nodes; i++) {
			if (rr_node[i].type == CHANX || rr_node[i].type == CHANY) {
				rr_node[i].capacity = chan_width;
			}
		}
	}

	rr_graph_externals(timing_inf, segment_inf, num_seg_types, nodes_per_chan,
			wire_to_ipin_switch, base_cost_type);
	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH)) {
		dump_rr_graph(getEchoFileName(E_ECHO_RR_GRAPH));
	} else
		;

	check_rr_graph(graph_type, types, L_nx, L_ny, nodes_per_chan, Fs,
			num_seg_types, num_switches, segment_inf, global_route_switch,
			delayless_switch, wire_to_ipin_switch, seg_details, Fc_in, Fc_out,
			opin_to_track_map, ipin_to_track_map, track_to_ipin_lookup,
			switch_block_conn, perturb_ipins);

	/* Free all temp structs */
	if (seg_details) {
		free_seg_details(seg_details, nodes_per_chan);
		seg_details = NULL;
	}
	if (Fc_in) {
		free_matrix(Fc_in,0, L_num_types, 0, sizeof(int));
		Fc_in = NULL;
	}
	if (Fc_out) {
		free_matrix(Fc_out,0, L_num_types, 0, sizeof(int));
		Fc_out = NULL;
	}
	if (perturb_ipins) {
		free(perturb_ipins);
		perturb_ipins = NULL;
	}
	if (switch_block_conn) {
		free_switch_block_conn(switch_block_conn, nodes_per_chan);
		switch_block_conn = NULL;
	}
	if (L_rr_edge_done) {
		free(L_rr_edge_done);
		L_rr_edge_done = NULL;
	}
	if (Fc_xofs) {
		free_matrix(Fc_xofs, 0, L_ny, 0, sizeof(int));
		Fc_xofs = NULL;
	}
	if (Fc_yofs) {
		free_matrix(Fc_yofs, 0, L_nx, 0, sizeof(int));
		Fc_yofs = NULL;
	}
	if (unidir_sb_pattern) {
		free_sblock_pattern_lookup(unidir_sb_pattern);
		unidir_sb_pattern = NULL;
	}
	if (opin_to_track_map) {
		for (i = 0; i < L_num_types; ++i) {
			free_matrix4(opin_to_track_map[i], 0, types[i].num_pins - 1, 0,
					types[i].height - 1, 0, 3, 0, sizeof(int));
		}
		free(opin_to_track_map);
	}

	free_type_pin_to_track_map(ipin_to_track_map, types);
	free_type_track_to_ipin_map(track_to_ipin_lookup, types, nodes_per_chan);
	if(clb_to_clb_directs != NULL) {
		free(clb_to_clb_directs);
	}
}

static void rr_graph_externals(t_timing_inf timing_inf,
		t_segment_inf * segment_inf, int num_seg_types, int nodes_per_chan,
		int wire_to_ipin_switch, enum e_base_cost_type base_cost_type) {
	add_rr_graph_C_from_switches(timing_inf.C_ipin_cblock);
	alloc_and_load_rr_indexed_data(segment_inf, num_seg_types, rr_node_indices,
			nodes_per_chan, wire_to_ipin_switch, base_cost_type);

	alloc_net_rr_terminals();
	load_net_rr_terminals(rr_node_indices);
	alloc_and_load_rr_clb_source(rr_node_indices);
}

static boolean *
alloc_and_load_perturb_ipins(INP int nodes_per_chan, INP int L_num_types,
		INP int **Fc_in, INP int **Fc_out, INP enum e_directionality directionality) {
	int i;
	float Fc_ratio;
	boolean *result = NULL;

	result = (boolean *) my_malloc(L_num_types * sizeof(boolean));

	if (BI_DIRECTIONAL == directionality) {
		result[0] = FALSE;
		for (i = 1; i < L_num_types; ++i) {
			result[i] = FALSE;

			if (Fc_in[i][0] > Fc_out[i][0]) {
				Fc_ratio = (float) Fc_in[i][0] / (float) Fc_out[i][0];
			} else {
				Fc_ratio = (float) Fc_out[i][0] / (float) Fc_in[i][0];
			}

			if ((Fc_in[i][0] <= nodes_per_chan - 2)
					&& (fabs(Fc_ratio - nint(Fc_ratio))
							< (0.5 / (float) nodes_per_chan))) {
				result[i] = TRUE;
			}
		}
	} else {
		/* Unidirectional routing uses mux balancing patterns and 
		 * thus shouldn't need perturbation. */
		assert(UNI_DIRECTIONAL == directionality);
		for (i = 0; i < L_num_types; ++i) {
			result[i] = FALSE;
		}
	}

	return result;
}

static t_seg_details *
alloc_and_load_global_route_seg_details(INP int nodes_per_chan,
		INP int global_route_switch) {
	t_seg_details *result = NULL;

	assert(nodes_per_chan == 1);
	result = (t_seg_details *) my_malloc(sizeof(t_seg_details));

	result->index = 0;
	result->length = 1;
	result->wire_switch = global_route_switch;
	result->opin_switch = global_route_switch;
	result->longline = FALSE;
	result->direction = BI_DIRECTION;
	result->Cmetal = 0.0;
	result->Rmetal = 0.0;
	result->start = 1;
	result->drivers = MULTI_BUFFERED;
	result->cb = (boolean *) my_malloc(sizeof(boolean) * 1);
	result->cb[0] = TRUE;
	result->sb = (boolean *) my_malloc(sizeof(boolean) * 2);
	result->sb[0] = TRUE;
	result->sb[1] = TRUE;
	result->group_size = 1;
	result->group_start = 0;

	return result;
}

/* Calculates the actual Fc values for the given nodes_per_chan value */
static int **
alloc_and_load_actual_fc(INP int L_num_types, INP t_type_ptr types,
		INP int nodes_per_chan, INP boolean is_Fc_out,
		INP enum e_directionality directionality, OUTP boolean * Fc_clipped, INP boolean ignore_Fc_0) {

	int i, j;
	int **Result = NULL;
	int fac, num_sets;

	*Fc_clipped = FALSE;

	/* Unidir tracks formed in pairs, otherwise no effect. */
	fac = 1;
	if (UNI_DIRECTIONAL == directionality) {
		fac = 2;
	}

	assert((nodes_per_chan % fac) == 0);
	num_sets = nodes_per_chan / fac;

	int max_pins = types[0].num_pins;
	for (i = 1; i < L_num_types; ++i) {
		if (types[i].num_pins > max_pins) {
			max_pins = types[i].num_pins;
		}
	}

	Result = (int **) alloc_matrix(0, L_num_types, 0, max_pins, sizeof(int));

	for (i = 1; i < L_num_types; ++i) { 
		float *Fc = (float *) my_malloc(sizeof(float) * types[i].num_pins); /* [0..num_pins-1] */ 
		for (j = 0; j < types[i].num_pins; ++j) {
			Fc[j] = types[i].Fc[j];
		
			if(Fc[j] == 0 && ignore_Fc_0 == FALSE) {
				/* Special case indicating that this pin does not connect to general-purpose routing */
				Result[i][j] = 0;
			} else {
				/* General case indicating that this pin connects to general-purpose routing */
				if (types[i].is_Fc_frac[j]) {
					Result[i][j] = fac * nint(num_sets * Fc[j]);
				} else {
					Result[i][j] = (int)Fc[j];
				}

				if (is_Fc_out && types[i].is_Fc_full_flex[j]) {
					Result[i][j] = nodes_per_chan;
				}

				Result[i][j] = std::max(Result[i][j], fac);
				if (Result[i][j] > nodes_per_chan) {
					*Fc_clipped = TRUE;
					Result[i][j] = nodes_per_chan;
				}
			}
			assert(Result[i][j] % fac == 0);
		}
		free(Fc);
	}

	return Result;
}

/* frees the track to ipin mapping for each physical grid type */
static void free_type_track_to_ipin_map(struct s_ivec**** track_to_pin_map,
		t_type_ptr types, int nodes_per_chan) {
	int i, itrack, ioff, iside;
	for (i = 0; i < num_types; i++) {
		if (track_to_pin_map[i] != NULL) {
			for (itrack = 0; itrack < nodes_per_chan; itrack++) {
				for (ioff = 0; ioff < types[i].height; ioff++) {
					for (iside = 0; iside < 4; iside++) {
						if (track_to_pin_map[i][itrack][ioff][iside].list
								!= NULL) {
							free(track_to_pin_map[i][itrack][ioff][iside].list);
						}
					}
				}
			}
			free_matrix3(track_to_pin_map[i], 0, nodes_per_chan - 1, 0,
					types[i].height - 1, 0, sizeof(struct s_ivec));
		}
	}
	free(track_to_pin_map);
}

/* frees the ipin to track mapping for each physical grid type */
static void free_type_pin_to_track_map(int***** ipin_to_track_map,
		t_type_ptr types) {
	int i;
	for (i = 0; i < num_types; i++) {
		free_matrix4(ipin_to_track_map[i], 0, types[i].num_pins - 1, 0,
				types[i].height - 1, 0, 3, 0, sizeof(int));
	}
	free(ipin_to_track_map);
}

/* Does the actual work of allocating the rr_graph and filling all the *
 * appropriate values.  Everything up to this was just a prelude!      */
static void alloc_and_load_rr_graph(INP int num_nodes,
		INP t_rr_node * L_rr_node, INP int num_seg_types,
		INP t_seg_details * seg_details, INP boolean * L_rr_edge_done,
		INP struct s_ivec ****track_to_ipin_lookup,
		INP int *****opin_to_track_map, INP struct s_ivec ***switch_block_conn,
		INP struct s_grid_tile **L_grid, INP int L_nx, INP int L_ny, INP int Fs,
		INP short *****sblock_pattern, INP int **Fc_out, INP int **Fc_xofs,
		INP int **Fc_yofs, INP t_ivec *** L_rr_node_indices,
		INP int nodes_per_chan, INP enum e_switch_block_type sb_type,
		INP int delayless_switch, INP enum e_directionality directionality,
		INP int wire_to_ipin_switch, OUTP boolean * Fc_clipped,
		INP t_direct_inf *directs, INP int num_directs,
		INP t_clb_to_clb_directs *clb_to_clb_directs) {

	int i, j;
	boolean clipped;
	int *opin_mux_size = NULL;

	/* If Fc gets clipped, this will be flagged to true */
	*Fc_clipped = FALSE;

	/* Connection SINKS and SOURCES to their pins. */
	for (i = 0; i <= (L_nx + 1); i++) {
		for (j = 0; j <= (L_ny + 1); j++) {
			build_rr_sinks_sources(i, j, L_rr_node, L_rr_node_indices,
					delayless_switch, L_grid);
		}
	}

	/* Build opins */
	for (i = 0; i <= (L_nx + 1); ++i) {
		for (j = 0; j <= (L_ny + 1); ++j) {
			if (BI_DIRECTIONAL == directionality) {
				build_bidir_rr_opins(i, j, L_rr_node, L_rr_node_indices,
						opin_to_track_map, Fc_out, L_rr_edge_done, seg_details,
						L_grid, delayless_switch,
						directs, num_directs, clb_to_clb_directs);
			} else {
				assert(UNI_DIRECTIONAL == directionality);
				build_unidir_rr_opins(i, j, L_grid, Fc_out, nodes_per_chan,
						seg_details, Fc_xofs, Fc_yofs, L_rr_node,
						L_rr_edge_done, &clipped, L_rr_node_indices, delayless_switch,
						directs, num_directs, clb_to_clb_directs);
				if (clipped) {
					*Fc_clipped = TRUE;
				}
			}
		}
	}

	/* We make a copy of the current fanin values for the nodes to 
	 * know the number of OPINs driving each mux presently */
	opin_mux_size = (int *) my_malloc(sizeof(int) * num_nodes);
	for (i = 0; i < num_nodes; ++i) {
		opin_mux_size[i] = L_rr_node[i].fan_in;
	}

	/* Build channels */
	assert(Fs % 3 == 0);
	for (i = 0; i <= L_nx; i++) {
		for (j = 0; j <= L_ny; j++) {
			if (i > 0) {
				build_rr_xchan(i, j, track_to_ipin_lookup, switch_block_conn,
						CHANX_COST_INDEX_START, nodes_per_chan, opin_mux_size,
						sblock_pattern, Fs / 3, seg_details, L_rr_node_indices,
						L_rr_edge_done, L_rr_node, wire_to_ipin_switch,
						directionality);
			}
			if (j > 0) {
				build_rr_ychan(i, j, track_to_ipin_lookup, switch_block_conn,
						CHANX_COST_INDEX_START + num_seg_types, nodes_per_chan,
						opin_mux_size, sblock_pattern, Fs / 3, seg_details,
						L_rr_node_indices, L_rr_edge_done, L_rr_node,
						wire_to_ipin_switch, directionality);
			}
		}
	}
	free(opin_mux_size);
}

static void build_bidir_rr_opins(INP int i, INP int j,
		INOUTP t_rr_node * L_rr_node, INP t_ivec *** L_rr_node_indices,
		INP int *****opin_to_track_map, INP int **Fc_out,
		INP boolean * L_rr_edge_done, INP t_seg_details * seg_details,
		INP struct s_grid_tile **L_grid, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs) {

	int ipin, inode, num_edges, *Fc, ofs;
	t_type_ptr type;
	struct s_linked_edge *edge_list, *next;

	/* OPINP edges need to be done at once so let the offset 0
	 * block do the work. */
	if (L_grid[i][j].offset > 0) {
		return;
	}

	type = L_grid[i][j].type;
	Fc = Fc_out[type->index];

	for (ipin = 0; ipin < type->num_pins; ++ipin) {
		/* We only are working with opins so skip non-drivers */
		if (type->class_inf[type->pin_class[ipin]].type != DRIVER) {
			continue;
		}

		num_edges = 0;
		edge_list = NULL;
		if(Fc[ipin] != 0) {
			for (ofs = 0; ofs < type->height; ++ofs) {
				num_edges += get_bidir_opin_connections(i, j + ofs, ipin,
						&edge_list, opin_to_track_map, Fc[ipin], L_rr_edge_done,
						L_rr_node_indices, seg_details);
			}
		}

		/* Add in direct connections */
		num_edges += get_opin_direct_connecions(i, j, ipin, &edge_list,	L_rr_node_indices, delayless_switch, directs, num_directs, clb_to_clb_directs);

		inode = get_rr_node_index(i, j, OPIN, ipin, L_rr_node_indices);
		alloc_and_load_edges_and_switches(L_rr_node, inode, num_edges,
				L_rr_edge_done, edge_list);
		while (edge_list != NULL) {
			next = edge_list->next;
			free(edge_list);
			edge_list = next;
		}
	}
}

void free_rr_graph(void) {
	int i;

	/* Frees all the routing graph data structures, if they have been       *
	 * allocated.  I use rr_mem_chunk_list_head as a flag to indicate       *
	 * whether or not the graph has been allocated -- if it is not NULL,    *
	 * a routing graph exists and can be freed.  Hence, you can call this   *
	 * routine even if you're not sure of whether a rr_graph exists or not. */

	if (rr_mem_ch.chunk_ptr_head == NULL) /* Nothing to free. */
		return;

	free_chunk_memory(&rr_mem_ch); /* Frees ALL "chunked" data */

	/* Before adding any more free calls here, be sure the data is NOT chunk *
	 * allocated, as ALL the chunk allocated data is already free!           */

	if(net_rr_terminals != NULL) {
		free(net_rr_terminals);
	}
	for (i = 0; i < num_rr_nodes; i++) {
		if (rr_node[i].edges != NULL) {
			free(rr_node[i].edges);
		}
		if (rr_node[i].switches != NULL) {
			free(rr_node[i].switches);
		}
	}

	assert(rr_node_indices);
	free_rr_node_indices(rr_node_indices);
	free(rr_node);
	free(rr_indexed_data);
	for (i = 0; i < num_blocks; i++) {
		free(rr_blk_source[i]);
	}
	free(rr_blk_source);
	rr_blk_source = NULL;
	net_rr_terminals = NULL;
	rr_node = NULL;
	rr_node_indices = NULL;
	rr_indexed_data = NULL;
	num_rr_nodes = 0;
}

static void alloc_net_rr_terminals(void) {
	int inet;

	net_rr_terminals = (int **) my_malloc(num_nets * sizeof(int *));

	for (inet = 0; inet < num_nets; inet++) {
		net_rr_terminals[inet] = (int *) my_chunk_malloc(
				(clb_net[inet].num_sinks + 1) * sizeof(int),
				&rr_mem_ch);
	}
}

void load_net_rr_terminals(t_ivec *** L_rr_node_indices) {

	/* Allocates and loads the net_rr_terminals data structure.  For each net   *
	 * it stores the rr_node index of the SOURCE of the net and all the SINKs   *
	 * of the net.  [0..num_nets-1][0..num_pins-1].  Entry [inet][pnum] stores  *
	 * the rr index corresponding to the SOURCE (opin) or SINK (ipin) of pnum.  */

	int inet, ipin, inode, iblk, i, j, node_block_pin, iclass;
	t_type_ptr type;

	for (inet = 0; inet < num_nets; inet++) {
		for (ipin = 0; ipin <= clb_net[inet].num_sinks; ipin++) {
			iblk = clb_net[inet].node_block[ipin];
			i = block[iblk].x;
			j = block[iblk].y;
			type = block[iblk].type;

			/* In the routing graph, each (x, y) location has unique pins on it
			 * so when there is capacity, blocks are packed and their pin numbers
			 * are offset to get their actual rr_node */
			node_block_pin = clb_net[inet].node_block_pin[ipin];

			iclass = type->pin_class[node_block_pin];

			inode = get_rr_node_index(i, j, (ipin == 0 ? SOURCE : SINK), /* First pin is driver */
			iclass, L_rr_node_indices);
			net_rr_terminals[inet][ipin] = inode;
		}
	}
}

static void alloc_and_load_rr_clb_source(t_ivec *** L_rr_node_indices) {

	/* Saves the rr_node corresponding to each SOURCE and SINK in each CLB      *
	 * in the FPGA.  Currently only the SOURCE rr_node values are used, and     *
	 * they are used only to reserve pins for locally used OPINs in the router. *
	 * [0..num_blocks-1][0..num_class-1].  The values for blocks that are pads  *
	 * are NOT valid.                                                           */

	int iblk, i, j, iclass, inode;
	int class_low, class_high;
	t_rr_type rr_type;
	t_type_ptr type;

	rr_blk_source = (int **) my_malloc(num_blocks * sizeof(int *));

	for (iblk = 0; iblk < num_blocks; iblk++) {
		type = block[iblk].type;
		get_class_range_for_block(iblk, &class_low, &class_high);
		rr_blk_source[iblk] = (int *) my_malloc(type->num_class * sizeof(int));
		for (iclass = 0; iclass < type->num_class; iclass++) {
			if (iclass >= class_low && iclass <= class_high) {
				i = block[iblk].x;
				j = block[iblk].y;

				if (type->class_inf[iclass].type == DRIVER)
					rr_type = SOURCE;
				else
					rr_type = SINK;

				inode = get_rr_node_index(i, j, rr_type, iclass,
						L_rr_node_indices);
				rr_blk_source[iblk][iclass] = inode;
			} else {
				rr_blk_source[iblk][iclass] = OPEN;
			}
		}
	}
}

static void build_rr_sinks_sources(INP int i, INP int j,
		INP t_rr_node * L_rr_node, INP t_ivec *** L_rr_node_indices,
		INP int delayless_switch, INP struct s_grid_tile **L_grid) {

	/* Loads IPIN, SINK, SOURCE, and OPIN. 
	 * Loads IPINP to SINK edges, and SOURCE to OPINP edges */

	int ipin, iclass, inode, pin_num, to_node, num_edges;
	int num_class, num_pins;
	t_type_ptr type;
	struct s_class *class_inf;
	int *pin_class;
	const t_pb_graph_node *pb_graph_node;
	int iport, ipb_pin, iporttype, z;

	/* Since we share nodes within a large block, only 
	 * start tile can initialize sinks, sources, and pins */
	if (L_grid[i][j].offset > 0)
		return;

	type = L_grid[i][j].type;
	num_class = type->num_class;
	class_inf = type->class_inf;
	num_pins = type->num_pins;
	pin_class = type->pin_class;
	z = 0;

	/* SINKS and SOURCE to OPINP edges */
	for (iclass = 0; iclass < num_class; iclass++) {
		if (class_inf[iclass].type == DRIVER) { /* SOURCE */
			inode = get_rr_node_index(i, j, SOURCE, iclass, L_rr_node_indices);

			num_edges = class_inf[iclass].num_pins;
			L_rr_node[inode].num_edges = num_edges;
			L_rr_node[inode].edges = (int *) my_malloc(num_edges * sizeof(int));
			L_rr_node[inode].switches = (short *) my_malloc(
					num_edges * sizeof(short));

			for (ipin = 0; ipin < class_inf[iclass].num_pins; ipin++) {
				pin_num = class_inf[iclass].pinlist[ipin];
				to_node = get_rr_node_index(i, j, OPIN, pin_num,
						L_rr_node_indices);
				L_rr_node[inode].edges[ipin] = to_node;
				L_rr_node[inode].switches[ipin] = delayless_switch;

				++L_rr_node[to_node].fan_in;
			}

			L_rr_node[inode].cost_index = SOURCE_COST_INDEX;
			L_rr_node[inode].type = SOURCE;
		} else { /* SINK */
			assert(class_inf[iclass].type == RECEIVER);
			inode = get_rr_node_index(i, j, SINK, iclass, L_rr_node_indices);

			/* NOTE:  To allow route throughs through clbs, change the lines below to  *
			 * make an edge from the input SINK to the output SOURCE.  Do for just the *
			 * special case of INPUTS = class 0 and OUTPUTS = class 1 and see what it  *
			 * leads to.  If route throughs are allowed, you may want to increase the  *
			 * base cost of OPINs and/or SOURCES so they aren't used excessively.      */

			/* Initialize to unconnected to fix values */
			L_rr_node[inode].num_edges = 0;
			L_rr_node[inode].edges = NULL;
			L_rr_node[inode].switches = NULL;

			L_rr_node[inode].cost_index = SINK_COST_INDEX;
			L_rr_node[inode].type = SINK;
		}

		/* Things common to both SOURCEs and SINKs.   */
		L_rr_node[inode].capacity = class_inf[iclass].num_pins;
		L_rr_node[inode].occ = 0;
		L_rr_node[inode].xlow = i;
		L_rr_node[inode].xhigh = i;
		L_rr_node[inode].ylow = j;
		L_rr_node[inode].yhigh = j + type->height - 1;
		L_rr_node[inode].R = 0;
		L_rr_node[inode].C = 0;
		L_rr_node[inode].ptc_num = iclass;
		L_rr_node[inode].direction = (enum e_direction)OPEN;
		L_rr_node[inode].drivers = (enum e_drivers)OPEN;
	}

	iporttype = iport = ipb_pin = 0;

	pb_graph_node = type->pb_graph_head;
	if(pb_graph_node != NULL && pb_graph_node->num_input_ports == 0) {
		iporttype = 1;
	}
	/* Connect IPINS to SINKS and dummy for OPINS */
	for (ipin = 0; ipin < num_pins; ipin++) {
		iclass = pin_class[ipin];
		z = ipin / (type->pb_type->num_clock_pins + type->pb_type->num_output_pins + type->pb_type->num_input_pins);

		if (class_inf[iclass].type == RECEIVER) {
			inode = get_rr_node_index(i, j, IPIN, ipin, L_rr_node_indices);
			to_node = get_rr_node_index(i, j, SINK, iclass, L_rr_node_indices);

			L_rr_node[inode].num_edges = 1;
			L_rr_node[inode].edges = (int *) my_malloc(sizeof(int));
			L_rr_node[inode].switches = (short *) my_malloc(sizeof(short));

			L_rr_node[inode].edges[0] = to_node;
			L_rr_node[inode].switches[0] = delayless_switch;

			++L_rr_node[to_node].fan_in;

			L_rr_node[inode].cost_index = IPIN_COST_INDEX;
			L_rr_node[inode].type = IPIN;

			/* Add in information so that I can identify which cluster pin this rr_node connects to later */
			L_rr_node[inode].z = z;
			if(iporttype == 0) {
				L_rr_node[inode].pb_graph_pin = &pb_graph_node->input_pins[iport][ipb_pin];
				ipb_pin++;
				if(ipb_pin >= pb_graph_node->num_input_pins[iport]) {
					iport++;
					ipb_pin = 0;
					if(iport >= pb_graph_node->num_input_ports) {
						iporttype++;
						iport = 0;
						if(pb_graph_node->num_clock_ports == 0) {
							iporttype = 0;
						}
					}
				}
			} else {
				assert(iporttype == 1);
				L_rr_node[inode].pb_graph_pin = &pb_graph_node->clock_pins[iport][ipb_pin];
				if(ipb_pin >= pb_graph_node->num_clock_pins[iport]) {
					iport++;
					ipb_pin = 0;
					if(iport >= pb_graph_node->num_clock_ports) {
						iporttype = 0;
						iport = 0;
						if(pb_graph_node->num_input_ports == 0) {
							iporttype = 1;
						}
					}
				}
			}
		} else {
			assert(class_inf[iclass].type == DRIVER);

			inode = get_rr_node_index(i, j, OPIN, ipin, L_rr_node_indices);
			
			/* Add in information so that I can identify which cluster pin this rr_node connects to later */
			L_rr_node[inode].z = z;
			
			L_rr_node[inode].num_edges = 0;
			L_rr_node[inode].edges = NULL;

			L_rr_node[inode].switches = NULL;

			L_rr_node[inode].cost_index = OPIN_COST_INDEX;
			L_rr_node[inode].type = OPIN;
			
			L_rr_node[inode].pb_graph_pin = &pb_graph_node->output_pins[iport][ipb_pin];
			if(ipb_pin >= pb_graph_node->num_output_pins[iport]) {
				iport++;
				ipb_pin = 0;
				if(iport >= pb_graph_node->num_output_ports) {
					iport = 0;
					if(pb_graph_node->num_input_ports == 0) {
						iporttype = 1;
					} else {
						iporttype = 0;
					}
				}
			}
		}

		/* Common to both DRIVERs and RECEIVERs */
		L_rr_node[inode].capacity = 1;
		L_rr_node[inode].occ = 0;
		L_rr_node[inode].xlow = i;
		L_rr_node[inode].xhigh = i;
		L_rr_node[inode].ylow = j;
		L_rr_node[inode].yhigh = j + type->height - 1;
		L_rr_node[inode].C = 0;
		L_rr_node[inode].R = 0;
		L_rr_node[inode].ptc_num = ipin;
		L_rr_node[inode].direction = (enum e_direction)OPEN;
		L_rr_node[inode].drivers = (enum e_drivers)OPEN;
	}
}

static void build_rr_xchan(INP int i, INP int j,
		INP struct s_ivec ****track_to_ipin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int nodes_per_chan, INP int *opin_mux_size,
		INP short *****sblock_pattern, INP int Fs_per_side,
		INP t_seg_details * seg_details, INP t_ivec *** L_rr_node_indices,
		INOUTP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality) {

	/* Loads up all the routing resource nodes in the x-directed channel      *
	 * segments starting at (i,j).                                            */

	int itrack, istart, iend, num_edges, inode, length;
	struct s_linked_edge *edge_list, *next;

	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		istart = get_seg_start(seg_details, itrack, j, i);
		iend = get_seg_end(seg_details, itrack, istart, j, nx);

		if (i > istart)
			continue; /* Not the start of this segment. */

		edge_list = NULL;

		/* First count number of edges and put the edges in a linked list. */
		num_edges = 0;

		num_edges += get_track_to_ipins(istart, j, itrack, &edge_list,
				L_rr_node_indices, track_to_ipin_lookup, seg_details, CHANX, nx,
				wire_to_ipin_switch, directionality);

		if (j > 0) {
			num_edges += get_track_to_tracks(j, istart, itrack, CHANX, j, CHANY,
					nx, nodes_per_chan, opin_mux_size, Fs_per_side,
					sblock_pattern, &edge_list, seg_details, directionality,
					L_rr_node_indices, L_rr_edge_done, switch_block_conn);
		}

		if (j < ny) {
			num_edges += get_track_to_tracks(j, istart, itrack, CHANX, j + 1,
					CHANY, nx, nodes_per_chan, opin_mux_size, Fs_per_side,
					sblock_pattern, &edge_list, seg_details, directionality,
					L_rr_node_indices, L_rr_edge_done, switch_block_conn);
		}

		if (istart > 1) {
			num_edges += get_track_to_tracks(j, istart, itrack, CHANX,
					istart - 1, CHANX, nx, nodes_per_chan, opin_mux_size,
					Fs_per_side, sblock_pattern, &edge_list, seg_details,
					directionality, L_rr_node_indices, L_rr_edge_done,
					switch_block_conn);
		}

		if (iend < nx) {
			num_edges += get_track_to_tracks(j, istart, itrack, CHANX, iend + 1,
					CHANX, nx, nodes_per_chan, opin_mux_size, Fs_per_side,
					sblock_pattern, &edge_list, seg_details, directionality,
					L_rr_node_indices, L_rr_edge_done, switch_block_conn);
		}

		inode = get_rr_node_index(i, j, CHANX, itrack, L_rr_node_indices);
		alloc_and_load_edges_and_switches(L_rr_node, inode, num_edges,
				L_rr_edge_done, edge_list);

		while (edge_list != NULL) {
			next = edge_list->next;
			free(edge_list);
			edge_list = next;
		}

		/* Edge arrays have now been built up.  Do everything else.  */
		L_rr_node[inode].cost_index = cost_index_offset
				+ seg_details[itrack].index;
		L_rr_node[inode].occ = 0;

		L_rr_node[inode].capacity = 1; /* GLOBAL routing handled elsewhere */

		L_rr_node[inode].xlow = istart;
		L_rr_node[inode].xhigh = iend;
		L_rr_node[inode].ylow = j;
		L_rr_node[inode].yhigh = j;

		length = iend - istart + 1;
		L_rr_node[inode].R = length * seg_details[itrack].Rmetal;
		L_rr_node[inode].C = length * seg_details[itrack].Cmetal;

		L_rr_node[inode].ptc_num = itrack;
		L_rr_node[inode].type = CHANX;
		L_rr_node[inode].direction = seg_details[itrack].direction;
		L_rr_node[inode].drivers = seg_details[itrack].drivers;
	}
}

static void build_rr_ychan(INP int i, INP int j,
		INP struct s_ivec ****track_to_ipin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int nodes_per_chan, INP int *opin_mux_size,
		INP short *****sblock_pattern, INP int Fs_per_side,
		INP t_seg_details * seg_details, INP t_ivec *** L_rr_node_indices,
		INP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality) {

	/* Loads up all the routing resource nodes in the y-directed channel      *
	 * segments starting at (i,j).                                            */

	int itrack, istart, iend, num_edges, inode, length;
	struct s_linked_edge *edge_list, *next;

	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		istart = get_seg_start(seg_details, itrack, i, j);
		iend = get_seg_end(seg_details, itrack, istart, i, ny);

		if (j > istart)
			continue; /* Not the start of this segment. */

		edge_list = NULL;

		/* First count number of edges and put the edges in a linked list. */
		num_edges = 0;

		num_edges += get_track_to_ipins(istart, i, itrack, &edge_list,
				L_rr_node_indices, track_to_ipin_lookup, seg_details, CHANY, ny,
				wire_to_ipin_switch, directionality);

		if (i > 0) {
			num_edges += get_track_to_tracks(i, istart, itrack, CHANY, i, CHANX,
					ny, nodes_per_chan, opin_mux_size, Fs_per_side,
					sblock_pattern, &edge_list, seg_details, directionality,
					L_rr_node_indices, L_rr_edge_done, switch_block_conn);
		}

		if (i < nx) {
			num_edges += get_track_to_tracks(i, istart, itrack, CHANY, i + 1,
					CHANX, ny, nodes_per_chan, opin_mux_size, Fs_per_side,
					sblock_pattern, &edge_list, seg_details, directionality,
					L_rr_node_indices, L_rr_edge_done, switch_block_conn);
		}

		if (istart > 1) {
			num_edges += get_track_to_tracks(i, istart, itrack, CHANY,
					istart - 1, CHANY, ny, nodes_per_chan, opin_mux_size,
					Fs_per_side, sblock_pattern, &edge_list, seg_details,
					directionality, L_rr_node_indices, L_rr_edge_done,
					switch_block_conn);
		}

		if (iend < ny) {
			num_edges += get_track_to_tracks(i, istart, itrack, CHANY, iend + 1,
					CHANY, ny, nodes_per_chan, opin_mux_size, Fs_per_side,
					sblock_pattern, &edge_list, seg_details, directionality,
					L_rr_node_indices, L_rr_edge_done, switch_block_conn);
		}

		inode = get_rr_node_index(i, j, CHANY, itrack, L_rr_node_indices);
		alloc_and_load_edges_and_switches(L_rr_node, inode, num_edges,
				L_rr_edge_done, edge_list);

		while (edge_list != NULL) {
			next = edge_list->next;
			free(edge_list);
			edge_list = next;
		}

		/* Edge arrays have now been built up.  Do everything else.  */
		L_rr_node[inode].cost_index = cost_index_offset
				+ seg_details[itrack].index;
		L_rr_node[inode].occ = 0;

		L_rr_node[inode].capacity = 1; /* GLOBAL routing handled elsewhere */

		L_rr_node[inode].xlow = i;
		L_rr_node[inode].xhigh = i;
		L_rr_node[inode].ylow = istart;
		L_rr_node[inode].yhigh = iend;

		length = iend - istart + 1;
		L_rr_node[inode].R = length * seg_details[itrack].Rmetal;
		L_rr_node[inode].C = length * seg_details[itrack].Cmetal;

		L_rr_node[inode].ptc_num = itrack;
		L_rr_node[inode].type = CHANY;
		L_rr_node[inode].direction = seg_details[itrack].direction;
		L_rr_node[inode].drivers = seg_details[itrack].drivers;
	}
}

void watch_edges(int inode, t_linked_edge * edge_list_head) {
	t_linked_edge *list_ptr;
	int i, to_node;

	list_ptr = edge_list_head;
	i = 0;

	vpr_printf(TIO_MESSAGE_TRACE, "!!! Watching Node %d !!!!\n", inode);
	print_rr_node(stdout, rr_node, inode);
	vpr_printf(TIO_MESSAGE_TRACE, "Currently connects to:\n");
	while (list_ptr != NULL) {
		to_node = list_ptr->edge;
		print_rr_node(stdout, rr_node, to_node);
		list_ptr = list_ptr->next;
		i++;
	}
}

void alloc_and_load_edges_and_switches(INP t_rr_node * L_rr_node, INP int inode,
		INP int num_edges, INOUTP boolean * L_rr_edge_done,
		INP t_linked_edge * edge_list_head) {

	/* Sets up all the edge related information for rr_node inode (num_edges,  * 
	 * the edges array and the switches array).  The edge_list_head points to  *
	 * a list of the num_edges edges and switches to put in the arrays.  This  *
	 * linked list is freed by this routine. This routine also resets the      *
	 * rr_edge_done array for the next rr_node (i.e. set it so that no edges   *
	 * are marked as having been seen before).                                 */

	t_linked_edge *list_ptr;
	int i;

	/* Check we aren't overwriting edges */
	assert(L_rr_node[inode].num_edges < 1);
	assert(NULL == L_rr_node[inode].edges);
	assert(NULL == L_rr_node[inode].switches);

	L_rr_node[inode].num_edges = num_edges;
	L_rr_node[inode].edges = (int *) my_malloc(num_edges * sizeof(int));
	L_rr_node[inode].switches = (short *) my_malloc(num_edges * sizeof(short));

	i = 0;
	list_ptr = edge_list_head;
	while (list_ptr && (i < num_edges)) {
		L_rr_node[inode].edges[i] = list_ptr->edge;
		L_rr_node[inode].switches[i] = list_ptr->iswitch;

		++L_rr_node[list_ptr->edge].fan_in;

		/* Unmark the edge since we are done considering fanout from node. */
		L_rr_edge_done[list_ptr->edge] = FALSE;

		list_ptr = list_ptr->next;
		++i;
	}
	assert(list_ptr == NULL);
	assert(i == num_edges);
}

static int ****
alloc_and_load_pin_to_track_map(INP enum e_pin_type pin_type,
		INP int nodes_per_chan, INP int *Fc, INP t_type_ptr Type,
		INP boolean perturb_switch_pattern,
		INP enum e_directionality directionality) {

	int **num_dir; /* [0..height][0..3] Number of *physical* pins on each side.          */
	int ***dir_list; /* [0..height][0..3][0..num_pins-1] list of pins of correct type  *
	 * * on each side. Max possible space alloced for simplicity */

	int i, j, k, iside, ipin, iclass, num_phys_pins, pindex, ioff;
	int *pin_num_ordering, *side_ordering, *offset_ordering;
	int **num_done_per_dir; /* [0..height][0..3] */
	int ****tracks_connected_to_pin; /* [0..num_pins-1][0..height][0..3][0..Fc-1] */

	/* NB:  This wastes some space.  Could set tracks_..._pin[ipin][ioff][iside] = 
	 * NULL if there is no pin on that side, or that pin is of the wrong type. 
	 * Probably not enough memory to worry about, esp. as it's temporary.      
	 * If pin ipin on side iside does not exist or is of the wrong type,       
	 * tracks_connected_to_pin[ipin][iside][0] = OPEN.                               */

	if (Type->num_pins < 1) {
		return NULL;
	}

	/* Currently, only two possible Fc values exist: 0 or default. 
	 * Finding the max. value of Fc in block will result in the 
	 * default value, which works for now. In the future, when 
	 * the Fc values of all pins can vary, the max value will continue
	 * to work for matrix (de)allocation purposes. However, all looping 
	 * will have to be modified to account for pin-based Fc values. */
	int max_Fc = 0;
	for (i = 0; i < Type->num_pins; ++i) {
		iclass = Type->pin_class[i];
		if (Fc[i] > max_Fc && Type->class_inf[iclass].type == pin_type) {
			max_Fc = Fc[i];
		}
	}

	tracks_connected_to_pin = (int ****) alloc_matrix4(0, Type->num_pins - 1, 0,
			Type->height - 1, 0, 3, 0, max_Fc, sizeof(int));

	for (ipin = 0; ipin < Type->num_pins; ipin++) {
		for (ioff = 0; ioff < Type->height; ioff++) {
			for (iside = 0; iside < 4; iside++) {
				for (i = 0; i < max_Fc; ++i) {
					tracks_connected_to_pin[ipin][ioff][iside][i] = OPEN; /* Unconnected. */
				}
			}
		}
	}

	num_dir = (int **) alloc_matrix(0, Type->height - 1, 0, 3, sizeof(int));
	dir_list = (int ***) alloc_matrix3(0, Type->height - 1, 0, 3, 0,
			Type->num_pins - 1, sizeof(int));

	/* Defensive coding.  Try to crash hard if I use an unset entry.  */
	for (i = 0; i < Type->height; i++)
		for (j = 0; j < 4; j++)
			for (k = 0; k < Type->num_pins; k++)
				dir_list[i][j][k] = (-1);

	for (i = 0; i < Type->height; i++)
		for (j = 0; j < 4; j++)
			num_dir[i][j] = 0;

	for (ipin = 0; ipin < Type->num_pins; ipin++) {
		iclass = Type->pin_class[ipin];
		if (Type->class_inf[iclass].type != pin_type) /* Doing either ipins OR opins */
			continue;

		/* Pins connecting only to global resources get no switches -> keeps the    *
		 * area model accurate.                                                     */

		if (Type->is_global_pin[ipin])
			continue;
		for (ioff = 0; ioff < Type->height; ioff++) {
			for (iside = 0; iside < 4; iside++) {
				if (Type->pinloc[ioff][iside][ipin] == 1) {
					dir_list[ioff][iside][num_dir[ioff][iside]] = ipin;
					num_dir[ioff][iside]++;
				}
			}
		}
	}

	num_phys_pins = 0;
	for (ioff = 0; ioff < Type->height; ioff++) {
		for (iside = 0; iside < 4; iside++)
			num_phys_pins += num_dir[ioff][iside]; /* Num. physical pins per type */
	}
	num_done_per_dir = (int **) alloc_matrix(0, Type->height - 1, 0, 3,
			sizeof(int));
	for (ioff = 0; ioff < Type->height; ioff++) {
		for (iside = 0; iside < 4; iside++) {
			num_done_per_dir[ioff][iside] = 0;
		}
	}
	pin_num_ordering = (int *) my_malloc(num_phys_pins * sizeof(int));
	side_ordering = (int *) my_malloc(num_phys_pins * sizeof(int));
	offset_ordering = (int *) my_malloc(num_phys_pins * sizeof(int));

	/* Connection block I use distributes pins evenly across the tracks      *
	 * of ALL sides of the clb at once.  Ensures that each pin connects      *
	 * to spaced out tracks in its connection block, and that the other      *
	 * pins (potentially in other C blocks) connect to the remaining tracks  *
	 * first.  Doesn't matter for large Fc, but should make a fairly         *
	 * good low Fc block that leverages the fact that usually lots of pins   *
	 * are logically equivalent.                                             */

	iside = LEFT;
	ioff = Type->height - 1;
	ipin = 0;
	pindex = -1;

	while (ipin < num_phys_pins) {
		if (iside == TOP) {
			iside = RIGHT;
		} else if (iside == RIGHT) {
			if (ioff <= 0) {
				iside = BOTTOM;
			} else {
				ioff--;
			}
		} else if (iside == BOTTOM) {
			iside = LEFT;
		} else {
			assert(iside == LEFT);
			if (ioff >= Type->height - 1) {
				pindex++;
				iside = TOP;
			} else {
				ioff++;
			}
		}

		assert(pindex < num_phys_pins);
		/* Number of physical pins bounds number of logical pins */

		if (num_done_per_dir[ioff][iside] >= num_dir[ioff][iside])
			continue;
		pin_num_ordering[ipin] = dir_list[ioff][iside][pindex];
		side_ordering[ipin] = iside;
		offset_ordering[ipin] = ioff;
		assert(Type->pinloc[ioff][iside][dir_list[ioff][iside][pindex]]);
		num_done_per_dir[ioff][iside]++;
		ipin++;
	}

	if (perturb_switch_pattern) {
		load_perturbed_switch_pattern(Type, tracks_connected_to_pin,
				num_phys_pins, pin_num_ordering, side_ordering, offset_ordering,
				nodes_per_chan, max_Fc, directionality);
	} else {
		load_uniform_switch_pattern(Type, tracks_connected_to_pin,
				num_phys_pins, pin_num_ordering, side_ordering, offset_ordering,
				nodes_per_chan, max_Fc, directionality);
	}
	check_all_tracks_reach_pins(Type, tracks_connected_to_pin, nodes_per_chan,
			max_Fc, pin_type);

	/* Free all temporary storage. */
	free_matrix(num_dir, 0, Type->height - 1, 0, sizeof(int));
	free_matrix3(dir_list, 0, Type->height - 1, 0, 3, 0, sizeof(int));
	free_matrix(num_done_per_dir, 0, Type->height - 1, 0, sizeof(int));
	free(pin_num_ordering);
	free(side_ordering);
	free(offset_ordering);

	return tracks_connected_to_pin;
}

static void load_uniform_switch_pattern(INP t_type_ptr type,
		INOUTP int ****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *offset_ordering, INP int nodes_per_chan, INP int Fc,
		enum e_directionality directionality) {

	/* Loads the tracks_connected_to_pin array with an even distribution of     *
	 * switches across the tracks for each pin.  For example, each pin connects *
	 * to every 4.3rd track in a channel, with exactly which tracks a pin       *
	 * connects to staggered from pin to pin.                                   */

	int i, j, ipin, iside, ioff, itrack, k;
	float f_track, fc_step;
	int group_size;
	float step_size;

	/* Uni-directional drive is implemented to ensure no directional bias and this means 
	 * two important comments noted below                                                */
	/* 1. Spacing should be (W/2)/(Fc/2), and step_size should be spacing/(num_phys_pins),
	 *    and lay down 2 switches on an adjacent pair of tracks at a time to ensure
	 *    no directional bias. Basically, treat W (even) as W/2 pairs of tracks, and
	 *    assign switches to a pair at a time. Can do this because W is guaranteed to 
	 *    be even-numbered; however same approach cannot be applied to Fc_out pattern
	 *    when L > 1 and W <> 2L multiple. 
	 *
	 * 2. This generic pattern should be considered the tileable physical layout,
	 *    meaning all track # here are physical #'s,
	 *    so later must use vpr_to_phy conversion to find actual logical #'s to connect.
	 *    This also means I will not use get_output_block_companion_track to ensure
	 *    no bias, since that describes a logical # -> that would confuse people.  */

	step_size = (float) nodes_per_chan / (float) (Fc * num_phys_pins);

	if (directionality == BI_DIRECTIONAL) {
		group_size = 1;
	} else {
		assert(directionality == UNI_DIRECTIONAL);
		group_size = 2;
	}

	assert((nodes_per_chan % group_size == 0) && (Fc % group_size == 0));

	fc_step = (float) nodes_per_chan / (float) Fc;

	for (i = 0; i < num_phys_pins; i++) {
		ipin = pin_num_ordering[i];
		iside = side_ordering[i];
		ioff = offset_ordering[i];

		/* Bi-directional treats each track separately, uni-directional works with pairs of tracks */
		for (j = 0; j < (Fc / group_size); j++) {
			f_track = (i * step_size) + (j * fc_step);
			itrack = ((int) f_track) * group_size;

			/* Catch possible floating point round error */
			itrack = std::min(itrack, nodes_per_chan - group_size);

			/* Assign the group of tracks for the Fc pattern */
			for (k = 0; k < group_size; ++k) {
				tracks_connected_to_pin[ipin][ioff][iside][group_size * j + k] =
						itrack + k;
			}
		}
	}
}

static void load_perturbed_switch_pattern(INP t_type_ptr type,
		INOUTP int ****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *offset_ordering, INP int nodes_per_chan, INP int Fc,
		enum e_directionality directionality) {

	/* Loads the tracks_connected_to_pin array with an unevenly distributed     *
	 * set of switches across the channel.  This is done for inputs when        *
	 * Fc_input = Fc_output to avoid creating "pin domains" -- certain output   *
	 * pins being able to talk only to certain input pins because their switch  *
	 * patterns exactly line up.  Distribute Fc/2 + 1 switches over half the    *
	 * channel and Fc/2 - 1 switches over the other half to make the switch     * 
	 * pattern different from the uniform one of the outputs.  Also, have half  *
	 * the pins put the "dense" part of their connections in the first half of  *
	 * the channel and the other half put the "dense" part in the second half,  *
	 * to make sure each track can connect to about the same number of ipins.   */

	int i, j, ipin, iside, itrack, ihalf, iconn, ioff;
	int Fc_dense, Fc_sparse, Fc_half[2];
	float f_track, spacing_dense, spacing_sparse, spacing[2];
	float step_size;

	assert(directionality == BI_DIRECTIONAL);

	step_size = (float) nodes_per_chan / (float) (Fc * num_phys_pins);

	Fc_dense = (Fc / 2) + 1;
	Fc_sparse = Fc - Fc_dense; /* Works for even or odd Fc */

	spacing_dense = (float) nodes_per_chan / (float) (2 * Fc_dense);
	spacing_sparse = (float) nodes_per_chan / (float) (2 * Fc_sparse);

	for (i = 0; i < num_phys_pins; i++) {
		ipin = pin_num_ordering[i];
		iside = side_ordering[i];
		ioff = offset_ordering[i];

		/* Flip every pin to balance switch density */
		spacing[i % 2] = spacing_dense;
		Fc_half[i % 2] = Fc_dense;
		spacing[(i + 1) % 2] = spacing_sparse;
		Fc_half[(i + 1) % 2] = Fc_sparse;

		f_track = i * step_size; /* Start point.  Staggered from pin to pin */
		iconn = 0;

		for (ihalf = 0; ihalf < 2; ihalf++) { /* For both dense and sparse halves. */
			for (j = 0; j < Fc_half[ihalf]; ++j) {
				itrack = (int) f_track;

				/* Can occasionally get wraparound due to floating point rounding. 
				 This is okay because the starting position > 0 when this occurs
				 so connection is valid and fine */
				itrack = itrack % nodes_per_chan;
				tracks_connected_to_pin[ipin][ioff][iside][iconn] = itrack;

				f_track += spacing[ihalf];
				iconn++;
			}
		}
	} /* End for all physical pins. */
}

static void check_all_tracks_reach_pins(t_type_ptr type,
		int ****tracks_connected_to_pin, int nodes_per_chan, int Fc,
		enum e_pin_type ipin_or_opin) {

	/* Checks that all tracks can be reached by some pin.   */

	int iconn, iside, itrack, ipin, ioff;
	int *num_conns_to_track; /* [0..nodes_per_chan-1] */

	assert(nodes_per_chan > 0);

	num_conns_to_track = (int *) my_calloc(nodes_per_chan, sizeof(int));

	for (ipin = 0; ipin < type->num_pins; ipin++) {
		for (ioff = 0; ioff < type->height; ioff++) {
			for (iside = 0; iside < 4; iside++) {
				if (tracks_connected_to_pin[ipin][ioff][iside][0] != OPEN) { /* Pin exists */
					for (iconn = 0; iconn < Fc; iconn++) {
						itrack =
								tracks_connected_to_pin[ipin][ioff][iside][iconn];
						num_conns_to_track[itrack]++;
					}
				}
			}
		}
	}

	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		if (num_conns_to_track[itrack] <= 0) {
			vpr_printf(TIO_MESSAGE_ERROR, "check_all_tracks_reach_pins: Track %d does not connect to any CLB %ss.\n", 
				itrack, (ipin_or_opin == DRIVER ? "OPIN" : "IPIN"));
		}
	}

	free(num_conns_to_track);
}

/* Allocates and loads the track to ipin lookup for each physical grid type. This
 * is the same information as the ipin_to_track map but accessed in a different way. */

static struct s_ivec ***
alloc_and_load_track_to_pin_lookup(INP int ****pin_to_track_map, INP int *Fc,
		INP int height, INP int num_pins, INP int nodes_per_chan) {
	int ipin, iside, itrack, iconn, ioff, pin_counter;
	struct s_ivec ***track_to_pin_lookup;

	/* [0..nodes_per_chan-1][0..height][0..3].  For each track number it stores a vector  
	 * for each of the four sides.  x-directed channels will use the TOP and   
	 * BOTTOM vectors to figure out what clb input pins they connect to above  
	 * and below them, respectively, while y-directed channels use the LEFT    
	 * and RIGHT vectors.  Each vector contains an nelem field saying how many 
	 * ipins it connects to.  The list[0..nelem-1] array then gives the pin    
	 * numbers.                                                                */

	/* Note that a clb pin that connects to a channel on its RIGHT means that  *
	 * that channel connects to a clb pin on its LEFT.  The convention used    *
	 * here is always in the perspective of the CLB                            */

	if (num_pins < 1) {
		return NULL;
	}

	/* Alloc and zero the the lookup table */
	track_to_pin_lookup = (struct s_ivec ***) alloc_matrix3(0,
			nodes_per_chan - 1, 0, height - 1, 0, 3, sizeof(struct s_ivec));
	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		for (ioff = 0; ioff < height; ioff++) {
			for (iside = 0; iside < 4; iside++) {
				track_to_pin_lookup[itrack][ioff][iside].nelem = 0;
				track_to_pin_lookup[itrack][ioff][iside].list = NULL;
			}
		}
	}

	/* Counting pass.  */
	for (ipin = 0; ipin < num_pins; ipin++) {
		for (ioff = 0; ioff < height; ioff++) {
			for (iside = 0; iside < 4; iside++) {
				if (pin_to_track_map[ipin][ioff][iside][0] == OPEN)
					continue;

				for (iconn = 0; iconn < Fc[ipin]; iconn++) {
					itrack = pin_to_track_map[ipin][ioff][iside][iconn];
					track_to_pin_lookup[itrack][ioff][iside].nelem++;
				}
			}
		}
	}

	/* Allocate space.  */
	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		for (ioff = 0; ioff < height; ioff++) {
			for (iside = 0; iside < 4; iside++) {
				track_to_pin_lookup[itrack][ioff][iside].list = NULL; /* Defensive code */
				if (track_to_pin_lookup[itrack][ioff][iside].nelem != 0) {
					track_to_pin_lookup[itrack][ioff][iside].list =
							(int *) my_malloc(
									track_to_pin_lookup[itrack][ioff][iside].nelem
											* sizeof(int));
					track_to_pin_lookup[itrack][ioff][iside].nelem = 0;
				}
			}
		}
	}

	/* Loading pass. */
	for (ipin = 0; ipin < num_pins; ipin++) {
		for (ioff = 0; ioff < height; ioff++) {
			for (iside = 0; iside < 4; iside++) {
				if (pin_to_track_map[ipin][ioff][iside][0] == OPEN)
					continue;

				for (iconn = 0; iconn < Fc[ipin]; iconn++) {
					itrack = pin_to_track_map[ipin][ioff][iside][iconn];
					pin_counter =
							track_to_pin_lookup[itrack][ioff][iside].nelem;
					track_to_pin_lookup[itrack][ioff][iside].list[pin_counter] =
							ipin;
					track_to_pin_lookup[itrack][ioff][iside].nelem++;
				}
			}
		}
	}

	return track_to_pin_lookup;
}

/* A utility routine to dump the contents of the routing resource graph   *
 * (everything -- connectivity, occupancy, cost, etc.) into a file.  Used *
 * only for debugging.                                                    */
void dump_rr_graph(INP const char *file_name) {

	int inode;
	FILE *fp;

	fp = my_fopen(file_name, "w", 0);

	for (inode = 0; inode < num_rr_nodes; inode++) {
		print_rr_node(fp, rr_node, inode);
		fprintf(fp, "\n");
	}

#if 0
	fprintf(fp, "\n\n%d rr_indexed_data entries.\n\n", num_rr_indexed_data);

	for (index = 0; index < num_rr_indexed_data; index++)
	{
		print_rr_indexed_data(fp, index);
		fprintf(fp, "\n");
	}
#endif

	fclose(fp);
}

/* Prints all the data about node inode to file fp.                    */
void print_rr_node(FILE * fp, t_rr_node * L_rr_node, int inode) {

	static const char *name_type[] = { "SOURCE", "SINK", "IPIN", "OPIN",
			"CHANX", "CHANY", "INTRA_CLUSTER_EDGE" };
	static const char *direction_name[] = { "OPEN", "INC_DIRECTION",
			"DEC_DIRECTION", "BI_DIRECTION" };
	static const char *drivers_name[] = { "OPEN", "MULTI_BUFFER", "SINGLE" };

	t_rr_type rr_type;
	int iconn;

	rr_type = L_rr_node[inode].type;

	/* Make sure we don't overrun const arrays */
	assert((int)rr_type < (int)(sizeof(name_type) / sizeof(char *)));
	assert(
			(L_rr_node[inode].direction + 1) < (int)(sizeof(direction_name) / sizeof(char *)));
	assert(
			(L_rr_node[inode].drivers + 1) < (int)(sizeof(drivers_name) / sizeof(char *)));

	fprintf(fp, "Node: %d %s ", inode, name_type[rr_type]);
	if ((L_rr_node[inode].xlow == L_rr_node[inode].xhigh)
			&& (L_rr_node[inode].ylow == L_rr_node[inode].yhigh)) {
		fprintf(fp, "(%d, %d) ", L_rr_node[inode].xlow, L_rr_node[inode].ylow);
	} else {
		fprintf(fp, "(%d, %d) to (%d, %d) ", L_rr_node[inode].xlow,
				L_rr_node[inode].ylow, L_rr_node[inode].xhigh,
				L_rr_node[inode].yhigh);
	}
	fprintf(fp, "Ptc_num: %d ", L_rr_node[inode].ptc_num);
	fprintf(fp, "Direction: %s ",
			direction_name[L_rr_node[inode].direction + 1]);
	fprintf(fp, "Drivers: %s ", drivers_name[L_rr_node[inode].drivers + 1]);
	fprintf(fp, "\n");

	fprintf(fp, "%d edge(s):", L_rr_node[inode].num_edges);
	for (iconn = 0; iconn < L_rr_node[inode].num_edges; iconn++)
		fprintf(fp, " %d", L_rr_node[inode].edges[iconn]);
	fprintf(fp, "\n");

	fprintf(fp, "Switch types:");
	for (iconn = 0; iconn < L_rr_node[inode].num_edges; iconn++)
		fprintf(fp, " %d", L_rr_node[inode].switches[iconn]);
	fprintf(fp, "\n");

	fprintf(fp, "Occ: %d  Capacity: %d\n", L_rr_node[inode].occ,
			L_rr_node[inode].capacity);
	if (rr_type != INTRA_CLUSTER_EDGE) {
		fprintf(fp, "R: %g  C: %g\n", L_rr_node[inode].R, L_rr_node[inode].C);
	}
	fprintf(fp, "Cost_index: %d\n", L_rr_node[inode].cost_index);
}

/* Prints all the rr_indexed_data of index to file fp.   */
void print_rr_indexed_data(FILE * fp, int index) {

	fprintf(fp, "Index: %d\n", index);

	fprintf(fp, "ortho_cost_index: %d  ",
			rr_indexed_data[index].ortho_cost_index);
	fprintf(fp, "base_cost: %g  ", rr_indexed_data[index].saved_base_cost);
	fprintf(fp, "saved_base_cost: %g\n",
			rr_indexed_data[index].saved_base_cost);

	fprintf(fp, "Seg_index: %d  ", rr_indexed_data[index].seg_index);
	fprintf(fp, "inv_length: %g\n", rr_indexed_data[index].inv_length);

	fprintf(fp, "T_linear: %g  ", rr_indexed_data[index].T_linear);
	fprintf(fp, "T_quadratic: %g  ", rr_indexed_data[index].T_quadratic);
	fprintf(fp, "C_load: %g\n", rr_indexed_data[index].C_load);
}

static void build_unidir_rr_opins(INP int i, INP int j,
		INP struct s_grid_tile **L_grid, INP int **Fc_out,
		INP int nodes_per_chan, INP t_seg_details * seg_details,
		INOUTP int **Fc_xofs, INOUTP int **Fc_yofs,
		INOUTP t_rr_node * L_rr_node, INOUTP boolean * L_rr_edge_done,
		OUTP boolean * Fc_clipped, INP t_ivec *** L_rr_node_indices, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs) {
	/* This routine returns a list of the opins rr_nodes on each
	 * side/offset of the block. You must free the result with
	 * free_matrix. */

	t_type_ptr type;
	int ipin, iclass, ofs, chan, seg, max_len, inode, max_Fc = -1;
	enum e_side side;
	t_rr_type chan_type;
	t_linked_edge *edge_list = NULL, *next;
	boolean clipped, vert, pos_dir;
	int num_edges;
	int **Fc_ofs;

	*Fc_clipped = FALSE;

	/* Only the base block of a set should use this function */
	if (L_grid[i][j].offset > 0) {
		return;
	}

	type = L_grid[i][j].type;

	/* Currently, only two possible Fc values exist: 0 or default. 
	 * Finding the max. value of Fc in block will result in the 
	 * default value, which works for now. In the future, when 
	 * the Fc values of all pins can vary, the max value will continue
	 * to work for matrix allocation purposes. However, all looping 
	 * will have to be modified to account for pin-based Fc values. */
	if (type->index > 0) {
		max_Fc = 0;
		for (ipin = 0; ipin < type->num_pins; ++ipin) {
			iclass = type->pin_class[ipin];
			if (Fc_out[type->index][ipin] > max_Fc && type->class_inf[iclass].type == DRIVER) {
				max_Fc = Fc_out[type->index][ipin];
			}
		}
	}

	/* Go through each pin and find its fanout. */
	for (ipin = 0; ipin < type->num_pins; ++ipin) {
		/* Skip global pins and ipins */
		iclass = type->pin_class[ipin];
		if (type->class_inf[iclass].type != DRIVER) {
			continue;
		}
		if (type->is_global_pin[ipin]) {
			continue;
		}

		num_edges = 0;
		edge_list = NULL;
		if(Fc_out[type->index][ipin] != 0) {
			for (ofs = 0; ofs < type->height; ++ofs) {
				for (side = (enum e_side)0; side < 4; side = (enum e_side)(side + 1)) {
					/* Can't do anything if pin isn't at this location */
					if (0 == type->pinloc[ofs][side][ipin]) {
						continue;
					}

					/* Figure out the chan seg at that side. 
					 * side is the side of the logic or io block. */
					vert = (boolean) ((side == TOP) || (side == BOTTOM));
					pos_dir = (boolean) ((side == TOP) || (side == RIGHT));
					chan_type = (vert ? CHANX : CHANY);
					chan = (vert ? (j + ofs) : i);
					seg = (vert ? i : (j + ofs));
					max_len = (vert ? nx : ny);
					Fc_ofs = (vert ? Fc_xofs : Fc_yofs);
					if (FALSE == pos_dir) {
						--chan;
					}

					/* Skip the location if there is no channel. */
					if (chan < 0) {
						continue;
					}
					if (seg < 1) {
						continue;
					}
					if (seg > (vert ? nx : ny)) {
						continue;
					}
					if (chan > (vert ? ny : nx)) {
						continue;
					}

					/* Get the list of opin to mux connections for that chan seg. */
					num_edges += get_unidir_opin_connections(chan, seg,
							max_Fc, chan_type, seg_details, &edge_list,
							Fc_ofs, L_rr_edge_done, max_len, nodes_per_chan,
							L_rr_node_indices, &clipped);
					if (clipped) {
						*Fc_clipped = TRUE;
					}
				}
			}
		}

		/* Add in direct connections */
		num_edges += get_opin_direct_connecions(i, j, ipin, &edge_list,	L_rr_node_indices, delayless_switch, directs, num_directs, clb_to_clb_directs);

		/* Add the edges */
		inode = get_rr_node_index(i, j, OPIN, ipin, L_rr_node_indices);
		alloc_and_load_edges_and_switches(rr_node, inode, num_edges,
				L_rr_edge_done, edge_list);
		while (edge_list != NULL) {
			next = edge_list->next;
			free(edge_list);
			edge_list = next;
		}
	}
}
#if 0
static void
load_uniform_opin_switch_pattern_paired(INP int *Fc_out,
		INP int num_pins,
		INP int *pins_in_chan_seg,
		INP int num_wire_inc_muxes,
		INP int num_wire_dec_muxes,
		INP int *wire_inc_muxes,
		INP int *wire_dec_muxes,
		INOUTP t_rr_node * L_rr_node,
		INOUTP boolean * L_rr_edge_done,
		INP t_seg_details * seg_details,
		OUTP boolean * Fc_clipped)
{

	/* Directionality is assumed to be uni-directional */

	/* Make turn-based assignment to avoid overlap when Fc_ouput is low. This is a bipartite
	 * matching problem. Out of "num_wire_muxes" muxes "Fc_output" of them is assigned
	 * to each outpin (total "num_pins" of them); assignment is uniform (spacing - spreadout) 
	 * and staggered to avoid overlap when Fc_output is low. */

	/* The natural order wires muxes are stored in wire_muxes is alternating in directionality 
	 * already (by my implementation), so no need to do anything extra to avoid directional bias */

	/* TODO: Due to spacing, it's possible to have directional bias: all Fc_out wires connected 
	 * to one opin goes in either INC or DEC -> whereas I want a mix of both.
	 * SOLUTION: Use quantization of 2 to ensure that if an opin connects to one wire, it 
	 * must also connect to its companion wire, which runs in the opposite direction. This 
	 * means instead of having num_wire_muxes as the matching set, pick out the INC wires
	 * in num_wires_muxes as the matching set (the DEC wires are their companions)  April 17, 2007 
	 * NEWS: That solution does not work, as treating wires in groups will lead to serious
	 * abnormal patterns (conns crossing multiple blocks) for W nonquantized to multiples of 2L.
	 * So, I'm chaning that approach to a new one that avoids directional bias: I will separate
	 * the INC muxes and DEC muxes into two sets. Each set is uniformly assigned to opins with
	 * Fc_output/2; this should be identical as before for normal cases and contains all conns
	 * in the same chan segment for the nonquantized cases. */

	/* Finally, separated the two approaches: 1. Take all wire muxes and assign them to opins
	 * one at a time (load_uniform_opin_switch_pattern) 2. Take pairs (by companion) 
	 * of wire muxes and assign them to opins a pair at a time (load_uniform_opin_switch_pattern_paired). 
	 * The first is used for fringe channel segments (ends of channels, where
	 * there are lots of muxes due to partial wire segments) and the second is used in core */

	/* float spacing, step_size, f_mux; */
	int ipin, iconn, num_edges, init_mux;
	int from_node, to_node, to_track;
	int xlow, ylow;
	t_linked_edge *edge_list;
	int *wire_muxes;
	int k, num_wire_muxes, Fc_output_per_side, CurFc;
	int count_inc, count_dec;
	t_type_ptr type;

	*Fc_clipped = FALSE;

	count_inc = count_dec = 0;

	for (ipin = 0; ipin < num_pins; ipin++)
	{
		from_node = pins_in_chan_seg[ipin];
		xlow = L_rr_node[from_node].xlow;
		ylow = L_rr_node[from_node].ylow;
		type = grid[xlow][ylow].type;
		edge_list = NULL;
		num_edges = 0;

		/* Assigning the INC muxes first, then DEC muxes */
		for (k = 0; k < 2; ++k)
		{
			if (k == 0)
			{
				num_wire_muxes = num_wire_inc_muxes;
				wire_muxes = wire_inc_muxes;
			}
			else
			{
				num_wire_muxes = num_wire_dec_muxes;
				wire_muxes = wire_dec_muxes;
			}

			/* Half the Fc will be assigned for each direction. */
			assert(Fc_out[type->index] % 2 == 0);
			Fc_output_per_side = Fc_out[type->index] / 2;

			/* Clip the demand. Make sure to use a new variable so
			 * on the second pass it is not clipped. */
			CurFc = Fc_output_per_side;
			if (Fc_output_per_side > num_wire_muxes)
			{
				*Fc_clipped = TRUE;
				CurFc = num_wire_muxes;
			}

			if (k == 0)
			{
				init_mux = (count_inc) % num_wire_muxes;
				count_inc += CurFc;
			}
			else
			{
				init_mux = (count_dec) % num_wire_muxes;
				count_dec += CurFc;
			}

			for (iconn = 0; iconn < CurFc; iconn++)
			{
				/* FINALLY, make the outpin to mux connection */
				/* Latest update: I'm not using Uniform Pattern, but a similarly staggered pattern */
				to_node =
				wire_muxes[(init_mux +
						iconn) % num_wire_muxes];

				L_rr_node[to_node].num_opin_drivers++; /* keep track of mux size */
				to_track = L_rr_node[to_node].ptc_num;

				if (FALSE == L_rr_edge_done[to_node])
				{
					/* Use of alloc_and_load_edges_and_switches 
					 * must be accompanied by rr_edge_done check. */
					L_rr_edge_done[to_node] = TRUE;
					edge_list =
					insert_in_edge_list(edge_list,
							to_node,
							seg_details
							[to_track].
							wire_switch);
					num_edges++;
				}
			}
		}

		if (num_edges < 1)
		{
			vpr_printf(TIO_MESSAGE_ERROR, "opin %d at (%d,%d) does not connect to any tracks.\n", 
					L_rr_node[from_node].ptc_num, L_rr_node[from_node].xlow, L_rr_node[from_node].ylow);
			exit(1);
		}

		alloc_and_load_edges_and_switches(L_rr_node, from_node, num_edges,
				L_rr_edge_done, edge_list);
	}
}
#endif

#if MUX_SIZE_DIST_DISPLAY
/* This routine prints and dumps statistics on the mux sizes on a sblock 
 * per sblock basis, over the entire chip. Mux sizes should be balanced (off by
 * at most 1) for all muxes in the same sblock in the core, and corner sblocks.
 * Fringe sblocks will have imbalance due to missing one side and constrains on 
 * where wires must connect. Comparing two core sblock sblocks, muxes need not 
 * be balanced if W is not quantized to 2L multiples, again for reasons that 
 * there could be sblocks with different number of muxes but same number of incoming
 * wires that need to make connections to these muxes (we don't want to under-connect
 * user-specified Fc and Fs). */
static void
view_mux_size_distribution(t_ivec *** L_rr_node_indices,
		int nodes_per_chan,
		t_seg_details * seg_details_x,
		t_seg_details * seg_details_y)
{

	int i, j, itrack, seg_num, chan_num, max_len;
	int start, end, inode, max_value, min_value;
	int array_count, k, num_muxes;
	short direction, side;
	float *percent_range_array;
	float percent_range, percent_range_sum, avg_percent_range;
	float std_dev_percent_range, deviation_f;
	int range, *range_array, global_max_range;
	float avg_range, range_sum, std_dev_range;
	t_seg_details *seg_details;
	t_mux *new_mux, *sblock_mux_list_head, *current, *next;

#ifdef ENABLE_DUMP
	FILE *dump_file_per_sblock, *dump_file;
#endif /* ENABLE_DUMP */
	t_mux_size_distribution *distr_list, *distr_current, *new_distribution,
	*distr_next;

#ifdef ENABLE_DUMP
	dump_file = my_fopen("mux_size_dump.txt", "w", 0);
	dump_file_per_sblock = my_fopen("mux_size_per_sblock_dump.txt", "w", 0);
#endif /* ENABLE_DUMP */

	sblock_mux_list_head = NULL;
	percent_range_array =
	(float *)my_malloc((nx - 1) * (ny - 1) * sizeof(float));
	range_array = (int *)my_malloc((nx - 1) * (ny - 1) * sizeof(int));
	array_count = 0;
	percent_range_sum = 0.0;
	range_sum = 0.0;
	global_max_range = 0;
	min_value = 0;
	max_value = 0;
	seg_num = 0;
	chan_num = 0;
	direction = 0;
	seg_details = 0;
	max_len = 0;
	distr_list = NULL;

	/* With the specified range, I'm only looking at core sblocks */
	for (j = (ny - 1); j > 0; j--)
	{
		for (i = 1; i < nx; i++)
		{
			num_muxes = 0;
			for (side = 0; side < 4; side++)
			{
				switch (side)
				{
					case LEFT:
					seg_num = i;
					chan_num = j;
					direction = DEC_DIRECTION; /* only DEC have muxes in that sblock */
					seg_details = seg_details_x;
					max_len = nx;
					break;

					case RIGHT:
					seg_num = i + 1;
					chan_num = j;
					direction = INC_DIRECTION;
					seg_details = seg_details_x;
					max_len = nx;
					break;

					case TOP:
					seg_num = j + 1;
					chan_num = i;
					direction = INC_DIRECTION;
					seg_details = seg_details_y;
					max_len = ny;
					break;

					case BOTTOM:
					seg_num = j;
					chan_num = i;
					direction = DEC_DIRECTION;
					seg_details = seg_details_y;
					max_len = ny;
					break;

					default:
					assert(FALSE);
				}

				assert(nodes_per_chan > 0);
				for (itrack = 0; itrack < nodes_per_chan; itrack++)
				{
					start =
					get_seg_start(seg_details, itrack,
							seg_num, chan_num);
					end =
					get_seg_end(seg_details, itrack,
							start, chan_num, max_len);

					if ((seg_details[itrack].direction ==
									direction) && (((start == seg_num)
											&& (direction ==
													INC_DIRECTION))
									|| ((end == seg_num)
											&& (direction ==
													DEC_DIRECTION))))
					{ /* mux found */
						num_muxes++;
						if (side == LEFT || side == RIGHT)
						{ /* CHANX */
							inode =
							get_rr_node_index
							(seg_num, chan_num,
									CHANX, itrack,
									L_rr_node_indices);
						}
						else
						{
							assert((side == TOP) || (side == BOTTOM)); /* CHANY */
							inode =
							get_rr_node_index
							(chan_num, seg_num,
									CHANY, itrack,
									L_rr_node_indices);
						}

						new_mux = (t_mux *)
						my_malloc(sizeof(t_mux));
						new_mux->size =
						rr_node[inode].
						num_wire_drivers +
						rr_node[inode].
						num_opin_drivers;
						new_mux->next = NULL;

						/* insert in linked list, descending */
						if (sblock_mux_list_head == NULL)
						{
							/* first entry */
							sblock_mux_list_head =
							new_mux;
						}
						else if (sblock_mux_list_head->
								size < new_mux->size)
						{
							/* insert before head */
							new_mux->next =
							sblock_mux_list_head;
							sblock_mux_list_head =
							new_mux;
						}
						else
						{
							/* insert after head */
							current =
							sblock_mux_list_head;
							next = current->next;

							while ((next != NULL)
									&& (next->size >
											new_mux->size))
							{
								current = next;
								next =
								current->next;
							}

							if (next == NULL)
							{
								current->next =
								new_mux;
							}
							else
							{
								new_mux->next =
								current->next;
								current->next =
								new_mux;
							}
						}
						/* end of insert in linked list */
					}
				}
			} /* end of mux searching over all four sides of sblock */
			/* now sblock_mux_list_head holds a linked list of all muxes in this sblock */

			current = sblock_mux_list_head;

#ifdef ENABLE_DUMP
			fprintf(dump_file_per_sblock,
					"sblock at (%d, %d) has mux sizes: {", i, j);
#endif /* ENABLE_DUMP */

			if (current != NULL)
			{
				max_value = min_value = current->size;
			}
			while (current != NULL)
			{
				if (max_value < current->size)
				max_value = current->size;
				if (min_value > current->size)
				min_value = current->size;

#ifdef ENABLE_DUMP
				fprintf(dump_file_per_sblock, "%d ",
						current->size);
				fprintf(dump_file, "%d\n", current->size);
#endif /* ENABLE_DUMP */

				current = current->next;
			}

#ifdef ENABLE_DUMP
			fprintf(dump_file_per_sblock, "}\n\tmax: %d\tmin:%d",
					max_value, min_value);
#endif /* ENABLE_DUMP */

			range = max_value - min_value;
			percent_range = ((float)range) / ((float)min_value);

			if (global_max_range < range)
			global_max_range = range;

#ifdef ENABLE_DUMP
			fprintf(dump_file_per_sblock,
					"\t\trange: %d\t\tpercent range:%.2f\n",
					range, percent_range);
#endif /* ENABLE_DUMP */

			percent_range_array[array_count] = percent_range;
			range_array[array_count] = range;

			percent_range_sum += percent_range;
			range_sum += range;

			array_count++;

			/* I will use a distribution for each (core) sblock type. 
			 * There are more than 1 type of sblocks, 
			 * when quantization of W to 2L multiples is not observed. */

			distr_current = distr_list;
			while (distr_current != NULL
					&& distr_current->mux_count != num_muxes)
			{
				distr_current = distr_current->next;
			}

			if (distr_current == NULL)
			{
				/* Create a distribution for the new sblock type, 
				 * and put it as head of linked list by convention */

				new_distribution = (t_mux_size_distribution *)
				my_malloc(sizeof(t_mux_size_distribution));
				new_distribution->mux_count = num_muxes;
				new_distribution->max_index = max_value;
				new_distribution->distr =
				(int *)my_calloc(max_value + 1, sizeof(int));

				/* filling in the distribution */
				current = sblock_mux_list_head;
				while (current != NULL)
				{
					assert(current->size <=
							new_distribution->max_index);
					new_distribution->distr[current->size]++;
					current = current->next;
				}

				/* add it to head */
				new_distribution->next = distr_list;
				distr_list = new_distribution;
			}
			else
			{
				/* distr_current->mux_count == num_muxes so add this sblock's mux sizes in this distribution */
				current = sblock_mux_list_head;

				while (current != NULL)
				{
					if (current->size >
							distr_current->max_index)
					{
						/* needs to realloc to expand the distribution array to hold the new large-valued data */
						distr_current->distr =
						my_realloc(distr_current->
								distr,
								(current->size +
										1) * sizeof(int));

						/* initializing the newly allocated elements */
						for (k =
								(distr_current->max_index +
										1); k <= current->size; k++)
						distr_current->distr[k] = 0;

						distr_current->max_index =
						current->size;
						distr_current->distr[current->
						size]++;
					}
					else
					{
						distr_current->distr[current->
						size]++;
					}
					current = current->next;
				}
			}

			/* done - now free memory */
			current = sblock_mux_list_head;
			while (current != NULL)
			{
				next = current->next;
				free(current);
				current = next;
			}
			sblock_mux_list_head = NULL;
		}
	}

	avg_percent_range = (float)percent_range_sum / array_count;
	avg_range = (float)range_sum / array_count;

	percent_range_sum = 0.0;
	range_sum = 0.0;
	for (k = 0; k < array_count; k++)
	{
		deviation_f = (percent_range_array[k] - avg_percent_range);
		percent_range_sum += deviation_f * deviation_f;

		deviation_f = ((float)range_array[k] - avg_range);
		range_sum += deviation_f * deviation_f;
	}
	std_dev_percent_range =
	sqrt(percent_range_sum / ((float)array_count - 1.0));
	std_dev_range = sqrt(range_sum / ((float)array_count - 1.0));
	vpr_printf(TIO_MESSAGE_INFO, "==== MUX size statistics ====\n");
	vpr_printf(TIO_MESSAGE_INFO, "Max range of mux size within a sblock: %d\n", global_max_range);
	vpr_printf(TIO_MESSAGE_INFO, "Average range of mux size within a sblock: %.2f\n", avg_range);
	vpr_printf(TIO_MESSAGE_INFO, "Std dev of range of mux size within a sblock: %.2f\n", std_dev_range);
	vpr_printf(TIO_MESSAGE_INFO, "Average percent range of mux size within a sblock: %.2f%%\n", avg_percent_range * 100.0);
	vpr_printf(TIO_MESSAGE_INFO, "Std dev of percent range of mux size within a sblock: %.2f%%\n", std_dev_percent_range * 100.0);

	vpr_printf(TIO_MESSAGE_INFO, " -- Detailed MUX size distribution by sblock type -- \n");
	distr_current = distr_list;
	while (distr_current != NULL)
	{
		print_distribution(stdout, distr_current);

		/* free */
		distr_next = distr_current->next;

		free(distr_current->distr);
		free(distr_current);

		distr_current = distr_next;
	}

	free(percent_range_array);
	free(range_array);
#ifdef ENABLE_DUMP
	fclose(dump_file_per_sblock);
	fclose(dump_file);
#endif /* ENABLE_DUMP */
}

static void
print_distribution(FILE * fptr,
		t_mux_size_distribution * distr_struct)
{
	int *distr;
	int k;
	float sum;
	boolean zeros;

	distr = distr_struct->distr;
	fprintf(fptr,
			"For Sblocks containing %d MUXes, the MUX size distribution is:\n",
			distr_struct->mux_count);
	fprintf(fptr, "\t\t\tSize\t\t\tFrequency (percent)\n");

	sum = 0.0;
	for (k = 0; k <= distr_struct->max_index; k++)
	sum += distr[k];

	zeros = TRUE;
	for (k = 0; k <= distr_struct->max_index; k++)
	{
		if (zeros && (distr[k] == 0))
		{
			/* do nothing for leading string of zeros */
		}
		else
		{
			zeros = FALSE; /* leading string of zeros ended */
			fprintf(fptr, "\t\t\t%d\t\t\t%d (%.2f%%)\n", k, distr[k],
					(float)distr[k] / sum * 100.0);
		}
	}
	fprintf(fptr, "\nEnd of this Sblock MUX size distribution.\n");

}
#endif

/**
 * Parse out which CLB pins should connect directly to which other CLB pins then store that in a clb_to_clb_directs data structure
 * This data structure supplements the the info in the "directs" data structure
 * TODO: The function that does this parsing in placement is poorly done because it lacks generality on heterogeniety, should replace with this one
 */
static t_clb_to_clb_directs * alloc_and_load_clb_to_clb_directs(INP t_direct_inf *directs, INP int num_directs) {
	int i, j;
	t_clb_to_clb_directs *clb_to_clb_directs;
	char *pb_type_name, *port_name;
	int start_pin_index, end_pin_index;
	t_pb_type *pb_type;

	clb_to_clb_directs = (t_clb_to_clb_directs*)my_calloc(num_directs, sizeof(t_clb_to_clb_directs));

	pb_type_name = NULL;
	port_name = NULL;

	for(i = 0; i < num_directs; i++) {
		pb_type_name = (char*)my_malloc((strlen(directs[i].from_pin) + strlen(directs[i].to_pin)) * sizeof(char));
		port_name = (char*)my_malloc((strlen(directs[i].from_pin) + strlen(directs[i].to_pin)) * sizeof(char));

		// Load from pins
		// Parse out the pb_type name, port name, and pin range
		parse_direct_pin_name(directs[i].from_pin, directs[i].line, &start_pin_index, &end_pin_index, pb_type_name, port_name);

		// Figure out which type, port, and pin is used
		for(j = 0; j < num_types; j++) {
			if(strcmp(type_descriptors[j].name, pb_type_name) == 0) {
				break;
			}
		}
		assert(j < num_types);
		clb_to_clb_directs[i].from_clb_type = &type_descriptors[j];
		pb_type = clb_to_clb_directs[i].from_clb_type->pb_type;

		for(j = 0; j < pb_type->num_ports; j++) {
			if(strcmp(pb_type->ports[j].name, port_name) == 0) {
				break;
			}
		}
		assert(j < pb_type->num_ports);

		if(start_pin_index == OPEN) {
			assert(start_pin_index == end_pin_index);
			start_pin_index = 0;
			end_pin_index = pb_type->ports[j].num_pins - 1;
		}
		get_blk_pin_from_port_pin(clb_to_clb_directs[i].from_clb_type->index, j, start_pin_index, &clb_to_clb_directs[i].from_clb_pin_start_index);
		get_blk_pin_from_port_pin(clb_to_clb_directs[i].from_clb_type->index, j, end_pin_index, &clb_to_clb_directs[i].from_clb_pin_end_index);

		// Load to pins
		// Parse out the pb_type name, port name, and pin range
		parse_direct_pin_name(directs[i].to_pin, directs[i].line, &start_pin_index, &end_pin_index, pb_type_name, port_name);

		// Figure out which type, port, and pin is used
		for(j = 0; j < num_types; j++) {
			if(strcmp(type_descriptors[j].name, pb_type_name) == 0) {
				break;
			}
		}
		assert(j < num_types);
		clb_to_clb_directs[i].to_clb_type = &type_descriptors[j];
		pb_type = clb_to_clb_directs[i].to_clb_type->pb_type;

		for(j = 0; j < pb_type->num_ports; j++) {
			if(strcmp(pb_type->ports[j].name, port_name) == 0) {
				break;
			}
		}
		assert(j < pb_type->num_ports);

		if(start_pin_index == OPEN) {
			assert(start_pin_index == end_pin_index);
			start_pin_index = 0;
			end_pin_index = pb_type->ports[j].num_pins - 1;
		}

		get_blk_pin_from_port_pin(clb_to_clb_directs[i].to_clb_type->index, j, start_pin_index, &clb_to_clb_directs[i].to_clb_pin_start_index);
		get_blk_pin_from_port_pin(clb_to_clb_directs[i].to_clb_type->index, j, end_pin_index, &clb_to_clb_directs[i].to_clb_pin_end_index);

		if(abs(clb_to_clb_directs[i].from_clb_pin_start_index - clb_to_clb_directs[i].from_clb_pin_end_index) != abs(clb_to_clb_directs[i].to_clb_pin_start_index - clb_to_clb_directs[i].to_clb_pin_end_index)) {
			vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] Range mismatch from %s to %s.\n", directs[i].line, directs[i].from_pin, directs[i].to_pin);
				exit(1);
		}

		free(pb_type_name);
		free(port_name);
	}
	return clb_to_clb_directs;
}

/* Add all direct clb-pin-to-clb-pin edges to given opin */ 
static int get_opin_direct_connecions(int x, int y, int opin, INOUTP t_linked_edge ** edge_list_ptr, INP t_ivec *** L_rr_node_indices, 
	INP int delayless_switch, INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs) {
	t_type_ptr type;
	int grid_ofs;
	int i, ipin, inode;
	t_linked_edge *edge_list_head;
	int max_index, min_index, offset, swap;
	int new_edges;

	type = grid[x][y].type;
	edge_list_head = *edge_list_ptr;
	new_edges = 0;

	/* Iterate through all direct connections */
	for(i = 0; i < num_directs; i++) {
		/* Find matching direct clb-to-clb connections with the same type as current grid location */
		if(clb_to_clb_directs[i].from_clb_type == type) {
			/* Compute index of opin with regards to given pins */ 
			if(clb_to_clb_directs[i].from_clb_pin_start_index > clb_to_clb_directs[i].from_clb_pin_end_index) {
				swap = TRUE;
				max_index = clb_to_clb_directs[i].from_clb_pin_start_index;
				min_index = clb_to_clb_directs[i].from_clb_pin_end_index;
			} else {
				swap = FALSE;
				min_index = clb_to_clb_directs[i].from_clb_pin_start_index;
				max_index = clb_to_clb_directs[i].from_clb_pin_end_index;
			}
			if(max_index >= opin && min_index <= opin) {
				offset = opin - min_index;
				/* This opin is specified to connect directly to an ipin, now compute which ipin to connect to */
				if(x + directs[i].x_offset < nx + 1 &&
				   x + directs[i].x_offset > 0 &&
				   y + directs[i].y_offset < ny + 1 &&
				   y + directs[i].y_offset > 0) {
					   ipin = OPEN;
						if(clb_to_clb_directs[i].to_clb_pin_start_index > clb_to_clb_directs[i].to_clb_pin_end_index) {
							if(swap == TRUE) {
								ipin = clb_to_clb_directs[i].to_clb_pin_end_index + offset;
							} else {
								ipin = clb_to_clb_directs[i].to_clb_pin_start_index - offset;
							}
						} else {
							if(swap == TRUE) {
								ipin = clb_to_clb_directs[i].to_clb_pin_end_index - offset;
							} else {
								ipin = clb_to_clb_directs[i].to_clb_pin_start_index + offset;
							}
						}
						/* Add new ipin edge to list of edges */
					   grid_ofs = grid[x + directs[i].x_offset][y + directs[i].y_offset].offset;
					   inode = get_rr_node_index(x + directs[i].x_offset, y + directs[i].y_offset - grid_ofs, IPIN, ipin, L_rr_node_indices);
					   edge_list_head = insert_in_edge_list(edge_list_head, inode, delayless_switch);
					   new_edges++;
				}
			}
		}
	}
	*edge_list_ptr = edge_list_head;
	return new_edges;
}



