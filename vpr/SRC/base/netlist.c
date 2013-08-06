/* 
 * This file contains subroutines used to populate data structures
 * that are used to store netlists information.
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


static bool check_global_net_with_array(INP t_net* net_arr,
	INP int num_net_arr, OUTP t_netlist* g_nlist);

void load_global_net_from_array(INP t_net* net_arr,
	INP int num_net_arr, OUTP t_netlist* g_nlist){

	int i, j;
		
	if(g_nlist == NULL){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Global netlist variable has not been allocated!");
	}

	// Net vector should be not have been allocated
	if(!g_nlist->net.empty()){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Nets vector of global netlist variable should be empty prior to loading!");
	}

	// Resize vector memory for nets in array
	g_nlist->net.resize(num_net_arr);

	for(i = 0; i < num_net_arr; i++){
		g_nlist->net[i].name = my_strdup(net_arr[i].name);
		g_nlist->net[i].is_routed = net_arr[i].is_routed;
		g_nlist->net[i].is_fixed = net_arr[i].is_fixed;
		g_nlist->net[i].is_global = net_arr[i].is_global;
		g_nlist->net[i].is_const_gen = net_arr[i].is_const_gen;

		// Power info may be removed from net ?
		//g_nlist->nets[i].net_power = new t_net_power(*net_arr[i].net_power);

		assert(g_nlist->net[i].nodes.empty());
		g_nlist->net[i].nodes.resize(net_arr[i].num_sinks + 1);
		for(j = 0; j <= net_arr[i].num_sinks; j++){

			g_nlist->net[i].nodes[j].block = net_arr[i].node_block[j];

			// Usage exception: post-packed netlist of CLB's do not make
			//			use of node_block_port array (not allocated).
			if(net_arr[i].node_block_port) 
				g_nlist->net[i].nodes[j].block_port = net_arr[i].node_block_port[j];

			g_nlist->net[i].nodes[j].block_pin = net_arr[i].node_block_pin[j];
		}
	}

	if(!check_global_net_with_array(net_arr, num_net_arr, g_nlist))
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Internal error while loading global net vector from net array");

	return;
}

void echo_global_nlist_net(INP t_netlist* g_nlist){

	unsigned int i, j;

	vpr_printf_info("********Dumping clb netlist info contained in vectors*******\n");

	for(i = 0; i < g_nlist->net.size(); i++){
		vpr_printf_info("Net name %s %s\n", g_nlist->net[i].name, clb_net[i].name);
		vpr_printf_info("Routed %d %d \nfixed %d %d \nglobal %d %d const_gen %d %d\n", 
			g_nlist->net[i].is_routed, clb_net[i].is_routed,
			g_nlist->net[i].is_fixed , clb_net[i].is_fixed,
			g_nlist->net[i].is_global, clb_net[i].is_global,  
			g_nlist->net[i].is_const_gen, clb_net[i].is_const_gen);
		for(j = 0; j < g_nlist->net[i].nodes.size(); j++){
			vpr_printf_info("Block index %d %d port %d %d pin %d %d \n", 
				g_nlist->net[i].nodes[j].block, clb_net[i].node_block[j],
				g_nlist->net[i].nodes[j].block_port, (clb_net[i].node_block_port ? clb_net[i].node_block_port[j] : 0), 
				g_nlist->net[i].nodes[j].block_pin, clb_net[i].node_block_pin[j]);
		
		}
		vpr_printf_info("\n");
	}


	vpr_printf_info("********Finished dumping clb netlist info contained in vectors*******\n");
}

static bool check_global_net_with_array(INP t_net* net_arr,
	INP int num_net_arr, OUTP t_netlist* g_nlist){

	int i, j;

	for(i = 0; i < num_net_arr; i++){
		if(strcmp(net_arr[i].name,g_nlist->net[i].name))
			return false;
		if(net_arr[i].is_routed != g_nlist->net[i].is_routed)
			return false;
		if(net_arr[i].is_fixed != g_nlist->net[i].is_fixed)
			return false;
		if(net_arr[i].is_global != g_nlist->net[i].is_global)
			return false;
		if(net_arr[i].is_const_gen != g_nlist->net[i].is_const_gen)
			return false;

		for(j = 0; j <= net_arr[i].num_sinks; j++){
			if(net_arr[i].node_block[j] != g_nlist->net[i].nodes[j].block)
				return false;

			if(net_arr[i].node_block_port)
				if(net_arr[i].node_block_port[j] != g_nlist->net[i].nodes[j].block_port)
					return false;

			if(net_arr[i].node_block_pin[j] != g_nlist->net[i].nodes[j].block_pin)
				return false;
		}
	
	}

	assert(num_net_arr == (int)g_nlist->net.size());

	return true;
}