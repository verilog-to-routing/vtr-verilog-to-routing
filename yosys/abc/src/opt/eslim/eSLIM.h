/**CFile****************************************************************

  FileName    [eSLIM.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Interface to the eSLIM package.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: eSLIM.h,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__OPT__ESLIM__ESLIM_h
#define ABC__OPT__ESLIM__ESLIM_h

#include "misc/util/abc_namespaces.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_HEADER_START

  typedef struct eSLIM_ParamStruct_ eSLIM_ParamStruct;
  struct eSLIM_ParamStruct_ {
    int forbidden_pairs;                                // Allow forbidden pairs in the selected subcircuits
    int extended_normality_processing;                  // Additional checks for non normal subcircuits
    int fill_subcircuits;                               // If a subcircuit has fewer than subcircuit_size_bound gates, try to fill it with rejected gates.
    int apply_strash;                                   
    int fix_seed;                                       
    int trial_limit_active;
    int apply_inprocessing;
    
    unsigned int timeout;                               // available time in seconds (soft limit)
    unsigned int timeout_inprocessing;
    unsigned int iterations;                            // maximal number of iterations. No limit if 0
    unsigned int subcircuit_size_bound;                 // upper bound for the subcircuit sizes
    unsigned int strash_intervall;    
    unsigned int nselection_trials;
    unsigned int nruns;

    double expansion_probability;                       // the probability that a node is added to the subcircuit

    int mode;                                           // 0: Cadical Incremental, 1: Kissat Oneshot                                       
    int seed;
    int verbosity_level;                                        

  };

  void seteSLIMParams(eSLIM_ParamStruct* params);

  Gia_Man_t* applyeSLIM(Gia_Man_t * pGia, const eSLIM_ParamStruct* params);
  Gia_Man_t* applyeSLIMIncremental(Gia_Man_t * pGia, const eSLIM_ParamStruct* params, unsigned int restarts, unsigned int deepsynTimeout);


ABC_NAMESPACE_HEADER_END

#endif