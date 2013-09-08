/* 
 * Date:	Aug 19th, 2013
 * Author:	Daniel Chen
 * 
 * This file contains the global netlist data structures and its associated 
 * load/echo functions. t_netlist contains 2 vectors: one contains all the nets 
 * the other the blocks. To-do: implement block data structure. 
 */


#ifndef NETLIST_H
#define NETLIST_H

#include "vpr_types.h"
#include <vector>
using namespace std;

/* Contains information about each pin a net is connected to. 
 * block:		net.pins[0..pins.size()-1].block. 
 *				Block to which the nodes of this pin connect. The source 
 *				block is net.pins[0].block and the sink blocks are the remaining pins.
 * block_port:  net.pins[0..pins.size()-1].block_port.
 *				Port index (on a block) to which each net terminal connects. 
 * block_pin:   net.pins[0..pins.size()-1].block_pin. 
 *				Pin index (on a block) to which each net terminal connects. 
 */
struct t_net_pin{
	int block;
	int block_port;
	int block_pin;

	t_net_pin(){
		block = block_port = block_pin = UNDEFINED;
	}
};

/* Contains basic information about the net properties and its pin connections
 * name:		 ASCII net name for informative annotations in the output.          
 * is_routed:	 not routed (has been pre-routed)
 * is_fixed:	 not routed (has been pre-routed)
 * is_global:	 not routed
 * is_const_gen: constant generator (does not affect timing) 
 * t_net_pin:  [0..pins.size()-1]. Contains the nodes this net connects to.
 */
struct t_vnet{
	char* name;
	unsigned int is_routed    : 1;
	unsigned int is_fixed     : 1;
	unsigned int is_global    : 1;
	unsigned int is_const_gen : 1;
	vector<t_net_pin> pins;
	t_net_power * net_power; // Daniel to-do: Take out?

	t_vnet(){
		name = NULL;
		is_routed = is_fixed = is_global = is_const_gen = 0;
		net_power = NULL;
	}

	// Returns the number of sinks of the net, computed by looking 
	// at the the nodes vector's size. 
	int num_sinks(){
		return (int) (pins.size() ? pins.size() - 1 : 0);
	}
};

/* 
 * Note: Indices for t_netlist.net[] are also used as ID's/indices in 
 * 		  several other parallel(global) data structures, e.g.   
 * 		  net_rr_terminals[]. 
 */

struct t_netlist{
	//vector<t_blocks> blocks; To-do: Need to implement later
	vector<t_vnet>  net;
};

void load_global_net_from_array(INP t_net* net_arr,
	INP int num_net_arr, OUTP t_netlist* g_nlist);

void echo_global_nlist_net(INP t_netlist* g_nlist);

void free_global_nlist_net(INP t_netlist* g_nlist);

#endif
