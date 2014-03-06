#include <cstdio>
#include <cstring>
#include <cmath>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "rr_graph_timing_params.h"
#include "rr_graph_indexed_data.h"
#include "check_rr_graph.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"

#ifdef INTERPOSER_BASED_ARCHITECTURE
#include "rr_graph_multi.h"
#endif

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
	int switch_index; //The switch type used by this direct connection
} t_clb_to_clb_directs;

/* UDSD Modifications by WMF End */

/******************* Variables local to this module. ***********************/

/* Create an array of strings which maps to an enumeration (t_rr_type)  *
 * defined in vpr_types.h, so a routing resource name can be identified *
 * by its index in the array. To retrieve desired rr-type name, call    *
 * member function rr_get_type_string() of the structure s_rr_node.     */
static const char *name_type[] = { "SOURCE", "SINK", "IPIN", "OPIN", "CHANX", 
							"CHANY", "INTRA_CLUSTER_EDGE" };

/* Used to free "chunked" memory.  If NULL, no rr_graph exists right now.  */
static t_chunk rr_mem_ch = {NULL, 0, NULL};

/* Status of current chunk being dished out by calls to my_chunk_malloc.   */

/******************* Subroutines exported by rr_graph.c ********************/

/* Member function of "struct s_rr_node" used to retrieve a routing *
 * resource type string by its index, which is defined by           *
 * "t_rr_type type".												*/
const char *s_rr_node::rr_get_type_string()
{
	return name_type[type];
}

/********************* Subroutines local to this module. *******************/
static int *****alloc_and_load_pin_to_track_map(
		INP enum e_pin_type pin_type,
		INP t_chan_width *nodes_per_chan, INP int *Fc, 
		INP t_type_ptr Type, INP boolean perturb_switch_pattern,
		INP enum e_directionality directionality);

static struct s_ivec ****alloc_and_load_track_to_pin_lookup(
		INP int *****pin_to_track_map, INP int *Fc, 
		INP int width, INP int height,
		INP int num_pins, INP int max_chan_width);

static void build_bidir_rr_opins(
		INP int i, INP int j,
		INOUTP t_rr_node * L_rr_node, INP t_ivec *** L_rr_node_indices,
		INP int ******opin_to_track_map, INP int **Fc_out,
		INP boolean * L_rr_edge_done, INP t_seg_details * seg_details,
		INP struct s_grid_tile **L_grid, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs);

static void build_unidir_rr_opins(
		INP int i, INP int j,
		INP struct s_grid_tile **L_grid, INP int **Fc_out,
		INP int max_chan_width,
		INP t_chan_details * chan_details_x, INP t_chan_details * chan_details_y, 
		INOUTP int **Fc_xofs, INOUTP int **Fc_yofs,
		INOUTP t_rr_node * L_rr_node, INOUTP boolean * L_rr_edge_done,
		OUTP boolean * Fc_clipped, INP t_ivec *** L_rr_node_indices, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs);

static int get_opin_direct_connecions(
		int x, int y, int opin, 
		INOUTP t_linked_edge ** edge_list_ptr, INP t_ivec *** L_rr_node_indices, 
		INP t_direct_inf *directs, INP int num_directs, 
		INP t_clb_to_clb_directs *clb_to_clb_directs);

static void alloc_and_load_rr_graph(
		INP int num_nodes,
		INP t_rr_node * L_rr_node, INP int num_seg_types,
		INP t_seg_details * seg_details, 
		INP t_chan_details * chan_details_x, INP t_chan_details * chan_details_y, 
		INP boolean * L_rr_edge_done,
		INP struct s_ivec *****track_to_pin_lookup,
		INP int ******opin_to_track_map, INP struct s_ivec ***switch_block_conn,
		INP struct s_grid_tile **L_grid, INP int L_nx, INP int L_ny, INP int Fs,
		INP short ******sblock_pattern, INP int **Fc_out, INP int **Fc_xofs,
		INP int **Fc_yofs, INP t_ivec *** L_rr_node_indices,
		INP int max_chan_width, INP enum e_switch_block_type sb_type,
		INP int delayless_switch, INP enum e_directionality directionality,
		INP int wire_to_ipin_switch, OUTP boolean * Fc_clipped, 
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs,
		INP boolean ignore_overrides);

static void load_uniform_switch_pattern(
		INP t_type_ptr type,
		INOUTP int *****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *width_ordering, INP int *height_ordering, 
		INP int x_chan_width, INP int y_chan_width, INP int Fc, 
		INP enum e_directionality directionality);

static void load_perturbed_switch_pattern(
		INP t_type_ptr type,
		INOUTP int *****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *width_ordering, INP int *height_ordering, 
		INP int x_chan_width, INP int y_chan_width, INP int Fc, 
		INP enum e_directionality directionality);

static boolean get_perturb_opins(INP t_type_ptr type, INP int *Fc_out, INP int max_chan_width,
		INP int num_segments, INP t_segment_inf *segment_inf);

#ifdef ENABLE_CHECK_ALL_TRACKS
static void check_all_tracks_reach_pins(
		t_type_ptr type,
		int *****tracks_connected_to_pin,
		int max_chan_width, int Fc,
		enum e_pin_type ipin_or_opin);
#endif

static boolean *alloc_and_load_perturb_ipins(
		INP int max_chan_width,
		INP int L_num_types, INP int **Fc_in, INP int **Fc_out,
		INP enum e_directionality directionality);

static void build_rr_sinks_sources(
		INP int i, INP int j,
		INP t_rr_node * L_rr_node, INP t_ivec *** L_rr_node_indices,
		INP int delayless_switch, INP struct s_grid_tile **L_grid);

static void build_rr_xchan(
		INP int i, INP int j,
		INP struct s_ivec *****track_to_pin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int max_chan_width, INP int tracks_per_chan, INP int *opin_mux_size,
		INP short ******sblock_pattern, INP int Fs_per_side,
		INP t_chan_details * chan_details_x, INP t_chan_details * chan_details_y, 
		INP t_ivec *** L_rr_node_indices,
		INP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality);

static void build_rr_ychan(
		INP int i, INP int j,
		INP struct s_ivec *****track_to_pin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int max_chan_width, INP int tracks_per_chan, INP int *opin_mux_size,
		INP short ******sblock_pattern, INP int Fs_per_side,
		INP t_chan_details * chan_details_y, INP t_chan_details * chan_details_x, 
		INP t_ivec *** L_rr_node_indices,
		INP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality);

static void rr_graph_externals(
		t_timing_inf timing_inf,
		t_segment_inf * segment_inf, int num_seg_types, int max_chan_width,
		int wire_to_ipin_switch, enum e_base_cost_type base_cost_type);

void alloc_and_load_edges_and_switches(
		INP t_rr_node * L_rr_node, INP int inode,
		INP int num_edges, INP boolean * L_rr_edge_done,
		INP t_linked_edge * edge_list_head);

static void alloc_net_rr_terminals(void);

static void alloc_and_load_rr_clb_source(t_ivec *** L_rr_node_indices);

static t_clb_to_clb_directs *alloc_and_load_clb_to_clb_directs(INP t_direct_inf *directs, INP int num_directs,
        INP int delayless_switch);

#if 0
static void load_uniform_opin_switch_pattern_paired(
		INP int *Fc_out,
		INP int num_pins,
		INP int *pins_in_chan_seg,
		INP int num_wire_inc_muxes,
		INP int num_wire_dec_muxes,
		INP int *wire_inc_muxes,
		INP int *wire_dec_muxes,
		INOUTP t_rr_node * L_rr_node,
		INOUTP boolean * L_rr_edge_done,
		INP t_seg_details * seg_details,
		OUTP boolean * Fc_clipped);
#endif
void watch_edges(int inode, t_linked_edge * edge_list_head);
#if MUX_SIZE_DIST_DISPLAY
static void view_mux_size_distribution(
		t_ivec *** L_rr_node_indices,
		int max_chan_width,
		t_seg_details * seg_details_x,
		t_seg_details * seg_details_y);
static void print_distribution(FILE * fptr,
		t_mux_size_distribution * distr_struct);
#endif

static void free_type_pin_to_track_map(
		int ******ipin_to_track_map,
		t_type_ptr types);

static void free_type_track_to_pin_map(
		struct s_ivec *****track_to_pin_map,
		t_type_ptr types, int max_chan_width);

static t_seg_details *alloc_and_load_global_route_seg_details(
		INP int global_route_switch,
		OUTP int * num_seg_details = 0);

/* UDSD Modifications by WMF End */

static int **alloc_and_load_actual_fc(
		INP int L_num_types, INP t_type_ptr types,
		INP int max_chan_width, INP boolean is_Fc_out,
		INP enum e_directionality directionality, 
		OUTP boolean *Fc_clipped, INP boolean ignore_Fc_0);

/******************* Subroutine definitions *******************************/

