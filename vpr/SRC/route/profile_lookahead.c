#include "profile_lookahead.h"

using namespace std;

void print_start_end_to_sink(bool is_start, int itry, int inet, int target_node, int itarget) {
    if (itry == DB_ITRY && target_node == DB_TARGET_NODE){
        if (is_start) {
            printf("\n**** itry %d\tSTART ROUTE TO net %d\ttarget %d\tinode %d\n", 
                    itry, inet, itarget, target_node);
            printf("COORD: (%d,%d)\n", rr_node[target_node].get_xlow(), rr_node[target_node].get_ylow());
        } else {
            printf("\n**** itry %d\tFINISH ROUTE TO net %d\ttarget %d\tinode %d\n\n", 
                    itry, inet, itarget, target_node);
        }
    } 
}

void print_expand_neighbours(bool is_parent, int inode, int target_node, 
    int target_chan_type, int target_pin_x, int target_pin_y, 
    float expected_cost, float future_Tdel, float new_tot_cost, 
    float cur_basecost, float cong_cost) {
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
}

void setup_max_min_criticality(float &max_crit, float &min_crit, 
        int &max_inet, int &min_inet, int &max_ipin, int &min_ipin, t_slack * slacks) {
    /*
     *  set up max/min criticality && where the max/min happens
     *  called in try_timing_driven_route()
     */
    float max_crit_len_product = -1;
    float min_crit_len_product = pow(10, 20);
    max_crit = -1;
    min_crit = pow(10, 20);
    max_inet = -1;
    max_ipin = -1;
    min_inet = -1;
    min_ipin = -1;
    for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
        if (should_route_net(inet) == false)
            continue; 
        if (g_clbs_nlist.net[inet].is_global == true)
            continue;
        if (g_clbs_nlist.net[inet].is_fixed == true)
            continue;
        int source_inode = net_rr_terminals[inet][0];
        int x_source_mid = (rr_node[source_inode].get_xhigh() + rr_node[source_inode].get_xlow()) / 2;
        int y_source_mid = (rr_node[source_inode].get_yhigh() + rr_node[source_inode].get_ylow()) / 2;
        unsigned int pin_upbound = (g_clbs_nlist.net[inet].pins.size() > 2) ? 2 : g_clbs_nlist.net[inet].pins.size();
        for (unsigned int ipin = 1; ipin < pin_upbound; ipin++) {
            float pin_criticality = slacks->timing_criticality[inet][ipin];
            int target_inode = net_rr_terminals[inet][ipin];
            int x_target_mid = (rr_node[target_inode].get_xhigh() + rr_node[target_inode].get_xlow()) / 2;
            int y_target_mid = (rr_node[target_inode].get_yhigh() + rr_node[target_inode].get_ylow()) / 2;
            // print long nets only
            int bb_len = abs(x_source_mid - x_target_mid + 1) + abs(y_source_mid - y_target_mid + 1);
            float len_crit_product = bb_len * pin_criticality;
            float len_inv_crit_product = (1./bb_len) * pin_criticality;
            max_inet = (len_crit_product > max_crit_len_product) ? inet : max_inet;
            max_ipin = (len_crit_product > max_crit_len_product) ? ipin : max_ipin;
            min_inet = (len_inv_crit_product < min_crit_len_product) ? inet : min_inet;
            min_ipin = (len_inv_crit_product < min_crit_len_product) ? ipin : min_ipin;
            max_crit = (len_crit_product > max_crit_len_product) ? pin_criticality : max_crit;
            min_crit = (len_inv_crit_product < min_crit_len_product) ? pin_criticality : min_crit;
            max_crit_len_product = (len_crit_product > max_crit_len_product) ? len_crit_product : max_crit_len_product;
            min_crit_len_product = (len_inv_crit_product < min_crit_len_product) ? len_inv_crit_product : min_crit_len_product;
        }
    }
    //printf("max_inet %d\tmax_ipin %d\tmin_inet %d\tmin_ipin %d\n", max_inet, max_ipin, min_inet, min_ipin);
    //printf("max_crit_len_product %f\tmin_crit %f\n", max_crit_len_product, min_crit);
    //printf("\n");

}

