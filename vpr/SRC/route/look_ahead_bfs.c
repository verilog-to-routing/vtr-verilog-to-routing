#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>

#include <assert.h>

#include <map>
#include <vector>
#include <algorithm>

#include "vpr_utils.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "heapsort.h"
#include "path_delay.h"
#include "net_delay.h"
#include "stats.h"
#include "ReadOptions.h"
#include "look_ahead_bfs.h"

using namespace std;

#define DEBUG_SEG_TYPE 1
#define DEBUG_CHAN_TYPE 0
#define DEBUG_OFFSET_X 19
#define DEBUG_OFFSET_Y 3

/*
 * This file is just for experimenting a new look ahead
 * General procedure:
 *  1. start from each type of wire: pick the inode at the corner (not the real corner(1,1), but a little bit inside (2,2))??
 *  2. BFS to every chip location (in terms of clb) -- wire connection and wire cb pattern follows the arch specification
 *  3. calculate the min cost of (x, y) by comparing cost of all wires ending there
 *     -- also differentiate the ending side (whether ends at chany or chanx wire)
 */
/*
 * a revised version of s_heap:
 *    inode: rr_node index
 *    position: index on heap (useful when you try to remove a node from heap)
 *    cost & R_upstream: the same as s_heap
 */
/**************************************************************************/
// some heap functions revised
static void initialize_structs(int num_segment);
static void initialize_heap_2(int start_inode, float cost_start_inode, 
                                float R_upstream, float C_upstream, float basecost_upstream);
static void node_to_heap_2(int prev_node, int inode, float cost, float R_upstream, 
                            float C_upstream, float basecost_upstream);
