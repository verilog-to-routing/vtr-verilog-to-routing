/* 
 * Date:	Aug 19th, 2013
 * Author:	Daniel Chen
 *
 * This file contains subroutines used to populate the global netlist 
 * data structures. See netlist.h for more info about data structure. 
 */


#include <cstdio>
#include <cstring>

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"

#include "netlist.h"
#include "vpr_api.h"


using namespace std;


void echo_global_nlist_net(const t_netlist* g_nlist){

	unsigned int i, j;

	if(g_nlist == NULL){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Global netlist variable has not been allocated!");
	}

	vtr::printf_info("********Dumping clb netlist info contained in vectors*******\n");

	for(i = 0; i < g_nlist->net.size(); i++){
		vtr::printf_info("Net name %s\n", g_nlist->net[i].name);
		vtr::printf_info("Routed %d fixed %d global %d const_gen %d\n", 
			g_nlist->net[i].is_routed,
			g_nlist->net[i].is_fixed ,
			g_nlist->net[i].is_global, 
			g_nlist->net[i].is_const_gen);
		for(j = 0; j < g_nlist->net[i].pins.size(); j++){
			vtr::printf_info("Block index %d port %d pin %d \n", 
				g_nlist->net[i].pins[j].block, 
				g_nlist->net[i].pins[j].block_port,  
				g_nlist->net[i].pins[j].block_pin);
		
		}
		vtr::printf_info("\n");
	}
	vtr::printf_info("********Finished dumping clb netlist info contained in vectors*******\n");
}

void free_global_nlist_net(t_netlist* g_nlist){
	
	if(g_nlist == NULL){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Global netlist variable has not been allocated!");
	}

	if(!g_nlist->net.empty()){
		for(unsigned int i = 0; i < g_nlist->net.size(); i++){
			free(g_nlist->net[i].name);
			g_nlist->net[i].name = NULL;
			g_nlist->net[i].pins.clear();
		}
		g_nlist->net.clear();
	}

}
