#include "aig/gia/gia.h"

#include "opt/rrr/rrr.h"
#include "opt/rrr/rrrAbc.h"

ABC_NAMESPACE_IMPL_START

Gia_Man_t *Gia_ManRrr(Gia_Man_t *pGia, int iSeed, int nWords, int nTimeout, int nSchedulerVerbose, int nPartitionerVerbose, int nOptimizerVerbose, int nAnalyzerVerbose, int nSimulatorVerbose, int nSatSolverVerbose, int fUseBddCspf, int fUseBddMspf, int nConflictLimit, int nSortType, int nOptimizerFlow, int nSchedulerFlow, int nPartitionType, int nDistance, int nJobs, int nThreads, int nPartitionSize, int nPartitionSizeMin, int fDeterministic, int nParallelPartitions, int fOptOnInsert, int fGreedy) {
  rrr::AndNetwork ntk;
  ntk.Read(pGia, rrr::GiaReader<rrr::AndNetwork>);
  rrr::Parameter Par;
  Par.iSeed = iSeed;
  Par.nWords = nWords;
  Par.nTimeout = nTimeout;
  Par.nSchedulerVerbose = nSchedulerVerbose;
  Par.nPartitionerVerbose = nPartitionerVerbose;
  Par.nOptimizerVerbose = nOptimizerVerbose;
  Par.nAnalyzerVerbose = nAnalyzerVerbose;
  Par.nSimulatorVerbose = nSimulatorVerbose;
  Par.nSatSolverVerbose = nSatSolverVerbose;
  Par.fUseBddCspf = fUseBddCspf;
  Par.fUseBddMspf = fUseBddMspf;
  Par.nConflictLimit = nConflictLimit;
  Par.nSortType = nSortType;
  Par.nOptimizerFlow = nOptimizerFlow;
  Par.nSchedulerFlow = nSchedulerFlow;
  Par.nPartitionType = nPartitionType;
  Par.nDistance = nDistance;
  Par.nJobs = nJobs;
  Par.nThreads = nThreads;
  Par.nPartitionSize = nPartitionSize;
  Par.nPartitionSizeMin = nPartitionSizeMin;
  Par.fDeterministic = fDeterministic;
  Par.nParallelPartitions = nParallelPartitions;
  Par.fOptOnInsert = fOptOnInsert;
  Par.fGreedy = fGreedy;
  rrr::Perform(&ntk, &Par);
  Gia_Man_t *pNew = rrr::CreateGia(&ntk);
  return pNew;
}

ABC_NAMESPACE_IMPL_END
