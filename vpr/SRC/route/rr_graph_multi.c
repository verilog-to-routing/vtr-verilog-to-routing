#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <set>
#include <vector>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_multi.h"
#include "rr_graph_sbox.h"
#include "check_rr_graph.h"
#include "rr_graph_timing_params.h"
#include "rr_graph_indexed_data.h"
#include "vpr_utils.h"

#ifdef INTERPOSER_BASED_ARCHITECTURE


/* ---------------------------------------------------------------------------
 * Control Behavior of of the code by using the following variables/macros
 * ---------------------------------------------------------------------------*/
//#define DUMP_CUTTING_PATTERN
//#define DUMP_DEBUG_FILES
#define USE_INTERPOSER_NODE_ADDITION_METHODOLOGY
//#define USE_SIMPLER_SWITCH_MODIFICATION_METHODOLOGY

static CutPatternType s_pattern_type = CHUNK_CUT_WITHOUT_ROTATION;

// if you want the rr_node ID to appear in the dump file, change this variable to true
// the rr_node ID is not guaranteed to be the same between different runs so,
// not including the rr_node ID will allow you to compare sorted versions of the rr_graph between different runs
bool include_rr_node_IDs_in_rr_graph_dump = false;


/* ---------------------------------------------------------------------------
 * 				Helper data structures
 * ---------------------------------------------------------------------------*/

static std::set<int> cut_node_set; /* This set will contain IDs of the routing wires we will cut */
static int*** interposer_node_loc; 
static int* interposer_nodes;
static int s_num_interposer_nodes = 0;

/*
 * reverse_map is a data structure that keeps tracks of fanins of all rr_nodes.
 * if rr_node with index 'i' has 'k' fanins (i.e. rr_node[i].fan_in is equal to 'k'),
 * then, reverse_map[i][0..k-1] is the rr_node indeces of its fanins
 */
int **reverse_map = 0;


/* ---------------------------------------------------------------------------
 * 				Functions begin 
 * ---------------------------------------------------------------------------*/

/*
 * Description: 
 * Allocates and populates the "reverse_map"
 * reverse_map is a data structure that keeps tracks of fanins of all rr_nodes.
 * if rr_node with index 'i' has 'k' fanins (i.e. rr_node[i].fan_in is equal to 'k'),
 * then, reverse_map[i][0..k-1] is the rr_node indeces of its fanins
 *
 * Returns: none.
 */
void alloc_and_build_reverse_map(int num_all_rr_nodes)
{
	reverse_map = (int **) my_malloc(num_all_rr_nodes * sizeof(int*));
	int i,j, inode, iedge, dst_node;
	for(i=0; i<num_all_rr_nodes; i++)
	{
		reverse_map[i] = (int*) my_malloc(rr_node[i].fan_in * sizeof(int));
	}
	
	for(i=0;i<num_all_rr_nodes;i++)
	{
		for(j=0;j<rr_node[i].fan_in;j++)
		{
			reverse_map[i][j]=-1;
		}
	}

	for(inode=0; inode<num_all_rr_nodes; inode++)
	{	// for each routing node
		
		for(iedge = 0; iedge < rr_node[inode].num_edges; iedge++)
		{	// for each of its outgoing edges

			dst_node = rr_node[inode].edges[iedge];
			
			if(dst_node == -1)
			{
				continue;
			}
			else
			{
				// find the first available slot
				for(i=0; i<rr_node[dst_node].fan_in; i++)
				{
					if(reverse_map[dst_node][i]==-1)
						break;
				}

				// if i==rr_node[dst_node].fan_in, that means, we are trying to insert something 
				// into the reverse_map of a node, but there's no more space to do that
				// that means that number of fanins ( ".fan_in" ) was not correct in the first place.
				assert(i < rr_node[dst_node].fan_in);

				// add to the reverse map
				reverse_map[dst_node][i] = inode;
			}
		}
	}
}

/*
 * Description: Frees reverse_map data structure
 * 
 * Returns: none.
 */
void free_reverse_map(int num_all_rr_nodes)
{
	int i;
	for(i=0; i<num_all_rr_nodes; i++)
	{
		free(reverse_map[i]);
	}
	free(reverse_map);
}

void print_stats_signal_direction_at_cuts(int nodes_per_chan)
{
	int i, inet, icut;
	int *num_signals_going_up_at_cut = (int*) malloc(num_cuts * sizeof(int));
	int *num_signals_going_dn_at_cut = (int*) malloc(num_cuts * sizeof(int));

	bool *net_crosses_cut_up = (bool*) malloc(num_cuts * sizeof(bool));
	bool *net_crosses_cut_dn = (bool*) malloc(num_cuts * sizeof(bool));

	for(i=0; i<num_cuts; ++i)
	{
		num_signals_going_up_at_cut[i]=0;
		num_signals_going_dn_at_cut[i]=0;
	}
	
	for (inet = 0; inet < num_nets; inet++) 
	{
		for(icut = 0; icut < num_cuts; ++icut)
		{
			net_crosses_cut_up[icut] = false;
			net_crosses_cut_dn[icut] = false;
		}

		int source_block = clb_net[inet].node_block[0];		
		int ipin = clb_net[inet].node_block_pin[0];
		int source_y = block[source_block].y + block[source_block].type->pin_height[ipin];
		
		for (i = 1; i < (clb_net[inet].num_sinks + 1); i++) 
		{
			int sink_block = clb_net[inet].node_block[i];
			ipin = clb_net[inet].node_block_pin[i];
			int sink_y = block[sink_block].y + block[sink_block].type->pin_height[ipin];
			
			for(icut = 0; icut < num_cuts; ++icut)
			{
				int cut_y = arch_cut_locations[icut];
				if( source_y <= cut_y && sink_y > cut_y )
				{
					net_crosses_cut_up[icut] = true;

				}
				if ( source_y > cut_y && sink_y <= cut_y )
				{
					net_crosses_cut_dn[icut] = true;
				}
			}
		}

		for(icut = 0; icut < num_cuts; ++icut)
		{
			if(net_crosses_cut_up[icut]) {
				num_signals_going_up_at_cut[icut]++;
			}
			if(net_crosses_cut_dn[icut]) {
				num_signals_going_dn_at_cut[icut]++;
			}
		}
	}
	
	int num_wires_cut = (nodes_per_chan * percent_wires_cut) / 100;
	if(num_wires_cut % 2) 
	{
		num_wires_cut++;
	}
	assert(num_wires_cut%2==0);
	int num_wires_going_through = nodes_per_chan - num_wires_cut;
	assert(num_wires_going_through%2==0);
	int num_wires_going_through_in_each_direction =  num_wires_going_through / 2;
	int interopser_capacity_in_each_direction_at_each_cut = (nx+1) * (num_wires_going_through_in_each_direction);
	vpr_printf_info("Info: Interposer capacity in each direction at each cut:%d\n", interopser_capacity_in_each_direction_at_each_cut);

	for(i=0; i<num_cuts; ++i)
	{
		vpr_printf_info("Info: Cut#%d: Number of signals crossing up:%d \t Number of signals crossing down:%d\n", 
						i, 
						num_signals_going_up_at_cut[i], 
						num_signals_going_dn_at_cut[i]
						);
	}
}

/*
 * Description: Helper function for debugging. Prints out fanins and fanouts of a specific rr_node
 * 
 * Returns: none.
 */
void print_fanins_and_fanouts_of_rr_node(int inode)
{
	printf("Fanouts of Node %d: ", inode);
	int i;
	for(i=0; i<rr_node[inode].num_edges;++i)
	{
		printf("%d,",rr_node[inode].edges[i]);
	}
	printf("\n");

	printf("Fanins of Node %d: ", inode);
	for(i=0; i<rr_node[inode].fan_in; ++i)
	{
		printf("%d,", reverse_map[inode][i]);
	}
	printf("\n");
}

/*
 * Description: Helper function that dumps rr_graph connections
 * 
 * Returns: none.
 */
void dump_rr_graph_connections(FILE* fp)
{
	// dump the whole rr_graph
	int iedge, inode, dst_node;
	char* type[7] = {"SOURCE", "SINK", "IPIN", "OPIN", "CHANX", "CHANY", "INTRA_CLUSTER_EDGE"};

	for(inode=0; inode<num_rr_nodes;++inode)
	{
		for(iedge = 0; iedge < rr_node[inode].num_edges; iedge++)
		{
			dst_node = rr_node[inode].edges[iedge];
			const char* inode_dir = (rr_node[inode].direction == INC_DIRECTION) ? "INC" : (rr_node[inode].direction == DEC_DIRECTION) ? "DEC" : "BIDIR";
			const char* dst_node_dir = (rr_node[dst_node].direction == INC_DIRECTION) ? "INC" : (rr_node[dst_node].direction == DEC_DIRECTION) ? "DEC" : "BIDIR";

			if(include_rr_node_IDs_in_rr_graph_dump)
			{
				fprintf(fp, "(%s,%d,%d,%d,%d,%d,%s) \t ", type[rr_node[inode].type], inode, rr_node[inode].xlow, rr_node[inode].xhigh, rr_node[inode].ylow, rr_node[inode].yhigh, inode_dir );
				fprintf(fp, "(%s,%d,%d,%d,%d,%d,%s) \t ", type[rr_node[dst_node].type], dst_node, rr_node[dst_node].xlow, rr_node[dst_node].xhigh, rr_node[dst_node].ylow, rr_node[dst_node].yhigh, dst_node_dir );
			}
			else
			{
				fprintf(fp, "(%s,%d,%d,%d,%d,%s) \t ", type[rr_node[inode].type], rr_node[inode].xlow, rr_node[inode].xhigh, rr_node[inode].ylow, rr_node[inode].yhigh, inode_dir );
				fprintf(fp, "(%s,%d,%d,%d,%d,%s) \t ", type[rr_node[dst_node].type], rr_node[dst_node].xlow, rr_node[dst_node].xhigh, rr_node[dst_node].ylow, rr_node[dst_node].yhigh, dst_node_dir );
			}
			int switch_index = rr_node[inode].switches[iedge];
			fprintf(fp, "switch_delay[%d]=%g\n",switch_index, switch_inf[switch_index].Tdel);
		}
	}
}

/*
 * Description: This is where cutting happens!
 * Some vertical (CHANY to CHANY) connections are cut based on cutting pattern
 *
 * One of the cutting patterns implemented here is called "uniform cut with rotation"
 *
 * Uniform means that we spread the track# that we want to cut over the width of the channel
 * For instance: if channel width is 10, and %wires_cut=60% ( | = 1 track wire, x = cut )
 * 
 *  x x     x x     x x
 *  | | | | | | | | | |
 * 
 * Also note that we cut wires in pairs because of the alternating INC DEC pattern of the wires
 * For instance: if you cut wires in singles (instead of pairs), and %wires_cut=50%, 
 * you could end up cutting all INC vertical wires in a channel and that causes a big Fmax hit
 *
 * Rotation means that the offset at which we start cutting will change based on X-coordinate of the channel
 * For instance, at X=0, we may start cutting at track 0, and at x=1, we may start cutting at track 4
 *
 *
 * The other cutting pattern implemented is called "chunk cut"
 * As the name suggests, this method cuts a chunk of the wires until %wires_cut is reached.
 * For instance: if channel width is 10, and %wires_cut=60% ( | = 1 track wire, x = cut )
 * 
 *  x x x x x x
 *  | | | | | | | | | |
 * 
 * Returns: None.
 */


/*
 * Description: implements uniform cut without rotation (see cut pattern descriptions above)
 * 
 * Returns: none.
 */
void cut_rr_graph_edges_uniform_cut_with_rotation(int nodes_per_chan)
{

#ifdef DUMP_CUTTING_PATTERN
	FILE* fp = my_fopen("cutting_pattern.echo", "w", 0);
#endif

	int i, itrack, ifanout, step, offset, num_chunks, cut_counter, ifanin;
	int num_wires_cut_so_far = 0;
	int interposer_node_index;
	t_rr_node* interposer_node;

	// Find the number of wires that should be cut at each horizontal cut
	int num_wires_cut = (nodes_per_chan * percent_wires_cut) / 100;
	assert(percent_wires_cut==0 || num_wires_cut <= nodes_per_chan);

	// num_wires_cut should be an even number
	if(num_wires_cut % 2)
	{
			num_wires_cut++;
	}

	if(num_wires_cut == 0)
	{	// nothing to do :)
		return;
	}

	assert(num_wires_cut > 0);

	for(i=0; i<=nx; ++i)
	{
		offset = (i * nodes_per_chan) / nx;
		if(offset % 2) 
		{
			offset++;
		}
		offset = offset%nodes_per_chan; // to keep offset between 0 and nodes_per_chan-1

		// Example: if the step is 1.66, make the step 2.
		step = ceil (float(nodes_per_chan) / float(num_wires_cut));


		// cutting chunks of wires. each chunk has 2 wires (a pair)
		num_chunks = num_wires_cut/2;
		step = nodes_per_chan/num_chunks;

		if(step<=2)
		{
			// it can be proven that if %wires_cut>66%, then step=2.
			// step=2 means that there will be no gap between pairs of wires that will be cut
			// therefore, the cut pattern becomes a 'chunk cut'.
			// to avoid that, we will cap the number of chunks.
			// we require step to be greater than or equal to 3.
			step = 3;
			num_chunks = nodes_per_chan / 3;
		}

		for(cut_counter=0; cut_counter < num_cuts; ++cut_counter)
		{
			int ichunk = 0;
			for(itrack=offset, num_wires_cut_so_far=0 ; num_wires_cut_so_far<num_wires_cut; itrack=(itrack+1)%nodes_per_chan)
			{
				for(ichunk=0; ichunk<num_chunks && num_wires_cut_so_far<num_wires_cut; ichunk++)
				{
					int track_to_cut = (itrack+(step*ichunk))%nodes_per_chan;

					#ifdef DUMP_CUTTING_PATTERN
					fprintf(fp, "Cutting interposer node at i=%d, j=%d, track=%d \n", i, arch_cut_locations[cut_counter], track_to_cut);
					#endif

					interposer_node_index = interposer_node_loc[i][cut_counter][track_to_cut];
					interposer_node = &rr_node[interposer_node_index];

					// cut all fanout connections of the interposer node
					for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
					{
						delete_rr_connection(interposer_node_index, interposer_node->edges[ifanout]);
						--ifanout;
					}
					assert(rr_node[interposer_node_index].num_edges==0);

					// cut all fanin connections of the interposer node
					for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
					{
						int fanin_node_index = reverse_map[interposer_node_index][ifanin];
						delete_rr_connection(fanin_node_index, interposer_node_index);
						--ifanin;
					}
					assert(rr_node[interposer_node_index].fan_in==0);

					num_wires_cut_so_far++;
				}
			}
		}
	}

	assert(num_wires_cut_so_far==num_wires_cut);

#ifdef DUMP_CUTTING_PATTERN
	fclose(fp);
#endif

}

