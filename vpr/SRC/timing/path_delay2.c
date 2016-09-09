
#include <stack>
#include <vector>
#include <algorithm>


#include "vtr_assert.h"
#include "vtr_list.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "path_delay2.h"
#include "read_xml_arch_file.h"

#include "path_delay.h"
/************* Variables (globals) shared by all path_delay modules **********/

int num_tnode_levels; /* Number of levels in the timing graph. */

vtr::t_ivec *tnodes_at_level;
/* [0..num__tnode_levels - 1].  Count and list of tnodes at each level of    
 * the timing graph, to make topological searches easier. Level-0 nodes are
 * sources to the timing graph (types TN_FF_SOURCE, TN_INPAD_SOURCE
 * and TN_CONSTANT_GEN_SOURCE). Level-N nodes are in the immediate fanout of 
 * nodes with level at most N-1. */

/******************* Subroutines local to this module ************************/

static int *alloc_and_load_tnode_fanin_and_check_edges(int *num_sinks_ptr);

void break_timing_graph_combinational_loops(std::vector<std::vector<int> >& tnode_comb_loops);

void break_timing_graph_combinational_loop(std::vector<int>& loop_tnodes);

std::vector<std::vector<int> > detect_timing_graph_combinational_loops();

std::vector<std::vector<int> > identify_strongly_connected_components(size_t min_size);

void strongconnect(int& index, int* tnode_indexes, int* tnode_lowlinks, bool* tnode_instack, 
                   std::stack<int>& tnode_stack, std::vector<std::vector<int> >& tnode_sccs,
                   size_t min_size, int inode);

void print_comb_loop(std::vector<int>& loop_tnodes);
/************************** Subroutine definitions ***************************/

static int *
alloc_and_load_tnode_fanin_and_check_edges(int *num_sinks_ptr) {

	/* Allocates an array and fills it with the number of in-edges (inputs) to   *
	 * each tnode.  While doing this it also checks that each edge in the timing *
	 * graph points to a valid tnode. Also counts the number of sinks.           */

	int inode, iedge, to_node, num_edges, error, num_sinks;
	int *tnode_num_fanin;
	t_tedge *tedge;

	tnode_num_fanin = (int *) vtr::calloc(num_tnodes, sizeof(int));
	error = 0;
	num_sinks = 0;

	for (inode = 0; inode < num_tnodes; inode++) {
		num_edges = tnode[inode].num_edges;

		if (num_edges > 0) {
			tedge = tnode[inode].out_edges;
			for (iedge = 0; iedge < num_edges; iedge++) {
				to_node = tedge[iedge].to_node;
				if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

				if (to_node < 0 || to_node >= num_tnodes) {
					vtr::printf_error(__FILE__, __LINE__, 
							"in alloc_and_load_tnode_fanin_and_check_edges:\n");
					vtr::printf_error(__FILE__, __LINE__, 
							"\ttnode #%d edge #%d goes to illegal node #%d.\n",
							inode, iedge, to_node);
					error++;
				}

				tnode_num_fanin[to_node]++;
			}
		}

		else if (num_edges == 0) {
			num_sinks++;
		}

		else {
			vtr::printf_error(__FILE__, __LINE__, 
					"in alloc_and_load_tnode_fanin_and_check_edges:\n");
			vtr::printf_error(__FILE__, __LINE__, 
					"\ttnode #%d has %d edges.\n", 
					inode, num_edges);
			error++;
		}

	}

	if (error != 0) {
		vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, 
				"Found %d Errors in the timing graph. Aborting.\n", error);
	}

	*num_sinks_ptr = num_sinks;
	return (tnode_num_fanin);
}