void build_rr_graph(
		INP t_graph_type graph_type, INP int L_num_types,
		INP t_type_ptr types, INP int L_nx, INP int L_ny,
		INP struct s_grid_tile **L_grid, 
		INP struct s_chan_width *nodes_per_chan,
		INP struct s_chan_width_dist *chan_capacity_inf,
		INP enum e_switch_block_type sb_type, INP int Fs, 
		INP int num_seg_types, INP int num_switches, 
		INP t_segment_inf * segment_inf,
		INP int global_route_switch, INP int delayless_switch,
		INP t_timing_inf timing_inf, INP int wire_to_ipin_switch,
		INP enum e_base_cost_type base_cost_type, 
		INP boolean trim_empty_channels,
		INP boolean trim_obs_channels,
		INP t_direct_inf *directs, INP int num_directs, 
		INP boolean ignore_Fc_0, INP boolean ignore_overrides,
		OUTP int *Warnings) {

	/* Reset warning flag */
	*Warnings = RR_GRAPH_NO_WARN;

	/* Decode the graph_type */
	boolean is_global_graph = (GRAPH_GLOBAL == graph_type ? TRUE : FALSE);
	boolean use_full_seg_groups = (GRAPH_UNIDIR_TILEABLE == graph_type ? TRUE : FALSE);
	enum e_directionality directionality = (GRAPH_BIDIR == graph_type ? BI_DIRECTIONAL : UNI_DIRECTIONAL);
	if (is_global_graph) {
		directionality = BI_DIRECTIONAL;
	}

	/* Global routing uses a single longwire track */
	int max_chan_width = (is_global_graph ? 1 : nodes_per_chan->max);
	assert(max_chan_width > 0);

	t_clb_to_clb_directs *clb_to_clb_directs = NULL;
	if(num_directs > 0) {
		clb_to_clb_directs = alloc_and_load_clb_to_clb_directs(directs, num_directs, delayless_switch);
	}

	/* START SEG_DETAILS */
	int num_seg_details = 0;
	t_seg_details *seg_details = NULL;

	if (is_global_graph) {
		/* Sets up a single unit length segment type for global routing. */
		seg_details = alloc_and_load_global_route_seg_details(
				global_route_switch, &num_seg_details);
	} else {
		/* Setup segments including distrubuting tracks and staggering.
		 * If use_full_seg_groups is specified, max_chan_width may be 
		 * changed. Warning should be singled to caller if this happens. */
		seg_details = alloc_and_load_seg_details(&max_chan_width,
				max(L_nx, L_ny), num_seg_types, segment_inf,
				use_full_seg_groups, is_global_graph, directionality,
				&num_seg_details);
		if ((is_global_graph ? 1 : nodes_per_chan->max) != max_chan_width) {
			nodes_per_chan->max = max_chan_width;
			*Warnings |= RR_GRAPH_WARN_CHAN_WIDTH_CHANGED;
		}

		if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_SEG_DETAILS)) {
			dump_seg_details(seg_details, max_chan_width,
					getEchoFileName(E_ECHO_SEG_DETAILS));
		}
	}
	/* END SEG_DETAILS */

	/* START CHAN_DETAILS */
	t_chan_details *chan_details_x = NULL;
	t_chan_details *chan_details_y = NULL;

	alloc_and_load_chan_details(L_nx, L_ny, nodes_per_chan, 
			trim_empty_channels, trim_obs_channels,
			num_seg_details, seg_details,
			&chan_details_x, &chan_details_y);

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CHAN_DETAILS)) {
		dump_chan_details( chan_details_x, chan_details_y, max_chan_width, nx, ny,
				getEchoFileName(E_ECHO_CHAN_DETAILS));
	}
	/* END CHAN_DETAILS */

	/* START FC */
	/* Determine the actual value of Fc */
	int **Fc_in = NULL; /* [0..num_types-1][0..num_pins-1] */
	int **Fc_out = NULL; /* [0..num_types-1][0..num_pins-1] */

	if (is_global_graph) {
		Fc_in = (int **) my_malloc(sizeof(int) * L_num_types);
		Fc_out = (int **) my_malloc(sizeof(int) * L_num_types);
		for (int i = 0; i < L_num_types; ++i) {
			for (int j = 0; j < types[i].num_pins; ++j) {
				Fc_in[i][j] = 1;
				Fc_out[i][j] = 1;
			}
		}
	} else {
		boolean Fc_clipped = FALSE;
		Fc_in = alloc_and_load_actual_fc(L_num_types, types, max_chan_width,
				FALSE, directionality, &Fc_clipped, ignore_Fc_0);
		if (Fc_clipped) {
			*Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
		}
		Fc_clipped = FALSE;
		Fc_out = alloc_and_load_actual_fc(L_num_types, types, max_chan_width,
				TRUE, directionality, &Fc_clipped, ignore_Fc_0);
		if (Fc_clipped) {
			*Warnings |= RR_GRAPH_WARN_FC_CLIPPED;
		}

#ifdef VERBOSE
		for (i = 1; i < L_num_types; ++i) { /* Skip "<EMPTY>" */
			for (j = 0; j < type_descriptors[i].num_pins; ++j) { 
				if (type_descriptors[i].is_Fc_full_flex[j]) {
					vpr_printf_info("Fc Actual Values: type = %s, Fc_out = full, Fc_in = %d.\n",
							type_descriptors[i].name, Fc_in[i][j]);
				}
				else {
					vpr_printf_info("Fc Actual Values: type = %s, Fc_out = %d, Fc_in = %d.\n",
							type_descriptors[i].name, Fc_out[i][j], Fc_in[i][j]);
				}
			}
		}
#endif /* VERBOSE */
	}

	boolean *perturb_ipins = alloc_and_load_perturb_ipins(max_chan_width, L_num_types,
			Fc_in, Fc_out, directionality);
	/* END FC */
	
	/* Alloc node lookups, count nodes, alloc rr nodes */
	num_rr_nodes = 0;

	rr_node_indices = alloc_and_load_rr_node_indices(max_chan_width, L_nx, L_ny,
			&num_rr_nodes, chan_details_x, chan_details_y);
	rr_node = (t_rr_node *) my_malloc(sizeof(t_rr_node) * num_rr_nodes);
	memset(rr_node, 0, sizeof(t_rr_node) * num_rr_nodes);
	boolean *L_rr_edge_done = (boolean *) my_malloc(sizeof(boolean) * num_rr_nodes);
	memset(L_rr_edge_done, 0, sizeof(boolean) * num_rr_nodes);

	/* These are data structures used by the the unidir opin mapping. */
	int **Fc_xofs = NULL; /* [0..ny-1][0..nx-1] */
	int **Fc_yofs = NULL; /* [0..nx-1][0..ny-1] */
	if (UNI_DIRECTIONAL == directionality) {
		Fc_xofs = (int **) alloc_matrix(0, L_ny, 0, L_nx, sizeof(int));
		Fc_yofs = (int **) alloc_matrix(0, L_nx, 0, L_ny, sizeof(int));
		for (int i = 0; i <= L_nx; ++i) {
			for (int j = 0; j <= L_ny; ++j) {
				Fc_xofs[j][i] = 0;
				Fc_yofs[i][j] = 0;
			}
		}
	}

	/* START SB LOOKUP */
	/* Alloc and load the switch block lookup */
	t_ivec ***switch_block_conn = NULL;
	short ******unidir_sb_pattern = NULL;

	if (is_global_graph) {
		switch_block_conn = alloc_and_load_switch_block_conn(1, SUBSET, 3);
	} else if (BI_DIRECTIONAL == directionality) {
		switch_block_conn = alloc_and_load_switch_block_conn(max_chan_width,
				sb_type, Fs);
	} else {
		assert(UNI_DIRECTIONAL == directionality);

		unidir_sb_pattern = alloc_sblock_pattern_lookup(L_nx, L_ny, max_chan_width);
		for (int i = 0; i <= L_nx; i++) {
			for (int j = 0; j <= L_ny; j++) {
				load_sblock_pattern_lookup(i, j, nodes_per_chan,
						chan_details_x, chan_details_y,
						Fs, sb_type, unidir_sb_pattern);

				if (!ignore_overrides) {
					override_sblock_pattern_lookup(i, j, max_chan_width,
							unidir_sb_pattern);
				}
			}
		}

		if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_SBLOCK_PATTERN)) {
			dump_sblock_pattern( unidir_sb_pattern, max_chan_width, L_nx, L_ny,
					getEchoFileName(E_ECHO_SBLOCK_PATTERN));
		}
	}
	/* END SB LOOKUP */

	/* START IPINP MAP */
	/* Create ipin map lookups */
	int ******ipin_to_track_map = NULL; /* [0..num_types-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */
	t_ivec *****track_to_pin_lookup = NULL; /* [0..num_types-1][0..max_chan_width-1][0..width][0..height][0..3] */

	ipin_to_track_map = (int ******) my_malloc(sizeof(int *****) * L_num_types);
	track_to_pin_lookup = (struct s_ivec *****) my_malloc(sizeof(struct s_ivec ****) * L_num_types);
	for (int i = 0; i < L_num_types; ++i) {
		ipin_to_track_map[i] = alloc_and_load_pin_to_track_map(RECEIVER,
				nodes_per_chan, Fc_in[i], &types[i], perturb_ipins[i], directionality);
		track_to_pin_lookup[i] = alloc_and_load_track_to_pin_lookup(
				ipin_to_track_map[i], Fc_in[i], types[i].width, types[i].height,
				types[i].num_pins, max_chan_width);
	}
	/* END IPINP MAP */

	/* START OPINP MAP */
	/* Create opin map lookups */
	int ******opin_to_track_map = NULL; /* [0..num_types-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */

	if (BI_DIRECTIONAL == directionality) {
		opin_to_track_map = (int ******) my_malloc(sizeof(int *****) * L_num_types);
		for (int i = 0; i < L_num_types; ++i) {
			boolean perturb_opins = get_perturb_opins(&types[i], Fc_out[i], 
					max_chan_width,	num_seg_types, segment_inf);
			opin_to_track_map[i] = alloc_and_load_pin_to_track_map(DRIVER,
					nodes_per_chan, Fc_out[i], &types[i], perturb_opins, directionality);
		} 
	}
	/* END OPINP MAP */

	/* UDSD Modifications by WMF begin */
	/* I'm adding 2 new fields to t_rr_node, and I want them initialized to 0. */
	for (int i = 0; i < num_rr_nodes; i++) {
		rr_node[i].num_wire_drivers = 0;
		rr_node[i].num_opin_drivers = 0;
	}

	boolean Fc_clipped = FALSE;
	alloc_and_load_rr_graph(num_rr_nodes, rr_node, num_seg_types, 
			seg_details, chan_details_x, chan_details_y,
			L_rr_edge_done, track_to_pin_lookup, opin_to_track_map,
			switch_block_conn, L_grid, L_nx, L_ny, Fs, unidir_sb_pattern,
			Fc_out, Fc_xofs, Fc_yofs, rr_node_indices, max_chan_width, sb_type,
			delayless_switch, directionality, wire_to_ipin_switch, &Fc_clipped, 
			directs, num_directs, clb_to_clb_directs, ignore_overrides);

#ifdef MUX_SIZE_DIST_DISPLAY
	if (UNI_DIRECTIONAL == directionality)
	{
		view_mux_size_distribution(rr_node_indices, max_chan_width,
				seg_details, seg_details);
	}
#endif

	/* Update rr_nodes capacities if global routing */
	if (graph_type == GRAPH_GLOBAL) {
		for (int i = 0; i < num_rr_nodes; i++) {
			if (rr_node[i].type == CHANX || rr_node[i].type == CHANY) {
				rr_node[i].capacity = chan_width.max;
			}
		}
	}

	rr_graph_externals(timing_inf, segment_inf, num_seg_types, max_chan_width,
			wire_to_ipin_switch, base_cost_type);
	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_RR_GRAPH)) {
		dump_rr_graph(getEchoFileName(E_ECHO_RR_GRAPH));
	}

