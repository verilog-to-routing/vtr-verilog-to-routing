#include "profile_lookahead.h"

using namespace std;

void print_start_end_to_sink(bool is_start, int itry, int inet, int target_node, int itarget) {
#if DEBUGEXPANSIONRATIO == 1
    if (itry == DB_ITRY && target_node == DB_TARGET_NODE){
        if (is_start) {
            printf("\n**** itry %d\tSTART ROUTE TO net %d\ttarget %d\tinode %d\n", itry, inet, itarget, target_node);
            printf("COORD: (%d,%d)\n", rr_node[target_node].get_xlow(), rr_node[target_node].get_ylow());
        } else {
            printf("\n**** itry %d\tFINISH ROUTE TO net %d\ttarget %d\tinode %d\n\n", itry, inet, itarget, target_node);
        }
    } 
#endif
}

void print_expand_neighbours(bool is_parent, int inode, int target_node, 
    int target_chan_type, int target_pin_x, int target_pin_y, 
    float expected_cost, float future_Tdel, float new_tot_cost, 
    float cur_basecost, float cong_cost) {
#if DEBUGEXPANSIONRATIO == 1
    if (itry_share == DB_ITRY && target_node == DB_TARGET_NODE) {
        const char *dir_name[] = {"INC", "DEC", NULL};
        const char *node_seg_type[] = {"source", "sink", "ipin", "opin", "chanX", "chanY", NULL};   
        int dir = (rr_node[inode].get_direction() == INC_DIRECTION) ? 0 : 1;
        int inode_type = rr_node[inode].type;
        int inode_seg_index = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
        float basecost, C, Tdel;
        int s_x, s_y, e_x, e_y, len;
        get_unidir_seg_start(inode, &s_x, &s_y);
        get_unidir_seg_end(inode, &e_x, &e_y);
        len = rr_node[inode].get_len();
        if (is_parent) {
            if (target_chan_type == UNDEFINED) {
                printf("TARGET CHAN TYPE UNDEFINED\n");
            } else {
                printf("TARGET CHAN TYPE: %d\tX %d\tY %d\n", target_chan_type, target_pin_x, target_pin_y);
            }
            Tdel = get_timing_driven_future_Tdel(inode, target_node, &C, &basecost);
            printf("EXPAND inode %d(L%d)\t%s\t%s-%d\t(%d,%d)\t(%d,%d)\tpath cost %.3f\texp cost %.3f\n",
                inode, len, dir_name[dir], node_seg_type[inode_type],
                inode_seg_index, s_x, s_y, e_x, e_y, 
                rr_node_route_inf[inode].path_cost * pow(10, 10),
                Tdel * pow(10, 10) );
        } else {
            printf("\texpand %d(L%d)\t%s\t%s-%d\t(%d,%d)\t(%d,%d)\t",
                inode, len, dir_name[dir], node_seg_type[inode_type], 
                inode_seg_index, s_x, s_y, e_x, e_y);
            printf("exp cost %.3f\tb-Tdel %.3f\ttot cost %.3f\tbCost %.3fcCost %.3f\n",
                expected_cost * pow(10, 10), future_Tdel * pow(10, 10), 
                new_tot_cost * pow(10, 10), cur_basecost * pow(10, 10),
                cong_cost * pow(10, 10));
        }
    }
#endif
}