void print_critical_path_from_rt(struct s_heap *hptr, float target_criticality, int new_nodes_count, float actual_Tdel) {
    int target_node = hptr->index;
    if (target_node == DB_TARGET_NODE && itry_share == DB_ITRY) {
        int inode = hptr->u.prev_node;
        printf("CRITICAL PATH %d: back trace start\tcrit: %fNEW NODES %d\n", target_node, target_criticality, new_nodes_count);
        while (rr_node_route_inf[inode].prev_node != NO_PREVIOUS) {
            // this is backtrace, so print in anti-direction order
            int ix_s, ix_e, iy_s, iy_e;
            get_unidir_seg_end(inode, &ix_s, &iy_s);
            get_unidir_seg_start(inode, &ix_e, &iy_e);
            int prev_inode = rr_node_route_inf[inode].prev_node;
            float bTdel = rr_node_route_inf[inode].back_Tdel;
            float bbTdel = rr_node_route_inf[prev_inode].back_Tdel;
            int wire_type = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
            float dummy, basecost = 0;
            printf("\tid:%d - %d\tstart(%d,%d)\tend(%d,%d)\tbackTdel:%einodeTdel:%e\tactual future Tdel:%.3f\test future Tdel:%.3f\n", 
                    inode, wire_type, ix_s, iy_s, ix_e, iy_e,
                    bTdel, (bTdel - bbTdel), 
                    (actual_Tdel - bTdel) * pow(10, 10), 
                    get_timing_driven_future_Tdel(inode, target_node, &dummy, &basecost) * pow(10, 10));
            inode = prev_inode;
        }
    }
}

void setup_new_lookahead_dev(int furthest_wire_inode, int target_node, float total_Tdel, int new_nodes_count) {
    /*
    float estimated_future_cost = rr_node_route_inf[inode].path_cost 
                            - rr_node_route_inf[inode].backward_path_cost;
    float actual_future_cost = actual_tot_cost 
                            - rr_node_route_inf[inode].backward_path_cost;
    */
    float new_nodes_dev = 0.;
    float estimated_future_cost, actual_future_cost;
    float c_downstream, basecost;
    estimated_future_cost = get_timing_driven_future_Tdel(furthest_wire_inode, target_node, &c_downstream, &basecost);
    actual_future_cost = total_Tdel - rr_node_route_inf[furthest_wire_inode].back_Tdel;

    if (subtree_count.count(new_nodes_count) <= 0) {
        subtree_count[new_nodes_count] = 0;
        subtree_size_avg[new_nodes_count] = 0;
        subtree_est_dev_avg[new_nodes_count] = 0;
    }
    if (actual_future_cost != 0) {
        new_nodes_dev = (estimated_future_cost / actual_future_cost - 1);
    }
    subtree_count[new_nodes_count] ++;
    int tree_counter = subtree_count[new_nodes_count];
    // running avg: geo mean for subtree size; arithmetic mean for dev
    if (subtree_size_avg[new_nodes_count] == 0) {
        subtree_size_avg[new_nodes_count] = subtree_size_avg[0];
    } else {
        float temp = pow(subtree_size_avg[new_nodes_count], (tree_counter - 1) / (float)tree_counter);
        subtree_size_avg[new_nodes_count] = temp * pow(subtree_size_avg[0], 1.0/(float)tree_counter);
    }
    if (target_node == DB_TARGET_NODE) {
        print_db_node_inf(DB_TARGET_NODE);
    }
    float cur_dev = new_nodes_dev;
    if (cur_dev < -0.6 && new_nodes_count > 10)
        printf("\nXXXXX TARGET NODE %d\n\n", target_node);
    subtree_est_dev_avg[new_nodes_count] = ((tree_counter - 1) * subtree_est_dev_avg[new_nodes_count]
                                    + cur_dev) / (float)tree_counter;
}
