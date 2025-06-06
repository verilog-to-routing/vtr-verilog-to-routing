/**CFile****************************************************************

  FileName    [utils.hpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Utilities for the eSLIM package.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: utils.hpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__OPT__ESLIM__UTILS_h
#define ABC__OPT__ESLIM__UTILS_h

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "misc/util/abc_namespaces.h"
#include "misc/vec/vec.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  struct eSLIMConfig {                           
    bool extended_normality_processing = false;                   
    bool apply_strash = true;                                   
    bool fix_seed = false;    
    bool fill_subcircuits = false;                              
    bool trial_limit_active = true;
    bool allow_xors = false;               

    unsigned int timeout = 3600;                    
    unsigned int iterations = 0;                          
    unsigned int subcircuit_size_bound = 6;            
    unsigned int strash_intervall = 100;    
    int seed = 0;
    unsigned int nselection_trials = 100;
    double expansion_probability = 0.6;  

    // times given in sec
    int minimum_sat_timeout = 1;
    int base_sat_timeout = 120;
    int minimum_dynamic_timeout_sample_size = 50;
    double dynamic_timeout_buffer_factor = 1.4;

    int verbosity_level = 0;
  };

  struct eSLIMLog {
    unsigned int iteration_count = 0;
    double relation_generation_time = 0;
    double synthesis_time = 0;
    unsigned int subcircuits_with_forbidden_pairs = 0;

    std::vector<int> nof_analyzed_circuits_per_size;
    std::vector<int> nof_replaced_circuits_per_size;
    std::vector<int> nof_reduced_circuits_per_size;

    std::vector<ABC_UINT64_T> cummulative_sat_runtimes_per_size;
    std::vector<int> nof_sat_calls_per_size;
    std::vector<ABC_UINT64_T> cummulative_unsat_runtimes_per_size;
    std::vector<int> nof_unsat_calls_per_size;

    eSLIMLog(int size);
  };

  struct Subcircuit {
    Vec_Int_t* nodes;
    Vec_Int_t* io;
    unsigned int nof_inputs;
    std::unordered_map<int, std::unordered_set<int>> forbidden_pairs;
    void free();
  };

  inline void Subcircuit::free() {
    Vec_IntFree(nodes);
    Vec_IntFree(io);
  }

  inline eSLIMLog::eSLIMLog(int size) 
          : nof_analyzed_circuits_per_size(size + 1, 0), nof_replaced_circuits_per_size(size + 1, 0),
            nof_reduced_circuits_per_size(size + 1, 0), cummulative_sat_runtimes_per_size(size + 1, 0),
            nof_sat_calls_per_size(size + 1, 0), cummulative_unsat_runtimes_per_size(size + 1, 0), 
            nof_unsat_calls_per_size(size + 1, 0) {
  }

}

ABC_NAMESPACE_CXX_HEADER_END

#endif