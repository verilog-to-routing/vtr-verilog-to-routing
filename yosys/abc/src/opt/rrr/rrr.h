#pragma once

#include "rrrAndNetwork.h"
#include "rrrScheduler.h"
#include "rrrOptimizer.h"
#include "rrrBddAnalyzer.h"
#include "rrrBddMspfAnalyzer.h"
#include "rrrAnalyzer.h"
#include "rrrSatSolver.h"
#include "rrrSimulator.h"
#include "rrrPartitioner.h"
#include "rrrLevelBasePartitioner.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  template <typename Ntk>
  void Perform(Ntk *pNtk, Parameter const *pPar) {
    assert(!pPar->fUseBddCspf || !pPar->fUseBddMspf);
    switch(pPar->nPartitionType) {
    case 0:
      if(pPar->fUseBddCspf) {
        Scheduler<Ntk, Optimizer<Ntk, BddAnalyzer<Ntk>>, Partitioner<Ntk>> sch(pNtk, pPar);
        sch.Run();
      } else if(pPar->fUseBddMspf) {
        Scheduler<Ntk, Optimizer<Ntk, BddMspfAnalyzer<Ntk>>, Partitioner<Ntk>> sch(pNtk, pPar);
        sch.Run();
      } else {
        Scheduler<Ntk, Optimizer<Ntk, Analyzer<Ntk, Simulator<Ntk>, SatSolver<Ntk>>>, Partitioner<Ntk>> sch(pNtk, pPar);
        sch.Run();
      }
      break;
    case 1:
      if(pPar->fUseBddCspf) {
        Scheduler<Ntk, Optimizer<Ntk, BddAnalyzer<Ntk>>, LevelBasePartitioner<Ntk>> sch(pNtk, pPar);
        sch.Run();
      } else if(pPar->fUseBddMspf) {
        Scheduler<Ntk, Optimizer<Ntk, BddMspfAnalyzer<Ntk>>, LevelBasePartitioner<Ntk>> sch(pNtk, pPar);
        sch.Run();
      } else {
        Scheduler<Ntk, Optimizer<Ntk, Analyzer<Ntk, Simulator<Ntk>, SatSolver<Ntk>>>, LevelBasePartitioner<Ntk>> sch(pNtk, pPar);
        sch.Run();
      }
      break;
    default:
      assert(0);
    }
  }
  
}

ABC_NAMESPACE_CXX_HEADER_END