/*
 * Description: implements uniform cut without rotation (see cut pattern descriptions above)
 * 
 * Returns: none.
 */
void cut_rr_graph_edges_uniform_cut_without_rotation(int nodes_per_chan)
{

#ifdef DUMP_CUTTING_PATTERN
	FILE* fp = my_fopen("cutting_pattern.echo", "w", 0);
#endif

	int i, itrack, ifanout, step, num_chunks, cut_counter, ifanin;
	int num_wires_cut_so_far = 0;
	int interposer_node_index;
	t_rr_node* interposer_node;

	// Find the number of wires that should be cut at each horizontal cut
	int num_wires_cut = (nodes_per_chan * percent_wires_cut) / 100;
	assert(percent_wires_cut==0 || num_wires_cut <= nodes_per_chan);

	// num_wires_cut should be an even number
	if(num_wires_cut % 2)
	{
			num_wires_cut++;
	}

	if(num_wires_cut == 0)
	{	// nothing to do :)
		return;
	}

	assert(num_wires_cut > 0);

	for(i=0; i<=nx; ++i)
	{
		// Example: if the step is 1.66, make the step 2.
		step = ceil (float(nodes_per_chan) / float(num_wires_cut));

		// cutting chunks of wires. each chunk has 2 wires (a pair)
		num_chunks = num_wires_cut/2;
		step = nodes_per_chan/num_chunks;

		if(step<=2)
		{
			// it can be proven that if %wires_cut>66%, then step=2.
			// step=2 means that there will be no gap between pairs of wires that will be cut
			// therefore, the cut pattern becomes a 'chunk cut'.
			// to avoid that, we will cap the number of chunks.
			// we require step to be greater than or equal to 3.
			step = 3;
			num_chunks = nodes_per_chan / 3;
		}

		for(cut_counter=0; cut_counter < num_cuts; ++cut_counter)
		{
			int ichunk = 0;
			for(itrack=0, num_wires_cut_so_far=0 ; num_wires_cut_so_far<num_wires_cut; itrack=(itrack+1)%nodes_per_chan)
			{
				for(ichunk=0; ichunk<num_chunks && num_wires_cut_so_far<num_wires_cut; ichunk++)
				{
					int track_to_cut = (itrack+(step*ichunk))%nodes_per_chan;

					#ifdef DUMP_CUTTING_PATTERN
					fprintf(fp, "Cutting interposer node at i=%d, j=%d, track=%d \n", i, arch_cut_locations[cut_counter], track_to_cut);
					#endif

					interposer_node_index = interposer_node_loc[i][cut_counter][track_to_cut];
					interposer_node = &rr_node[interposer_node_index];

					// cut all fanout connections of the interposer node
					for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
					{
						delete_rr_connection(interposer_node_index, interposer_node->edges[ifanout]);
						--ifanout;
					}
					assert(rr_node[interposer_node_index].num_edges==0);

					// cut all fanin connections of the interposer node
					for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
					{
						int fanin_node_index = reverse_map[interposer_node_index][ifanin];
						delete_rr_connection(fanin_node_index, interposer_node_index);
						--ifanin;
					}
					assert(rr_node[interposer_node_index].fan_in==0);

					num_wires_cut_so_far++;
				}
			}
		}
	}

	assert(num_wires_cut_so_far==num_wires_cut);

#ifdef DUMP_CUTTING_PATTERN
	fclose(fp);
#endif

}

/*
 * Description: implements chunk cut with rotation (see cut pattern descriptions above)
 * 
 * Returns: none.
 */
void cut_rr_graph_edges_chunk_cut_with_rotation(int nodes_per_chan)
{

#ifdef DUMP_CUTTING_PATTERN
	FILE* fp = my_fopen("cutting_pattern.echo", "w", 0);
#endif

	int i, itrack, ifanout, offset, cut_counter, ifanin;
	int num_wires_cut_so_far = 0;
	int interposer_node_index;
	t_rr_node* interposer_node;

	// Find the number of wires that should be cut at each horizontal cut
	int num_wires_cut = (nodes_per_chan * percent_wires_cut) / 100;
	assert(percent_wires_cut==0 || num_wires_cut <= nodes_per_chan);

	// num_wires_cut should be an even number
	if(num_wires_cut % 2)
	{
			num_wires_cut++;
	}

	if(num_wires_cut == 0)
	{	// nothing to do :)
		return;
	}

	assert(num_wires_cut > 0);

	for(i=0; i<=nx; ++i)
	{
		offset = (i * nodes_per_chan) / nx;
		if(offset % 2) 
		{
			offset++;
		}
		offset = offset%nodes_per_chan; // to keep offset between 0 and nodes_per_chan-1

		for(cut_counter=0; cut_counter < num_cuts; ++cut_counter)
		{
			for(itrack=offset, num_wires_cut_so_far=0 ; num_wires_cut_so_far<num_wires_cut; itrack=(itrack+1)%nodes_per_chan)
			{
				int track_to_cut = (itrack)%nodes_per_chan;

				#ifdef DUMP_CUTTING_PATTERN
				fprintf(fp, "Cutting interposer node at i=%d, j=%d, track=%d \n", i, arch_cut_locations[cut_counter], track_to_cut);
				#endif

				interposer_node_index = interposer_node_loc[i][cut_counter][track_to_cut];
				interposer_node = &rr_node[interposer_node_index];

				// cut all fanout connections of the interposer node
				for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
				{
					delete_rr_connection(interposer_node_index, interposer_node->edges[ifanout]);
					--ifanout;
				}
				assert(rr_node[interposer_node_index].num_edges==0);

				// cut all fanin connections of the interposer node
				for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
				{
					int fanin_node_index = reverse_map[interposer_node_index][ifanin];
					delete_rr_connection(fanin_node_index, interposer_node_index);
					--ifanin;
				}
				assert(rr_node[interposer_node_index].fan_in==0);

				num_wires_cut_so_far++;
			}
		}
	}

	assert(num_wires_cut_so_far==num_wires_cut);

#ifdef DUMP_CUTTING_PATTERN
	fclose(fp);
#endif

}

/*
 * Description: implements chunk cut without rotation (see cut pattern descriptions above)
 * 
 * if pct_of_interposer_nodes_each_chany_can_drive==100, we connect each CHANY wire to ALL interposer nodes (maximum connectivity)
 * if pct_of_interposer_nodes_each_chany_can_drive==20, and channel width is 50 (i.e. 25 interposer wires in each direction), then:
 * each CHANY wire will connect to 5 interposer wires (20% * 25 = 5)
 *
 * if pct_of_chany_wires_an_interposer_node_can_drive==100, we connect each interposer node to ALL vertical wires on the other side of the cut (maximum connectivity)
 * if pct_of_chany_wires_an_interposer_node_can_drive==20, and channel width is 50 (i.e. 25 wires in each direction), then:
 * each interposer wire will drive 5 chany wires on the other side of the cut (20% * 25 = 5)
 *
 * Returns: none.
 */
