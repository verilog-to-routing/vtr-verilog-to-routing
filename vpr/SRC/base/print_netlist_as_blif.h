/* 
Print VPR netlist as blif so that formal equivalence checking can be done to verify that the input blif netlist == the internal VPR netlist

Author: Jason Luu
Date: Oct 14, 2013
 */

#ifndef PRINT_NELIST_AS_BLIF_H
#define PRINT_NELIST_AS_BLIF_H

void print_preplace_netlist(INP const t_arch *Arch, INP const char *filename);

#endif

