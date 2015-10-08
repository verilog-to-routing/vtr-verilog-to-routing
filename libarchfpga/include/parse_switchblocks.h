#ifndef PARSE_SWITCHBLOCKS_H
#define PARSE_SWITCHBLOCKS_H

#include <vector>
#include "ezxml.h"


/**** Structs ****/
/* contains data passed in to the switchblock parser */
struct s_formula_data{
	int dest_W; 	/* number of potential wire segments we can connect to in the destination channel */
	int wire;	/* incoming wire index */
};


/**** Function Declarations ****/
/* Loads permutation funcs specified under Node into t_switchblock_inf */
void read_sb_switchfuncs( ezxml_t Node, t_switchblock_inf *sb );

/* Reads-in the wire connections specified for the switchblock in the xml arch file */
void read_sb_wireconns( t_arch_switch_inf *switches, int num_switches, ezxml_t Node, t_switchblock_inf *sb );

/* checks for correctness of switch block read-in from the XML architecture file */
void check_switchblock( t_switchblock_inf *sb );

/* returns integer result according to the specified formula and data */
int get_sb_formula_result( const char* formula, const s_formula_data &mydata );



#endif /* PARSE_SWITCHBLOCKS_H */