void cut_rr_graph_edges_chunk_cut_without_rotation(int nodes_per_chan)
{

#ifdef DUMP_CUTTING_PATTERN
	FILE* fp = my_fopen("cutting_pattern.echo", "w", 0);
#endif

	int i, track_to_cut, ifanout, cut_counter, ifanin;
	int num_wires_cut_so_far = 0;
	int interposer_node_index;
	int num_wires_going_through = 0;

	t_rr_node* interposer_node;
	// Find the number of wires that should be cut at each horizontal cut
	int num_wires_cut = (nodes_per_chan * percent_wires_cut) / 100;
	assert(percent_wires_cut==0 || num_wires_cut <= nodes_per_chan);

	// num_wires_cut should be an even number
	if(num_wires_cut % 2)
	{
			num_wires_cut++;
	}

	if(num_wires_cut == 0)
	{	// nothing to do :)
		return;
	}

	num_wires_going_through = nodes_per_chan - num_wires_cut;

	assert(num_wires_cut > 0);
	assert(num_wires_cut%2==0);
	assert(num_wires_going_through > 0);
	assert(num_wires_going_through%2==0);

	// since wires are either INC or DEC, each wire can at most drive half of the interposer nodes that are going through
	// for example: if chan_width=100 and %wires_cut=60% (therefore 40 wires are going through).
	// out of the 40 wires going through, 20 are INC and 20 are DEC.
	// if pct_of_interposer_nodes_each_chany_can_drive=50%, then, we should connect each CHANY wire to 10 interposer nodes (50% * 20 = 10)
	int num_available_interposer_nodes_in_each_direction_after_cutting = num_wires_going_through / 2;

	int num_interposer_nodes_to_connect_to = (int)ceil( (float)pct_of_interposer_nodes_each_chany_can_drive * 
														(float)num_available_interposer_nodes_in_each_direction_after_cutting / 
														100.0);

	int num_interposer_wires_to_be_driven_by = (int)ceil((float)pct_of_chany_wires_an_interposer_node_can_drive * 
														 (float)num_available_interposer_nodes_in_each_direction_after_cutting / 
														 100.0);

	// some info messages would be nice
	if(transfer_interposer_fanins)
	{
		printf("Info: For every interposer wire that is cut, all of its fanins are being connected to another available interposer wire.\n");
	}
	if(transfer_interposer_fanouts)
	{
		printf("Info: For every interposer wire that is cut, all of its fanouts are being connected to another available interposer wire.\n");
	}
	if(	transfer_interposer_fanins &&
		allow_additional_interposer_fanins && 
		num_interposer_nodes_to_connect_to > 1)
	{
		printf("Info: Creating additional connections such that every wire at the cutline connects to %d interposer wires.\n", num_interposer_nodes_to_connect_to);
	}
	if(	transfer_interposer_fanouts &&
		allow_additional_interposer_fanouts && 
		num_interposer_wires_to_be_driven_by > 1)
	{
		printf("Info: Creating additional connections such that every wire on the other side of a cut is driven by %d interposer wires\n", num_interposer_wires_to_be_driven_by);
	}

#ifdef DUMP_DEBUG_FILES
	// DEBUGGING
	FILE* fp_0 = my_fopen("interposer_nodes_before.echo", "w", 0);
	int initial_total_num_interposer_fanins = 0, initial_total_num_interposer_fanouts = 0;
	for(i=0; i<=0; ++i)
	{
		for(cut_counter=0; cut_counter < 1; ++cut_counter)
		{
			for(int itrack=0; itrack < nodes_per_chan; itrack=itrack+2)
			{
				interposer_node_index = interposer_node_loc[i][cut_counter][itrack];
				interposer_node = &rr_node[interposer_node_index];
				int num_chanx_fanouts = 0, num_chany_fanouts = 0;
				int num_chanx_fanins = 0, num_chany_fanins = 0;
				for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
				{
					initial_total_num_interposer_fanouts++;
					if(rr_node[interposer_node->edges[ifanout]].type==CHANX)
						num_chanx_fanouts++;
					else if(rr_node[interposer_node->edges[ifanout]].type==CHANY)
						num_chany_fanouts++;
				}
				for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
				{
					initial_total_num_interposer_fanins++;
					int fanin_node_index = reverse_map[interposer_node_index][ifanin];
					if(rr_node[fanin_node_index].type==CHANX)
						num_chanx_fanins++;
					else if(rr_node[fanin_node_index].type==CHANY)
						num_chany_fanins++;
				}
				fprintf(fp_0, "interposer_node_id:%d, dir:%s, fanins:%d, fanouts:%d, x_fanins:%d, y_fanins:%d, x_fanouts:%d, y_fanouts:%d\n", 
							interposer_node_index, 
							(interposer_node->direction==INC_DIRECTION)?"INC":"DEC", 
							interposer_node->fan_in, 
							interposer_node->num_edges,
							num_chanx_fanins,
							num_chany_fanins,
							num_chanx_fanouts,
							num_chany_fanouts);
			}
			for(int itrack=1; itrack < nodes_per_chan; itrack=itrack+2)
			{
				interposer_node_index = interposer_node_loc[i][cut_counter][itrack];
				interposer_node = &rr_node[interposer_node_index];
				int num_chanx_fanouts = 0, num_chany_fanouts = 0;
				int num_chanx_fanins = 0, num_chany_fanins = 0;
				for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
				{
					if(rr_node[interposer_node->edges[ifanout]].type==CHANX)
						num_chanx_fanouts++;
					else if(rr_node[interposer_node->edges[ifanout]].type==CHANY)
						num_chany_fanouts++;
				}
				for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
				{
					int fanin_node_index = reverse_map[interposer_node_index][ifanin];
					if(rr_node[fanin_node_index].type==CHANX)
						num_chanx_fanins++;
					else if(rr_node[fanin_node_index].type==CHANY)
						num_chany_fanins++;
				}
				fprintf(fp_0, "interposer_node_id:%d, dir:%s, fanins:%d, fanouts:%d, x_fanins:%d, y_fanins:%d, x_fanouts:%d, y_fanouts:%d\n", 
							interposer_node_index, 
							(interposer_node->direction==INC_DIRECTION)?"INC":"DEC", 
							interposer_node->fan_in, 
							interposer_node->num_edges,
							num_chanx_fanins,
							num_chany_fanins,
							num_chanx_fanouts,
							num_chany_fanouts);
			}
		}
	}
	fprintf(fp_0, "total_fanins:%d, total_fanouts:%d\n", initial_total_num_interposer_fanins, initial_total_num_interposer_fanouts);
	fclose(fp_0);
#endif 

	for(i=0; i<=nx; ++i)
	{
		for(cut_counter=0; cut_counter < num_cuts; ++cut_counter)
		{
			for(track_to_cut=0, num_wires_cut_so_far=0 ; num_wires_cut_so_far<num_wires_cut; track_to_cut=(track_to_cut+1)%nodes_per_chan)
			{
				#ifdef DUMP_CUTTING_PATTERN
				fprintf(fp, "Cutting interposer node at i=%d, j=%d, track=%d \n", i, arch_cut_locations[cut_counter], track_to_cut);
				#endif

				interposer_node_index = interposer_node_loc[i][cut_counter][track_to_cut];
				interposer_node = &rr_node[interposer_node_index];

				// cut all fanout connections of the interposer node
				for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
				{
					int fanout_node_index = interposer_node->edges[ifanout];
					int iswitch = get_connection_switch_index(interposer_node_index, fanout_node_index);
					int offset = ifanout;

					// delete the connection
					delete_rr_connection(interposer_node_index, fanout_node_index);
					--ifanout;

					// transfer the connection to another interposer node if needed.
					if(transfer_interposer_fanouts)
					{
						int track_number_of_next_avialable_interposer_node = ( (track_to_cut+2*offset) % num_wires_going_through ) + num_wires_cut;

						assert( 0 <= track_number_of_next_avialable_interposer_node &&
									 track_number_of_next_avialable_interposer_node < nodes_per_chan);
						assert( num_wires_cut <= track_number_of_next_avialable_interposer_node);

						int next_available_interposer_node_index = interposer_node_loc[i][cut_counter][track_number_of_next_avialable_interposer_node];
						assert(rr_node[next_available_interposer_node_index].direction == rr_node[interposer_node_index].direction);

						if(rr_node[fanout_node_index].type==CHANX)
							assert(rr_node[next_available_interposer_node_index].direction==DEC_DIRECTION);

						create_rr_connection(next_available_interposer_node_index, fanout_node_index, iswitch);
					}
				}
				assert(rr_node[interposer_node_index].num_edges==0);

				// cut all fanin connections of the interposer node
				for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
				{
					int fanin_node_index = reverse_map[interposer_node_index][ifanin];
					int iswitch = get_connection_switch_index(fanin_node_index, interposer_node_index);
					int offset = ifanin;

					// delete the connection
					delete_rr_connection(fanin_node_index, interposer_node_index);
					--ifanin;

					// transfer the connection to another interposer node if needed.
					if(transfer_interposer_fanins)
					{
						int track_number_of_next_avialable_interposer_node = ( (track_to_cut+2*offset) % num_wires_going_through ) + num_wires_cut;

						assert( 0 <= track_number_of_next_avialable_interposer_node &&
									 track_number_of_next_avialable_interposer_node < nodes_per_chan);
						assert( num_wires_cut <= track_number_of_next_avialable_interposer_node);

						int next_available_interposer_node_index = interposer_node_loc[i][cut_counter][track_number_of_next_avialable_interposer_node];
						assert(rr_node[next_available_interposer_node_index].direction == rr_node[interposer_node_index].direction);

						if(rr_node[fanin_node_index].type==CHANX)
							assert( rr_node[next_available_interposer_node_index].direction==INC_DIRECTION);

						create_rr_connection(fanin_node_index, next_available_interposer_node_index, iswitch);
					}
				}
				assert(rr_node[interposer_node_index].fan_in==0);

				num_wires_cut_so_far++;
			}
			assert(num_wires_cut_so_far==num_wires_cut);

			if( transfer_interposer_fanins &&
				allow_additional_interposer_fanins &&
				num_interposer_nodes_to_connect_to > 1)
			{
				// by this point, we know that every CHANY wire connects to at least 1 interposer node
				// (even if its interposer node was deleted, it was connected to the next available interposer node
				//  because transfer_interposer_fanins = true)
				// now we want to add additional connections.
				// for interposer nodes that are cut, don't do anything.
				// for other interposer nodes, go over their fanins, and connect each fanin to additional interposer nodes

				// 1. get a list of all wires that drive interposer nodes at this (x,y) location
				std::set< std::pair<int,int> > fanin_wires_at_cutline;
				for(int itrack=0; itrack<nodes_per_chan; ++itrack)
				{
					interposer_node_index = interposer_node_loc[i][cut_counter][itrack];
					for(ifanin=0; ifanin < rr_node[interposer_node_index].fan_in; ++ifanin)
					{
						// according to chunk cut, the first 0..num_wires_cut-1 are supposed to be cut
						// so if we are here, it means this interposer node had some fanins... sanity check
						assert(itrack >= num_wires_cut);

						int fanin_node_index = reverse_map[interposer_node_index][ifanin];
						int iswitch = get_connection_switch_index(fanin_node_index, interposer_node_index);
						std::pair <int,int> connection (fanin_node_index, iswitch);
						fanin_wires_at_cutline.insert(connection);

						if( rr_node[fanin_node_index].type==CHANX )
							assert(rr_node[interposer_node_index].direction==INC_DIRECTION);
					}
				}

				// 2. for each one of the wires, connect them to more interposer wires if needed.
				std::set< std::pair<int,int> >::iterator fanin_iter = fanin_wires_at_cutline.begin();
				std::set< std::pair<int,int> >::iterator fanin_iter_end = fanin_wires_at_cutline.end();
				for( ; fanin_iter!=fanin_iter_end; ++fanin_iter)
				{
					int fanin_node_index = (*fanin_iter).first;
					int iswitch = (*fanin_iter).second;
					int num_interposer_nodes_currently_driven_by_fanin_node = get_num_interposer_loads(fanin_node_index, nodes_per_chan);
					int num_new_connections_to_make = num_interposer_nodes_to_connect_to - num_interposer_nodes_currently_driven_by_fanin_node;
					if(num_new_connections_to_make > 0)
					{
						int counter = 1;
						int offset = 1;
						while(counter <= num_new_connections_to_make)
						{
							int track_number = rr_node[fanin_node_index].ptc_num;
							int track_number_of_next_avialable_interposer_node = ((track_number+2*offset)%num_wires_going_through)+num_wires_cut;

							assert( num_wires_cut <= track_number_of_next_avialable_interposer_node);
							assert( 0<= track_number_of_next_avialable_interposer_node &&
										track_number_of_next_avialable_interposer_node < nodes_per_chan);								

							int next_available_interposer_node_index = interposer_node_loc[i][cut_counter][track_number_of_next_avialable_interposer_node];
							assert(rr_node[next_available_interposer_node_index].direction == rr_node[fanin_node_index].direction);

							// at the cut location, the CHANX wires, can physically only drive the interposer wires in INC direction
							// the interposer wires in the DEC direction can drive CHANX wires at the cut.
							// So, CHANX wires at the cutline in DEC direction need to be connected to the right interposer node in the INC direction
							if(rr_node[fanin_node_index].type==CHANX && rr_node[fanin_node_index].direction==DEC_DIRECTION)
							{
								// give it a nudge
								track_number_of_next_avialable_interposer_node = ((track_number_of_next_avialable_interposer_node+1)%num_wires_going_through)+num_wires_cut;
								next_available_interposer_node_index = interposer_node_loc[i][cut_counter][track_number_of_next_avialable_interposer_node];
							}

							if(rr_node[fanin_node_index].type==CHANX)
							{
								assert(rr_node[next_available_interposer_node_index].direction==INC_DIRECTION);
							}

							// create the connection
							bool new_connection_made =  create_rr_connection(fanin_node_index, next_available_interposer_node_index, iswitch);
							if(new_connection_made)
							{
								counter++;
							}

							offset++;
						}

						// sanity check: any wire at the boundary should be feeding 'num_interposer_nodes_to_connect_to' interposer nodes in total
						assert(get_num_interposer_loads(fanin_node_index, nodes_per_chan)==num_interposer_nodes_to_connect_to);
					}
				}
			}

			if( transfer_interposer_fanouts &&
				allow_additional_interposer_fanouts &&
				num_interposer_wires_to_be_driven_by > 1)
			{
				// 1. temporary storage for all the vertical wires at the cutline driven by interposer wires
				std::set< std::pair<int,int> > fanout_wires_at_cutline;
				for(int itrack=0; itrack<nodes_per_chan; ++itrack)
				{
					interposer_node_index = interposer_node_loc[i][cut_counter][itrack];
					interposer_node = &rr_node[interposer_node_index];
					for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
					{
						int fanout_node_index = interposer_node->edges[ifanout];
						int iswitch = get_connection_switch_index(interposer_node_index, fanout_node_index);
						std::pair <int,int> connection (fanout_node_index, iswitch);
						fanout_wires_at_cutline.insert(connection);
					}
				}

				// 2. for each one of the interposer wires, connect them to more fanout wires if needed.
				std::set< std::pair<int,int> >::iterator fanout_iter = fanout_wires_at_cutline.begin();
				std::set< std::pair<int,int> >::iterator fanout_iter_end = fanout_wires_at_cutline.end();
				for( ; fanout_iter!=fanout_iter_end; ++fanout_iter)
				{
					int fanout_node_index = (*fanout_iter).first;
					int iswitch = (*fanout_iter).second;
					int num_interposer_nodes_currently_driving_this_fanout_node = get_num_interposer_drivers(fanout_node_index, nodes_per_chan);
					int num_new_connections_to_make = num_interposer_wires_to_be_driven_by - num_interposer_nodes_currently_driving_this_fanout_node;
					if(num_new_connections_to_make > 0)
					{
						int counter = 1;
						int offset = 1;
						while(counter <= num_new_connections_to_make)
						{
							int track_number = rr_node[fanout_node_index].ptc_num;
							int track_number_of_next_avialable_interposer_node = ((track_number+2*offset)%num_wires_going_through)+num_wires_cut;

							assert( num_wires_cut <= track_number_of_next_avialable_interposer_node);
							assert( 0<= track_number_of_next_avialable_interposer_node &&
										track_number_of_next_avialable_interposer_node < nodes_per_chan);								

							int next_available_interposer_node_index = interposer_node_loc[i][cut_counter][track_number_of_next_avialable_interposer_node];
							assert(rr_node[next_available_interposer_node_index].direction == rr_node[fanout_node_index].direction);

							// at the cut location, only the interposer wires in the DEC direction can drive the CHANX wires below the cut.
							// So, both INC and DEC CHANX wires need to be fed driven by DEC interposer wires
							if(rr_node[fanout_node_index].type==CHANX && rr_node[fanout_node_index].direction==INC_DIRECTION)
							{
								// give it a nudge
								track_number_of_next_avialable_interposer_node = ((track_number_of_next_avialable_interposer_node+1)%num_wires_going_through)+num_wires_cut;
								next_available_interposer_node_index = interposer_node_loc[i][cut_counter][track_number_of_next_avialable_interposer_node];
							}

							if(rr_node[fanout_node_index].type==CHANX)
							{
								assert(rr_node[next_available_interposer_node_index].direction==DEC_DIRECTION);
							}

							// create the connection
							bool new_connection_made =  create_rr_connection(next_available_interposer_node_index, fanout_node_index, iswitch);
							if(new_connection_made)
							{
								counter++;
							}

							offset++;
						}

						// sanity check: any wire at the boundary should be driven by 'num_interposer_wires_to_be_driven_by' interposer wires
						assert(get_num_interposer_drivers(fanout_node_index, nodes_per_chan)==num_interposer_wires_to_be_driven_by);
					}
				}
			}
		}
	}

#ifdef DUMP_DEBUG_FILES
	// DEBUGGING
	FILE* fp_1 = my_fopen("interposer_nodes_after.echo", "w", 0);
	int final_total_num_interposer_fanins = 0, final_total_num_interposer_fanouts = 0;
	for(i=0; i<=0; ++i)
	{
		for(cut_counter=0; cut_counter < 1; ++cut_counter)
		{
			for(int itrack=0; itrack < nodes_per_chan; itrack=itrack+2)
			{
				interposer_node_index = interposer_node_loc[i][cut_counter][itrack];
				interposer_node = &rr_node[interposer_node_index];
				int num_chanx_fanouts = 0, num_chany_fanouts = 0;
				int num_chanx_fanins = 0, num_chany_fanins = 0;
				for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
				{
					final_total_num_interposer_fanouts++;
					if(rr_node[interposer_node->edges[ifanout]].type==CHANX)
						num_chanx_fanouts++;
					else if(rr_node[interposer_node->edges[ifanout]].type==CHANY)
						num_chany_fanouts++;
				}
				for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
				{
					final_total_num_interposer_fanins++;
					int fanin_node_index = reverse_map[interposer_node_index][ifanin];
					if(rr_node[fanin_node_index].type==CHANX)
						num_chanx_fanins++;
					else if(rr_node[fanin_node_index].type==CHANY)
						num_chany_fanins++;
				}
				fprintf(fp_1, "interposer_node_id:%d, dir:%s, fanins:%d, fanouts:%d, x_fanins:%d, y_fanins:%d, x_fanouts:%d, y_fanouts:%d\n", 
							interposer_node_index, 
							(interposer_node->direction==INC_DIRECTION)?"INC":"DEC", 
							interposer_node->fan_in, 
							interposer_node->num_edges,
							num_chanx_fanins,
							num_chany_fanins,
							num_chanx_fanouts,
							num_chany_fanouts);
			}
			for(int itrack=1; itrack < nodes_per_chan; itrack=itrack+2)
			{
				interposer_node_index = interposer_node_loc[i][cut_counter][itrack];
				interposer_node = &rr_node[interposer_node_index];
				int num_chanx_fanouts = 0, num_chany_fanouts = 0;
				int num_chanx_fanins = 0, num_chany_fanins = 0;
				for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
				{
					if(rr_node[interposer_node->edges[ifanout]].type==CHANX)
						num_chanx_fanouts++;
					else if(rr_node[interposer_node->edges[ifanout]].type==CHANY)
						num_chany_fanouts++;
				}
				for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
				{
					int fanin_node_index = reverse_map[interposer_node_index][ifanin];
					if(rr_node[fanin_node_index].type==CHANX)
						num_chanx_fanins++;
					else if(rr_node[fanin_node_index].type==CHANY)
						num_chany_fanins++;
				}
				fprintf(fp_1, "interposer_node_id:%d, dir:%s, fanins:%d, fanouts:%d, x_fanins:%d, y_fanins:%d, x_fanouts:%d, y_fanouts:%d\n", 
							interposer_node_index, 
							(interposer_node->direction==INC_DIRECTION)?"INC":"DEC", 
							interposer_node->fan_in, 
							interposer_node->num_edges,
							num_chanx_fanins,
							num_chany_fanins,
							num_chanx_fanouts,
							num_chany_fanouts);
			}
		}
	}
	fprintf(fp_1, "total_fanins:%d, total_fanouts:%d\n", final_total_num_interposer_fanins, final_total_num_interposer_fanouts); 
	fclose(fp_1);
#endif 

#ifdef DUMP_CUTTING_PATTERN
	fclose(fp);
#endif

}

