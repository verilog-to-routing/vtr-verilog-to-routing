// A. Hurst ahurst@eecs.berkeley.edu

#ifndef ABC__phys__place__libhmetis_h
#define ABC__phys__place__libhmetis_h


ABC_NAMESPACE_HEADER_START


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



ABC_NAMESPACE_HEADER_END

#endif
