// A. Hurst ahurst@eecs.berkeley.edu

#ifndef LIBHMETIS_H_
#define LIBHMETIS_H_

static void HMETIS_PartRecursive(int nvtxs, 
			  int nhedges, 
			  int *vwgts, 
			  int *eptr,
			  int *eind,
			  int *hewgts, 
			  int nparts, 
			  int nbfactor, 
			  int *options, 
			  int *part, 
              int *edgecnt ) {} //;


static void HMETIS_PartKway(int nvtxs, 
		     int nhedges, 
		     int *vwgts, 
		     int *eptr, 
		     int *eind,
		     int *hewgts, 
		     int nparts, 
		     int nbfactor, 
		     int *options, 
		     int *part, 
             int *edgecnt ) {} //;

#endif