/*
 * Description: 
 * This function increases the switch delay for interposer nodes.
 * The switch driving the interposer node will have added delays
 * Returns: none.
 */
void increase_rr_graph_edge_delays(int nodes_per_chan)
{
	int inode, i, j;
	int old_switch, new_switch;
	for(i=0; i<s_num_interposer_nodes; ++i)
	{
		inode = interposer_nodes[i];

		for(j=0; j < rr_node[inode].fan_in; ++j)
		{
			int fanin_node_id = reverse_map[inode][j];
			int cnt;
			for(cnt=0; cnt<rr_node[fanin_node_id].num_edges && rr_node[fanin_node_id].edges[cnt]!=inode; ++cnt);
			old_switch = rr_node[fanin_node_id].switches[cnt];
			new_switch = increased_delay_edge_map[old_switch];
			rr_node[fanin_node_id].switches[cnt] = new_switch;

			// printf("Changing from switch_delay[%d]=%g to switch_delay[%d]=%g\n", old_switch, switch_inf[old_switch].Tdel, new_switch, switch_inf[new_switch].Tdel);
		}
	}
}

/*
 * Description: This function is used for experimentation. Decided which cutting pattern to use based on s_pattern_type
 *
 * Returns: none.
 */
void cut_rr_connections(int nodes_per_chan)
{
	switch(s_pattern_type)
	{
	case UNIFORM_CUT_WITH_ROTATION:
		cut_rr_graph_edges_uniform_cut_with_rotation(nodes_per_chan);
		break;
	case UNIFORM_CUT_WITHOUT_ROTATION:
		cut_rr_graph_edges_uniform_cut_without_rotation(nodes_per_chan);
		break;
	case CHUNK_CUT_WITH_ROTATION:
		cut_rr_graph_edges_chunk_cut_with_rotation(nodes_per_chan);
		break;
	case CHUNK_CUT_WITHOUT_ROTATION:
		cut_rr_graph_edges_chunk_cut_without_rotation(nodes_per_chan);
		break;
	default:
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Unknown cutting pattern!\n");
		break;
	}
}

/*
 * Purpose: To measure the effect of allowing chanx_to_interpoer_node connections.
 * Description: This function cuts all chanx connections to interposer_nodes.
 *
 * Returns: none.
 */
void cut_chanx_interposer_node_connections(int nodes_per_chan)
{
	int i, ifanout, ifanin;
	for(i=0; i<s_num_interposer_nodes; ++i)
	{
		int interposer_node_id = interposer_nodes[i];
		t_rr_node* interposer_node = &rr_node[interposer_node_id];
		assert(interposer_node->type==CHANY);

		// cut all fanout connections driving a CHANX
		for(ifanout=0; ifanout < interposer_node->num_edges; ++ifanout)
		{
			int fanout_node_id = interposer_node->edges[ifanout];
			if(rr_node[fanout_node_id].type==CHANX)
			{
				delete_rr_connection(interposer_node_id, fanout_node_id);
				--ifanout;
			}
		}

		// cut all fanin connections driven by a CHANX
		for(ifanin=0; ifanin < interposer_node->fan_in; ++ifanin)
		{
			int fanin_node_id = reverse_map[interposer_node_id][ifanin];
			if(rr_node[fanin_node_id].type==CHANX)
			{
				delete_rr_connection(fanin_node_id, interposer_node_id);
				--ifanin;
			}
		}
	}
}

/*
 * Description: The channel contains PAIRs of tracks. For each wire, its pair is the track right next to it with the opposite direction.
 *              For wire X, we call its pair X'.
 *              This function takes the index of X and returns the index of X'.
 *              For instance: (0 and 1) are a pair. (2 and 3) are a pair, etc.
 */
int get_pair_track_index(int index)
{
	if(index%2==0)
	{
		return index+1;
	}
	else
	{
		return index-1;
	}
}

/*
 * Description: SET method for datastructure "vertical_wires_at_cut_locations"
 * 
 * Returns: none.
*/
void set_wire_index_at_location(int x, int y, int itrack, int ***vertical_wires_at_cut_locations, int node_id)
{
	assert(vertical_wires_at_cut_locations!=0);

	// e.g. if cuts are at y=10, y=20 and y=30
	// this map will have:
	// 10->0, 
	// 11->1
	// 20->2
	// 21->3
	// 30->4
	// 31->5
	std::map<int,int> y_coord_to_cut_index_map;
	int i=0;
	for(i=0; i<num_cuts; ++i)
	{
		y_coord_to_cut_index_map[ arch_cut_locations[i] ] = 2*i;
		y_coord_to_cut_index_map[ arch_cut_locations[i]+1 ] = 2*i+1;
	}

	int cut_location_index = y_coord_to_cut_index_map[y];
	vertical_wires_at_cut_locations[x][cut_location_index][itrack] = node_id;
}

/*
 * Description: GET method for datastructure "vertical_wires_at_cut_locations"
 * 
 * Returns: none.
*/
int get_wire_index_at_location(int x, int y, int itrack, int ***vertical_wires_at_cut_locations)
{
	assert(vertical_wires_at_cut_locations!=0);

	// e.g. if cuts are at y=10, y=20 and y=30
	// this map will have:
	// 10->0, 
	// 11->1
	// 20->2
	// 21->3
	// 30->4
	// 31->5
	std::map<int,int> y_coord_to_cut_index_map;
	int i=0;
	for(i=0; i<num_cuts; ++i)
	{
		y_coord_to_cut_index_map[ arch_cut_locations[i] ] = 2*i;
		y_coord_to_cut_index_map[ arch_cut_locations[i]+1 ] = 2*i+1;
	}

	int cut_location_index = y_coord_to_cut_index_map[y];
	return vertical_wires_at_cut_locations[x][cut_location_index][itrack];
}

/*
 * Description: Frees the datastructure allocated by allocate_and_initialize_vertical_wires_at_cut_locations
 * 
 * Returns: none.
*/ 
void free_vertical_wires_at_cut_locations(int nodes_per_chan, int*** vertical_wires_at_cut_locations)
{
	int i,j;
	for(i=0; i<nx+1; ++i)
	{
		for(j=0; j<2*num_cuts; ++j)
		{
			free(vertical_wires_at_cut_locations[i][j]);
		}
	}
	for(i=0; i<nx+1; ++i)
	{
		free(vertical_wires_at_cut_locations[i]);
	}
	free(vertical_wires_at_cut_locations);
}

/*
 * Description: Allocate memory for a helper datastructure that stores the rr_node_id for a given vertical wire (x,y,ptc_num) at the cut location.
 *              Some vertical wires are just below the cut and some are just above the cut
 *              vertical_wires_at_cut_locations[x][y][track#] = inode.
 *              initializes all entries to -1
 * Returns: none.
*/ 
int*** allocate_and_initialize_vertical_wires_at_cut_locations(int nodes_per_chan)
{
	// Make a list of wires that can feed or be fed by bidirectional interposer wires
	// vertical_wires_at_cut_locations[x][y][track#]
	int i, j, k;
	int num_vertical_channels = nx+1;
	int ***vertical_wires_at_cut_locations = (int***) my_malloc(num_vertical_channels * sizeof(int**));
	for(i=0; i<num_vertical_channels; ++i)
	{
		// at every cut, there are 2 wires per interposer node (1 wire below the cut, and 1 wire above the cut)
		vertical_wires_at_cut_locations[i] = (int**)my_malloc(2*num_cuts * sizeof(int*));
	}
	for(i=0; i<num_vertical_channels; ++i)
	{
		for(j=0; j<2*num_cuts; ++j)
		{
			vertical_wires_at_cut_locations[i][j] = (int*)my_malloc(nodes_per_chan * sizeof(int));
		}
	}
	for(i=0; i<num_vertical_channels; ++i)
	{
		for(j=0; j<2*num_cuts; ++j)
		{
			for(k=0; k<nodes_per_chan; ++k)
			{
				set_wire_index_at_location(i,j,k,vertical_wires_at_cut_locations,-1);
			}
		}
	}
	return vertical_wires_at_cut_locations;
}


/*
 * Description: Go over all interposer wires, change their direction from INC or DEC to BIDIR
 *              The channel contains PAIRs of tracks. For each wire, its pair is the track right next to it with the opposite direction.
 *              For wire X, we call its pair X'.
 *              If wire X is feeding an interposer wire, wire X' should be fed by the same interposer wire.
 *              If wire X is fed  by an interposer wire, wire X' should feed the same interposer wire.
 * Returns: none.
*/ 
int*** get_vertical_wires_at_cut_locations(int nodes_per_chan)
{
	
	// Make a list of wires that can feed or be fed by bidirectional interposer wires
	// vertical_wires_at_cut_locations[x][y][track#]
	int*** vertical_wires_at_cut_locations = allocate_and_initialize_vertical_wires_at_cut_locations(nodes_per_chan);

	int i, j, k;
	for(i=0; i<s_num_interposer_nodes; ++i)
	{
		int interposer_node_id = interposer_nodes[i];
		t_rr_node* interposer_node = &rr_node[interposer_node_id];
		
		assert(interposer_node->ylow==interposer_node->yhigh);

		int cut_position = interposer_node->ylow;

		// go over all fanins
		for(j=0; j < interposer_node->fan_in; ++j)
		{
			int fanin_node_id = reverse_map[interposer_node_id][j];
			if(rr_node[fanin_node_id].type==CHANY)
			{
				assert(rr_node[fanin_node_id].xlow==rr_node[fanin_node_id].xhigh);
				int x = rr_node[fanin_node_id].xlow;
				int y = (rr_node[fanin_node_id].ylow > cut_position) ? cut_position+1 : cut_position;
				int itrack = rr_node[fanin_node_id].ptc_num;
				set_wire_index_at_location(x, y, itrack, vertical_wires_at_cut_locations, fanin_node_id);
			}
		}
		// go over all fanouts
		for(j=0; j < interposer_node->num_edges; ++j)
		{
			int fanout_node_id = interposer_node->edges[j];
			if(rr_node[fanout_node_id].type==CHANY)
			{
				assert(rr_node[fanout_node_id].xlow==rr_node[fanout_node_id].xhigh);
				int x = rr_node[fanout_node_id].xlow;
				int y = (rr_node[fanout_node_id].ylow > cut_position) ? cut_position+1 : cut_position;
				int itrack = rr_node[fanout_node_id].ptc_num;
				set_wire_index_at_location(x, y, itrack, vertical_wires_at_cut_locations, fanout_node_id);
			}
		}
	}

	// sanity check
	for(i=0; i<nx+1; ++i)
		for(j=0; j<2*num_cuts; ++j)
			for(k=0; k<nodes_per_chan; ++k)
				assert( get_wire_index_at_location(i, j, k, vertical_wires_at_cut_locations) != -1 );

	return vertical_wires_at_cut_locations;
}

/*
 * Description: Change interposer wire directions from INC/DEC to BIDIR. Also create connections from the bidir interposer wires
 *              to vertical wires in the proper direction on both sides of the cut
*/
void make_interposer_wires_bidirectional(int nodes_per_chan, int ***vertical_wires_at_cut_locations)
{
	int i, j;

	assert(vertical_wires_at_cut_locations!=0);

	// for all interposer wires
	for(i=0; i<s_num_interposer_nodes; ++i)
	{
		// 1. Change the wire direction to BIDIR
		int interposer_node_id = interposer_nodes[i];
		t_rr_node* interposer_node = &rr_node[interposer_node_id];
		interposer_node->direction=BI_DIRECTION;

		// 2. Find all wires feeding this interposer wire ( call such wire X ) (i.e. fanins of the interposer wire)
		//    Find the pair of X ( call it X' ) (For each wire, its pair is the track right next to it with the opposite direction)
		//    have the interposer wire feed X'
		for(j=0; j < interposer_node->fan_in; ++j)
		{
			int fanin_node_id = reverse_map[interposer_node_id][j];
			if(rr_node[fanin_node_id].type==CHANY)
			{
				assert(interposer_node->ylow==interposer_node->yhigh);
				assert(rr_node[fanin_node_id].xlow==rr_node[fanin_node_id].xhigh);
				int iswitch = get_connection_switch_index(fanin_node_id,interposer_node_id);
				assert(iswitch!=-1);
				int cut_position = interposer_node->ylow;
				int x = rr_node[fanin_node_id].xlow;
				int y = (rr_node[fanin_node_id].ylow > cut_position) ? cut_position+1 : cut_position;
				int itrack = rr_node[fanin_node_id].ptc_num;
				int pair_track_index = get_pair_track_index(itrack);
				int pair_rr_node_id = get_wire_index_at_location(x, y, pair_track_index, vertical_wires_at_cut_locations);
				create_rr_connection(interposer_node_id, pair_rr_node_id, iswitch);
			}
		}		

		// 3. Find all wires fed by this interposer wire ( call such wire X ) (i.e. fanouts of the interposer wire)
		//    Find the pair of X ( call it X' ) (For each wire, its pair is the track right next to it with the opposite direction)
		//    have X' drive the interposer wire
		for(j=0; j < interposer_node->num_edges; ++j)
		{
			int fanout_node_id = interposer_node->edges[j];
			if(rr_node[fanout_node_id].type==CHANY)
			{
				assert(interposer_node->ylow==interposer_node->yhigh);
				assert(rr_node[fanout_node_id].xlow==rr_node[fanout_node_id].xhigh);
				int iswitch = get_connection_switch_index(interposer_node_id,fanout_node_id); 
				assert(iswitch!=-1);
				int cut_position = interposer_node->ylow;
				int x = rr_node[fanout_node_id].xlow;
				int y = (rr_node[fanout_node_id].ylow > cut_position) ? cut_position+1 : cut_position;
				int itrack = rr_node[fanout_node_id].ptc_num;
				int pair_track_index = get_pair_track_index(itrack);
				int pair_rr_node_id = get_wire_index_at_location(x, y, pair_track_index, vertical_wires_at_cut_locations);
				create_rr_connection(pair_rr_node_id, interposer_node_id, iswitch);
			}
		}		
		
	}
}


/*
 * Description: This is the MAIN entry point for rr_graph modifications for interposer based architectures
 *           
 *  Note: either USE_SIMPLER_SWITCH_MODIFICATION_METHODOLOGY or 
 *               USE_INTERPOSER_NODE_ADDITION_METHODOLOGY
 *        must be defined
 *                    
 * Returns: none.
 */