static void remove_heap_2_node(int position);
static void setup_one_level_to_seg(int inode);
static struct s_heap_2 * pop_heap_2_head();
static void setup_seg_conn_inf(int num_segment);
static void expand_neighbours_2(struct s_heap_2 *cheapest, int start_seg_index, t_segment_inf *segment_inf);
static void get_leave_pt_by_cb (bool is_conn_ipin, int offset, int inode, int *term_x, int *term_y);
static void fill_cost_map(t_segment_inf *segment_inf, int seg_type, int chan_type);
static void print_seg_conn_matrix(t_segment_inf *segment_inf, int num_segment);
// TODO: rr_indexed_data
//       segment_inf
struct s_bfs_cost {
    // XXX: prev_node is only added for debugging bfs
    int prev_node;
    int rank;
    float cost;
    // C_upstream is used as C_downstream in router look ahead
    float C_upstream;
    float basecost_upstream;
};
/***************************************************************************/
// globals
// this array is to efficiently identify the hptr to be removed from heap
int g_bfs_num_seg = 0;
s_heap_2 **hptr_map =  NULL;
struct s_bfs_cost *rr_node_ranking = NULL;
int num_nodes_ranked;
int heap_2_size;
int heap_2_tail;
// heap_2 can point to anything at the start
struct s_heap_2 **heap_2 = NULL;
// if [i][j] = 1, then seg_type i can connect to seg j through the normal (core) switch block
int **seg_conn_inf = NULL;
t_segment_inf *ptr_segment_inf = NULL;
/****************************************************************************/
// function definition
// to be called in try_route()
static void initialize_structs(int num_segment) {
    g_bfs_num_seg = num_segment;
    hptr_map = (s_heap_2**) my_malloc(sizeof(struct s_heap_2 *) * num_rr_nodes);
    rr_node_ranking = (struct s_bfs_cost *) my_malloc(sizeof(struct s_bfs_cost) * num_rr_nodes);
    num_nodes_ranked = 0;
    heap_2_size = 0;
    heap_2_tail = 1;
    // heap_2 can point to anything at the start
    heap_2 = NULL;
    //cost_ranking_map = (int **)alloc_matrix(0, nx-3, 0, ny-3, sizeof(int));
    //cost_map = (float **)alloc_matrix(0, nx-3, 0, ny-3, sizeof(float));
    if (bfs_lookahead_info == NULL) {
        bfs_lookahead_info = (struct s_bfs_cost_inf ****)alloc_matrix4(0, nx-3, 0, ny-3, 0, num_segment-1, 0, 1, sizeof(struct s_bfs_cost_inf));
    }
    for (int ix = 0; ix < nx-2; ix ++) {
        for (int iy = 0; iy < ny-2; iy ++) {
            for (int iseg = 0; iseg < num_segment; iseg ++) {
                for (int ichan = 0; ichan < 2; ichan ++) {
                    bfs_lookahead_info[ix][iy][iseg][ichan].Tdel_x = HUGE_POSITIVE_FLOAT;
                    bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_x = HUGE_POSITIVE_FLOAT;
                    bfs_lookahead_info[ix][iy][iseg][ichan].acc_C_x = HUGE_POSITIVE_FLOAT;
                    bfs_lookahead_info[ix][iy][iseg][ichan].Tdel_y = HUGE_POSITIVE_FLOAT;
                    bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_y = HUGE_POSITIVE_FLOAT;
                    bfs_lookahead_info[ix][iy][iseg][ichan].acc_C_y = HUGE_POSITIVE_FLOAT;
                }
            }
        }
    }
    //memset(bfs_lookahead_info, );
    if (seg_conn_inf == NULL)
        seg_conn_inf = (int **)alloc_matrix(0, num_segment - 1, 0, num_segment - 1, sizeof(int));
    for (int i = 0; i < num_segment; i ++) {
        for (int j = 0; j < num_segment; j ++) {
            seg_conn_inf[i][j] = 0;
        }
    }
}
void pre_cal_look_ahead(t_segment_inf *segment_inf, int num_segment) {
    ptr_segment_inf = segment_inf;
    initialize_structs(num_segment);
    setup_seg_conn_inf(num_segment);
    printf("\n\nfinish seg conn setup\n\n");
    print_seg_conn_matrix(segment_inf, num_segment);
    /*
     * should be [2..nx-1][2..ny-1]
     * +-------------------------+
     * | +---------------------+ | 
     * | |                     | |
     * | |                     | |
     * | ^                     | |
     * | |                     | |
     * | +-->------------------+ |
     * +-------------------------+
     * get rid of the fringe
     * start at (2, 2); calculate for each type, and CHANX and CHANY
     */
    for (int i = 0; i < num_segment; i ++) {
        int chan_type[2] = {CHANX, CHANY};
        for (int ichan = 0; ichan < 2; ichan ++) {
            //int length = segment_inf[i].length;
            int start_inode = -1;
            // first try to find wire start at the left bottom corner
            for (int inode = 0; inode < num_rr_nodes; inode ++) {
                // start wire is either INC_DIR or BI_DIR
                if (rr_node[inode].get_direction() == DEC_DIRECTION)
                    continue;
                int i_seg_index = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
                if (i_seg_index != i)
                    continue;
                if (rr_node[inode].type == chan_type[ichan]
                 && rr_node[inode].get_xlow() == 2
                 && rr_node[inode].get_ylow() == 2) {
                    /*
                    if (i_seg_index == 0) {
                        int num_edges = rr_node[inode].get_num_edges();
                        for (int iconn = 0; iconn < num_edges; iconn++) {
                            int to_node = rr_node[inode].edges[iconn];
                            int to_node_seg_index = rr_indexed_data[rr_node[to_node].get_cost_index()].seg_index;
                            if (to_node_seg_index == 1 || to_node_seg_index == 2) {
                                start_inode = inode;
                                inode = num_rr_nodes;
                                break;
                            }
                        }
                    } else {
                    */
                    start_inode = inode;
                    inode = num_rr_nodes;
                    //}
                }
            }
            // if not found such wire (maybe due to a too small chip size / too long wire)
            // then try the right bottom corner
            if (start_inode == -1) {
                for (int inode = 0; inode < num_rr_nodes; inode ++) {
                    if (rr_node[inode].get_direction() == INC_DIRECTION)
                        continue;
                    if (rr_indexed_data[rr_node[inode].get_cost_index()].seg_index != i)
                        continue;
                    if (rr_node[inode].type == chan_type[ichan]
                     && rr_node[inode].get_xhigh() == nx - 1
                     && rr_node[inode].get_yhigh() == ny - 1) {
                        start_inode = inode;
                        inode = num_rr_nodes;
                    }
                }
            }
            if (start_inode != -1) {
                if (i == DEBUG_SEG_TYPE && ichan == DEBUG_CHAN_TYPE) {
                    printf("== DEBUG ==: start node %d\n", start_inode);
                }
                printf("**** START BFS for segment %s\tchan type %d\n", segment_inf[i].name, ichan);
                clock_t begin = clock();
                num_nodes_ranked = 0;
                look_ahead_bfs(start_inode, segment_inf);
                clock_t end = clock();
                vpr_printf_info("\tThis BFS took %g seconds.\n", (float)(end - begin) / CLOCKS_PER_SEC);
                printf("**** END BFS for segment %s\n", segment_inf[i].name);
                fill_cost_map(segment_inf, i, ichan);
                //print_C_map(bfs_C_downstream_map[make_pair(i, chan_type[ichan])]);
                start_inode = -1;
            } else {
                vpr_printf_error(__FILE__, __LINE__, 
                    "segment %s has no wire starting at corner\n", segment_inf[i].name);
            }
        }
    }
    print_cost_map("init_bfs_map");
    /*
    for (int ix = 0; ix < nx - 2; ix ++) {
        for (int iy = 0; iy < ny - 2; iy ++) {
            for (int iseg = 0; iseg < num_segment; iseg ++) {
                for (int ichan = 0; ichan < 2; ichan ++) {
                    assert(bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_x < 2 &&
                        bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_y < 2);
                }
            }
        }
    }
    */
    free(hptr_map);
    free(rr_node_ranking);
    free(heap_2+1);
    free_matrix(seg_conn_inf, 0, num_segment - 1, 0, sizeof(int));
    hptr_map = NULL;
    rr_node_ranking = NULL;
    heap_2 = NULL;
    seg_conn_inf = NULL;
}

