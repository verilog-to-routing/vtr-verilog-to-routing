
#include <cmath>
#include <vector>
#include "vpr_types.h"
#include "timing_driven_lookup.h"

using namespace std;

typedef vector< vector<float> > t_delay_map;

/******** File-Scope Variables ********/

/* The delay map */
t_delay_map f_delay_map;




/******** File-Scope Functions ********/

/*

*/




/******** Function Definitions ********/

/*
	TODO: function for fetching a delay value
*/

void compute_timing_driven_lookahead(){

	/* 
	Compute delay table that spans the entire chip. 
	Mirror the approach in timing_place_lookup -- compute delays by placing sink at different points in the chip

	*/

}