void modify_rr_graph_for_interposer_based_arch
(
	int nodes_per_chan,
	enum e_directionality directionality
)
{

#ifdef USE_SIMPLER_SWITCH_MODIFICATION_METHODOLOGY
	modify_rr_graph_using_switch_modification_methodology(nodes_per_chan, directionality);
#endif

#ifdef USE_INTERPOSER_NODE_ADDITION_METHODOLOGY
	modify_rr_graph_using_interposer_node_addition_methodology(nodes_per_chan, directionality);
#endif
}


/*
 * Description: This is the main entry point to rr_graph modifications for interposer based architectures
 *
 * FIRST 
 *
 * Find all rr_nodes that cross the interposer.
 * for every wire that crosses a cut, we break it into two nodes.
 * for instance, if a cut is at y=8 and we have an INC CHANY ylow=6-->yhigh=10,
 * then we create an extra rr_node for that wire: 
 * the "original node" will be changed to: INC CHANY ylow=6-->yhigh=8
 * the "new node" will be:                 INC CHANY ylow=9-->yhigh=10
 * The fan-ins and fan-outs of the original node may have to be moved.
 * Fanins on the same side of the cut as the original node will continue to drive the original node.
 * Fanins on the other side of the cut will now be feeding the new node.
 * Similar thing goes for fanouts
 *
 *  So, we are basically turning this:
 *
 *    O   O    O
 *    |   |    |
 *    \   |   /
 *      \ | /
 *        O                   <---- this is the node that's crossing the interposer (inode)
 *       / \
 *      /   \
 *     O     O
 *
 * into this:
 *
 *
 *    O   O    
 *    |   |    
 *     \  |   
 *      \ | 
 *       \|
 *        O                   <---- this is the new node (new_node)
 *        |
 *        |
 *        |
 *        | O
 *        |/
 *        O                   <---- this is the original node (inode)
 *       / \
 *      /   \
 *     O     O
 *
 * in this example, one of the fanouts of the original node remained on the same side of the cut.
 * the other two fanouts were transferred to be fanouts of the new node
 * by the end of this step, there should be *no* wire that crosses a cut
 *
 * SECOND
 *
 * insert an "interposer node" between any two nodes that are connected on two different sides of a cut
 *
 *    O   O    
 *    |   |    
 *     \  |   
 *      \ | 
 *       \|
 *        O                   <---- this is the new node (new_node)
 *        |
 *        O                   <---- This is the interposer_node
 *        |
 *        | O
 *        |/
 *        O                   <---- this is the original node (inode)
 *       / \
 *      /   \
 *     O     O
 *
 * Interposer nodes in the rr_graph have the type of CHANY.
 * Interposer nodes in the rr_graph have ylow=yhigh=y_coordinate_of_cut
 * 
 * Connections to interposer nodes have larger switch delay (e.g 1.058ns)
 * Connections from interposer nodes to wires on the other side have 0 delay
 * in every channel, num_interposer_nodes = chan_width
 * in every channel, (%wires_cut*num_interposer_nodes) are selected and all their fanins and fanouts are cut
 * 
 * 
 * Returns: None.
 */
void modify_rr_graph_using_interposer_node_addition_methodology
(
	int nodes_per_chan,
	enum e_directionality directionality
)
{
	if(directionality == BI_DIRECTIONAL) /* Ignored for now TODO */
		return;

	if(num_cuts <=0)
		return;

#ifdef	DUMP_DEBUG_FILES
	// Before doing anything, let's dump the connections in the rr_graph
	FILE* fp = my_fopen("before_cutting.txt", "w", 0);
	dump_rr_graph_connections(fp);
	fclose(fp);
#endif

	s_num_interposer_nodes = (nx+1)*(num_cuts)*(nodes_per_chan);

	int *rr_nodes_that_cross = 0;
	int num_rr_nodes_that_cross = 0;
	int ***vertical_wires_at_cut_locations = 0;

	// 1. find all CHANY wires that cross the interposer
	find_all_CHANY_wires_that_cross_the_interposer(nodes_per_chan, &rr_nodes_that_cross, &num_rr_nodes_that_cross);
	
	// 2. This the tough part: add new rr_nodes and fix up fanins and fanouts and switches
	expand_rr_graph(rr_nodes_that_cross, num_rr_nodes_that_cross, nodes_per_chan); 

	// 2b. set up a helper data structure that will be used when making bidirectional interposer wires
	if(allow_bidir_interposer_wires)
	{
		vertical_wires_at_cut_locations = get_vertical_wires_at_cut_locations(nodes_per_chan);
	}

	// 3. cut some of the wires
	cut_rr_connections(nodes_per_chan);

	// 4. increase the delay of interposer nodes that were not cut
	increase_rr_graph_edge_delays(nodes_per_chan);

	// 5. additional graph ops here for experimentation (cut vs. not cut the CHANX to interposer wire connections
	if(!allow_chanx_interposer_connections)
	{
		vpr_printf_info("Info: Not connecting horizontal wires below the cuts to interposer mux inputs\n");
		cut_chanx_interposer_node_connections(nodes_per_chan);
	}
	else
	{
		vpr_printf_info("Info: connecting horizontal wires below the cuts to interposer mux inputs\n");
	}

	// 6. make interposer wires bidirectional and connect them to vertical wires of in both directions
	if(allow_bidir_interposer_wires)
	{
		// First, let's see some stats about number of signals crossing in each direction at the cuts
		print_stats_signal_direction_at_cuts(nodes_per_chan);

		vpr_printf_info("Info: Changing interposer wires into Bidirectional wires.\n");
		make_interposer_wires_bidirectional(nodes_per_chan, vertical_wires_at_cut_locations);
		free_vertical_wires_at_cut_locations(nodes_per_chan, vertical_wires_at_cut_locations);
	}

	// 5.1 free helper data-structures that are not needed anymore
	free_reverse_map(num_rr_nodes);
	free(interposer_nodes);

	// 5.2 free helper data-structures that are not needed anymore
	int i,j;
	for(i=0; i<nx+1; ++i)
	{
		for(j=0; j<num_cuts; ++j)
		{
			free(interposer_node_loc[i][j]);
		}
	}
	for(i=0; i<nx+1; ++i)
	{
		free(interposer_node_loc[i]);
	}
	free(interposer_node_loc);

#ifdef	DUMP_DEBUG_FILES
	// dump after all rr_Graph modifications are done
	FILE* fp2 = my_fopen("after_cutting.txt", "w", 0);
	dump_rr_graph_connections(fp2);
	fclose(fp2);
#endif
}

/*
 * Description: finds out which wires cross an interposer cut.
 *
 * Returns: the rr_node IDs of the wires that cross an interposer cut (returned by param ref)
 *          the number of rr_nodes       that cross an interposer cut (returned by param ref)
 */
void find_all_CHANY_wires_that_cross_the_interposer(int nodes_per_chan, int** rr_nodes_that_cross, int* num_rr_nodes_that_cross)
{
	int inode, cut_pos, cut_counter;
	int total_chany_wires = 0;
	*num_rr_nodes_that_cross = 0;

	// at the very most (if all vertical wires in the channel cross the interposer)
	// the number of nodes that cross the interposer will be: num_nodes_per_channel * nx * num_cuts
	int max_num_nodes_that_cross = nodes_per_chan*(nx+1)*num_cuts;
	*rr_nodes_that_cross = (int*) my_malloc(max_num_nodes_that_cross * sizeof(int));
	for(inode=0; inode<max_num_nodes_that_cross; ++inode)
	{
		(*rr_nodes_that_cross)[inode] = -1;
	}

	for(inode=0; inode<num_rr_nodes;++inode)
	{
		if(rr_node[inode].type==CHANY)
		{
			total_chany_wires++;
			for(cut_counter=0; cut_counter < num_cuts; cut_counter++)
			{
				cut_pos = arch_cut_locations[cut_counter];
				if(rr_node[inode].ylow <= cut_pos && rr_node[inode].yhigh > cut_pos)
				{
					(*rr_nodes_that_cross)[*num_rr_nodes_that_cross] = inode;
					(*num_rr_nodes_that_cross)++;
					break;
				}
			}
		}
	}
	
	// DEBUG MESSAGES
	/*
	printf("Found %d CHANY nodes that cross the interposer:\n", *num_rr_nodes_that_cross);
	int i;
	for(i=0;i<*num_rr_nodes_that_cross;++i)
	{
		inode = (*rr_nodes_that_cross)[i];
		printf("Node: %d,%d,%d,%d,%d\n", inode, rr_node[inode].xlow, rr_node[inode].xhigh, rr_node[inode].ylow, rr_node[inode].yhigh);
	}
	*/
}

/*
 * Description: this function deletes the connection between src node and dst node
 *              it will take care of updating all necessary data-structures
 *              (e.g. fanins and fanouts of src and dst, switches, num edges, etc).
 *              if no connection exists between src and dst, it will return without doing anything
 *
 *      Note: This function will modify the fanin and fanout list of the src and dst,
 *      hence, the caller should not assume that rr_node[src_node].edges[i] is the same node
 *      before and after a call to this function.
 *      For instance, the following code (which is attempting to delete all fanouts of inode) has a bug:
 *
 *      for(int i=0; i<rr_node[inode].num_edges; ++i)
 *      {
 *          delete_rr_connection(inode, rr_node[inode].edges[i];
 *      }
 *                    
 *      In the first iteration of the loop, the first fanout of inode is deleted,
 *      hence, rr_node[inode].edges[1] be moved to rr_node[inodes].edges[0] after the first delete is called
 *
 *      The following code correctly deletes all fanouts of inode
 *      
 *      for(int i=0; i<rr_node[inode].num_edges; ++i)
 *      {
 *          delete_rr_connection(inode, rr_node[inode].edges[i];
 *          i--;
 *      }
 *
 * Returns: Nothing
 * 
 */
void delete_rr_connection(int src_node, int dst_node)
{
	// Debug
	/*
	printf("before deleting edge from %d to %d\n", src_node, dst_node);
	print_fanins_and_fanouts_of_rr_node(src_node);
	print_fanins_and_fanouts_of_rr_node(dst_node);
	*/

	// 0. return if the connection doesn't exist
	int i, counter;
	bool connected = false;
	for(i=0; i<rr_node[src_node].num_edges; ++i)
	{
		if(rr_node[src_node].edges[i]==dst_node)
		{
			connected = true;
		}
	}
	if(!connected)
	{
		return;
	}


	// 1. take care of the source node side
	// this will work fine even if num_src_fanouts_after_cutting is 0
	int num_src_fanouts_before_cutting = rr_node[src_node].num_edges;
	int num_src_fanouts_after_cutting  = num_src_fanouts_before_cutting - 1;

	int   *temp_src_edges    = (int*)   my_malloc(num_src_fanouts_after_cutting * sizeof(int));
	short *temp_src_switches = (short*) my_malloc(num_src_fanouts_after_cutting * sizeof(short));
	counter = 0;
	for(i=0; i < num_src_fanouts_before_cutting; ++i)
	{
		if(rr_node[src_node].edges[i]!=dst_node)
		{
			temp_src_edges[counter]    = rr_node[src_node].edges[i];
			temp_src_switches[counter] = rr_node[src_node].switches[i];
			counter++;
		}
	}
	assert(counter==num_src_fanouts_after_cutting);
	assert(num_src_fanouts_after_cutting >= 0);

	rr_node[src_node].num_edges = num_src_fanouts_after_cutting;
	free(rr_node[src_node].edges);
	free(rr_node[src_node].switches);
	rr_node[src_node].edges = temp_src_edges;
	rr_node[src_node].switches = temp_src_switches;


	// 2. take care of the destination node side
	// this will work fine even if num_dst_fanins_after_cutting is 0
	int num_dst_fanins_before_cutting = rr_node[dst_node].fan_in;
	int num_dst_fanins_after_cutting  = num_dst_fanins_before_cutting - 1;
	int* temp_dst_fanins = (int*) my_malloc(num_dst_fanins_after_cutting * sizeof(int));
	i=0;
	counter=0;
	for(i=0; i < num_dst_fanins_before_cutting; ++i)
	{
		if(reverse_map[dst_node][i]!=src_node)
		{
			temp_dst_fanins[counter] = reverse_map[dst_node][i];
			counter++;
		}
	}
	assert(counter==num_dst_fanins_after_cutting);

	free(reverse_map[dst_node]);
	reverse_map[dst_node] = temp_dst_fanins;

	rr_node[dst_node].fan_in = num_dst_fanins_after_cutting;
	assert(rr_node[dst_node].fan_in>=0);

	// Debug
	/*
	printf("after deleting edge from %d to %d\n", src_node, dst_node);
	print_fanins_and_fanouts_of_rr_node(src_node);
	print_fanins_and_fanouts_of_rr_node(dst_node);
	*/
}

/*
 * Description: this function creates a new connection from SRC to DST node
 *              it will take care of updating all necessary data-structures
 *              the connection will use a switch with ID of connection_switch_index
 *              if connection from src to dst already exists, it returns without doing anything
 *
 * Returns: True  if a new connection is created. 
 *          False if the connection already exsited, and hence a new connection was not created.
 */
bool create_rr_connection(int src_node, int dst_node, int connection_switch_index)
{
	// Debug
	/*
	printf("before creating edge from %d to %d\n", src_node, dst_node);
	print_fanins_and_fanouts_of_rr_node(src_node);
	print_fanins_and_fanouts_of_rr_node(dst_node);
	*/

	// 0. if connection already exists, return
	int i;
	bool already_connected = false;
	for(i=0; i<rr_node[src_node].num_edges; ++i)
	{
		if(rr_node[src_node].edges[i]==dst_node)
		{
			already_connected = true;
		}
	}
	if(already_connected)
	{
		return false;
	}

	// 1. take care of the source node side
	// realloc will behave like malloc if pointer was NULL before
	rr_node[src_node].num_edges++;
	int num_src_fanouts_after_new_connection = rr_node[src_node].num_edges;
	assert(num_src_fanouts_after_new_connection > 0);
	rr_node[src_node].edges =    (int*)   my_realloc(rr_node[src_node].edges,    sizeof(int)*(num_src_fanouts_after_new_connection));
	rr_node[src_node].switches = (short*) my_realloc(rr_node[src_node].switches, sizeof(int)*(num_src_fanouts_after_new_connection));
	rr_node[src_node].edges[num_src_fanouts_after_new_connection-1] = dst_node;
	rr_node[src_node].switches[num_src_fanouts_after_new_connection-1] = connection_switch_index;

	// 2. take care of the dst node side
	rr_node[dst_node].fan_in++;
	int num_dst_fanins_after_new_connection = rr_node[dst_node].fan_in;
	reverse_map[dst_node] = (int*)my_realloc(reverse_map[dst_node], sizeof(int)*(num_dst_fanins_after_new_connection));
	reverse_map[dst_node][num_dst_fanins_after_new_connection-1] = src_node;

	// Debug
	/*
	printf("after creating edge from %d to %d\n", src_node, dst_node);
	print_fanins_and_fanouts_of_rr_node(src_node);
	print_fanins_and_fanouts_of_rr_node(dst_node);
	*/

	return true;
}

