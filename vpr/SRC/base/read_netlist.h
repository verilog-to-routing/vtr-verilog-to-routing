/**
 * Author: Jason Luu
 * Date: May 2009
 * 
 * Read a circuit netlist in XML format and populate the netlist data structures for VPR
 */

#ifndef READ_NETLIST_H
#define READ_NETLIST_H

void read_netlist(INP const char *net_file, 
		INP const t_arch *arch,
		OUTP int *L_num_blocks, 
		OUTP struct s_block *block_list[],
		OUTP int *L_num_nets, 
		OUTP struct s_net *net_list[]);

void free_logical_blocks(void);
void free_logical_nets(void);

#endif