void look_ahead_bfs(int start_inode, t_segment_inf *segment_inf) {
    // 1. node to heap (start_inode)
    // 2. get_heap_head
    //    while (head != null) {
    //        remove head from heap;
    //        set cost ranking of head (1 ~ num_rr_nodes)
    //        expand neighbours & put them onto heap
    //        get head for next itr
    //    }
    for (int i = 0; i < num_rr_nodes; i ++) {
        hptr_map[i] = NULL;
        rr_node_ranking[i].prev_node = -1;
        rr_node_ranking[i].rank = -1;
        rr_node_ranking[i].cost = -1;
        rr_node_ranking[i].C_upstream = -1;
        rr_node_ranking[i].basecost_upstream = -1;
    }
    float R_start_inode = rr_node[start_inode].R;
    float cost_start_inode = rr_node[start_inode].C * (0.5 * R_start_inode);
    R_start_inode += 0.;
    cost_start_inode += 0.;
    // I would like to change the heap to facilitate removal of replicate nodes
    //node_to_heap_2(start_inode, cost_start_inode, R_start_inode);
    node_to_heap_2(-1, start_inode, 0.0, 0.0, 0.0, 0.0);
    struct s_heap_2* cheapest = pop_heap_2_head();
    int start_seg_index = rr_indexed_data[rr_node[start_inode].get_cost_index()].seg_index;
    while (cheapest != NULL) {
        expand_neighbours_2(cheapest, start_seg_index, segment_inf);
        free(cheapest);
        cheapest = pop_heap_2_head();
    }
}
/*
 * to test how many other type of segments a certain type of wire can 
 * connect to, via the standard switch block (sb which are at the core place of chip)
 * 
 * the reason for doing so:
 *     for some weird arch, for example, a certain wire can only connect to wires of its
 *     same type, via sb in core area on chip. but it may be able to connect to other wire
 *     types via corner / fringe sb. --> this will cause inaccuracy of bfs, as we pick the 
 *     starting point to be near the corner
 */
