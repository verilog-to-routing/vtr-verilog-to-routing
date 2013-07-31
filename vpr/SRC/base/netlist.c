/* 
 * This file contains subroutines used to populate vectors of data structures
 * that are used to store information about netlists.
 *
 *
 *
 */
#include <cstdio>
#include <cstring>
#include <assert.h>
#include "netlist.h"
#include "util.h"
#include "vpr_api.h"

using namespace std;

void load_global_net_from_array(INP t_net* net_arr,
	INP int num_net_arr, OUTP t_netlist* g_nlist){

	int i, j;
		
	if(g_nlist == NULL){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Global netlist variable has not been allocated!");
	}

	// Net vector should be not have been allocated
	if(!g_nlist->nets.empty()){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Nets vector of global netlist variable should be empty prior to loading!");
	}

	// Resize vector memory for nets in array
	g_nlist->nets.resize(num_net_arr);

	for(i = 0; i < num_net_arr; i++){
		g_nlist->nets[i].name = my_strdup(net_arr[i].name);
		g_nlist->nets[i].is_routed = net_arr[i].is_routed;
		g_nlist->nets[i].is_fixed = net_arr[i].is_fixed;
		g_nlist->nets[i].is_global = net_arr[i].is_global;
		g_nlist->nets[i].is_const_gen = net_arr[i].is_const_gen;
		g_nlist->nets[i].net_power = net_arr[i].net_power;

		assert(g_nlist->nets[i].nodes.empty());
		g_nlist->nets[i].nodes.resize(net_arr[i].num_sinks + 1);
		for(j = 0; j <= net_arr[i].num_sinks; j++){
			g_nlist->nets[i].nodes[j].block = net_arr[i].node_block[j];
			//g_nlist->nets[i].nodes[j].block_port = net_arr[i].node_block_port[j];
			g_nlist->nets[i].nodes[j].block_pin = net_arr[i].node_block_pin[j];
		}
	}

	return;
}

void echo_global_nlist_net(INP t_netlist* g_nlist){
	vector<t_vnet>::iterator cur_net;
	vector<t_net_nodes>::iterator cur_node;

	vpr_printf_info("********Dumping clb netlist info contained in vectors*******\n");

	for(cur_net = g_nlist->nets.begin(); cur_net != g_nlist->nets.end(); cur_net++){
		vpr_printf_info("Net name %s\n", (*cur_net).name);
		vpr_printf_info("Routed %d fixed %d global %d const_gen %d\n", 
			(*cur_net).is_routed, (*cur_net).is_fixed ,(*cur_net).is_global, (*cur_net).is_const_gen);
		for(cur_node = (*cur_net).nodes.begin(); cur_node != (*cur_net).nodes.end(); cur_node++){
			vpr_printf_info("Block index %d port %d pin %d\n", 
				(*cur_node).block, (*cur_node).block_port, (*cur_node).block_pin);
		
		}
		vpr_printf_info("\n");
	}


	vpr_printf_info("********Finished dumping clb netlist info contained in vectors*******\n");
}