#ifdef INTERPOSER_BASED_ARCHITECTURE
	/* Main Entry Point to rr_graph modifications for interposer-based architectures */
	if(num_cuts > 0)
	{
		modify_rr_graph_for_interposer_based_arch(max_chan_width, directionality);
	}
#endif

	check_rr_graph(graph_type, L_nx, L_ny, num_switches, Fc_in);

	/* Free all temp structs */
	if (seg_details) {
		free_seg_details(seg_details, max_chan_width);
		seg_details = NULL;
	}
	if (chan_details_x || chan_details_y) {
		free_chan_details(chan_details_x, chan_details_y, max_chan_width, L_nx, L_ny);
		chan_details_x = NULL;
		chan_details_y = NULL;
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
		free_switch_block_conn(switch_block_conn, max_chan_width);
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
		for (int i = 0; i < L_num_types; ++i) {
			free_matrix5(opin_to_track_map[i], 0, types[i].num_pins - 1,
					0, types[i].width - 1, 0, types[i].height - 1, 
					0, 3, 0, sizeof(int));
		}
		free(opin_to_track_map);
	}

	free_type_pin_to_track_map(ipin_to_track_map, types);
	free_type_track_to_pin_map(track_to_pin_lookup, types, max_chan_width);
	if(clb_to_clb_directs != NULL) {
		free(clb_to_clb_directs);
	}
}

static void rr_graph_externals(t_timing_inf timing_inf,
		t_segment_inf * segment_inf, int num_seg_types, int max_chan_width,
		int wire_to_ipin_switch, enum e_base_cost_type base_cost_type) {

	add_rr_graph_C_from_switches(timing_inf.C_ipin_cblock);
	alloc_and_load_rr_indexed_data(segment_inf, num_seg_types, rr_node_indices,
			max_chan_width, wire_to_ipin_switch, base_cost_type);

	alloc_net_rr_terminals();
	load_net_rr_terminals(rr_node_indices);
	alloc_and_load_rr_clb_source(rr_node_indices);
}

static boolean *alloc_and_load_perturb_ipins(INP int max_chan_width, INP int L_num_types,
		INP int **Fc_in, INP int **Fc_out, 
		INP enum e_directionality directionality) {

	boolean *result = (boolean *) my_malloc(L_num_types * sizeof(boolean));

	if (BI_DIRECTIONAL == directionality) {
		result[0] = FALSE;
		for (int i = 1; i < L_num_types; ++i) {
			result[i] = FALSE;

			float Fc_ratio;
			if (Fc_in[i][0] > Fc_out[i][0]) {
				Fc_ratio = (float) Fc_in[i][0] / (float) Fc_out[i][0];
			} else {
				Fc_ratio = (float) Fc_out[i][0] / (float) Fc_in[i][0];
			}

			if ((Fc_in[i][0] <= max_chan_width - 2)
					&& (fabs(Fc_ratio - nint(Fc_ratio))
							< (0.5 / (float) max_chan_width))) {
				result[i] = TRUE;
			}
		}
	} else {
		/* Unidirectional routing uses mux balancing patterns and 
		 * thus shouldn't need perturbation. */
		assert(UNI_DIRECTIONAL == directionality);
		for (int i = 0; i < L_num_types; ++i) {
			result[i] = FALSE;
		}
	}
	return result;
}

static t_seg_details *alloc_and_load_global_route_seg_details(
		INP int global_route_switch,
		OUTP int * num_seg_details) {

	t_seg_details *seg_details = (t_seg_details *) my_malloc(sizeof(t_seg_details));

	seg_details->index = 0;
	seg_details->length = 1;
	seg_details->wire_switch = global_route_switch;
	seg_details->opin_switch = global_route_switch;
	seg_details->longline = FALSE;
	seg_details->direction = BI_DIRECTION;
	seg_details->Cmetal = 0.0;
	seg_details->Rmetal = 0.0;
	seg_details->start = 1;
	seg_details->drivers = MULTI_BUFFERED;
	seg_details->cb = (boolean *) my_malloc(sizeof(boolean) * 1);
	seg_details->cb[0] = TRUE;
	seg_details->sb = (boolean *) my_malloc(sizeof(boolean) * 2);
	seg_details->sb[0] = TRUE;
	seg_details->sb[1] = TRUE;
	seg_details->group_size = 1;
	seg_details->group_start = 0;
	seg_details->seg_start = -1;
	seg_details->seg_end = -1;

	if (num_seg_details) {
		*num_seg_details = 1;
	}
	return seg_details;
}

/* Calculates the actual Fc values for the given max_chan_width value */
static int **alloc_and_load_actual_fc(INP int L_num_types, INP t_type_ptr types,
		INP int max_chan_width, INP boolean is_Fc_out,
		INP enum e_directionality directionality, 
		OUTP boolean *Fc_clipped, INP boolean ignore_Fc_0) {

	int i, j;
	int **Result = NULL;
	int fac, num_sets;

	*Fc_clipped = FALSE;

	/* Unidir tracks formed in pairs, otherwise no effect. */
	fac = 1;
	if (UNI_DIRECTIONAL == directionality) {
		fac = 2;
	}

	assert((max_chan_width % fac) == 0);
	num_sets = max_chan_width / fac;

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
					Result[i][j] = max_chan_width;
				}

				Result[i][j] = max(Result[i][j], fac);
				if (Result[i][j] > max_chan_width) {
					*Fc_clipped = TRUE;
					Result[i][j] = max_chan_width;
				}
			}
			assert(Result[i][j] % fac == 0);
		}
		free(Fc);
	}

	return Result;
}

/* frees the ipin to track mapping for each physical grid type */
static void free_type_pin_to_track_map(int ******ipin_to_track_map,
		t_type_ptr types) {

	for (int i = 0; i < num_types; ++i) {
		free_matrix5(ipin_to_track_map[i], 0, types[i].num_pins - 1,
				0, types[i].width - 1, 0, types[i].height - 1, 
				0, 3, 0, sizeof(int));
	}
	free(ipin_to_track_map);
}

/* frees the track to ipin mapping for each physical grid type */
static void free_type_track_to_pin_map(struct s_ivec***** track_to_pin_map,
		t_type_ptr types, int max_chan_width) {

	for (int i = 0; i < num_types; i++) {
		if (track_to_pin_map[i] != NULL) {
			for (int track = 0; track < max_chan_width; ++track) {
				for (int width = 0; width < types[i].width; ++width) {
					for (int height = 0; height < types[i].height; ++height) {
						for (int side = 0; side < 4; ++side) {
							if (track_to_pin_map[i][track][width][height][side].list != NULL) {
								free(track_to_pin_map[i][track][width][height][side].list);
							}
						}
					}
				}
			}
			free_matrix4(track_to_pin_map[i], 0, max_chan_width - 1,
					0, types[i].width - 1, 0, types[i].height - 1, 
					0, sizeof(struct s_ivec));
		}
	}
	free(track_to_pin_map);
}

/* Does the actual work of allocating the rr_graph and filling all the *
 * appropriate values.  Everything up to this was just a prelude!      */