static void setup_seg_conn_inf(int num_segment) {
    vector<int> seg_type_to_check;
    for (int iseg = 0; iseg < num_segment; iseg ++) {
        seg_type_to_check.push_back(iseg);
    }
    vector< pair<int, int> > nearest_to_core_pos;
    for (int iseg = 0; iseg < num_segment; iseg ++) {
        nearest_to_core_pos.push_back(make_pair(-1, -1));
    }
    // we have to go through this step, cuz on some small chips, there are just no 
    // long wires that have their mid-point going through exactly (nx / 2) & (ny / 2)
    for (int inode = 0; inode < num_rr_nodes; inode ++) {
        int cur_seg_type = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
        if (cur_seg_type < 0 || cur_seg_type >= num_segment)
            continue;
        int cur_x_mid, cur_y_mid;
        cur_x_mid = (rr_node[inode].get_xhigh() + rr_node[inode].get_xlow()) / 2;
        cur_y_mid = (rr_node[inode].get_yhigh() + rr_node[inode].get_ylow()) / 2;
        if (abs(nearest_to_core_pos[cur_seg_type].first - nx / 2) > abs(cur_x_mid - nx / 2)
        && abs(nearest_to_core_pos[cur_seg_type].second - ny / 2) > abs(cur_y_mid - ny / 2)) {
            nearest_to_core_pos[cur_seg_type].first = cur_x_mid;
            nearest_to_core_pos[cur_seg_type].second = cur_y_mid;
        }
    }

    for (int inode = 0; inode < num_rr_nodes; inode ++) {
        int cur_seg_type = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
        if (cur_seg_type < 0 || cur_seg_type >= num_segment)
            continue;
        int cur_x_mid, cur_y_mid;
        cur_x_mid = (rr_node[inode].get_xhigh() + rr_node[inode].get_xlow()) / 2;
        cur_y_mid = (rr_node[inode].get_yhigh() + rr_node[inode].get_ylow()) / 2;
        vector<int>::iterator seg_pos = find(seg_type_to_check.begin(), seg_type_to_check.end(), cur_seg_type);
        if (seg_pos != seg_type_to_check.end()
         && cur_x_mid == nearest_to_core_pos[cur_seg_type].first 
         && cur_y_mid == nearest_to_core_pos[cur_seg_type].second) {
            seg_type_to_check.erase(seg_pos);
            setup_one_level_to_seg(inode);
        }
        if (seg_type_to_check.empty())
            break;
    }
    // next, test convergence for seg_conn_inf
    bool new_conn_added = false;
    do {
        new_conn_added = false;
        for (int from_seg = 0; from_seg < num_segment; from_seg ++) {
            for (int to_seg = 0; to_seg < num_segment; to_seg ++) {
                if (seg_conn_inf[from_seg][to_seg] == 0) 
                    continue;
                for (int to_to_seg = 0; to_to_seg < num_segment; to_to_seg ++) {
                    if (seg_conn_inf[to_seg][to_to_seg] == 0)
                        continue;
                    if (seg_conn_inf[from_seg][to_to_seg] == 0) {
                        seg_conn_inf[from_seg][to_to_seg] = 1;
                        new_conn_added = true;
                    }
                }
            }
        }
    } while (new_conn_added);
}
static void setup_one_level_to_seg(int inode) {
    assert(rr_node[inode].type == CHANX || rr_node[inode].type == CHANY);
    int num_edges = rr_node[inode].get_num_edges();
    int from_seg_type = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
    if (from_seg_type >= g_num_segment || from_seg_type < 0)
        return;
    for (int iconn = 0; iconn < num_edges; iconn ++) {
        int to_node = rr_node[inode].edges[iconn];
        int to_seg_type = rr_indexed_data[rr_node[to_node].get_cost_index()].seg_index;
        if (to_seg_type >= g_num_segment || to_seg_type < 0)
            continue;
        seg_conn_inf[from_seg_type][to_seg_type] = 1;
    }
}
static void expand_neighbours_2(struct s_heap_2 *cheapest, int start_seg_index, t_segment_inf *segment_inf) {
    int inode = cheapest->inode;
    int num_edges = rr_node[inode].get_num_edges();
    int iswitch;

    float cur_R_upstream = cheapest->R_upstream;
    float cur_C_upstream = cheapest->C_upstream;
    float cur_basecost_upstream = cheapest->basecost_upstream;
    float new_R_upstream, new_C_upstream, new_basecost_upstream;
    for (int iconn = 0; iconn < num_edges; iconn++) {
        int to_node = rr_node[inode].edges[iconn];
        if (rr_node[to_node].type != CHANX && rr_node[to_node].type != CHANY)
            continue;
        if (rr_node_ranking[to_node].rank != -1)
            continue;

        int to_seg_index = rr_indexed_data[rr_node[to_node].get_cost_index()].seg_index;
        if (seg_conn_inf[start_seg_index][to_seg_index] == 0)
            continue;
        /*
        // eliminate the wires that lie completely on the fringe track
        if ((rr_node[to_node].type == CHANX && rr_node[to_node].get_xlow() == 0)
         || (rr_node[to_node].type == CHANY && rr_node[to_node].get_ylow() == 0))
            continue;
        */
        // calculate the backward delay of this node
        // referring to route_timing.c: timing_driven_expand_neighbours()
        iswitch = rr_node[inode].switches[iconn];
        float cur_wire_R = segment_inf[to_seg_index].Rmetal * segment_inf[to_seg_index].length;
        float cur_wire_C = segment_inf[to_seg_index].Cmetal * segment_inf[to_seg_index].length;
        float cur_wire_basecost = rr_indexed_data[rr_node[to_node].get_cost_index()].base_cost;
		if (g_rr_switch_inf[iswitch].buffered) {
			new_R_upstream = g_rr_switch_inf[iswitch].R;
            // if switch is buffered, don't add C.
            // but this will still produce an overestimation of C
            new_C_upstream = cur_C_upstream;
		} else {
            printf("\nUN BUFFERED SWITCH\n");
			new_R_upstream = cur_R_upstream + g_rr_switch_inf[iswitch].R;
            //new_C_upstream += rr_node[to_node].C;
            new_C_upstream = cur_C_upstream + cur_wire_C;
		}
        // we cannot use the R stored in rr_node directly, as it may be an underestimated value
        // if wire is cut short at the fringe
        float to_node_cost = cur_wire_C * (new_R_upstream + 0.5 * cur_wire_R);
        to_node_cost += g_rr_switch_inf[iswitch].Tdel;
        to_node_cost += cheapest->cost;
        new_R_upstream += cur_wire_R;
        new_basecost_upstream = cur_basecost_upstream + cur_wire_basecost;
        if (hptr_map[to_node] != NULL) {
            if (hptr_map[to_node]->cost <= to_node_cost) {
                continue;
            } else {
                remove_heap_2_node(hptr_map[to_node]->position);
            }
        }
        node_to_heap_2(inode, to_node, to_node_cost, new_R_upstream, new_C_upstream, new_basecost_upstream);
    }
}