/*
 * Description: For a given src and dst node that are connected, it will return the switch index of the connection
 *              If connection does not exist, it returns -1
 *
 * Returns: Nothing.
 */
int get_connection_switch_index(int src, int dst)
{
	int iswitch = -1;
	int cnt = 0;
	for(cnt=0; cnt<rr_node[src].num_edges && rr_node[src].edges[cnt]!=dst; ++cnt);

	if(cnt < rr_node[src].num_edges)
	{
		iswitch = rr_node[src].switches[cnt];
	}

	return iswitch;
}

/*
 * Description: For a given rr_node, it tells you how many interposer wires it drives
 * 
 * Note: This function relies on the fact that interposer wires' ptc_num >= nodes_per_chan
 *
 * Returns: Nothing.
 */
int get_num_interposer_loads(int inode, int nodes_per_chan)
{
	int i, idst, num_interposer_loads = 0;
	
	for(i=0; i<rr_node[inode].num_edges; ++i)
	{
		idst = rr_node[inode].edges[i];
		if(rr_node[idst].ptc_num >= nodes_per_chan)
		{
			num_interposer_loads++;
		}
	}

	return num_interposer_loads;
}

/*
 * Description: returns the number of interposer wires that can drive this inode
 * 
 * Note: This function relies on the fact that interposer wires' ptc_num >= nodes_per_chan
 *
 * Returns: Nothing.
 */
int get_num_interposer_drivers(int inode, int nodes_per_chan)
{
	int i, isrc, num_drivers = 0;
	
	for(i=0; i<rr_node[inode].fan_in; ++i)
	{
		isrc = reverse_map[inode][i];
		if(rr_node[isrc].ptc_num >= nodes_per_chan)
		{
			num_drivers++;
		}
	}

	return num_drivers;
}

/*
 * Description: this function breaks wires that cross the interposer into 2 rr_nodes
 *              it will also add 1 interpoer node between every connection whose src and dst
 *              are on 2 sides of a cutline.
 *              most of the heavy lifting is inside this function.
 *              several legality checks are built into the function to ensure correctness of assumptions.
 *
 * Returns: Nothing.
 */