int alloc_and_load_timing_graph_levels(void) {

	/* Does a breadth-first search through the timing graph in order to levelize  *
	 * it.  This allows subsequent traversals to be done topologically for speed. *
	 * Also returns the number of sinks in the graph (nodes with no fanout).      */

	vtr::t_linked_int *free_list_head, *nodes_at_level_head;
	int inode, num_at_level, iedge, to_node, num_edges, num_sinks, num_levels,
			i;
	t_tedge *tedge;

	/* [0..num_tnodes-1]. # of in-edges to each tnode that have not yet been    *
	 * seen in this traversal.                                                  */

	int *tnode_fanin_left;

	tnode_fanin_left = alloc_and_load_tnode_fanin_and_check_edges(&num_sinks);

	free_list_head = NULL;
	nodes_at_level_head = NULL;

	/* Very conservative -> max number of levels = num_tnodes.  Realloc later.  *
	 * Temporarily need one extra level on the end because I look at the first  *
	 * empty level.                                                             */

	tnodes_at_level = (vtr::t_ivec *) vtr::malloc(
			(num_tnodes + 1) * sizeof(vtr::t_ivec));

	/* Scan through the timing graph, putting all the primary input nodes (no    *
	 * fanin) into level 0 of the level structure.                               */

	num_at_level = 0;

	for (inode = 0; inode < num_tnodes; inode++) {
		if (tnode_fanin_left[inode] == 0) {
			num_at_level++;
			nodes_at_level_head = insert_in_int_list(nodes_at_level_head, inode,
					&free_list_head);
		}
	}

	alloc_ivector_and_copy_int_list(&nodes_at_level_head, num_at_level,
			&tnodes_at_level[0], &free_list_head);

	num_levels = 0;

	while (num_at_level != 0) { /* Until there's nothing in the queue. */
		num_levels++;
		num_at_level = 0;

		for (i = 0; i < tnodes_at_level[num_levels - 1].nelem; i++) {
			inode = tnodes_at_level[num_levels - 1].list[i];
			tedge = tnode[inode].out_edges;
			num_edges = tnode[inode].num_edges;

			for (iedge = 0; iedge < num_edges; iedge++) {
				to_node = tedge[iedge].to_node;
				if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

				tnode_fanin_left[to_node]--;

				if (tnode_fanin_left[to_node] == 0) {
					num_at_level++;
					nodes_at_level_head = insert_in_int_list(
							nodes_at_level_head, to_node, &free_list_head);
				}
			}
		}

		alloc_ivector_and_copy_int_list(&nodes_at_level_head, num_at_level,
				&tnodes_at_level[num_levels], &free_list_head);
	}

	tnodes_at_level = (vtr::t_ivec *) vtr::realloc(tnodes_at_level,
			num_levels * sizeof(vtr::t_ivec));
	num_tnode_levels = num_levels;

	free(tnode_fanin_left);
	free_int_list(&free_list_head);
	return (num_sinks);
}

void check_timing_graph(int num_sinks) {

	/* Checks the timing graph to see that: (1) all the tnodes have been put    *
	 * into some level of the timing graph; */

	/* Addition error checks that need to be done but not yet implemented: (2) the number of primary inputs    *
	 * to the timing graph is equal to the number of input pads + the number of *
	 * constant generators; and (3) the number of sinks (nodes with no fanout)  *
	 * equals the number of output pads + the number of flip flops.             */

	int num_tnodes_check, ilevel, error;

	error = 0;
	num_tnodes_check = 0;

	/* TODO: Rework error checks for I/Os*/

	for (ilevel = 0; ilevel < num_tnode_levels; ilevel++)
		num_tnodes_check += tnodes_at_level[ilevel].nelem;

	if (num_tnodes_check != num_tnodes) {
		vtr::printf_error(__FILE__, __LINE__, 
				"Error in check_timing_graph: %d tnodes appear in the tnode level structure. Expected %d.\n", 
				num_tnodes_check, num_tnodes);
		vtr::printf_info("Checking the netlist for combinational cycles:\n");
		if (num_tnodes > num_tnodes_check) {
            std::vector< std::vector<int> > tnode_comb_loops = detect_timing_graph_combinational_loops();

            //Inform user about Combinational Loops
            size_t iloop;
            size_t itnode;
            for(iloop = 0; iloop < tnode_comb_loops.size(); iloop++) {
                vtr::printf_info("  Combinational Loop %d contains the following nodes:\n", iloop);
                for(itnode = 0; itnode < tnode_comb_loops[iloop].size(); itnode++) {
                    vtr::printf_info("   tnode: %d\n", tnode_comb_loops[iloop][itnode]);
                }
            }
		}
		error++;
	}
	/* Todo: Add error checks that # of flip-flops, memories, and other
	 black boxes match # of sinks/sources*/

	if (error != 0) {
		vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, 
				"Found %d Errors in the timing graph. Aborting.\n", error);
	}
}