static void initialize_heap_2(int start_inode, float cost_start_inode, float R_start_inode, 
                              float C_start_inode, float basecost_start_inode) {
    assert(heap_2 == NULL && heap_2_size == 0 && heap_2_tail == 1);
    heap_2_size = 1;
    heap_2_tail = 2;
    struct s_heap_2 *head = (struct s_heap_2 *) my_malloc(sizeof(struct s_heap_2));
    head->prev_node = -1;
    head->inode = start_inode;
    head->position = 1;
    head->cost = cost_start_inode;
    head->R_upstream = R_start_inode;
    head->C_upstream = C_start_inode;
    head->basecost_upstream = basecost_start_inode;
    heap_2 = (struct s_heap_2 **) my_malloc(sizeof(struct s_heap_2 *) * heap_2_size);
    heap_2 --;
    heap_2[1] = head;
}   

/*
 * when you call this function, it has already been made sure that
 * heap node for inode is removed
 */
static void node_to_heap_2(int prev_node, int inode, float cost, 
        float R_upstream, float C_upstream, float basecost_upstream) {
    if (heap_2_size == 0) {
        initialize_heap_2(inode, cost, R_upstream, C_upstream, basecost_upstream);
        return;
    }
    struct s_heap_2 *hptr = (struct s_heap_2 *) my_malloc(sizeof(s_heap_2));
    // XXX
    assert(hptr_map[inode] == NULL);
    hptr_map[inode] = hptr;
    hptr->prev_node = prev_node;
    hptr->inode = inode;
    hptr->position = -1;
    hptr->cost = cost;
    hptr->R_upstream = R_upstream;
    hptr->C_upstream = C_upstream;
    hptr->basecost_upstream = basecost_upstream;
    if (heap_2_tail > heap_2_size) {
        heap_2_size *= 2;
        heap_2 = (struct s_heap_2 **)my_realloc((void *)(heap_2 + 1), heap_2_size * sizeof(struct s_heap_2 *));
        heap_2 --;
    }
    heap_2[heap_2_tail] = hptr;
    // XXX
    hptr->position = heap_2_tail;
    int ifrom = heap_2_tail;
    int ito = ifrom / 2;
    heap_2_tail ++;

    struct s_heap_2 *temp_ptr = NULL;
    while ((ito >= 1) && (heap_2[ifrom]->cost < heap_2[ito]->cost)) {
        temp_ptr = heap_2[ito];
        // XXX
        heap_2[ifrom]->position = ito;
        heap_2[ito]->position = ifrom;
        heap_2[ito] = heap_2[ifrom];
        heap_2[ifrom] = temp_ptr;
        ifrom = ito;
        ito = ifrom / 2;
    }
}
/*
 * remove node of [position] from heap
 * sifting to maintain the heap property
 */
