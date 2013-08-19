/* 
 * Date:	Aug 19th, 2013
 * Author:	Daniel Chen
 * 
 * This file contains the global netlist data structures and its associated 
 * load/echo functions. Data members are organized similarly to the original
 * t_net, t_block, and t_logical_block(Only t_net has been rewritten at this
 * point). The definition and usage of the former t_net will no longer be 
 * global across VPR and will be kept file-scoped within read_blif.c and 
 * read_netlist.c. 
 */


#ifndef NETLIST_H
#define NETLIST_H

#include "vpr_types.h"
#include <vector>
using namespace std;

/*
 * block:		nodes[0..nodes.size()-1].block. 
 *				Block to which the nodes of this net connect. The source 
 *				block is nodes[0].block and the sink blocks are the remaining nodes.
 * block_port:  nodes[0..nodes.size()-1].block_port.
 *				Port index (on a block) to which each net terminal connects. 
 * block_pin:   nodes[0..nodes.size()-1].block_pin. 
 *				Pin index (on a block) to which each net terminal connects. 
 */
struct g_net_node{
	int block;
	int block_port;
	int block_pin;

	g_net_node(){
		block = block_port = block_pin = UNDEFINED;
	}
};

/* Adapted from original t_net:
 * name:		 ASCII net name for informative annotations in the output.          
 * is_routed:	 not routed (has been pre-routed)
 * is_fixed:	 not routed (has been pre-routed)
 * is_global:	 not routed
 * is_const_gen: constant generator (does not affect timing) 
 * g_net_nodes:  [0..nodes.size()-1]. Contains the nodes this net connects to.
 */
struct g_net{
	char* name;
	unsigned int is_routed    : 1;
	unsigned int is_fixed     : 1;
	unsigned int is_global    : 1;
	unsigned int is_const_gen : 1;
	vector<g_net_node> nodes;
	// Daniel to-do: Take out?
	t_net_power * net_power;

	g_net(){
		name = NULL;
		is_routed = is_fixed = is_global = is_const_gen = 0;
		net_power = NULL;
	}

	// Returns the number of sinks of the net, computed by looking 
	// at the the nodes vector's size. 
	int num_sinks(){
		return (int) (nodes.size() ? nodes.size() - 1 : 0);
	}
};

struct g_netlist{
	//vector<t_blocks> blocks; Need to implement later
	vector<g_net>  net;
};

void load_global_net_from_array(INP t_net* net_arr,
	INP int num_net_arr, OUTP g_netlist* g_nlist);

void echo_global_nlist_net(INP g_netlist* g_nlist, 
	t_net* net_arr);

#endif