static void alloc_and_load_rr_graph(INP int num_nodes,
		INP t_rr_node * L_rr_node, INP int num_seg_types,
		INP t_seg_details * seg_details, 
		INP t_chan_details * chan_details_x, INP t_chan_details * chan_details_y, 
		INP boolean * L_rr_edge_done,
		INP struct s_ivec *****track_to_pin_lookup,
		INP int ******opin_to_track_map, INP struct s_ivec ***switch_block_conn,
		INP struct s_grid_tile **L_grid, INP int L_nx, INP int L_ny, INP int Fs,
		INP short ******sblock_pattern, INP int **Fc_out, INP int **Fc_xofs,
		INP int **Fc_yofs, INP t_ivec *** L_rr_node_indices,
		INP int max_chan_width, INP enum e_switch_block_type sb_type,
		INP int delayless_switch, INP enum e_directionality directionality,
		INP int wire_to_ipin_switch, OUTP boolean * Fc_clipped,
		INP t_direct_inf *directs, INP int num_directs,
		INP t_clb_to_clb_directs *clb_to_clb_directs,
		INP boolean ignore_overrides) {

	/* If Fc gets clipped, this will be flagged to true */
	*Fc_clipped = FALSE;

	/* Connection SINKS and SOURCES to their pins. */
	for (int i = 0; i <= (L_nx + 1); ++i) {
		for (int j = 0; j <= (L_ny + 1); ++j) {
			build_rr_sinks_sources(i, j, L_rr_node, L_rr_node_indices,
					delayless_switch, L_grid);
		}
	}

	/* Build opins */
	for (int i = 0; i <= (L_nx + 1); ++i) {
		for (int j = 0; j <= (L_ny + 1); ++j) {
			if (BI_DIRECTIONAL == directionality) {
				build_bidir_rr_opins(i, j, L_rr_node, L_rr_node_indices,
						opin_to_track_map, Fc_out, L_rr_edge_done, seg_details,
						L_grid, delayless_switch,
						directs, num_directs, clb_to_clb_directs);
			} else {
				assert(UNI_DIRECTIONAL == directionality);
				boolean clipped;
				build_unidir_rr_opins(i, j, L_grid, Fc_out, max_chan_width,
						chan_details_x, chan_details_y, Fc_xofs, Fc_yofs, L_rr_node,
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
	int *opin_mux_size = (int *) my_malloc(sizeof(int) * num_nodes);
	for (int i = 0; i < num_nodes; ++i) {
		opin_mux_size[i] = L_rr_node[i].fan_in;
	}

	/* Build channels */
	assert(Fs % 3 == 0);
	for (int i = 0; i <= L_nx; ++i) {
		for (int j = 0; j <= L_ny; ++j) {
			if (i > 0) {
				build_rr_xchan(i, j, track_to_pin_lookup, switch_block_conn,
						CHANX_COST_INDEX_START, 
						max_chan_width, chan_width.x_list[j], opin_mux_size,
						sblock_pattern, Fs / 3, chan_details_x, chan_details_y,
						L_rr_node_indices, L_rr_edge_done, L_rr_node, 
						wire_to_ipin_switch, directionality);
			}
			if (j > 0) {
				build_rr_ychan(i, j, track_to_pin_lookup, switch_block_conn,
						CHANX_COST_INDEX_START + num_seg_types, 
						max_chan_width, chan_width.y_list[i], opin_mux_size,
						sblock_pattern, Fs / 3, chan_details_y, chan_details_x,
						L_rr_node_indices, L_rr_edge_done, L_rr_node,
						wire_to_ipin_switch, directionality);
			}
		}
	}

	if (!ignore_overrides) {
		override_cblock_edge_lists(num_rr_nodes, rr_node);
	}
	free(opin_mux_size);
}

static void build_bidir_rr_opins(INP int i, INP int j,
		INOUTP t_rr_node * L_rr_node, INP t_ivec *** L_rr_node_indices,
		INP int ******opin_to_track_map, INP int **Fc_out,
		INP boolean * L_rr_edge_done, INP t_seg_details * seg_details,
		INP struct s_grid_tile **L_grid, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs) {

	/* OPINP edges need to be done at once so let the offset 0
	 * block do the work. */
	if (L_grid[i][j].width_offset > 0 || L_grid[i][j].height_offset > 0) {
		return;
	}

	t_type_ptr type = L_grid[i][j].type;
	int *Fc = Fc_out[type->index];

	for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
		/* We only are working with opins so skip non-drivers */
		if (type->class_inf[type->pin_class[pin_index]].type != DRIVER) {
			continue;
		}

		int num_edges = 0;
		struct s_linked_edge *edge_list = NULL;
		if(Fc[pin_index] != 0) {
			for (int width = 0; width < type->width; ++width) {
				for (int height = 0; height < type->height; ++height) {
					num_edges += get_bidir_opin_connections(i + width, j + height, pin_index,
							&edge_list, opin_to_track_map, Fc[pin_index], L_rr_edge_done,
							L_rr_node_indices, seg_details);
				}
			}
		}

		/* Add in direct connections */
		num_edges += get_opin_direct_connecions(i, j, pin_index, &edge_list, L_rr_node_indices, 
				directs, num_directs, clb_to_clb_directs);

		int node_index = get_rr_node_index(i, j, OPIN, pin_index, L_rr_node_indices);
		alloc_and_load_edges_and_switches(L_rr_node, node_index, num_edges,
				L_rr_edge_done, edge_list);
		while (edge_list != NULL) {
			struct s_linked_edge *next_edge = edge_list->next;
			free(edge_list);
			edge_list = next_edge;
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
	unsigned int inet;

	net_rr_terminals = (int **) my_malloc(g_clbs_nlist.net.size() * sizeof(int *));

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		net_rr_terminals[inet] = (int *) my_chunk_malloc(
				g_clbs_nlist.net[inet].pins.size() * sizeof(int),
				&rr_mem_ch);
	}
}

void load_net_rr_terminals(t_ivec *** L_rr_node_indices) {

	/* Allocates and loads the net_rr_terminals data structure.  For each net   *
	 * it stores the rr_node index of the SOURCE of the net and all the SINKs   *
	 * of the net.  [0..g_clbs_nlist.net.size()-1][0..num_pins-1].  Entry [inet][pnum] stores  *
	 * the rr index corresponding to the SOURCE (opin) or SINK (ipin) of pnum.  */

	int inode, iblk, i, j, node_block_pin, iclass;
	unsigned int ipin, inet;
	t_type_ptr type;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		for (ipin = 0; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
			iblk = g_clbs_nlist.net[inet].pins[ipin].block;
			i = block[iblk].x;
			j = block[iblk].y;
			type = block[iblk].type;

			/* In the routing graph, each (x, y) location has unique pins on it
			 * so when there is capacity, blocks are packed and their pin numbers
			 * are offset to get their actual rr_node */
			node_block_pin = g_clbs_nlist.net[inet].pins[ipin].block_pin;

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

	/* Since we share nodes within a large block, only 
	 * start tile can initialize sinks, sources, and pins */
	if (L_grid[i][j].width_offset > 0 || L_grid[i][j].height_offset > 0)
		return;

	t_type_ptr type = L_grid[i][j].type;
	int num_class = type->num_class;
	struct s_class *class_inf = type->class_inf;
	int num_pins = type->num_pins;
	int *pin_class = type->pin_class;

	/* SINKS and SOURCE to OPIN edges */
	for (int iclass = 0; iclass < num_class; ++iclass) {
		int inode = 0;
		if (class_inf[iclass].type == DRIVER) { /* SOURCE */
			inode = get_rr_node_index(i, j, SOURCE, iclass, L_rr_node_indices);

			int num_edges = class_inf[iclass].num_pins;
			L_rr_node[inode].num_edges = num_edges;
			L_rr_node[inode].edges = (int *) my_malloc(num_edges * sizeof(int));
			L_rr_node[inode].switches = (short *) my_malloc(num_edges * sizeof(short));

			for (int ipin = 0; ipin < class_inf[iclass].num_pins; ++ipin) {
				int pin_num = class_inf[iclass].pinlist[ipin];
				int to_node = get_rr_node_index(i, j, OPIN, pin_num, L_rr_node_indices);
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
		L_rr_node[inode].xhigh = i + type->width - 1;
		L_rr_node[inode].ylow = j;
		L_rr_node[inode].yhigh = j + type->height - 1;
		L_rr_node[inode].R = 0;
		L_rr_node[inode].C = 0;
		L_rr_node[inode].ptc_num = iclass;
		L_rr_node[inode].direction = (enum e_direction)OPEN;
		L_rr_node[inode].drivers = (enum e_drivers)OPEN;
	}

	int ipb_pin = 0;
	int opb_pin = 0;
	int iport = 0;
	int oport = 0;
	int iporttype = 0;

	const t_pb_graph_node *pb_graph_node = type->pb_graph_head;
	if(pb_graph_node != NULL && pb_graph_node->num_input_ports == 0) {
		iporttype = 1;
	}
	/* Connect IPINS to SINKS and dummy for OPINS */
	for (int ipin = 0; ipin < num_pins; ++ipin) {
		int inode = 0;;
		int iclass = pin_class[ipin];
		int z = ipin / (type->pb_type->num_clock_pins + type->pb_type->num_output_pins + type->pb_type->num_input_pins);

		if (class_inf[iclass].type == RECEIVER) {
			inode = get_rr_node_index(i, j, IPIN, ipin, L_rr_node_indices);
			int to_node = get_rr_node_index(i, j, SINK, iclass, L_rr_node_indices);

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
				ipb_pin++;
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
			
			L_rr_node[inode].pb_graph_pin = &pb_graph_node->output_pins[oport][opb_pin];
			opb_pin++;
			if(opb_pin >= pb_graph_node->num_output_pins[oport]) {
				oport++;
				opb_pin = 0;
				if(oport >= pb_graph_node->num_output_ports) {
					oport = 0;
				}
			}
		}

		/* Common to both DRIVERs and RECEIVERs */
		L_rr_node[inode].capacity = 1;
		L_rr_node[inode].occ = 0;
		L_rr_node[inode].xlow = i;
		L_rr_node[inode].xhigh = i + type->width - 1;
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
		INP struct s_ivec *****track_to_pin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int max_chan_width, INP int tracks_per_chan, INP int *opin_mux_size,
		INP short ******sblock_pattern, INP int Fs_per_side,
		INP t_chan_details * chan_details_x, INP t_chan_details * chan_details_y,
		INP t_ivec *** L_rr_node_indices,
		INOUTP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality) {

	t_seg_details * seg_details = chan_details_x[i][j];

	/* Loads up all the routing resource nodes in the x-directed channel segments starting at (i,j). */
	for (int track = 0; track < tracks_per_chan; ++track) {

		if (seg_details[track].length == 0)
			continue;

		int start = get_seg_start(seg_details, track, j, i);
		int end = get_seg_end(seg_details, track, start, j, nx);

		if (i > start)
			continue; /* Not the start of this segment. */

		struct s_linked_edge *edge_list = NULL;

		/* First count number of edges and put the edges in a linked list. */
		int num_edges = 0;
		num_edges += get_track_to_pins(start, j, track, tracks_per_chan, &edge_list,
				L_rr_node_indices, track_to_pin_lookup, seg_details, CHANX, nx,
				wire_to_ipin_switch, directionality);

		if (j > 0) {
			t_seg_details * from_seg_details = chan_details_x[start][j];
			t_seg_details * to_seg_details = chan_details_y[start][j];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(j, start, track, CHANX, j,
						CHANY, nx, max_chan_width, opin_mux_size, 
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_y, 
						directionality,	L_rr_node_indices, L_rr_edge_done,
						 switch_block_conn);
			}
		}
		if (j < ny) {
			t_seg_details * from_seg_details = chan_details_x[start][j];
			t_seg_details * to_seg_details = chan_details_y[start][j+1];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(j, start, track, CHANX, j + 1,
						CHANY, nx, max_chan_width, opin_mux_size, 
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_y, 
						directionality,	L_rr_node_indices, L_rr_edge_done, 
						switch_block_conn);
			}
		}
		if (start > 1) {
			t_seg_details * from_seg_details = chan_details_x[start][j];
			t_seg_details * to_seg_details = chan_details_x[start-1][j];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(j, start, track, CHANX, start - 1,
						CHANX, nx, max_chan_width, opin_mux_size,
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_x,
						directionality, L_rr_node_indices, L_rr_edge_done,
						switch_block_conn);
			}
		}
		if (end < nx) {
			t_seg_details * from_seg_details = chan_details_x[start][j];
			t_seg_details * to_seg_details = chan_details_x[end+1][j];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(j, start, track, CHANX, end + 1,
						CHANX, nx, max_chan_width, opin_mux_size, 
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_x, 
						directionality,	L_rr_node_indices, L_rr_edge_done, 
						switch_block_conn);
			}
		}

		int node = get_rr_node_index(i, j, CHANX, track, L_rr_node_indices);
		alloc_and_load_edges_and_switches(L_rr_node, node, num_edges,
				L_rr_edge_done, edge_list);

		while (edge_list != NULL) {
			struct s_linked_edge *next_edge = edge_list->next;
			free(edge_list);
			edge_list = next_edge;
		}

		/* Edge arrays have now been built up.  Do everything else.  */
		L_rr_node[node].cost_index = cost_index_offset + seg_details[track].index;
		L_rr_node[node].occ = ( track < tracks_per_chan ? 0 : 1 );
		L_rr_node[node].capacity = 1; /* GLOBAL routing handled elsewhere */

		L_rr_node[node].xlow = start;
		L_rr_node[node].xhigh = end;
		L_rr_node[node].ylow = j;
		L_rr_node[node].yhigh = j;

		int length = end - start + 1;
		L_rr_node[node].R = length * seg_details[track].Rmetal;
		L_rr_node[node].C = length * seg_details[track].Cmetal;

		L_rr_node[node].ptc_num = track;
		L_rr_node[node].type = CHANX;
		L_rr_node[node].direction = seg_details[track].direction;
		L_rr_node[node].drivers = seg_details[track].drivers;
	}
}

static void build_rr_ychan(INP int i, INP int j,
		INP struct s_ivec *****track_to_pin_lookup,
		INP struct s_ivec ***switch_block_conn, INP int cost_index_offset,
		INP int max_chan_width, INP int tracks_per_chan, INP int *opin_mux_size,
		INP short ******sblock_pattern, INP int Fs_per_side,
		INP t_chan_details * chan_details_y, INP t_chan_details * chan_details_x,
		INP t_ivec *** L_rr_node_indices,
		INP boolean * L_rr_edge_done, INOUTP t_rr_node * L_rr_node,
		INP int wire_to_ipin_switch, INP enum e_directionality directionality) {

	t_seg_details * seg_details = chan_details_y[i][j];

	/* Loads up all the routing resource nodes in the y-directed channel segments starting at (i,j). */
	for (int track = 0; track < tracks_per_chan; ++track) {

		if (seg_details[track].length == 0)
			continue;

		int start = get_seg_start(seg_details, track, i, j);
		int end = get_seg_end(seg_details, track, start, i, ny);

		if (j > start)
			continue; /* Not the start of this segment. */

		struct s_linked_edge *edge_list = NULL;

		/* First count number of edges and put the edges in a linked list. */
		int num_edges = 0;
		num_edges += get_track_to_pins(start, i, track, tracks_per_chan, &edge_list,
				L_rr_node_indices, track_to_pin_lookup, seg_details, CHANY, ny,
				wire_to_ipin_switch, directionality);

		if (i > 0) {
			t_seg_details * from_seg_details = chan_details_y[i][start];
			t_seg_details * to_seg_details = chan_details_x[i][start];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(i, start, track, CHANY, i,
						CHANX, ny, max_chan_width, opin_mux_size, 
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_x, 
						directionality, L_rr_node_indices, L_rr_edge_done,
						switch_block_conn);
			}
		}
		if (i < nx) {
			t_seg_details * from_seg_details = chan_details_y[i][start];
			t_seg_details * to_seg_details = chan_details_x[i+1][start];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(i, start, track, CHANY, i + 1,
						CHANX, ny, max_chan_width, opin_mux_size, 
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_x, 
						directionality, L_rr_node_indices, L_rr_edge_done,
						switch_block_conn);
			}
		}
		if (start > 1) {
			t_seg_details * from_seg_details = chan_details_y[i][start];
			t_seg_details * to_seg_details = chan_details_y[i][start-1];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(i, start, track, CHANY, start - 1,
						CHANY, ny, max_chan_width, opin_mux_size,
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_y,
						directionality, L_rr_node_indices, L_rr_edge_done,
						switch_block_conn);
			}
		}
		if (end < ny) {
			t_seg_details * from_seg_details = chan_details_y[i][start];
			t_seg_details * to_seg_details = chan_details_y[i][end+1];
			if (to_seg_details->length > 0) {
				num_edges += get_track_to_tracks(i, start, track, CHANY, end + 1,
						CHANY, ny, max_chan_width, opin_mux_size, 
						Fs_per_side, sblock_pattern, &edge_list, 
						from_seg_details, to_seg_details, chan_details_y, 
						directionality, L_rr_node_indices, L_rr_edge_done,
						switch_block_conn);
			}
		}

		int node = get_rr_node_index(i, j, CHANY, track, L_rr_node_indices);
		alloc_and_load_edges_and_switches(L_rr_node, node, num_edges,
				L_rr_edge_done, edge_list);

		while (edge_list != NULL) {
			struct s_linked_edge *next_edge = edge_list->next;
			free(edge_list);
			edge_list = next_edge;
		}

		/* Edge arrays have now been built up.  Do everything else.  */
		L_rr_node[node].cost_index = cost_index_offset + seg_details[track].index;
		L_rr_node[node].occ = ( track < tracks_per_chan ? 0 : 1 );
		L_rr_node[node].capacity = 1; /* GLOBAL routing handled elsewhere */

		L_rr_node[node].xlow = i;
		L_rr_node[node].xhigh = i;
		L_rr_node[node].ylow = start;
		L_rr_node[node].yhigh = end;

		int length = end - start + 1;
		L_rr_node[node].R = length * seg_details[track].Rmetal;
		L_rr_node[node].C = length * seg_details[track].Cmetal;

		L_rr_node[node].ptc_num = track;
		L_rr_node[node].type = CHANY;
		L_rr_node[node].direction = seg_details[track].direction;
		L_rr_node[node].drivers = seg_details[track].drivers;
	}
}

void watch_edges(int inode, t_linked_edge * edge_list_head) {
	t_linked_edge *list_ptr;
	int i, to_node;

	list_ptr = edge_list_head;
	i = 0;

	vpr_printf_trace("!!! Watching Node %d !!!!\n", inode);
	print_rr_node(stdout, rr_node, inode);
	vpr_printf_trace("Currently connects to:\n");
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

static int *****alloc_and_load_pin_to_track_map(INP enum e_pin_type pin_type,
		INP t_chan_width *nodes_per_chan, INP int *Fc, 
		INP t_type_ptr Type, INP boolean perturb_switch_pattern,
		INP enum e_directionality directionality) {

	int ***num_dir; /* [0..width][0..height][0..3] - Number of *physical* pins on each side. */
	int ****dir_list; /* [0..width][0..height][0..3][0..num_pins-1] - List of pins of correct type on each side. Max possible space alloced for simplicity */

	int ***num_done_per_dir; /* [0..width][0..height][0..3] */
	int *****tracks_connected_to_pin; /* [0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */

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
	for (int pin_index = 0; pin_index < Type->num_pins; ++pin_index) {
		int pin_class = Type->pin_class[pin_index];
		if (Fc[pin_index] > max_Fc && Type->class_inf[pin_class].type == pin_type) {
			max_Fc = Fc[pin_index];
		}
	}

	tracks_connected_to_pin = (int *****) alloc_matrix5(0, Type->num_pins - 1,
			0, Type->width - 1, 0, Type->height - 1, 0, 3, 0, max_Fc, sizeof(int));

	for (int pin = 0; pin < Type->num_pins; ++pin) {
		for (int width = 0; width < Type->width; ++width) {
			for (int height = 0; height < Type->height; ++height) {
				for (int side = 0; side < 4; ++side) {
					for (int fc = 0; fc < max_Fc; ++fc) {
						tracks_connected_to_pin[pin][width][height][side][fc] = OPEN; /* Unconnected. */
					}
				}
			}
		}
	}

	num_dir = (int ***) alloc_matrix3(0, Type->width - 1, 0, Type->height - 1, 0, 3, sizeof(int));
	dir_list = (int ****) alloc_matrix4(0, Type->width - 1, 0, Type->height - 1, 0, 3, 0, Type->num_pins - 1, sizeof(int));

	/* Defensive coding.  Try to crash hard if I use an unset entry.  */
	for (int width = 0; width < Type->width; ++width)
		for (int height = 0; height < Type->height; ++height)
			for (int side = 0; side < 4; ++side)
				for (int pin = 0; pin < Type->num_pins; ++pin)
					dir_list[width][height][side][pin] = (-1);

	for (int width = 0; width < Type->width; ++width)
		for (int height = 0; height < Type->height; ++height)
			for (int side = 0; side < 4; ++side)
				num_dir[width][height][side] = 0;

	for (int pin = 0; pin < Type->num_pins; ++pin) {
		int pin_class = Type->pin_class[pin];
		if (Type->class_inf[pin_class].type != pin_type) /* Doing either ipins OR opins */
			continue;

		/* Pins connecting only to global resources get no switches -> keeps area model accurate. */

		if (Type->is_global_pin[pin])
			continue;

		for (int width = 0; width < Type->width; ++width) {
			for (int height = 0; height < Type->height; ++height) {
				for (int side = 0; side < 4; ++side) {
					if (Type->pinloc[width][height][side][pin] == 1) {
						dir_list[width][height][side][num_dir[width][height][side]] = pin;
						num_dir[width][height][side]++;
					}
				}
			}
		}
	}

	int num_phys_pins = 0;
	for (int width = 0; width < Type->width; ++width) {
		for (int height = 0; height < Type->height; ++height) {
			for (int side = 0; side < 4; ++side)
				num_phys_pins += num_dir[width][height][side]; /* Num. physical pins per type */
		}
	}
	num_done_per_dir = (int ***) alloc_matrix3(0, Type->width - 1, 0, Type->height - 1, 0, 3, sizeof(int));
	for (int width = 0; width < Type->width; ++width) {
		for (int height = 0; height < Type->height; ++height) {
			for (int side = 0; side < 4; ++side) {
				num_done_per_dir[width][height][side] = 0;
			}
		}
	}
	int *pin_num_ordering = (int *) my_malloc(num_phys_pins * sizeof(int));
	int *side_ordering = (int *) my_malloc(num_phys_pins * sizeof(int));
	int *width_ordering = (int *) my_malloc(num_phys_pins * sizeof(int));
	int *height_ordering = (int *) my_malloc(num_phys_pins * sizeof(int));

	/* Connection block I use distributes pins evenly across the tracks      *
	 * of ALL sides of the clb at once.  Ensures that each pin connects      *
	 * to spaced out tracks in its connection block, and that the other      *
	 * pins (potentially in other C blocks) connect to the remaining tracks  *
	 * first.  Doesn't matter for large Fc, but should make a fairly         *
	 * good low Fc block that leverages the fact that usually lots of pins   *
	 * are logically equivalent.                                             */

	int side = LEFT;
 	int width = 0;
	int height = Type->height - 1;
	int pin = 0;
	int pin_index = -1;

	while (pin < num_phys_pins) {
		if (side == TOP) {
			if (width >= Type->width - 1) {
				side = RIGHT;
			} else {
				width++;
			}
		} else if (side == RIGHT) {
			if (height <= 0) {
				side = BOTTOM;
			} else {
				height--;
			}
		} else if (side == BOTTOM) {
			if (width <= 0) {
				side = LEFT;
			} else {
				width--;
			}
		} else if (side == LEFT) {
			if (height >= Type->height - 1) {
				pin_index++;
				side = TOP;
			} else {
				height++;
			}
		}

		assert(pin_index < num_phys_pins);
		/* Number of physical pins bounds number of logical pins */

		if (num_done_per_dir[width][height][side] >= num_dir[width][height][side])
			continue;
		pin_num_ordering[pin] = dir_list[width][height][side][pin_index];
		side_ordering[pin] = side;
		width_ordering[pin] = width;
		height_ordering[pin] = height;
		assert(Type->pinloc[width][height][side][dir_list[width][height][side][pin_index]]);
		num_done_per_dir[width][height][side]++;
		pin++;
	}

	if (perturb_switch_pattern) {
		load_perturbed_switch_pattern(Type, tracks_connected_to_pin,
				num_phys_pins, pin_num_ordering, side_ordering, width_ordering, height_ordering,
				nodes_per_chan->x_min, nodes_per_chan->y_min, max_Fc, directionality);
	} else {
		load_uniform_switch_pattern(Type, tracks_connected_to_pin,
				num_phys_pins, pin_num_ordering, side_ordering, width_ordering, height_ordering,
				nodes_per_chan->x_min, nodes_per_chan->y_min, max_Fc, directionality);
	}

#ifdef ENABLE_CHECK_ALL_TRACKS
	check_all_tracks_reach_pins(Type, tracks_connected_to_pin, nodes_per_chan->max,
			max_Fc, pin_type);
#endif

	/* Free all temporary storage. */
	free_matrix3(num_dir, 0, Type->width - 1, 0, Type->height - 1, 0, sizeof(int));
	free_matrix4(dir_list, 0, Type->width - 1, 0, Type->height - 1, 0, 3, 0, sizeof(int));
	free_matrix3(num_done_per_dir, 0, Type->width - 1, 0, Type->height - 1, 0, sizeof(int));
	free(pin_num_ordering);
	free(side_ordering);
	free(width_ordering);
	free(height_ordering);

	return tracks_connected_to_pin;
}

static void load_uniform_switch_pattern(INP t_type_ptr type,
		INOUTP int *****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *width_ordering, INP int *height_ordering, 
 		INP int x_chan_width, INP int y_chan_width, INP int Fc,
		enum e_directionality directionality) {

	/* Loads the tracks_connected_to_pin array with an even distribution of     *
	 * switches across the tracks for each pin.  For example, each pin connects *
	 * to every 4.3rd track in a channel, with exactly which tracks a pin       *
	 * connects to staggered from pin to pin.                                   */

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

	int group_size;
	if (directionality == BI_DIRECTIONAL) {
		group_size = 1;
	} else {
		assert(directionality == UNI_DIRECTIONAL);
		group_size = 2;
	}

	assert((x_chan_width % group_size == 0) && (y_chan_width % group_size == 0) && (Fc % group_size == 0));

	for (int i = 0; i < num_phys_pins; ++i) {

		int pin = pin_num_ordering[i];
		int side = side_ordering[i];
		int width = width_ordering[i];
		int height = height_ordering[i];

		/* Bi-directional treats each track separately, uni-directional works with pairs of tracks */
		for (int j = 0; j < (Fc / group_size); ++j) {

			int max_chan_width = (side == 0 || side == 2 ? x_chan_width : y_chan_width);
			float step_size = (float) max_chan_width / (float) (Fc * num_phys_pins);
			float fc_step = (float) max_chan_width / (float) Fc;

			float ftrack = (i * step_size) + (j * fc_step);
			int itrack = ((int) ftrack) * group_size;

			/* Catch possible floating point round error */
			itrack = min(itrack, max_chan_width - group_size);

			/* Assign the group of tracks for the Fc pattern */
			for (int k = 0; k < group_size; ++k) {
				tracks_connected_to_pin[pin][width][height][side][group_size * j + k] = itrack + k;
			}
		}
	}
}

static void load_perturbed_switch_pattern(INP t_type_ptr type,
		INOUTP int *****tracks_connected_to_pin, INP int num_phys_pins,
		INP int *pin_num_ordering, INP int *side_ordering,
		INP int *width_ordering, INP int *height_ordering, 
 		INP int x_chan_width, INP int y_chan_width, INP int Fc,
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

	assert(directionality == BI_DIRECTIONAL);

	int Fc_dense = (Fc / 2) + 1;
	int Fc_sparse = Fc - Fc_dense; /* Works for even or odd Fc */
	int Fc_half[2];

	for (int i = 0; i < num_phys_pins; i++) {

		int pin = pin_num_ordering[i];
		int side = side_ordering[i];
		int width = width_ordering[i];
		int height = height_ordering[i];

		int max_chan_width = (side == 0 || side == 2 ? x_chan_width : y_chan_width);
		float step_size = (float) max_chan_width / (float) (Fc * num_phys_pins);

		float spacing_dense = (float) max_chan_width / (float) (2 * Fc_dense);
		float spacing_sparse = (float) max_chan_width / (float) (2 * Fc_sparse);
		float spacing[2];

		/* Flip every pin to balance switch density */
		spacing[i % 2] = spacing_dense;
		Fc_half[i % 2] = Fc_dense;
		spacing[(i + 1) % 2] = spacing_sparse;
		Fc_half[(i + 1) % 2] = Fc_sparse;

		float ftrack = i * step_size; /* Start point.  Staggered from pin to pin */
		int iconn = 0;

		for (int ihalf = 0; ihalf < 2; ihalf++) { /* For both dense and sparse halves. */
			for (int j = 0; j < Fc_half[ihalf]; ++j) {

				/* Can occasionally get wraparound due to floating point rounding. 
				 This is okay because the starting position > 0 when this occurs
				 so connection is valid and fine */
				int itrack = (int) ftrack;
				itrack = itrack % max_chan_width;
				tracks_connected_to_pin[pin][width][height][side][iconn] = itrack;

				ftrack += spacing[ihalf];
				iconn++;
			}
		}
	} /* End for all physical pins. */
}

#ifdef ENABLE_CHECK_ALL_TRACKS
static void check_all_tracks_reach_pins(t_type_ptr type,
		int *****tracks_connected_to_pin, int max_chan_width, int Fc,
		enum e_pin_type ipin_or_opin) {

	/* Checks that all tracks can be reached by some pin.   */
	assert(max_chan_width > 0);

	int *num_conns_to_track; /* [0..max_chan_width-1] */
	num_conns_to_track = (int *) my_calloc(max_chan_width, sizeof(int));

	for (int pin = 0; pin < type->num_pins; ++pin) {
		for (int width = 0; width < type->width; ++width) {
			for (int height = 0; height < type->height; ++height) {
				for (int side = 0; side < 4; ++side) {
					if (tracks_connected_to_pin[pin][width][height][side][0] != OPEN) { /* Pin exists */
						for (int conn = 0; conn < Fc; ++conn) {
							int track = tracks_connected_to_pin[pin][width][height][side][conn];
							num_conns_to_track[track]++;
						}
					}
				}
			}
		}
	}

	for (int track = 0; track < max_chan_width; ++track) {
		if (num_conns_to_track[track] <= 0) {
			vpr_printf_error(__FILE__, __LINE__,
				"check_all_tracks_reach_pins: Track %d does not connect to any CLB %ss.\n", 
				track, (ipin_or_opin == DRIVER ? "OPIN" : "IPIN"));
		}
	}
	free(num_conns_to_track);
}
#endif

/* Allocates and loads the track to ipin lookup for each physical grid type. This
 * is the same information as the ipin_to_track map but accessed in a different way. */

static struct s_ivec ****alloc_and_load_track_to_pin_lookup(
		INP int *****pin_to_track_map, INP int *Fc,
		INP int type_width, INP int type_height, 
		INP int num_pins, INP int max_chan_width) {

	struct s_ivec ****track_to_pin_lookup;
	/* [0..max_chan_width-1][0..width][0..height][0..3].  For each track number 
	 * it stores a vector for each of the four sides.  x-directed channels will 
	 * use the TOP and   BOTTOM vectors to figure out what clb input pins they 
	 * connect to above  and below them, respectively, while y-directed channels
	 * use the LEFT and RIGHT vectors.  Each vector contains an nelem field 
	 * saying how many ipins it connects to.  The list[0..nelem-1] array then 
	 * gives the pin numbers.                                                  */

	/* Note that a clb pin that connects to a channel on its RIGHT means that  *
	 * that channel connects to a clb pin on its LEFT.  The convention used    *
	 * here is always in the perspective of the CLB                            */

	if (num_pins < 1) {
		return NULL;
	}

	/* Alloc and zero the the lookup table */
	track_to_pin_lookup = (struct s_ivec ****) alloc_matrix4(0, max_chan_width - 1,
			0, type_width - 1, 0, type_height - 1, 0, 3, sizeof(struct s_ivec));

	for (int track = 0; track < max_chan_width; ++track) {
		for (int width = 0; width < type_width; ++width) {
			for (int height = 0; height < type_height; ++height) {
				for (int side = 0; side < 4; ++side) {
					track_to_pin_lookup[track][width][height][side].nelem = 0;
					track_to_pin_lookup[track][width][height][side].list = NULL;
				}
			}
		}
	}

	/* Counting pass.  */
	for (int pin = 0; pin < num_pins; ++pin) {
		for (int width = 0; width < type_width; ++width) {
			for (int height = 0; height < type_height; ++height) {
				for (int side = 0; side < 4; ++side) {
					if (pin_to_track_map[pin][width][height][side][0] == OPEN)
						continue;

					for (int conn = 0; conn < Fc[pin]; ++conn) {
						int track = pin_to_track_map[pin][width][height][side][conn];
						track_to_pin_lookup[track][width][height][side].nelem++;
					}
				}
			}
		}
	}

	/* Allocate space.  */
	for (int track = 0; track < max_chan_width; ++track) {
		for (int width = 0; width < type_width; ++width) {
			for (int height = 0; height < type_height; ++height) {
				for (int side = 0; side < 4; ++side) {
					track_to_pin_lookup[track][width][height][side].list = NULL; /* Defensive code */
					if (track_to_pin_lookup[track][width][height][side].nelem != 0) {
						track_to_pin_lookup[track][width][height][side].list =
							(int *) my_malloc(track_to_pin_lookup[track][width][height][side].nelem * sizeof(int));
						track_to_pin_lookup[track][width][height][side].nelem = 0;
					}
				}
			}
		}
	}

	/* Loading pass. */
	for (int pin = 0; pin < num_pins; ++pin) {
		for (int width = 0; width < type_width; ++width) {
			for (int height = 0; height < type_height; ++height) {
				for (int side = 0; side < 4; ++side) {
					if (pin_to_track_map[pin][width][height][side][0] == OPEN)
						continue;

					for (int conn = 0; conn < Fc[pin]; ++conn) {
						int track = pin_to_track_map[pin][width][height][side][conn];
						int pin_counter = track_to_pin_lookup[track][width][height][side].nelem;
						track_to_pin_lookup[track][width][height][side].list[pin_counter] = pin;
						track_to_pin_lookup[track][width][height][side].nelem++;
					}
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

	FILE *fp = my_fopen(file_name, "w", 0);

	for (int inode = 0; inode < num_rr_nodes; ++inode) {
		print_rr_node(fp, rr_node, inode);
		fprintf(fp, "\n");
	}

#if 0
	fprintf(fp, "\n\n%d rr_indexed_data entries.\n\n", num_rr_indexed_data);

	for (int index = 0; index < num_rr_indexed_data; ++index)
	{
		print_rr_indexed_data(fp, index);
		fprintf(fp, "\n");
	}
#endif

	fclose(fp);
}

/* Prints all the data about node inode to file fp.                    */
void print_rr_node(FILE * fp, t_rr_node * L_rr_node, int inode) {

	static const char *direction_name[] = { "OPEN", "INC_DIRECTION", "DEC_DIRECTION", "BI_DIRECTION" };
	static const char *drivers_name[] = { "OPEN", "MULTI_BUFFER", "SINGLE" };

	t_rr_type rr_type = L_rr_node[inode].type;

	/* Make sure we don't overrun const arrays */
	assert((L_rr_node[inode].direction + 1) < (int)(sizeof(direction_name) / sizeof(char *)));
	assert((L_rr_node[inode].drivers + 1) < (int)(sizeof(drivers_name) / sizeof(char *)));

	fprintf(fp, "Node: %d %s ", inode, L_rr_node[inode].rr_get_type_string());
	if ((L_rr_node[inode].xlow == L_rr_node[inode].xhigh)
			&& (L_rr_node[inode].ylow == L_rr_node[inode].yhigh)) {
		fprintf(fp, "(%d, %d) ", 
				L_rr_node[inode].xlow, L_rr_node[inode].ylow);
	} else {
		fprintf(fp, "(%d, %d) to (%d, %d) ", 
				L_rr_node[inode].xlow, L_rr_node[inode].ylow, 
				L_rr_node[inode].xhigh, L_rr_node[inode].yhigh);
	}
	fprintf(fp, "Ptc_num: %d ", L_rr_node[inode].ptc_num);
	fprintf(fp, "Direction: %s ", direction_name[L_rr_node[inode].direction + 1]);
	fprintf(fp, "Drivers: %s ", drivers_name[L_rr_node[inode].drivers + 1]);
	fprintf(fp, "\n");

	if( rr_type == IPIN || rr_type == OPIN)
	{
		fprintf(fp, "name %s\n", L_rr_node[inode].pb_graph_pin->port->name);
	}
	fprintf(fp, "%d edge(s):", L_rr_node[inode].num_edges);
	for (int iconn = 0; iconn < L_rr_node[inode].num_edges; ++iconn)
		fprintf(fp, " %d", L_rr_node[inode].edges[iconn]);
	fprintf(fp, "\n");

	fprintf(fp, "Switch types:");
	for (int iconn = 0; iconn < L_rr_node[inode].num_edges; ++iconn)
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

	fprintf(fp, "ortho_cost_index: %d  ", rr_indexed_data[index].ortho_cost_index);
	fprintf(fp, "base_cost: %g  ", rr_indexed_data[index].saved_base_cost);
	fprintf(fp, "saved_base_cost: %g\n", rr_indexed_data[index].saved_base_cost);

	fprintf(fp, "Seg_index: %d  ", rr_indexed_data[index].seg_index);
	fprintf(fp, "inv_length: %g\n", rr_indexed_data[index].inv_length);

	fprintf(fp, "T_linear: %g  ", rr_indexed_data[index].T_linear);
	fprintf(fp, "T_quadratic: %g  ", rr_indexed_data[index].T_quadratic);
	fprintf(fp, "C_load: %g\n", rr_indexed_data[index].C_load);
}

static void build_unidir_rr_opins(INP int i, INP int j,
		INP struct s_grid_tile **L_grid, INP int **Fc_out, INP int max_chan_width, 
		INP t_chan_details * chan_details_x, INP t_chan_details * chan_details_y,
		INOUTP int **Fc_xofs, INOUTP int **Fc_yofs,
		INOUTP t_rr_node * L_rr_node, INOUTP boolean * L_rr_edge_done,
		OUTP boolean * Fc_clipped, INP t_ivec *** L_rr_node_indices, INP int delayless_switch,
		INP t_direct_inf *directs, INP int num_directs, INP t_clb_to_clb_directs *clb_to_clb_directs) {

	/* This routine returns a list of the opins rr_nodes on each
	 * side/width/height of the block. You must free the result with
	 * free_matrix. */

	int max_Fc = -1;

	*Fc_clipped = FALSE;

	/* Only the base block of a set should use this function */
	if (L_grid[i][j].width_offset > 0 || L_grid[i][j].height_offset > 0) {
		return;
	}

	t_type_ptr type = L_grid[i][j].type;

	/* Currently, only two possible Fc values exist: 0 or default. 
	 * Finding the max. value of Fc in block will result in the 
	 * default value, which works for now. In the future, when 
	 * the Fc values of all pins can vary, the max value will continue
	 * to work for matrix allocation purposes. However, all looping 
	 * will have to be modified to account for pin-based Fc values. */
	if (type->index > 0) {
		max_Fc = 0;
		for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
			int class_index = type->pin_class[pin_index];
			if (Fc_out[type->index][pin_index] > max_Fc && type->class_inf[class_index].type == DRIVER) {
				max_Fc = Fc_out[type->index][pin_index];
			}
		}
	}

	/* Go through each pin and find its fanout. */
	for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
		/* Skip global pins and pins */
		int class_index = type->pin_class[pin_index];
		if (type->class_inf[class_index].type != DRIVER) {
			continue;
		}
		if (type->is_global_pin[pin_index]) {
			continue;
		}

		int num_edges = 0;
		t_linked_edge *edge_list = NULL;

		for (int width = 0; width < type->width; ++width) {
			for (int height = 0; height < type->height; ++height) {
				for (enum e_side side = (enum e_side)0; side < 4; side = (enum e_side)(side + 1)) {
 					/* Can't do anything if pin isn't at this location */
					if (0 == type->pinloc[width][height][side][pin_index]) {
						continue;
					}

					/* Figure out the chan seg at that side. 
					 * side is the side of the logic or io block. */
					boolean vert = (boolean) ((side == TOP) || (side == BOTTOM));
					boolean pos_dir = (boolean) ((side == TOP) || (side == RIGHT));
					t_rr_type chan_type = (vert ? CHANX : CHANY);
					int chan = (vert ? (j + height) : (i + width));
					int seg = (vert ? (i + width) : (j + height));
					int max_len = (vert ? nx : ny);
					int **Fc_ofs = (vert ? Fc_xofs : Fc_yofs);
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

					t_seg_details * seg_details = (chan_type == CHANX ? 
							chan_details_x[seg][chan] : chan_details_y[chan][seg]);
					if (seg_details[0].length == 0)
						continue;

					/* Get the list of opin to mux connections for that chan seg. */
					boolean clipped;
					num_edges += get_unidir_opin_connections(chan, seg,
							max_Fc, chan_type, seg_details, &edge_list,
							Fc_ofs, L_rr_edge_done, max_len, max_chan_width,
							L_rr_node_indices, &clipped);
					if (clipped) {
						*Fc_clipped = TRUE;
					}
				}
			}
		}

		/* Add in direct connections */
		num_edges += get_opin_direct_connecions(i, j, pin_index, &edge_list, L_rr_node_indices, 
				directs, num_directs, clb_to_clb_directs);

		/* Add the edges */
		int node = get_rr_node_index(i, j, OPIN, pin_index, L_rr_node_indices);
		alloc_and_load_edges_and_switches(rr_node, node, num_edges,
				L_rr_edge_done, edge_list);
		while (edge_list != NULL) {
			t_linked_edge *next_edge = edge_list->next;
			free(edge_list);
			edge_list = next_edge;
		}
	}
}

#if 0
static void load_uniform_opin_switch_pattern_paired(INP int *Fc_out,
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
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
				"opin %d at (%d,%d) does not connect to any tracks.\n", 
				L_rr_node[from_node].ptc_num, L_rr_node[from_node].xlow, L_rr_node[from_node].ylow);
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
static void view_mux_size_distribution(t_ivec *** L_rr_node_indices,
		int max_chan_width,
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

				assert(max_chan_width > 0);
				for (itrack = 0; itrack < max_chan_width; itrack++)
				{
					start = get_seg_start(seg_details, itrack, seg_num, chan_num);
					end = get_seg_end(seg_details, itrack, start, chan_num, max_len);

					if ((seg_details[itrack].direction == direction) 
							&& (((start == seg_num) && (direction == INC_DIRECTION))
								|| ((end == seg_num) && (direction == DEC_DIRECTION))))
					{ /* mux found */
						num_muxes++;
						if (side == LEFT || side == RIGHT)
						{ /* CHANX */
							inode =	get_rr_node_index(seg_num, chan_num,
									CHANX, itrack, L_rr_node_indices);
						}
						else
						{
							assert((side == TOP) || (side == BOTTOM)); /* CHANY */
							inode =	get_rr_node_index(chan_num, seg_num,
									CHANY, itrack, L_rr_node_indices);
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
							sblock_mux_list_head = new_mux;
						}
						else if (sblock_mux_list_head->
								size < new_mux->size)
						{
							/* insert before head */
							new_mux->next = sblock_mux_list_head;
							sblock_mux_list_head = new_mux;
						}
						else
						{
							/* insert after head */
							current = sblock_mux_list_head;
							next = current->next;

							while ((next != NULL)
									&& (next->size > new_mux->size))
							{
								current = next;
								next = current->next;
							}

							if (next == NULL)
							{
								current->next = new_mux;
							}
							else
							{
								new_mux->next = current->next;
								current->next = new_mux;
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
				fprintf(dump_file_per_sblock, "%d ", current->size);
				fprintf(dump_file, "%d\n", current->size);
#endif /* ENABLE_DUMP */

				current = current->next;
			}

#ifdef ENABLE_DUMP
			fprintf(dump_file_per_sblock, "}\n\tmax: %d\tmin:%d", max_value, min_value);
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
					assert(current->size <= new_distribution->max_index);
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
					if (current->size > distr_current->max_index)
					{
						/* needs to realloc to expand the distribution array to hold the new large-valued data */
						distr_current->distr = my_realloc(distr_current->distr,
								(current->size + 1) * sizeof(int));

						/* initializing the newly allocated elements */
						for (k = (distr_current->max_index + 1); k <= current->size; k++)
							distr_current->distr[k] = 0;

						distr_current->max_index = current->size;
						distr_current->distr[current->size]++;
					}
					else
					{
						distr_current->distr[current->size]++;
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
	vpr_printf_info("==== MUX size statistics ====\n");
	vpr_printf_info("Max range of mux size within a sblock: %d\n", global_max_range);
	vpr_printf_info("Average range of mux size within a sblock: %.2f\n", avg_range);
	vpr_printf_info("Std dev of range of mux size within a sblock: %.2f\n", std_dev_range);
	vpr_printf_info("Average percent range of mux size within a sblock: %.2f%%\n", avg_percent_range * 100.0);
	vpr_printf_info("Std dev of percent range of mux size within a sblock: %.2f%%\n", std_dev_percent_range * 100.0);

	vpr_printf_info(" -- Detailed MUX size distribution by sblock type -- \n");
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

static void print_distribution(FILE * fptr,
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
static t_clb_to_clb_directs * alloc_and_load_clb_to_clb_directs(INP t_direct_inf *directs, INP int num_directs, int delayless_switch) {
	int i, j;
	t_clb_to_clb_directs *clb_to_clb_directs;
	char *pb_type_name, *port_name;
	int start_pin_index, end_pin_index;
	t_pb_type *pb_type;

	clb_to_clb_directs = (t_clb_to_clb_directs*)my_calloc(num_directs, sizeof(t_clb_to_clb_directs));

	pb_type_name = NULL;
	port_name = NULL;

	for (i = 0; i < num_directs; i++) {
		pb_type_name = (char*)my_malloc((strlen(directs[i].from_pin) + strlen(directs[i].to_pin)) * sizeof(char));
		port_name = (char*)my_malloc((strlen(directs[i].from_pin) + strlen(directs[i].to_pin)) * sizeof(char));

		// Load from pins
		// Parse out the pb_type name, port name, and pin range
		parse_direct_pin_name(directs[i].from_pin, directs[i].line, &start_pin_index, &end_pin_index, pb_type_name, port_name);

		// Figure out which type, port, and pin is used
		for (j = 0; j < num_types; j++) {
			if(strcmp(type_descriptors[j].name, pb_type_name) == 0) {
				break;
			}
		}
		assert(j < num_types);
		clb_to_clb_directs[i].from_clb_type = &type_descriptors[j];
		pb_type = clb_to_clb_directs[i].from_clb_type->pb_type;

		for (j = 0; j < pb_type->num_ports; j++) {
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
		for (j = 0; j < num_types; j++) {
			if(strcmp(type_descriptors[j].name, pb_type_name) == 0) {
				break;
			}
		}
		assert(j < num_types);
		clb_to_clb_directs[i].to_clb_type = &type_descriptors[j];
		pb_type = clb_to_clb_directs[i].to_clb_type->pb_type;

		for (j = 0; j < pb_type->num_ports; j++) {
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
			vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), directs[i].line, 
				"Range mismatch from %s to %s.\n", directs[i].from_pin, directs[i].to_pin);
		}

        //Set the switch index
        if(directs[i].switch_type > 0) {
            //Use the specified switch
            clb_to_clb_directs[i].switch_index = directs[i].switch_type;
        } else {
            //Use the delayless switch by default
            clb_to_clb_directs[i].switch_index = delayless_switch;
            
        }
		free(pb_type_name);
		free(port_name);

        //We must be careful to clean-up anything that we may have incidentally allocated.
        //Specifically, we can be called while generating the dummy architecture
        //for placer delay estimation.  Since the delay estimation occurs on a 
        //'different' architecture it is almost certain that the f_blk_pin_from_port_pin allocated 
        //by calling get_blk_pin_from_port_pin() will later be invalid.
        //We therefore must free it now.
        free_blk_pin_from_port_pin();

	}
	return clb_to_clb_directs;
}

/* Add all direct clb-pin-to-clb-pin edges to given opin */ 
static int get_opin_direct_connecions(int x, int y, int opin, 
		INOUTP t_linked_edge ** edge_list_ptr, INP t_ivec *** L_rr_node_indices, 
		INP t_direct_inf *directs, INP int num_directs, 
		INP t_clb_to_clb_directs *clb_to_clb_directs) {

	t_type_ptr curr_type, target_type;
	int width_offset, height_offset;
	int i, ipin, inode;
	t_linked_edge *edge_list_head;
	int max_index, min_index, offset, swap;
	int new_edges;

	curr_type = grid[x][y].type;
	edge_list_head = *edge_list_ptr;
	new_edges = 0;

	/* Iterate through all direct connections */
	for (i = 0; i < num_directs; i++) {
		/* Find matching direct clb-to-clb connections with the same type as current grid location */
		if(clb_to_clb_directs[i].from_clb_type == curr_type) { //We are at a valid starting point

            //Offset must be in range
            if(x + directs[i].x_offset < nx + 1 &&
               x + directs[i].x_offset > 0 &&
               y + directs[i].y_offset < ny + 1 &&
               y + directs[i].y_offset > 0) {
            
                //Only add connections if the target clb type matches the type in the direct specification
                target_type = grid[x + directs[i].x_offset][y + directs[i].y_offset].type;
                if(clb_to_clb_directs[i].to_clb_type == target_type) {

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
                        width_offset = grid[x + directs[i].x_offset][y + directs[i].y_offset].width_offset;
                        height_offset = grid[x + directs[i].x_offset][y + directs[i].y_offset].height_offset;
                        inode = get_rr_node_index(x + directs[i].x_offset - width_offset, y + directs[i].y_offset - height_offset, 
                                IPIN, ipin, L_rr_node_indices);
                        edge_list_head = insert_in_edge_list(edge_list_head, inode, clb_to_clb_directs[i].switch_index);
                        new_edges++;
                    }
                }
            }
		}
	}
	*edge_list_ptr = edge_list_head;
	return new_edges;
}


/* Determines whether the output pins of the specified block type should be perturbed.	*
*  This is to prevent pathological cases where the output pin connections are		*
*  spaced such that the connection pattern always skips some types of wire (w.r.t.	*
*  starting points)									*/
static boolean get_perturb_opins(INP t_type_ptr type, INP int *Fc_out, 
		INP int max_chan_width, INP int num_segments, INP t_segment_inf *segment_inf){
	
	int i, Fc_max, iclass, num_wire_types;
	int num, max_primes, factor, num_factors;
	int *prime_factors;
	float step_size = 0;
	float n = 0;
	float threshold = 0.07;
	boolean perturb_opins = FALSE;
	
	i = Fc_max = iclass = 0;
	if (num_segments > 1){
		/* Segments of one length are grouped together in the channel.	*
		*  In the future we can determine if any of these segments will	*
		*  encounter the pathological step size case, and determine if	*
		*  we need to perturb based on the segment's frequency (if 	*
		*  frequency is small we should not perturb - testing has found	*
		*  that perturbing a channel when unnecessary increases needed	*
		*  W to achieve the same delay); but for now we just return.	*/
		return perturb_opins;
	} else {
		/* There are as many wire start points as the value of L */
		num_wire_types = segment_inf[0].length;
	}

	/* get Fc_max */
	for (i = 0; i < type->num_pins; ++i) {
		iclass = type->pin_class[i];
		if (Fc_out[i] > Fc_max && type->class_inf[iclass].type == DRIVER) {
			Fc_max = Fc_out[i];
		}
	}
	/* Nothing to perturb if Fc=0; no need to perturb if Fc = 1 */
	if (Fc_max == 0 || Fc_max == max_chan_width){
		return perturb_opins;
	}

	/* Pathological cases occur when the step size, W/Fc, is a multiple of	*
	*  the number of wire starting points, L. Specifically, when the step 	*
	*  size is a multiple of a prime factor of L, the connection pattern	*
	*  will always skip some wires. Thus, we perturb pins if we detect this	*
	*  case.								*/
	
	/* get an upper bound on the number of prime factors of num_wire_types	*/
	max_primes = (int)floor(log((float)num_wire_types)/log(2.0));
	prime_factors = (int *) my_malloc(max_primes * sizeof(int));
	for (i = 0; i < max_primes; i++){
		prime_factors[i] = 0;
	}
	
	/* Find the prime factors of num_wire_types */
	num = num_wire_types; 
	factor = 2;
	num_factors = 0;
	while ( pow((float)factor, 2) <= num ){
		if ( num % factor == 0 ){
			num /= factor;
			if ( factor != prime_factors[num_factors] ){
				prime_factors[num_factors] = factor;
				num_factors++;
			}
		} else {
			factor++;
		}
	}
	if (num_factors == 0){
		prime_factors[num_factors++] = num_wire_types;	/* covers cases when num_wire_types is prime */
	}

	/* Now see if step size is an approximate multiple of one of the factors. A 	*
	*  threshold is used because step size may not be an integer.			*/
	step_size = (float)max_chan_width / Fc_max;
	for (i = 0; i < num_factors; i++){
		if ( nint(step_size) < prime_factors[i] ){
			perturb_opins = FALSE;
			break;
		}
	
		n = step_size / prime_factors[i]; 
		n = n - (float)nint(n);			/* fractinal part */	
		if ( fabs(n) < threshold ){
			perturb_opins = TRUE;
			break;
		} else {
			perturb_opins = FALSE;
		}
	}
	free(prime_factors);

	return perturb_opins;
}