static void remove_heap_2_node(int position) {
    // XXX
    hptr_map[heap_2[position]->inode] = NULL;
    free(heap_2[position]);
    heap_2_tail --;
    heap_2[position] = heap_2[heap_2_tail];
    heap_2[heap_2_tail] = NULL;
    if (heap_2[position] != NULL)
        heap_2[position]->position = position;
    // Sifting:
    int ifrom = position;
    int ito = 2 * ifrom;
    struct s_heap_2 *temp;
    while (ito < heap_2_tail) {
        if (ito != heap_2_tail - 1 && heap_2[ito + 1]->cost < heap_2[ito]->cost)
            ito++;
        if (heap_2[ito]->cost > heap_2[ifrom]->cost)
            break;
        temp = heap_2[ito];
        heap_2[ito] = heap_2[ifrom];
        heap_2[ifrom] = temp;
        heap_2[ito]->position = ito;
        heap_2[ifrom]->position = ifrom;
        ifrom = ito;
        ito = 2 * ifrom;
    }
}
static struct s_heap_2 * pop_heap_2_head(void) {
    if (heap_2_tail == 1)
        return NULL;
    struct s_heap_2 * poped_head = (struct s_heap_2 *)my_malloc(sizeof (struct s_heap_2));
    memcpy(poped_head, heap_2[1], sizeof(struct s_heap_2));

    remove_heap_2_node(1);
    
    rr_node_ranking[poped_head->inode].prev_node = poped_head->prev_node;
    rr_node_ranking[poped_head->inode].rank = num_nodes_ranked;
    rr_node_ranking[poped_head->inode].cost = poped_head->cost;
    rr_node_ranking[poped_head->inode].C_upstream = poped_head->C_upstream;
    rr_node_ranking[poped_head->inode].basecost_upstream = poped_head->basecost_upstream;
    num_nodes_ranked ++;

