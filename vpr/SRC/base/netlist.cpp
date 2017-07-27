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

void echo_global_nlist_net(const t_netlist* nlist) {
	unsigned int i, j;

	if(nlist == NULL)
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Global netlist variable has not been allocated!");

	vtr::printf_info("********Dumping clb netlist info contained in vectors*******\n");

	for(i = 0; i < nlist->net.size(); i++) {
		vtr::printf_info("Net name %s\n", nlist->net[i].name);
		vtr::printf_info("Routed %d fixed %d\n", 
			nlist->net[i].is_routed,
			nlist->net[i].is_fixed);
		for(j = 0; j < nlist->net[i].pins.size(); j++) {
			vtr::printf_info("Block index %d pin %d \n", 
				nlist->net[i].pins[j].block, 
				nlist->net[i].pins[j].block_pin);
		}
		vtr::printf_info("\n");
	}
	vtr::printf_info("********Finished dumping clb netlist info contained in vectors*******\n");
}

void free_global_nlist_net(t_netlist* nlist) {
	if(nlist == NULL)
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Global netlist variable has not been allocated!");

	if(!nlist->net.empty()) {
		for(unsigned int i = 0; i < nlist->net.size(); i++) {
			free(nlist->net[i].name);
			nlist->net[i].name = NULL;
			nlist->net[i].pins.clear();
		}
		nlist->net.clear();
	}
}