void expand_rr_graph(int* rr_nodes_that_cross, int num_rr_nodes_that_cross, int nodes_per_chan)
{
	// note to self: also see: 
	// rr_node_to_rt_node
	// rr_node_route_inf

	int inode;
	int i, j, k, cnt, interposer_node_counter;

	// this is where we need to add a switch with EXTRA delay (this is the one that crosses the interposer)
	// EHSAN: this is not right. i need to find out the switch index for a correct CHANY_to_CHANY connection
	int correct_index_of_CHANY_to_CHANY_switch = 0;
	int zero_delay_switch_index = 4;


	// rr_node used to be rr_node[0..num_rr_nodes-1]
	// now, it will be bigger: rr_node[0.. num_rr_nodes+num_rr_nodes_that_cross-1]
	// the indeces of the newly created nodes will be: num_rr_nodes .. num_rr_nodes+num_rr_nodes_that_cross-1
	// we also decided to add nodes for the interposer. So, we are adding 1 extra node per vertical track (CHANY) at the cut locations
	// The indeces of the interposer nodes will be [num_rr_nodes+num_rr_nodes_that_cross .. num_rr_nodes+num_rr_nodes_that_cross+num_interposer_nodes-1]
	int num_vertical_channels = nx+1;
	int num_interposer_nodes = num_cuts * num_vertical_channels * nodes_per_chan;

	// expand
	rr_node = (t_rr_node *)my_realloc(rr_node, sizeof(t_rr_node)*(num_rr_nodes+num_rr_nodes_that_cross+num_interposer_nodes));
	//rr_node = (t_rr_node *)my_realloc(rr_node, sizeof(t_rr_node)*(num_rr_nodes+num_rr_nodes_that_cross));
	// initialize the new nodes to some initial state
	
	for(inode=num_rr_nodes; inode<num_rr_nodes+num_rr_nodes_that_cross+num_interposer_nodes; ++inode)
	{
		rr_node[inode].xlow = -1;
		rr_node[inode].xhigh = -1;
		rr_node[inode].ylow = -1;
		rr_node[inode].yhigh = -1;
		rr_node[inode].z=-1;
		rr_node[inode].ptc_num = -1;
		rr_node[inode].cost_index = -1;
		rr_node[inode].occ = -1;
		rr_node[inode].capacity = -1;
		
		// it's important to make fan_in=0 so that the reverse map doesn't freak out
		rr_node[inode].fan_in = 0;
		rr_node[inode].num_edges = 0;
		rr_node[inode].edges=0;
		rr_node[inode].switches=0;

		rr_node[inode].R=0;
		rr_node[inode].C=0;
		rr_node[inode].num_wire_drivers=0;
		rr_node[inode].num_opin_drivers=0;
		rr_node[inode].prev_node=0;
		rr_node[inode].prev_edge=0;
		rr_node[inode].net_num=0;
		rr_node[inode].pb_graph_pin=0;
		rr_node[inode].tnode=0;
		rr_node[inode].pack_intrinsic_cost=0.0;
	}

	alloc_and_build_reverse_map(num_rr_nodes+num_rr_nodes_that_cross+num_interposer_nodes);

	// for any wire that crosses the interposer, cut into 2 wires
	// 1 wire below the cut, and 1 wire above the cut
	for(i=0; i<num_rr_nodes_that_cross; ++i)
	{
		int original_node_index = rr_nodes_that_cross[i];
		int new_node_index = num_rr_nodes+i;

		t_rr_node* original_node = &rr_node[original_node_index];
		t_rr_node* new_node = &rr_node[new_node_index];

		// find which cut goes through the original node
		int cut_counter = 0, cut_pos = 0;
		for(cut_counter = 0; cut_counter < num_cuts; cut_counter++)
		{
			cut_pos = arch_cut_locations[cut_counter];
			if( original_node->ylow <= cut_pos && cut_pos < original_node->yhigh )
			{
				break;
			}
		}

		// remember the length of the original_node before it is cut to 2 pieces.
		int original_wire_len_before_cutting = original_node->yhigh - original_node->ylow + 1;

		// the y-coordinates should be fixed
		if(original_node->direction == INC_DIRECTION)
		{
			new_node->yhigh = original_node->yhigh;
			new_node->ylow = cut_pos+1;
			original_node->yhigh = cut_pos;
			// don't need to change original_node->ylow	
		}
		else if(original_node->direction == DEC_DIRECTION)
		{
			new_node->ylow = original_node->ylow;
			new_node->yhigh = cut_pos;
			original_node->ylow = cut_pos+1;
			// don't need to change original_node->yhigh
		}

		// the following attributes of the new node should be the same as the original node
		new_node->xlow = original_node->xlow;
		new_node->xhigh = original_node->xhigh;
		new_node->ptc_num = original_node->ptc_num;
		
		new_node->cost_index = original_node->cost_index;
		new_node->occ = original_node->occ;
		new_node->capacity = original_node->capacity;

		assert(original_node->type==CHANY);
		new_node->type = original_node->type;

		// Figure out how to distribute the R and C between the two wires
		int original_wire_len_after_cutting = original_node->yhigh - original_node->ylow + 1;
		int new_wire_len = new_node->yhigh - new_node->ylow + 1;
		assert(original_wire_len_before_cutting == original_wire_len_after_cutting+new_wire_len);
		new_node->R = original_node->R;
		new_node->C = ( (float)(new_wire_len) / (float)(original_wire_len_before_cutting) ) * original_node->C;
		original_node->C = ( (float)(original_wire_len_after_cutting) / (float)(original_wire_len_before_cutting) ) * original_node->C;

		new_node->direction = original_node->direction;
		new_node->drivers = original_node->drivers;
		new_node->num_wire_drivers = original_node->num_wire_drivers;
		new_node->num_opin_drivers = original_node->num_opin_drivers;

		// ######## Update fanouts of the original_node and new_node
		int num_org_node_fanouts_before_transformations = original_node->num_edges;
		int num_org_node_fanins_before_transformations = original_node->fan_in;

		for(cnt=0; cnt<original_node->num_edges; ++cnt)
		{
			int dnode = original_node->edges[cnt];
			short iswitch = original_node->switches[cnt];

			if( (original_node->direction==INC_DIRECTION && rr_node[dnode].ylow > cut_pos) ||
				(original_node->direction==DEC_DIRECTION && rr_node[dnode].ylow <= cut_pos))
			{
				if(original_node->direction==DEC_DIRECTION && rr_node[dnode].ylow == cut_pos && rr_node[dnode].type==CHANX)
				{
				}
				else
				{
					// this should be removed from fanout set of original_node 
					// and should be added to fanouts of the new_node
					create_rr_connection(new_node_index,dnode, iswitch);
					delete_rr_connection(original_node_index,dnode);
					cnt--;
				}
			}
		}

		// ######## Hook up original_node to new_node
		create_rr_connection(original_node_index,new_node_index,correct_index_of_CHANY_to_CHANY_switch);

		// ######## Update fanins of the original_node and new_node
		for(cnt=0; cnt < original_node->fan_in; ++cnt)
		{
			// for every fan-in of the current original_node

			int fanin_node_index = reverse_map[original_node_index][cnt];
			t_rr_node* fanin_node = &rr_node[fanin_node_index];

			if(	(original_node->direction==INC_DIRECTION && fanin_node->yhigh > cut_pos) ||
				(original_node->direction==DEC_DIRECTION && fanin_node->ylow  <= cut_pos ) ||
				(original_node->direction==INC_DIRECTION && fanin_node->yhigh == cut_pos && fanin_node->type == CHANX)
				)
			{
				// fanin_node should be removed from original_node fanin set
				// and should now feed the new_node instead of the original_node
				// use the same switch for the new connection
				int ifan;
				for(ifan=0; ifan<fanin_node->num_edges && fanin_node->edges[ifan]!=original_node_index; ++ifan);
				create_rr_connection(fanin_node_index,new_node_index, fanin_node->switches[ifan]);
				delete_rr_connection(fanin_node_index,original_node_index);
				cnt--;
			}
		}

		int num_org_node_fanouts_after_transformations = original_node->num_edges;
		int num_org_node_fanins_after_transformations = original_node->fan_in;
		int num_new_node_fanouts_after_transformations = new_node->num_edges;
		int num_new_node_fanins_after_transformations = new_node->fan_in;

		// +2 is because of 1 fanout and 1 fanin added by connecting original_node to new_node
		assert( (num_org_node_fanouts_before_transformations + num_org_node_fanins_before_transformations + 2) ==
				(num_org_node_fanouts_after_transformations + num_org_node_fanins_after_transformations +
				num_new_node_fanouts_after_transformations + num_new_node_fanins_after_transformations)
				);

		// The rest of the member variables are not used yet (they are 0 in the original_node)
		new_node->prev_node=0;
		new_node->prev_edge=0;
		new_node->net_num=0;
		new_node->pb_graph_pin=0;
		new_node->tnode=0;
		new_node->pack_intrinsic_cost=0.0;
	}

	// update the num_rr_nodes so far
	num_rr_nodes += num_rr_nodes_that_cross;

	//############# Begin: Legality Check ##################################################################
	for(inode=0; inode<num_rr_nodes; ++inode)
	{
		if(rr_node[inode].type==CHANY)
		{
			int cut_counter, cut_pos;
			for(cut_counter=0; cut_counter < num_cuts; cut_counter++)
			{
				cut_pos = arch_cut_locations[cut_counter];
				if(rr_node[inode].ylow <= cut_pos && rr_node[inode].yhigh > cut_pos)
				{
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
						"in expand_rr_graph: rr_node %d crosses the cut at y=%d\n", inode, cut_pos);
				}

				for(i=0; i<rr_node[inode].num_edges; ++i)
				{
					int dnode = rr_node[inode].edges[i];
					
					if(	rr_node[dnode].type==SINK   || 
						rr_node[dnode].type==SOURCE || 
						rr_node[dnode].type==IPIN   || 
						rr_node[dnode].type==OPIN)
					{
						if( (rr_node[inode].direction==INC_DIRECTION && rr_node[inode].yhigh <= cut_pos && rr_node[dnode].ylow > cut_pos) ||
							(rr_node[inode].direction==DEC_DIRECTION && rr_node[inode].ylow > cut_pos && rr_node[dnode].yhigh <= cut_pos))
						{
							vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
								"in expand_rr_graph: rr_node %d tries to connect to a pin on the other side of the cut at y=%d\n", inode, cut_pos);
						}
					}
				}
			}
		}
	}
	//############# End: Legality Check   ##################################################################
	

	//############# BEGIN: Add interposer nodes   ##########################################################
	// allocate stuff
	// interposer_nodes[x][y][pct]
	interposer_node_loc = (int***) my_malloc(num_vertical_channels * sizeof(int**));
	for(i=0; i<num_vertical_channels; ++i)
	{
		interposer_node_loc[i] = (int**)my_malloc(num_cuts * sizeof(int*));
	}
	for(i=0; i<num_vertical_channels; ++i)
	{
		for(j=0; j<num_cuts; ++j)
		{
			interposer_node_loc[i][j] = (int*)my_malloc(nodes_per_chan * sizeof(int));
		}
	}
	for(i=0; i<num_vertical_channels; ++i)
	{
		for(j=0; j<num_cuts; ++j)
		{
			for(k=0; k<nodes_per_chan; ++k)
			{
				interposer_node_loc[i][j][k] = -1;
			}
		}
	}

	// allocate stuff
	interposer_nodes = (int*)my_malloc(num_interposer_nodes * sizeof(int));

	// remember that interposer node IDs will be at 
	// [num_rr_nodes+num_rr_nodes_that_cross .. num_rr_nodes+num_rr_nodes_that_cross+num_interposer_nodes-1]
	// but since we already did num_rr_nodes += num_rr_nodes_that_cross, so the indeces will be at:
	// [num_rr_nodes .. num_rr_nodes+num_interposer_nodes-1]
	interposer_node_counter = 0;
	int interposer_node_id  = num_rr_nodes;
	for(inode=0; inode<num_rr_nodes; ++inode)
	{
		t_rr_node* node = &rr_node[inode];
		int cut_counter = 0;
		for(cut_counter = 0; cut_counter < num_cuts; cut_counter++)
		{
			int cut_pos = arch_cut_locations[cut_counter];

			if(node->type==CHANY && (node->ylow==cut_pos || node->yhigh==cut_pos))
			{
				if(node->direction==INC_DIRECTION)
				{
					// by this point, we should not have any wires that cross a cut
					// so, the yhigh should at most be cut_pos
					assert(node->yhigh==cut_pos);
					assert(node->xlow==node->xhigh);

					rr_node[interposer_node_id].xlow=node->xlow;
					rr_node[interposer_node_id].xhigh=node->xhigh;
					rr_node[interposer_node_id].ylow=cut_pos;
					rr_node[interposer_node_id].yhigh=cut_pos;
					rr_node[interposer_node_id].z=node->z;
					rr_node[interposer_node_id].ptc_num=node->ptc_num+nodes_per_chan;
					//rr_node[interposer_node_id].ptc_num=node->ptc_num;

					rr_node[interposer_node_id].cost_index=node->cost_index;
					rr_node[interposer_node_id].occ=0;
					rr_node[interposer_node_id].capacity=1;
					rr_node[interposer_node_id].type=CHANY;
					rr_node[interposer_node_id].direction=INC_DIRECTION;
					rr_node[interposer_node_id].R=0;
					rr_node[interposer_node_id].C=0;

					// SINGLE or MULTI_BUFFERED?
					rr_node[interposer_node_id].drivers = node->drivers;

					assert(node->num_wire_drivers==0);
					assert(node->num_opin_drivers==0);
					rr_node[interposer_node_id].num_wire_drivers = 0;
					rr_node[interposer_node_id].num_opin_drivers = 0;
					rr_node[interposer_node_id].prev_node=0;
					rr_node[interposer_node_id].prev_edge=0;
					rr_node[interposer_node_id].net_num=0;
					rr_node[interposer_node_id].pb_graph_pin=0;
					rr_node[interposer_node_id].tnode=0;
					rr_node[interposer_node_id].pack_intrinsic_cost=0.0;

					create_rr_connection(inode, interposer_node_id, correct_index_of_CHANY_to_CHANY_switch);

					// all the fanouts of 'node' that are on the other side of the cut,
					// should be transfered to the interposer_node
					for(i=0; i<node->num_edges; ++i)
					{
						int ifanout = node->edges[i];
						//int iswitch = node->switches[i];

						if(rr_node[ifanout].ylow > cut_pos)
						{
							// transfer the fanout
							//create_rr_connection(interposer_node_id,ifanout, iswitch);
							create_rr_connection(interposer_node_id,ifanout, zero_delay_switch_index);
							delete_rr_connection(inode, ifanout);
							--i;
						}
					}
				}
				else if(node->direction==DEC_DIRECTION)
				{
					// by this point, we should not have any wires that cross a cut
					// so, the yhigh should at most be cut_pos
					assert(node->yhigh==cut_pos);
					assert(node->xlow==node->xhigh);

					rr_node[interposer_node_id].xlow=node->xlow;
					rr_node[interposer_node_id].xhigh=node->xhigh;
					rr_node[interposer_node_id].ylow=cut_pos;
					rr_node[interposer_node_id].yhigh=cut_pos;
					rr_node[interposer_node_id].z=node->z;
					rr_node[interposer_node_id].ptc_num=node->ptc_num+nodes_per_chan;
					//rr_node[interposer_node_id].ptc_num=node->ptc_num;

					rr_node[interposer_node_id].cost_index=node->cost_index;
					rr_node[interposer_node_id].occ=0;
					rr_node[interposer_node_id].capacity=1;
					rr_node[interposer_node_id].type=CHANY;
					rr_node[interposer_node_id].direction=DEC_DIRECTION;
					rr_node[interposer_node_id].R=0;
					rr_node[interposer_node_id].C=0;

					// SINGLE or MULTI_BUFFERED?
					rr_node[interposer_node_id].drivers = node->drivers;

					assert(node->num_wire_drivers==0);
					assert(node->num_opin_drivers==0);
					rr_node[interposer_node_id].num_wire_drivers = 0;
					rr_node[interposer_node_id].num_opin_drivers = 0;
					rr_node[interposer_node_id].prev_node=0;
					rr_node[interposer_node_id].prev_edge=0;
					rr_node[interposer_node_id].net_num=0;
					rr_node[interposer_node_id].pb_graph_pin=0;
					rr_node[interposer_node_id].tnode=0;
					rr_node[interposer_node_id].pack_intrinsic_cost=0.0;

					//create_rr_connection(interposer_node_id, inode, correct_index_of_CHANY_to_CHANY_switch);
					create_rr_connection(interposer_node_id, inode, zero_delay_switch_index);

					// all the fanouts of 'node' that are on the other side of the cut,
					// should be transfered to the interposer_node
					for(i=0; i<node->fan_in; ++i)
					{
						int ifanin = reverse_map[inode][i];
						for (cnt=0; cnt<rr_node[ifanin].num_edges && rr_node[ifanin].edges[cnt]!=inode; ++cnt);
						int iswitch = rr_node[ifanin].switches[cnt];

						if(rr_node[ifanin].ylow > cut_pos)
						{
							// transfer the fanin
							create_rr_connection(ifanin, interposer_node_id, iswitch);
							delete_rr_connection(ifanin, inode);
							--i;
						}
					}
				}
				else
				{
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
						"rr_graph modifications for interposer based architectures currently supports unidirectional wires!\n");
				}
				interposer_nodes[interposer_node_counter] = interposer_node_id;
				interposer_node_loc[node->xlow][cut_counter][node->ptc_num] = interposer_node_id;
				interposer_node_id++;
				interposer_node_counter++;
			}
		}
	}
	assert(interposer_node_counter==num_interposer_nodes);
	assert(interposer_node_id == num_rr_nodes+num_interposer_nodes);

	for(inode=0; inode<num_rr_nodes; ++inode)
	{
		t_rr_node* node = &rr_node[inode];
		int cut_counter = 0;
		for(cut_counter = 0; cut_counter < num_cuts; cut_counter++)
		{
			int cut_pos = arch_cut_locations[cut_counter];
			if(node->type==CHANX && node->ylow==cut_pos)
			{
				assert(node->ylow==node->yhigh);  // because it's CHANX
				
				// go over all of its fanouts (it may drive a CHANY wire on the other side of the cut)
				for(i=0; i<node->num_edges; ++i)
				{
					int ifanout = node->edges[i];
					int iswitch = node->switches[i];
					t_rr_node* fanout = &rr_node[ifanout];

					if(fanout->ylow > cut_pos)
					{
						if( fanout->type==IPIN || fanout->type==OPIN ||
							fanout->type==SINK || fanout->type==SOURCE)
						{
							vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
								"Found CHANX wire below the cut that connects to a pin above the cut!\n");
						}
						else if(fanout->type==CHANX)
						{
							vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
								"Found CHANX wire below the cut that connects to a CHANX wire above the cut!\n");
						}
						else if(fanout->type==CHANY)
						{
							// transfer the fanout
							// first find out which interposer_node you want to connect to
							interposer_node_id = interposer_node_loc[fanout->xlow][cut_counter][fanout->ptc_num];
							create_rr_connection(inode, interposer_node_id, iswitch);
							create_rr_connection(interposer_node_id,ifanout, zero_delay_switch_index );
							delete_rr_connection(inode, ifanout);
							--i;
						}
					}
				}

				// go over all of its fanins (it may be driven by a CHANY wire above the cut)
				for(i=0; i<node->fan_in; ++i)
				{
					int ifanin = reverse_map[inode][i];
					t_rr_node* fanin = &rr_node[ifanin];
					for (cnt=0; cnt < fanin->num_edges && fanin->edges[cnt]!=inode; ++cnt);
					int iswitch = fanin->switches[cnt];

					if(fanin->ylow > cut_pos)
					{
						if( fanin->type==IPIN || fanin->type==OPIN ||
							fanin->type==SINK || fanin->type==SOURCE)
						{
							vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
								"Found CHANX wire below the cut that is driven by a pin above the cut!\n");
						}
						else if(fanin->type==CHANX)
						{
							vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
								"Found CHANX wire below the cut that is driven by a CHANX wire above the cut!\n");
						}
						else if(fanin->type==CHANY)
						{
							// transfer the fanout
							interposer_node_id = interposer_node_loc[fanin->xlow][cut_counter][fanin->ptc_num];
							create_rr_connection(ifanin, interposer_node_id, iswitch);
							create_rr_connection(interposer_node_id, inode, zero_delay_switch_index );
							delete_rr_connection(ifanin, inode);
							--i;
						}
					}
				}	
			}
		}
	}

	num_rr_nodes += num_interposer_nodes;

	//############# END: Add interposer nodes   ##########################################################


	//############# BEGIN: LEGALITY CHECK       ##########################################################
	// At this point, there should be no connection that crosses the interposer, UNLESS it goes through
	// an interposer node
	t_rr_node *node, *fanout_node; 
	for(inode=0; inode<num_rr_nodes;++inode)
	{
		int ifanout, cut_counter, cut_pos, node_to_check;
		bool crossing_using_interposer_node = false;
		node = &rr_node[inode];
		for(ifanout=0; ifanout < node->num_edges; ++ifanout)
		{
			fanout_node = &rr_node[node->edges[ifanout]];
			for(cut_counter=0; cut_counter<num_cuts; ++cut_counter)
			{
				cut_pos = arch_cut_locations[cut_counter];
				node_to_check = -1;
				crossing_using_interposer_node = false;

				if(node->yhigh <= cut_pos && fanout_node->ylow > cut_pos)
				{
					// this is a connection that crosses the interposer
					// make sure 'node' is an interposer node
					node_to_check = inode;
				}
				else if(node->ylow > cut_pos && fanout_node->yhigh <= cut_pos)
				{
					// this is a connection that crosses the interposer
					// make sure 'fanout_node' is an interposer node
					node_to_check = node->edges[ifanout];
				}

				if(node_to_check!=-1)
				{
					// make sure node_to_check is an interposer node
					for(i=0; i<num_interposer_nodes; ++i)
					{
						if(interposer_nodes[i]==node_to_check)
						{
							crossing_using_interposer_node = true;
							break;
						}
					}

					if(!crossing_using_interposer_node)
					{
						vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
								"Found a connection that crosses a cut without going through an interposer node!\n");
					}
				}
			}
		}
	}
	//############# END:   LEGALITY CHECK     ##########################################################
	
	//############# BEGIN: LEGALITY CHECK     ##########################################################
	// for every interposer node,
	// if INC: all fanin nodes should be below the cut, and all fanout nodes should be above the cut
	// if DEC: all fanin nodes should be above the cut, and all fanout nodes should be below the cut
	for(i=0; i<num_interposer_nodes; ++i)
	{
		int interposer_node_index = interposer_nodes[i];

		// all fanouts
		for(j=0; j< rr_node[interposer_node_index].num_edges; ++j)
		{
			int cut_pos = rr_node[interposer_node_index].ylow;
			int fanout_node_index = rr_node[interposer_node_index].edges[j];
			if(rr_node[interposer_node_index].direction==INC_DIRECTION)
			{
				assert(rr_node[fanout_node_index].ylow > cut_pos);
			}
			else if(rr_node[interposer_node_index].direction==DEC_DIRECTION)
			{
				assert(rr_node[fanout_node_index].yhigh <= cut_pos);
			}
		}

		// all fanins
		for(j=0; j< rr_node[interposer_node_index].fan_in; ++j)
		{
			int cut_pos = rr_node[interposer_node_index].ylow;
			int fanin_node_index = reverse_map[interposer_node_index][j];
			if(rr_node[interposer_node_index].direction==INC_DIRECTION)
			{
				assert(rr_node[fanin_node_index].yhigh <= cut_pos);
			}
			else if(rr_node[interposer_node_index].direction==DEC_DIRECTION)
			{
				assert(rr_node[fanin_node_index].ylow > cut_pos);
			}
		}
	}
	//############# END:   LEGALITY CHECK     ##########################################################

	//############# BEGIN: LEGALITY CHECK     ##########################################################
	// for every interposer node,
	// if INC: all fanin nodes should be below the cut && the fanin Y_HIGH should be at the cut
	for(i=0; i<num_interposer_nodes; ++i)
	{
		int interposer_node_index = interposer_nodes[i];
		if(rr_node[interposer_node_index].direction == INC_DIRECTION)
		{
			// all fanins
			for(j=0; j< rr_node[interposer_node_index].fan_in; ++j)
			{
				int cut_pos = rr_node[interposer_node_index].ylow;
				int fanin_node_index = reverse_map[interposer_node_index][j];
				assert(rr_node[fanin_node_index].yhigh == cut_pos);				
			}
		}
		if(rr_node[interposer_node_index].direction == DEC_DIRECTION)
		{
			// all fanouts
			for(j=0; j< rr_node[interposer_node_index].num_edges; ++j)
			{
				int cut_pos = rr_node[interposer_node_index].ylow;
				int fanout_node_index = rr_node[interposer_node_index].edges[j];
				assert(rr_node[fanout_node_index].yhigh == cut_pos);
			}
		}
	}
	//############# END:   LEGALITY CHECK     ##########################################################
}










//#######################################################################################################
//#######################################################################################################
//
// Below is all the code related to dealing with interposer architectures by modifying switch delays
//
// Authors: Andre Hahn Pereira 
// + some bug fixes by: Ehsan Nasiri
//
//#######################################################################################################
//#######################################################################################################

#ifdef USE_SIMPLER_SWITCH_MODIFICATION_METHODOLOGY

/*
 * Description: This function checks whether or not a specific connection crosses the interposer.
 * 
 * Returns: True if connection crosses the interposer. False otherwise.
 */