    return poped_head;
}


static void fill_cost_map(t_segment_inf *segment_inf, int wire_type, int chan_type) {
    int print_inode_x = -1, print_inode_y = -1;
    int term_x = -1;
    int term_y = -1;
    for (int inode = 0; inode < num_rr_nodes; inode++) {
        if (rr_node_ranking[inode].rank == -1)
            continue;
        // TODO: what about BI_DIR????
        int segment_index = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
        bool *cb_pattern = segment_inf[segment_index].cb;
        int target_chan_type = rr_node[inode].type;
        for (int offset = 0; offset < rr_node[inode].get_len(); offset ++) {
            get_leave_pt_by_cb(cb_pattern[offset], offset, inode, &term_x, &term_y);
            if (term_x == -1 || term_y == -1)
                continue;
            if (target_chan_type == CHANX) {
                if (bfs_lookahead_info[term_x][term_y][wire_type][chan_type].Tdel_x == UNDEFINED
                 || bfs_lookahead_info[term_x][term_y][wire_type][chan_type].Tdel_x > rr_node_ranking[inode].cost) {
                    bfs_lookahead_info[term_x][term_y][wire_type][chan_type].Tdel_x = rr_node_ranking[inode].cost;
                    bfs_lookahead_info[term_x][term_y][wire_type][chan_type].acc_C_x = rr_node_ranking[inode].C_upstream;
                    bfs_lookahead_info[term_x][term_y][wire_type][chan_type].acc_basecost_x = rr_node_ranking[inode].basecost_upstream;
                    
                    if (term_x == DEBUG_OFFSET_X && term_y == DEBUG_OFFSET_Y
                     && wire_type == DEBUG_SEG_TYPE && chan_type == DEBUG_CHAN_TYPE) {
                        print_inode_x = inode;
                    }
                    
                }
            } else {
                if (bfs_lookahead_info[term_x][term_y][wire_type][chan_type].Tdel_y == UNDEFINED
                 || bfs_lookahead_info[term_x][term_y][wire_type][chan_type].Tdel_y > rr_node_ranking[inode].cost) {
                    bfs_lookahead_info[term_x][term_y][wire_type][chan_type].Tdel_y = rr_node_ranking[inode].cost;
                    bfs_lookahead_info[term_x][term_y][wire_type][chan_type].acc_C_y = rr_node_ranking[inode].C_upstream;
                    bfs_lookahead_info[term_x][term_y][wire_type][chan_type].acc_basecost_y = rr_node_ranking[inode].basecost_upstream;
                    
                    if (term_x == DEBUG_OFFSET_X && term_y == DEBUG_OFFSET_Y
                     && wire_type == DEBUG_SEG_TYPE && chan_type == DEBUG_CHAN_TYPE) {
                        print_inode_y = inode;
                    }
                }
            }
        }
    }
    if (print_inode_x != -1) {
        printf("BFS SELECTED PATH (CHANX): [%d][%d][%d][%d]\n", DEBUG_OFFSET_X, DEBUG_OFFSET_Y, DEBUG_SEG_TYPE, DEBUG_CHAN_TYPE);
        print_bfs_optimal_path(print_inode_x);
    }
    if (print_inode_y != -1) {
        printf("BFS SELECTED PATH (CHANY): [%d][%d][%d][%d]\n", DEBUG_OFFSET_X, DEBUG_OFFSET_Y, DEBUG_SEG_TYPE, DEBUG_CHAN_TYPE);
        print_bfs_optimal_path(print_inode_y);
    }
}