float print_critical_path_node(FILE * fp, vtr::t_linked_int * critical_path_node, t_pb*** pin_id_to_pb_mapping) {

	/* Prints one tnode on the critical path out to fp. Returns the delay to the next node. */

	int inode, iblk, inet, downstream_node;
	t_pb_graph_pin * pb_graph_pin;
	e_tnode_type type;
	static const char *tnode_type_names[] = { "TN_INPAD_SOURCE", "TN_INPAD_OPIN",
			"TN_OUTPAD_IPIN", "TN_OUTPAD_SINK", "TN_CB_IPIN", "TN_CB_OPIN",
			"TN_INTERMEDIATE_NODE", "TN_PRIMITIVE_IPIN", "TN_PRIMITIVE_OPIN", "TN_FF_IPIN",
			"TN_FF_OPIN", "TN_FF_SINK", "TN_FF_SOURCE", "TN_FF_CLOCK", "TN_CONSTANT_GEN_SOURCE" };

	vtr::t_linked_int *next_crit_node;
	float Tdel;

	inode = critical_path_node->data;
	type = tnode[inode].type;
	iblk = tnode[inode].block;
	pb_graph_pin = tnode[inode].pb_graph_pin;

	fprintf(fp, "Node: %d  %s Block #%d (%s)\n", inode, tnode_type_names[type],
		iblk, block[iblk].name);

	if (pb_graph_pin == NULL) {
		VTR_ASSERT(
				type == TN_INPAD_SOURCE || type == TN_OUTPAD_SINK || type == TN_FF_SOURCE || type == TN_FF_SINK);
	}

	if (pb_graph_pin != NULL) {
		fprintf(fp, "Pin: %s.%s[%d] pb (%s)", pb_graph_pin->parent_node->pb_type->name,
			pb_graph_pin->port->name, pb_graph_pin->pin_number, pin_id_to_pb_mapping[iblk][pb_graph_pin->pin_count_in_cluster]->name);
	}
	if (type != TN_INPAD_SOURCE && type != TN_OUTPAD_SINK) {
		fprintf(fp, "\n");
	}

	fprintf(fp, "T_arr: %g  T_req: %g  ", tnode[inode].T_arr,
			tnode[inode].T_req);

	next_crit_node = critical_path_node->next;
	if (next_crit_node != NULL) {
		downstream_node = next_crit_node->data;
		Tdel = tnode[downstream_node].T_arr - tnode[inode].T_arr;
		fprintf(fp, "Tdel: %g\n", Tdel);
	} else { /* last node, no Tdel. */
		Tdel = 0.;
		fprintf(fp, "\n");
	}

	if (type == TN_CB_OPIN) {
		inet =
				block[iblk].pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_idx;
		inet = vpack_to_clb_net_mapping[inet];
		fprintf(fp, "External-to-Block Net: #%d (%s).  Pins on net: %d.\n",
			inet, g_clbs_nlist.net[inet].name, (int) g_clbs_nlist.net[inet].pins.size());
	} else if (pb_graph_pin != NULL) {
		inet =
			block[iblk].pb_route[pb_graph_pin->pin_count_in_cluster].atom_net_idx;
		fprintf(fp, "Internal Net: #%d (%s).  Pins on net: %d.\n", inet,
			g_atoms_nlist.net[inet].name, (int) g_atoms_nlist.net[inet].pins.size());
	}

	fprintf(fp, "\n");
	return (Tdel);
}

//Repeatedly detects combinational loops and remove timing edges to break them.
//
// The idea behind the implementation of is to identify Strongly
// Connected Components (SCCs) in the timing graph which, by definition,
// must contain cycles if they include more than one element. This is done using
// Tarjan's algorithm in O(V + E) time.  
//
// Once the SCCs are identified, an arbitrary edge in the timing graph is 
// disconnected to break the cycle. Since it may be possible for smaller sub-SCCs
// to result, this is done iteratively until no SCCs with more than one element
// are found.
void detect_and_fix_timing_graph_combinational_loops() {
    int comb_cycle_iter_count = 0;
    int comb_cycle_count = 0;

    vtr::printf_info("Iteratively removing timing edges to break combinational cycles in timing graph.\n");

    std::vector< std::vector<int> > tnode_comb_loops = detect_timing_graph_combinational_loops();

    //Repeat until all loops broken
    while(tnode_comb_loops.size() > 0) {
        comb_cycle_iter_count++;
        vtr::printf_info("Found %d Combinational Loops in the timing graph on iteration %d.\n", 
                        tnode_comb_loops.size(), comb_cycle_iter_count);
        vtr::printf_warning(__FILE__, __LINE__, 
                            "Combinational Loops can not be analyzed properly and will be "
                            "arbitrarily disconnected.\n");

        break_timing_graph_combinational_loops(tnode_comb_loops);

        comb_cycle_count += tnode_comb_loops.size();

        tnode_comb_loops = detect_timing_graph_combinational_loops();
    }
    vtr::printf_info("Removed %d combinational cycles from timing graph after %d iteration(s)\n", 
                    comb_cycle_count, comb_cycle_iter_count);
}

