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

void free_global_nlist_net(t_netlist* nlist) {
	if(nlist == NULL)
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"Global netlist variable has not been allocated!");

	if(!nlist->net.empty()) {
		for(unsigned int i = 0; i < nlist->net.size(); i++) {
			nlist->net[i].pins.clear();
		}
		nlist->net.clear();
	}
}