static void get_leave_pt_by_cb (bool is_conn_ipin, int offset, int inode, int *term_x, int *term_y) {
    if (!is_conn_ipin) {
        *term_x = -1;
        *term_y = -1;
        return;
    }
    int start_x = -1, start_y = -1;
    get_unidir_seg_start(inode, &start_x, &start_y);
    int wire_type = rr_node[inode].type;
    assert(wire_type == CHANX || wire_type == CHANY);
    if (wire_type == CHANX) {
        *term_y = start_y - 2;
        if (rr_node[inode].get_direction() == INC_DIRECTION)
            *term_x = start_x + offset - 2;
        else 
            *term_x = start_x - offset - 2;
    } else {
        *term_x = start_x - 2;
        if (rr_node[inode].get_direction() == INC_DIRECTION)
            *term_y = start_y + offset - 2;
        else 
            *term_y = start_y - offset - 2;
    }
    if (*term_x < 0 || *term_x > nx-3)
        *term_x = -1;
    if (*term_y < 0 || *term_y > ny-3)
        *term_y = -1;
}

void print_cost_map(char *fname) {
    FILE *fp;
    fp = fopen(fname, "w+");
    if (fp == NULL) {
        printf("cannot open file\n");
    }
    float max_basecost = -1;
    fprintf(fp, "START: COST RANKING MAP\n");
    fprintf(fp, "x\ty\tcost_x\tcost_y\n");
    for (int ix = 0; ix < nx-2; ix ++) {
        for (int iy = 0; iy < ny-2; iy ++) {
            for (int iseg = 0; iseg < g_bfs_num_seg; iseg ++) {
                for (int ichan = 0; ichan < 2; ichan ++) {
                    fprintf(fp, "%d\t%d\t%.1f e-10\t%.1f e-10\n", ix, iy, 
                        bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_x * pow(10, 10), 
                        bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_y * pow(10, 10));
                    if (bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_x > max_basecost)
                        max_basecost = bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_x;
                    if (bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_y > max_basecost)
                        max_basecost = bfs_lookahead_info[ix][iy][iseg][ichan].acc_basecost_y;
                }
            }
        }
    }
    printf("MAX basecost\t%e\n", max_basecost);
    fclose(fp);
    printf("END MAP PRINTING\n");
}

void print_C_map(float **c_map) {
    printf("START: COST RANKING MAP\n");
    printf("x\ty\tC_upstream\n");
    for (int ix = 0; ix < nx-2; ix++){
        for (int iy = 0; iy < ny-2; iy++){
            printf("%d\t%d\t%e\n", ix, iy, c_map[ix][iy]);
        }
    }
}

static void print_seg_conn_matrix(t_segment_inf *segment_inf, int num_segment) {
    printf(">> SEGMENT CONN INFO\n");
    for (int iseg = 0; iseg < num_segment; iseg ++) {
        printf("\tSEG %s\t-->\t", segment_inf[iseg].name);
        for (int i_to_seg = 0; i_to_seg < num_segment; i_to_seg ++) {
            if (seg_conn_inf[iseg][i_to_seg] == 1) {
                printf("%s\t", segment_inf[i_to_seg].name);
            }
        }
        printf("\n");
    }
}

void print_bfs_optimal_path(int term_node) {
    assert(term_node >= 0 && term_node < num_rr_nodes);
    int cur_node = term_node;
    printf("*** PRINT BFS OPTIMAL PATH\n");
    while (rr_node_ranking[cur_node].prev_node != -1) {
        int x_s, x_e, y_s, y_e;
        get_unidir_seg_end(cur_node, &x_s, &y_s);
        get_unidir_seg_start(cur_node, &x_e, &y_e);
        int len = rr_node[cur_node].get_len();
        printf("\tinode %d\tL%d\tstart(%d,%d)\tend(%d,%d)\tbackTdel %.3f\t0_basecost %.3f\tacc_basecost %.3f\n", cur_node, len, x_s, y_s, x_e, y_e, rr_node_ranking[cur_node].cost * pow(10,10), rr_indexed_data[rr_node[cur_node].get_cost_index()].base_cost * pow(10, 10), rr_node_ranking[cur_node].basecost_upstream * pow(10, 10));
        cur_node = rr_node_ranking[cur_node].prev_node;
    }
}