bool rr_edge_crosses_interposer(int src, int dst, int cut_location )
{
	// SRC node is always a vertical wire.
	// DST node can be either a horizontal or vertical wire.
	// 
	// SRC: Y_INC, Y_DEC,
	// DST: Y_INC, Y_DEC, X

	// here's trick to make my life easier.
	float cut = cut_location + 0.5;

	// start and end coordinates of src and dst dones
	// for horizontal nodes, ylow and yhigh are equal
	int src_ylow  = rr_node[src].ylow;
	int src_yhigh = rr_node[src].yhigh;
	int dst_ylow  = rr_node[dst].ylow;
	int dst_yhigh = rr_node[dst].yhigh;

	bool crosses_the_interposer = false;

	if( (rr_node[src].type==CHANY) &&
		(rr_node[dst].type==CHANX || rr_node[dst].type==CHANY))
	{
		// Case 1: SRC NODE IS VERTICAL AND INCREASING
		if(rr_node[src].direction==INC_DIRECTION)
		{
			if(rr_node[dst].type==CHANY && rr_node[dst].direction==INC_DIRECTION)
			{
				if(	(src_ylow < cut && cut < src_yhigh) ||
					(src_yhigh< cut && cut < dst_ylow))
				{
					crosses_the_interposer=true;
				}
			}
			else if(rr_node[dst].type==CHANY && rr_node[dst].direction==DEC_DIRECTION)
			{
				// this should never happen! (U-turn in vertical direction!
				vpr_printf_info("SRC= Y INC && DST= Y DEC\n");
				assert(false);
			}
			else if(rr_node[dst].type==CHANX)
			{
				assert(dst_ylow==dst_yhigh);
				assert(dst_ylow>=src_ylow);
				if( (src_ylow < cut && cut < src_yhigh && cut < dst_ylow) ||
					(src_yhigh < cut && cut < dst_ylow))
				{
					crosses_the_interposer=true;
				}
			}
		}
		// Case 2: SRC NODE IS VERTICAL AND DECREASING
		else if(rr_node[src].direction==DEC_DIRECTION)
		{
			if(rr_node[dst].type==CHANY && rr_node[dst].direction==INC_DIRECTION)
			{
				// this should never happen! (U-turn in vertical direction!
				vpr_printf_info("SRC= Y DEC && DST= Y INC\n");
				assert(false);
			}
			else if(rr_node[dst].type==CHANY && rr_node[dst].direction==DEC_DIRECTION)
			{
				if(	(src_ylow < cut && cut < src_yhigh) ||
					(dst_yhigh< cut && cut < src_ylow))
				{
					crosses_the_interposer=true;
				}
			}
			else if(rr_node[dst].type==CHANX)
			{
				assert(dst_ylow==dst_yhigh);
				assert(dst_ylow<=src_yhigh);
				if( (src_ylow < cut && cut < src_yhigh && dst_ylow < cut) ||
					(dst_yhigh < cut && cut < src_ylow))
				{
					crosses_the_interposer=true;
				}
			}
		}
	}

	if( (rr_node[src].type==CHANY) &&
		(rr_node[dst].type==IPIN || rr_node[dst].type==OPIN))
	{
		// OK, so sometimes we have a pin that looks like this
		// (IPIN,xlow=22,xhigh=22,ylow=13,yhigh=16,DEC)
		
		// Case 1: SRC NODE IS VERTICAL AND INCREASING
		if(rr_node[src].direction==INC_DIRECTION)
		{
			if( (src_ylow < cut && cut < src_yhigh && cut < dst_ylow) ||
				(src_yhigh < cut && cut < dst_ylow))
			{
				crosses_the_interposer=true;
			}			
		}
		// Case 2: SRC NODE IS VERTICAL AND DECREASING
		else if(rr_node[src].direction==DEC_DIRECTION)
		{
			if( (src_ylow < cut && cut < src_yhigh && dst_yhigh < cut) ||
				(dst_yhigh < cut && cut < src_ylow))
			{
				crosses_the_interposer=true;
			}
		}
	}

	return crosses_the_interposer;
}


/*
 * Description: This function cuts the edges which cross the cut for a given wire in the CHANY
 * 
 * Returns: None.
 */
void cut_rr_yedges(INP int cut_location, INP int inode)
{
	
	if(cut_node_set.find(inode)==cut_node_set.end())
	{
		cut_node_set.insert(inode);
	}

	int iedge, d_node, num_removed;
	int tmp;

	num_removed = 0;

	/* mark and remove the edges */
	for(iedge = 0; iedge < rr_node[inode].num_edges; iedge++)
	{
		d_node = rr_node[inode].edges[iedge];
		if(d_node == -1)
		{
			continue;
		}

		/* crosses the cut line, cut this edge */
		if(rr_edge_crosses_interposer(inode,d_node,cut_location))
		{
			rr_node[d_node].fan_in--;
			num_removed++;
			for(tmp = iedge; tmp+1 < rr_node[inode].num_edges; tmp++)
			{
				rr_node[inode].edges[tmp] = rr_node[inode].edges[tmp+1];
				rr_node[inode].switches[tmp] = rr_node[inode].switches[tmp+1];
			}
			rr_node[inode].edges[tmp] = -1; /* tmp = num_edges-1 */
			rr_node[inode].switches[tmp] = -1;

			iedge--; /* need to check the value just pulled into current pos */
		}
		else
		{
			/*printf(">>>> Did not cut this edge because it does not cross the boundary <<<<\n");*/
		}
	}

	/* fill the rest of the array with -1 for safety */
	for(iedge = rr_node[inode].num_edges - num_removed; iedge < rr_node[inode].num_edges; iedge++)
	{
		rr_node[inode].edges[iedge] = -1;
		rr_node[inode].switches[iedge] = -1;
	}
	rr_node[inode].num_edges -= num_removed;
	/* finished removing the edges */
}


/*
 * Description: This function cuts the edges which cross the cut for a given wire in the CHANX
 *
 * Returns: None.
 */
void cut_rr_xedges(int cut_location, int inode)
{
	
	int iedge, d_node, num_removed;
	int tmp;

	num_removed = 0;

	/* mark and remove the edges */
	for(iedge = 0; iedge < rr_node[inode].num_edges; iedge++)
	{
		d_node = rr_node[inode].edges[iedge];
		if(d_node == -1)
		{
			continue;
		}

		/* crosses the cut line, cut this edge, CHANX is always supposed to be
		 * below the cutline */
		if(	rr_node[d_node].ylow > cut_location && rr_node[d_node].type == CHANY)
		{
			rr_node[d_node].fan_in--;
			num_removed++;
			for(tmp = iedge; tmp+1 < rr_node[inode].num_edges; tmp++)
			{
				rr_node[inode].edges[tmp] = rr_node[inode].edges[tmp+1];
				rr_node[inode].switches[tmp] = rr_node[inode].switches[tmp+1];
			}
			rr_node[inode].edges[tmp] = -1; /* tmp = num_edges-1 */
			rr_node[inode].switches[tmp] = -1;

			iedge--; /* need to check the value just pulled into current pos */
		}
		else
		{
			/*printf(">>>> Did not cut this edge because it does not cross the boundary <<<<\n");*/
		}
	}

	/* fill the rest of the array with -1 for safety */
	for(iedge = rr_node[inode].num_edges - num_removed; iedge < rr_node[inode].num_edges; iedge++)
	{
		rr_node[inode].edges[iedge] = -1;
		rr_node[inode].switches[iedge] = -1;
	}
	rr_node[inode].num_edges -= num_removed;
	/* finished removing the edges */
}


void cut_connections_from_CHANY_wires
(
	int nodes_per_chan,
	int num_wires_cut,
	int cut_pos, 
	int i
)
{
	int itrack, inode, step, num_wires_cut_so_far, offset, num_chunks;

	if(num_wires_cut == 0)
	{	// nothing to do :)
		return;
	}

	offset = (i * nodes_per_chan) / nx;
	if(offset % 2) 
	{
		offset++;
	}
	offset = offset%nodes_per_chan; // to keep offset between 0 and nodes_per_chan-1

	if(num_wires_cut > 0)
	{
		// Example: if the step is 1.66, make the step 2.
		step = ceil (float(nodes_per_chan) / float(num_wires_cut));
	}
	else
	{
		step = 900900;
	}

	// cutting chunks of wires. each chunk has 2 wires (a pair)
	num_chunks = num_wires_cut/2;
	step = nodes_per_chan/num_chunks;

	if(step<=2)
	{
		// it can be proven that if %wires_cut>66%, then step=2.
		// step=2 means that there will be no gap between pairs of wires that will be cut
		// therefore, the cut pattern becomes a 'chunk cut'.
		// to avoid that, we will cap the number of chunks.
		// we require step to be greater than or equal to 3.
		step = 3;
		num_chunks = nodes_per_chan / 3;
	}

	int ichunk = 0;
	for(itrack=offset, num_wires_cut_so_far=0 ; num_wires_cut_so_far<num_wires_cut; itrack=(itrack+1)%nodes_per_chan)
	{
		for(ichunk=0; ichunk<num_chunks && num_wires_cut_so_far<num_wires_cut; ichunk++)
		{
			//printf("i=%d, j=%d, track=%d \n", i, cut_pos, itrack+(step*ichunk));
			inode = get_rr_node_index(i, cut_pos, CHANY, (itrack+(step*ichunk))%nodes_per_chan, rr_node_indices);
			cut_rr_yedges(cut_pos, inode);
			num_wires_cut_so_far++;
		}
	}
	
	// To make sure that we haven't cut the same wire twice
	assert(cut_node_set.size()==(size_t)num_wires_cut);
	cut_node_set.clear();
}

/*
 * Description: This is where cutting happens!
 * CHANX to CHANY connections will be cut if CHANX wire is below the cut and the CHANY wire is above the interposer
 *
 * Returns: None.
 */
void cut_connections_from_CHANX_wires(int i, int cut_pos, int nodes_per_chan)
{
	int itrack, inode;

	if(cut_pos + 1 >= ny)
	{
		return;
	}

	// From CHANX to CHANY, cut only the edges at the switches
	if(0 < i && i < nx)
	{
		for(itrack = 0; itrack < nodes_per_chan; itrack++)
		{
			inode = get_rr_node_index(i, cut_pos, CHANX, itrack, rr_node_indices);

			// printf("Going to cut Horizontal (X) edges from: i=%d, j=%d, track=%d \n", i, cut_pos, itrack);
			cut_rr_xedges(cut_pos, inode);
		}
	}
}


/*
 * Description: Takes care of cutting horizontal and vertical connections at the cut
 *
 * Returns: None.
 */
void cut_rr_graph_edges_at_cut_locations(int nodes_per_chan)
{
	int cut_step; // The interval at which the cuts should be made
	int counter;  // Number of cuts already made
	int i, j;     // horizontal and vertical coordinate numbers
	int num_wires_cut;

	// Find the number of wires that should be cut at each horizontal cut
	num_wires_cut = (nodes_per_chan * percent_wires_cut) / 100;
	assert(percent_wires_cut==0 || num_wires_cut <= nodes_per_chan);

	// num_wires_cut should be an even number
	if(num_wires_cut % 2)
	{
			num_wires_cut++;
	}

	printf("Info: cutting %d wires when channel width is %d\n", num_wires_cut, nodes_per_chan);

	counter = 0;
	cut_step = ny / (num_cuts + 1);
	for(j=cut_step; j<ny && counter<num_cuts; j+=cut_step, counter++)
	{
		for(i = 0; i <= nx; i++)
		{
			// 1. cut num_wires_cut wires at (x,y)=(i,j).
			cut_connections_from_CHANY_wires(nodes_per_chan, num_wires_cut, j, i);

			// 2. cut all CHANX wires connecting to CHANY wires on the other side of the interposer
			cut_connections_from_CHANX_wires(i, j, nodes_per_chan);
		}
	}
	assert(counter == num_cuts);
}


/*
 * Description: This function traverses the whole rr_graph
 * For every connection that crosses the interposer, it increases the switch delay
 *
 * Returns: None.
 */
void increase_delay_rr_edges()
{
	int iedge, inode, dst_node, cut_counter, cut_step;
	int cut_pos;

	cut_step = ny / (num_cuts+1);

	for(inode=0; inode<num_rr_nodes;++inode)
	{
		// we only increase the delay of connections from CHANY nodes to other nodes.
		if(rr_node[inode].type==CHANY)
		{
			for(iedge = 0; iedge < rr_node[inode].num_edges; iedge++)
			{
				dst_node = rr_node[inode].edges[iedge];

				if(dst_node==-1)
				{	// if it's a connection that's already cut, you don't need to increase its delay
					continue;
				}

				// see if the connection crosses any of the cuts
				cut_counter = 0;
				for(cut_pos = cut_step; cut_pos < ny && cut_counter < num_cuts; cut_counter++, cut_pos+=cut_step)
				{
					if(rr_edge_crosses_interposer(inode,dst_node,cut_pos))
					{
						rr_node[inode].switches[iedge] = increased_delay_edge_map[rr_node[inode].switches[iedge]];
						break;
					}
				}
			}
		}
	}
}

/*
 * Description: This is the main entry point for performing rr_graph modifications using Andre's methodology
 *
 * Returns: None.
 */
void modify_rr_graph_using_switch_modification_methodology
(
	int nodes_per_chan,
	enum e_directionality directionality
)
{
	if(directionality == BI_DIRECTIONAL) /* Ignored for now TODO */
		return;

	if(num_cuts <=0)
		return;

#ifdef	DUMP_DEBUG_FILES
	// Before doing anything, let's dump the connections in the rr_graph
	FILE* fp = my_fopen("before_cutting.txt", "w", 0);
	dump_rr_graph_connections(fp);
	fclose(fp);
#endif

	// 1. Cut as many wires as we need to.
	//    This will cut %wires_cut of vertical connections at the interposer. 
	//    It will also cut connections from CHANX wires below the interposer to CHANY wires above the interposer
	cut_rr_graph_edges_at_cut_locations(nodes_per_chan);

	// 2. Increase the delay of all the remaining tracks that pass the interposer
	//    by increasing the switch delays
	increase_delay_rr_edges();

#ifdef	DUMP_DEBUG_FILES
	// dump after all rr_Graph modifications are done
	FILE* fp2 = my_fopen("after_cutting.txt", "w", 0);
	dump_rr_graph_connections(fp2);
	fclose(fp2);
#endif
}

#endif  // USE_SIMPLER_SWITCH_MODIFICATION_METHODOLOGY


#endif //INTERPOSER_BASED_ARCHITECTURE