/*
 * Identify combinational loops in the timing graph
 */
std::vector<std::vector<int> > detect_timing_graph_combinational_loops() {
    //Combinational loops are SCC with >= 2 elements in the
    //timing graph
    return identify_strongly_connected_components(2);
}

/*
 * This function breaks every combinational loop passed to it. Each loop is represented
 * as a vector of tnode indicies*/
void break_timing_graph_combinational_loops(std::vector<std::vector<int> >& tnode_comb_loops) {
    size_t iloop;
    for(iloop = 0; iloop < tnode_comb_loops.size(); iloop++) {
        break_timing_graph_combinational_loop(tnode_comb_loops[iloop]);
    }
}

/*
 * Given a set of tnode indicies forming a combinational loop,
 * this breaks the loop by removing an arbitrary edge from the
 * cycle.
 */
void break_timing_graph_combinational_loop(std::vector<int>& loop_tnodes) {
    int i_first_tnode;
    int i_edge;
    int i_to_tnode;

    VTR_ASSERT(loop_tnodes.size() >= 2); //Must have atleast 2 nodes for a valid cycle

    //Find an edge between two tnodes in the loop set 
    // arbitrarily decide that it will be the first edge
    // from the first tnode which fans out to another tnode
    // in the loop set that will be cut
    i_first_tnode = loop_tnodes[0];

    for(i_edge = 0; i_edge < tnode[i_first_tnode].num_edges; i_edge++) {
        i_to_tnode = tnode[i_first_tnode].out_edges[i_edge].to_node;

        if(std::find(loop_tnodes.begin(), loop_tnodes.end(), i_to_tnode) != loop_tnodes.end()) {
            //This edge does fanout into the loop_tnodes set
            // so cut it
            vtr::printf_warning(__FILE__, __LINE__, "Disconnecting timing graph edge from tnode %d to tnode %d to break combinational cycle\n", i_first_tnode, i_to_tnode);

            //Mark the original target node as a combinational loop breakpoint
            tnode[i_to_tnode].is_comb_loop_breakpoint = true; 

            //Mark the edge as invalid
            tnode[i_first_tnode].out_edges[i_edge].to_node = DO_NOT_ANALYSE;

            return;
        }
    }
    vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, 
            "Could not find edge to break combinational loop in timing graph.\n");
}

/*
 * Tarjan's algorithm for finding Strongly Connected Components (SCCs) in
 *  a direct graph. Only SCCs with min_size or greater members are returned.
 * 
 *  We keep track of the following information:
 *    - The current 'index' of the node (stored in tnode_indexes), this 
 *      corresponds to the order the node was traversed in the DFS
 *    - The current 'lowlink' of the node (stored in tnode_lowlinks), this
 *      corresponds to the lowest node index which connects to the current
 *      node
 *    - Whether the node is currently in the stack (stored in tnode_instack)
 *    - A stack (tnode_stack) of elements in the current SCC
 * 
 *  The key idea behind the algorithm is that a node stays on the stack if it
 *  connects to a node earlier in the traversal.
 */
