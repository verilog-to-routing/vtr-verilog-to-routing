/**
 * Author: Jason Luu
 * Date: May 2009
 * 
 * Read a circuit netlist in XML format and populate the netlist data structures for VPR
 */

#ifndef READ_NETLIST_H
#define READ_NETLIST_H

void read_netlist(const char *net_file, 
		const t_arch *arch,
		int *L_num_blocks, 
		struct s_block *block_list[],
		int *L_num_nets, 
		struct s_net *net_list[]);

#endif

