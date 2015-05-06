#ifndef PARSE_SWITCHBLOCKS_H
#define PARSE_SWITCHBLOCKS_H

/**** Includes ****/
#include <vector>
#include "ezxml.h"


/**** Enums ****/


/**** Structs ****/
/* contains data passed in to the switchblock parser */
struct s_formula_data{
	int dest_W; 	/* W in destination channel */	//XXX why destination channel? why not source channel? also, is it W? or cardinality of destination segment set?
	int track;	/* incoming track index */
	int Fs;
	int Fc_in;
	int Fc_out;
};



/**** Classes ****/
class Switchblock_Lookup{	//XXX Switchblock_Conn is a better name?? This is more like a switchblock *connection* coordinate
public:
	int x_coord;	/* x coordinate of switchblock connection */
	int y_coord;	/* y coordinate of switchblock connection */
	int from_side;	/* source side of switchblock connection */
	int to_side;	/* target side of switchblock connection */
	int track_num;	/* from track */	//XXX rename to 'from_track_ind'

	/* Empty constructor initializes everything to 0 */
	Switchblock_Lookup(){
		x_coord = y_coord = from_side = to_side = track_num = 0;
	}

	/* Constructor for initializing member variables */
	Switchblock_Lookup(int set_x, int set_y, int set_from, int set_to, int set_track){
		x_coord = set_x;
		y_coord = set_y;
		from_side = set_from;
		to_side = set_to;
		track_num = set_track;
	}

	/* Function for setting the segment coordinates */
	void set_coords(int set_x, int set_y, int set_from, int set_to, int set_track){
		x_coord = set_x;
		y_coord = set_y;
		from_side = set_from;
		to_side = set_to;
		track_num = set_track;
	}

	/* Overload < operator which is used by std::map */
	bool operator < (const Switchblock_Lookup &obj) const;
}; /* class Switchblock_Lookup */
std::ostream& operator<<(std::ostream &os, const Switchblock_Lookup &obj);


/**** Typedefs ****/
typedef struct s_to_track_inf{
	int to_track;
	unsigned switch_ind;	
} t_to_track_inf;

/* Switchblock connections are made as [x][y][from_side][to_side][from_track_ind].
   The Switchblock_Lookup class specifies these dimensions.
   Furthermore, a source_track at a given 5-d coordinate may connect to multiple destination tracks so the value
   of the map is a vector of destination tracks.
   A matrix specifying connections for all switchblocks in an FPGA would be very large, and probably sparse,
   so we use a map to take advantage of the sparsity. */
typedef std::map< Switchblock_Lookup, std::vector< t_to_track_inf > > t_sb_connection_map;	//TODO switch to unordered map?


/**** Function Declarations ****/
/* Loads permutation funcs specified under Node into t_switchblock_inf */
void read_sb_switchfuncs( INP ezxml_t Node, INOUTP t_switchblock_inf *sb );

/* Reads-in the wire connections specified for the switchblock in the xml arch file */
void read_sb_wireconns( INP t_arch_switch_inf *switches, INP int num_switches, INP ezxml_t Node, INOUTP t_switchblock_inf *sb );

/* checks for correctness of switch block read-in from the XML architecture file */
void check_switchblock( INP t_switchblock_inf *sb );

/* returns integer result according to the specified formula and data */
int get_sb_formula_result( INP const char* formula, INP const s_formula_data &mydata );



#endif /* PARSE_SWITCHBLOCKS_H */