std::vector<std::vector<int> > identify_strongly_connected_components(size_t min_size) {
    int i;
    int index = 0; //The current index of the traversal
    std::vector<std::vector<int> > tnode_sccs;

    //Allocate book-keeping information
    int* tnode_indexes = (int*) vtr::calloc(num_tnodes, sizeof(int));
    int* tnode_lowlinks = (int*) vtr::calloc(num_tnodes, sizeof(int));
    bool* tnode_instack = (bool*) vtr::calloc(num_tnodes, sizeof(bool));

    //Initialize everything to unvisited
    for(i = 0; i < num_tnodes; i++) {
        tnode_indexes[i] = -1;
        tnode_lowlinks[i] = -1;
        tnode_instack[i] = false;
    }

    //The stack of nodes
    std::stack<int> tnode_stack;

    //We ensure that every node gets traversed
    for(i = 0 ; i < num_tnodes; i++) {
        if(tnode_indexes[i] == -1) {
            strongconnect(index, tnode_indexes, tnode_lowlinks, tnode_instack, tnode_stack, tnode_sccs, min_size, i);
        }
    }

    //Clean-up
    free(tnode_indexes);
    free(tnode_lowlinks);
    free(tnode_instack);

    return tnode_sccs;
}

void strongconnect(int& index, int* tnode_indexes, int* tnode_lowlinks, bool* tnode_instack, 
                   std::stack<int>& tnode_stack, std::vector<std::vector<int> >& tnode_sccs,
                   size_t min_size, int inode) {
    int iedge; //Index for out-going edges of the current node (inode)
    int iscc_element; //Index for the current SCC element (used when poping stack)
    int to_node_index; //Index to the sink node for the current edge

    //Mark this node as visited
    tnode_indexes[inode] = index;
    tnode_lowlinks[inode] = index;
    index += 1;

    //Add it to the stack
    tnode_stack.push(inode);
    tnode_instack[inode] = true;

    //Fanout of inode
    for(iedge = 0; iedge < tnode[inode].num_edges; iedge++) {
        to_node_index = tnode[inode].out_edges[iedge].to_node;
        if(to_node_index == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

        if(tnode_indexes[to_node_index] == -1) {
            //Haven't visited successor of inode (to_node) yet, recurse
            strongconnect(index, tnode_indexes, tnode_lowlinks, tnode_instack, tnode_stack, tnode_sccs, min_size, to_node_index);
            VTR_ASSERT(tnode_lowlinks[inode] >= 0);
            VTR_ASSERT(tnode_lowlinks[to_node_index] >= 0);

            //We are connected to to_node, so our lowest link should be either ourselves, or
            //to_node's lowest link
            tnode_lowlinks[inode] = min(tnode_lowlinks[inode], tnode_lowlinks[to_node_index]);
        } else if (tnode_instack[to_node_index]) {
            //to_node was in the stack, and so is part of the current SCC 
            VTR_ASSERT(tnode_lowlinks[inode] >= 0);
            VTR_ASSERT(tnode_indexes[to_node_index] >= 0);

            //to_node was on the stack, since we connect to it our lowest link is either ourselves
            //or the index of to_node (since it may have been traversed earlier)
            tnode_lowlinks[inode] = min(tnode_lowlinks[inode], tnode_indexes[to_node_index]);
        }
    }

    VTR_ASSERT(tnode_indexes[inode] >= 0);

    if(tnode_lowlinks[inode] == tnode_indexes[inode]) {
        //This inode is the root of a new SCC

        //Create a new SCC
        std::vector<int> scc;
        //Pop of elements of the stack until we reach ourselves
        do {
            iscc_element = tnode_stack.top();
            tnode_stack.pop();
            tnode_instack[iscc_element] = false;
            scc.push_back(iscc_element); //Add to the SCC
         } while(iscc_element != inode);

        //Add the SCC to the list of SCC if the meet
        // the minimum size requirement
        if(scc.size() >= min_size) {
            tnode_sccs.push_back(scc);
        }
    }
}

void print_comb_loop(std::vector<int>& loop_tnodes) {
    printf("Comb Loop:\n");
    for(std::vector<int>::iterator it = loop_tnodes.begin(); it != loop_tnodes.end(); it++) {
        int i_tnode = *it;
        if(tnode[i_tnode].pb_graph_pin != NULL) {
            printf("\ttnode: %d %s.%s[%d]\n", i_tnode,
                            tnode[i_tnode].pb_graph_pin->parent_node->pb_type->name, 
                            tnode[i_tnode].pb_graph_pin->port->name, 
                            tnode[i_tnode].pb_graph_pin->pin_number);
        } else {
            printf("\ttnode: %d\n", i_tnode);
        }
    }
}
