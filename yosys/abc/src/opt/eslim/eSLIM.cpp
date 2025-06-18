/**CFile****************************************************************

  FileName    [eSLIM.cpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Interface to the eSLIM package.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: eSLIM.cpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#include "utils.hpp"
#include "eSLIMMan.hpp"
#include "relationGeneration.hpp"
#include "selectionStrategy.hpp"

#include "misc/util/abc_namespaces.h"

#include "eSLIM.h"

ABC_NAMESPACE_HEADER_START
  Gia_Man_t * Gia_ManDeepSyn( Gia_Man_t * pGia, int nIters, int nNoImpr, int TimeOut, int nAnds, int Seed, int fUseTwo, int fVerbose );
ABC_NAMESPACE_HEADER_END

ABC_NAMESPACE_IMPL_START

eSLIM::eSLIMConfig getCfg(const eSLIM_ParamStruct* params) {
  eSLIM::eSLIMConfig config;
  config.extended_normality_processing = params->extended_normality_processing;
  config.apply_strash = params->apply_strash;
  config.fill_subcircuits = params->fill_subcircuits;
  config.fix_seed = params->fix_seed;
  config.trial_limit_active = params->trial_limit_active;
  config.timeout = params->timeout;
  config.iterations = params->iterations;
  config.subcircuit_size_bound = params->subcircuit_size_bound;
  config.strash_intervall = params->strash_intervall;
  config.nselection_trials = params->nselection_trials;
  config.seed = params->seed;
  config.verbosity_level = params->verbosity_level;
  config.expansion_probability = params->expansion_probability;
  return config;
}

void seteSLIMParams(eSLIM_ParamStruct* params) {
  params->forbidden_pairs = 1;
  params->extended_normality_processing = 0;
  params->fill_subcircuits = 0;
  params->apply_strash = 1;
  params->fix_seed = 0;
  params->trial_limit_active = 1;
  params->apply_inprocessing = 1;
  params->timeout = 1620;
  params->timeout_inprocessing = 180;
  params->iterations = 0;
  params->subcircuit_size_bound = 6;
  params->strash_intervall = 100;
  params->nselection_trials = 100;
  params->nruns = 1;
  params->mode = 0;
  params->seed = 0;
  params->verbosity_level = 1;
  params->expansion_probability = 0.4;           
}
   
Gia_Man_t* selectApproach(Gia_Man_t* pGia, eSLIM::eSLIMConfig params, eSLIM::eSLIMLog& log, int approach, bool allow_forbidden_pairs) {
  switch (approach) {
    case 0 : 
      if (allow_forbidden_pairs) {
        return eSLIM::eSLIM_Man<eSLIM::CadicalEngine, eSLIM::RelationGeneratorABC, eSLIM::randomizedBFSFP>::applyeSLIM(pGia, params, log);
      } else {
        return eSLIM::eSLIM_Man<eSLIM::CadicalEngine, eSLIM::RelationGeneratorABC, eSLIM::randomizedBFSnoFP>::applyeSLIM(pGia, params, log);
      };
    case 1:
      if (allow_forbidden_pairs) {
        return eSLIM::eSLIM_Man<eSLIM::KissatOneShot, eSLIM::RelationGeneratorABC, eSLIM::randomizedBFSFP>::applyeSLIM(pGia, params, log);
      } else {
        return eSLIM::eSLIM_Man<eSLIM::KissatOneShot, eSLIM::RelationGeneratorABC, eSLIM::randomizedBFSnoFP>::applyeSLIM(pGia, params, log);
      };
    case 2:
      if (allow_forbidden_pairs) {
        return eSLIM::eSLIM_Man<eSLIM::KissatCmdOneShot, eSLIM::RelationGeneratorABC, eSLIM::randomizedBFSFP>::applyeSLIM(pGia, params, log);
      } else {
        return eSLIM::eSLIM_Man<eSLIM::KissatCmdOneShot, eSLIM::RelationGeneratorABC, eSLIM::randomizedBFSnoFP>::applyeSLIM(pGia, params, log);
      };
    default:
      std::cerr << "eSLIM -- Invalid mode given\n";
      assert (false && "Invalid approach selected");
      return nullptr;
  }
}

Gia_Man_t* runInprocessing(Gia_Man_t * pGia, const eSLIM_ParamStruct* params, unsigned int it) {
  Gia_Man_t * tmp = Gia_ManDeepSyn( pGia, 1, ABC_INFINITY, params->timeout_inprocessing, 0, params->seed + it, 0, 0);
  if ( Gia_ManAndNum(pGia) > Gia_ManAndNum(tmp) ) {
    Gia_ManStop( pGia );
    pGia = tmp;
  } else {
    Gia_ManStop( tmp );
  }
  return pGia;
}

void printSetting(const eSLIM_ParamStruct* params) {
  std::cout << "Apply eSLIM (timeout: " << params->timeout << ") ";
  if (params->apply_inprocessing) {
    std::cout << "+ deepsyn (timeout: " << params->timeout_inprocessing << ") ";
  }
  std::cout << params->nruns << " times.\n";
  if (params->iterations > 0) {
    std::cout << "Stop eSLIM runs after " << params->iterations << " iterations.\n";
  }
  std::cout << "Consider subcircuits with up to " << params->subcircuit_size_bound << " gates.\n";
  printf("When expanding subcircuits, select gates with a probability of %.2f %%.\n", 100 * params->expansion_probability);
  if (params->forbidden_pairs) {
    std::cout << "Consider subcircuits containing gates, which are connected outside of the subcircuit.\n";
  }
  if (params->mode == 0) {
    std::cout << "Use an incremental SAT encoding for exact synthesis and the SAT solver CaDiCaL.\n";
  } else {
    std::cout << "Use a one-shot SAT encoding for exact synthesis and the SAT solver Kissat.\n";
  }
}

Gia_Man_t* applyeSLIM(Gia_Man_t * pGia, const eSLIM_ParamStruct* params) {
  eSLIM::eSLIMConfig config = getCfg(params);
  config.verbosity_level = params->verbosity_level;

  int eSLIM_reductions = 0;
  int deepsyn_reductions = 0;

  Gia_Man_t * pThis = Gia_ManDup(pGia);
  eSLIM::eSLIMLog log(params->subcircuit_size_bound);
  int initial_size = Gia_ManAndNum(pThis);
  if (params->verbosity_level > 0) {
    printSetting(params);
    std::cout << "Initial size: " << initial_size << "\n";
  }
  for (unsigned int i = 0; i < params->nruns; i++) {
    if (config.fix_seed) {
      config.seed = params->seed + i;
    }
    int size_iteration_start = Gia_ManAndNum(pThis);
    if (params->verbosity_level > 0) {
      std::cout << "Start eslim run " << i + 1 << "\n";
    }
    pThis = selectApproach(pThis, config, log, params->mode, params->forbidden_pairs);
    int size_eslim = Gia_ManAndNum(pThis);
    eSLIM_reductions += size_iteration_start - size_eslim;

    if (params->apply_inprocessing) {
      if (params->verbosity_level > 0) {
        std::cout << "Start inprocessing run " << i + 1 << "\n";
      }
      pThis = runInprocessing(pThis, params, i);
      if (params->verbosity_level > 0) {
        std::cout << "Inprocessing: Initial size: " << size_eslim << " final size: " << Gia_ManAndNum(pThis) << "\n";
      }
      deepsyn_reductions += size_eslim - Gia_ManAndNum(pThis);
    }
  }
  if (params->verbosity_level > 0) {
    int final_size = Gia_ManAndNum(pThis);
    std::cout << "Final size: " << final_size << "\n";
    int total_reductions = eSLIM_reductions + deepsyn_reductions;
    double reduction_ratio = 100 * static_cast<double>(total_reductions) / initial_size; 
    printf("Total reduction: %d (%.2f %%)\n", total_reductions, reduction_ratio);
    if (params->apply_inprocessing) {
      std::cout << "#Gates reduced by eSLIM: " << eSLIM_reductions << " ";
      std::cout << "#Gates reduced by deepsyn: " << deepsyn_reductions << "\n";
    } 
  }

  if (params->verbosity_level > 1) {
    std::cout << "Total #iterations: " << log.iteration_count << "\n";
    std::cout << "Total relation generation time (s): " << log.relation_generation_time << "\n";
    std::cout << "Total synthesis time (s): " << log.synthesis_time << "\n";
    std::cout << "#Iterations with forbidden pairs: " << log.subcircuits_with_forbidden_pairs << "\n";
  }
  if (params->verbosity_level > 2) {
    for (int i = 2; i < log.nof_analyzed_circuits_per_size.size(); i++) {
      int replaced = log.nof_replaced_circuits_per_size.size() > i ? log.nof_replaced_circuits_per_size[i] : 0;
      int reduced = log.nof_reduced_circuits_per_size.size() > i ? log.nof_reduced_circuits_per_size[i] : 0;
      std::cout << "#gates: " << i << " - #analysed: " << log.nof_analyzed_circuits_per_size[i] << " - #replaced: " << replaced << " - #reduced: " << reduced << "\n";
    }
    if (params->mode == 0) {
      for (int i = 0; i < log.nof_sat_calls_per_size.size(); i++) {
        if (log.nof_sat_calls_per_size[i] == 0) {
          std::cout << "#gates: " << i << " - avg. sat time: -\n";
        } else {
          std::cout << "#gates: " << i << " - avg. sat time: " << static_cast<double>(log.cummulative_sat_runtimes_per_size[i]) / (1000 * log.nof_sat_calls_per_size[i]) << " sec\n";
        }
      }
      for (int i = 0; i < log.nof_unsat_calls_per_size.size(); i++) {
        if (log.nof_unsat_calls_per_size[i] == 0) {
          std::cout << "#gates: " << i << " - avg. unsat time: -\n";
        } else {
          std::cout << "#gates: " << i << " - avg. unsat time: " << static_cast<double>(log.cummulative_unsat_runtimes_per_size[i]) / (1000* log.nof_unsat_calls_per_size[i]) << " sec\n";
        }
      }
    }
  }
  return pThis;
}

ABC_NAMESPACE_IMPL_END