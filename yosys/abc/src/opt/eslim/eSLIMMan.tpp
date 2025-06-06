/**CFile****************************************************************

  FileName    [eSLIMMan.tpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [eSLIM manager.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: eSLIMMan.tpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#include <cassert>
#include <queue>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <iostream>
#include <climits>

#include "eSLIMMan.hpp"
#include "synthesisEngine.hpp"
#include "relationGeneration.hpp"


ABC_NAMESPACE_HEADER_START
  Mini_Aig_t * Mini_AigDupCompl( Mini_Aig_t * p, int ComplIns, int ComplOuts );
  int Exa6_ManFindPolar( word First, int nOuts );
  Vec_Wrd_t * Exa6_ManTransformInputs( Vec_Wrd_t * vIns );
  Vec_Wrd_t * Exa6_ManTransformOutputs( Vec_Wrd_t * vOuts, int nOuts );
ABC_NAMESPACE_HEADER_END

ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  template <typename Y, typename R, typename S>
  Gia_Man_t* eSLIM_Man<Y, R, S>::applyeSLIM(Gia_Man_t * p, const eSLIMConfig& cfg, eSLIMLog& log) {
    eSLIM_Man<Y, R, S> eslim(p, cfg, log);
    eslim.minimize();
    return eslim.getCircuit();
  }



  template <typename Y, typename R, typename S>
  eSLIM_Man<Y, R, S>::eSLIM_Man(Gia_Man_t * pGia, const eSLIMConfig& cfg, eSLIMLog& log) 
            : gia_man(pGia), cfg(cfg), log(log), 
              subcircuit_selection(gia_man, cfg, log) {
    if (cfg.fix_seed) {
      subcircuit_selection.setSeed(cfg.seed);
    }
    relation_generation_time = log.relation_generation_time;
    synthesis_time = log.synthesis_time;
  }
  
  template <typename Y, typename R, typename S>
  void eSLIM_Man<Y, R, S>::minimize() {
    abctime clkStart    = Abc_Clock();
    abctime nTimeToStop = clkStart + cfg.timeout * CLOCKS_PER_SEC;
    unsigned int iteration = 0;
    unsigned int iterMax = cfg.iterations ? cfg.iterations - 1 : UINT_MAX;
    int current_size = Gia_ManAndNum(gia_man);
    while (Abc_Clock() <= nTimeToStop && iteration <= iterMax && !stopeSLIM) {
      iteration++;
      findReplacement();
      if (Gia_ManHasDangling(gia_man)) {
        Gia_Man_t* pTemp = gia_man;
        gia_man = Gia_ManCleanup( pTemp );
        Gia_ManStop( pTemp );
      }
      if (cfg.apply_strash && iteration % cfg.strash_intervall == 0) {
        Gia_Man_t* pTemp = gia_man;
        gia_man = Gia_ManRehash( pTemp, 0 );
        Gia_ManStop( pTemp );
      }
      if (cfg.verbosity_level > 0) {
        int sz = Gia_ManAndNum(gia_man);
        if (sz < current_size) {
          current_size = sz;
          printf("\rIteration %8d : #and = %7d elapsed time = %7.2f sec\n", iteration, sz, (float)1.0*(Abc_Clock() - clkStart)/CLOCKS_PER_SEC);
        } else {
          printf("\rIteration %8d", iteration);
          fflush(stdout);
        }
      }
    }
    log.iteration_count += iteration;
    if (cfg.verbosity_level > 0) {
      int sz = Gia_ManAndNum(gia_man);
      printf("\r#Iterations %8d #and = %7d elapsed time = %7.2f sec\n", iteration, sz, (float)1.0*(Abc_Clock() - clkStart)/CLOCKS_PER_SEC);
      if (cfg.verbosity_level > 1) {
        printf("Relation generation time: %.2f sec\n", log.relation_generation_time - relation_generation_time);
        printf("Synthesis time: %.2f sec\n", log.synthesis_time - synthesis_time);
      } 
    }
  }

  template <typename Y, typename R, typename S>
  Vec_Wrd_t* eSLIM_Man<Y, R, S>::getSimsIn(Abc_RData_t* relation) {
    Vec_Wrd_t* vSimsDiv = Vec_WrdStart( relation->nPats );
    for (int k = 0; k < relation->nIns; k++) {
      for (int i = 0; i < relation->nPats; i++) {
        if (Abc_RDataGetIn(relation, k, i)) {
          // The first bit corresponds to the constant divisor thus we have +1
          Abc_InfoSetBit((unsigned *)Vec_WrdEntryP(vSimsDiv, i), 1+k);
        }
      }
    }
    return vSimsDiv;
  }
  
  template <typename Y, typename R, typename S>
  Vec_Wrd_t* eSLIM_Man<Y, R, S>::getSimsOut(Abc_RData_t* relation) {
    Vec_Wrd_t* vSimsOut = Vec_WrdStart(relation->nPats);
    for (int k = 0; k < (1 << relation->nOuts); k++) {
      for (int i = 0; i < relation->nPats; i++) {
        if (Abc_RDataGetOut(relation, k, i)) {
          Abc_InfoSetBit((unsigned *)Vec_WrdEntryP(vSimsOut, i), k);
        }
      }
    }
    return vSimsOut;
  }

  template <typename Y, typename R, typename S>
  void eSLIM_Man<Y, R, S>::findReplacement() {
    Subcircuit subcir = subcircuit_selection.getSubcircuit();
    if (!subcircuit_selection.getStatus()) {
      if (cfg.trial_limit_active) {
        stopeSLIM = true;
      }
      return;
    }
    Mini_Aig_t* replacement = findMinimumAig(subcir);
    if (replacement != nullptr) {
      insertReplacement(replacement, subcir);
      Mini_AigStop(replacement);
    }
    subcir.free();
  }

  // Based on Exa_ManExactSynthesis6Int and Exa_ManExactSynthesis6
  template <typename Y, typename R, typename S>
  Mini_Aig_t* eSLIM_Man<Y, R, S>::findMinimumAig(const Subcircuit& subcir) {

    if (subcir.forbidden_pairs.size() > 0) {
      log.subcircuits_with_forbidden_pairs++;
    }
    
    abctime relation_generation_start = Abc_Clock();
    Abc_RData_t* relation = RelationGenerator<R>::computeRelation(gia_man, subcir);
    log.relation_generation_time += (double)1.0*(Abc_Clock() - relation_generation_start)/CLOCKS_PER_SEC;

    int nDivs = 0;
    int nOuts = relation->nOuts;
    int nVars = relation->nIns;
    if ( nVars == 0 ) {
      return nullptr;
    }
    // assert (nVars <= 8);
    Vec_Wrd_t* vSimsDiv = getSimsIn(relation);
    Vec_Wrd_t* vSimsOut = getSimsOut(relation);

    word original_all_false_behaviour = vSimsOut->pArray[0];
    // If the original circuit computed a non-normal circuit, but the relations allows to set the outputs to false in the all inputs false case
    // ABC does not "negate" the relation but forces the circuit to yield (0,...,0) for (0,...,0).
    // As a consequence it is not necessarily possible to find a same sized circuit.
    // In order to prevent this behaviour, we first modify the relation such that for the (0,...,0) input pattern only the original output patterns is allowed.
    vSimsOut->pArray[0] = getAllFalseBehaviour(subcir);

    int DivCompl = (int)Vec_WrdEntry(vSimsDiv, 0) >> 1;
    int OutCompl = Exa6_ManFindPolar( Vec_WrdEntry(vSimsOut, 0), nOuts );
    Vec_Wrd_t* vSimsDiv2 = Exa6_ManTransformInputs( vSimsDiv );
    Vec_Wrd_t* vSimsOut2 = Exa6_ManTransformOutputs( vSimsOut, nOuts );
    Mini_Aig_t* pMini = nullptr;
    int original_size = Vec_IntSize(subcir.nodes);
    int size = original_size;

    if (log.nof_analyzed_circuits_per_size.size() < original_size + 1) {
      log.nof_analyzed_circuits_per_size.resize(original_size + 1, 0);
      log.nof_replaced_circuits_per_size.resize(original_size + 1, 0);
      log.nof_reduced_circuits_per_size.resize(original_size + 1, 0);
    }
    log.nof_analyzed_circuits_per_size[original_size]++;
    
    abctime synthesis_start = Abc_Clock();
    std::tie(size, pMini) = reduce(vSimsDiv2, vSimsOut2, subcir.forbidden_pairs, nVars, nDivs, nOuts, size);
    log.synthesis_time += (double)1.0*(Abc_Clock() - synthesis_start)/CLOCKS_PER_SEC;
    
    if (size > original_size) {
      // Could not find a replacement. This can be caused by a timeout.
      Abc_RDataStop(relation);
      Vec_WrdFreeP( &vSimsDiv );
      Vec_WrdFreeP( &vSimsOut );
      Vec_WrdFreeP( &vSimsDiv2 );
      Vec_WrdFreeP( &vSimsOut2 );
      return nullptr;
    }

    // It is possible that a better replacement is obtained with a non normal behaviour.
    bool check_original_all_false_behaviour = cfg.extended_normality_processing && vSimsOut->pArray[0] != original_all_false_behaviour;
    if (check_original_all_false_behaviour) {
      vSimsOut->pArray[0] = original_all_false_behaviour;
      int OutCompl_alt = Exa6_ManFindPolar( Vec_WrdEntry(vSimsOut, 0), nOuts );
      Vec_Wrd_t* vSimsOut2_alt = Exa6_ManTransformOutputs( vSimsOut, nOuts );
      abctime synthesis_start = Abc_Clock();
      // auto [sz, mini] = reduce(vSimsDiv2, vSimsOut2_alt, subcir.forbidden_pairs, nVars, nDivs, nOuts, size - 1, true);
      auto result = reduce(vSimsDiv2, vSimsOut2_alt, subcir.forbidden_pairs, nVars, nDivs, nOuts, size - 1);
      int sz = result.first;
      Mini_Aig_t* mini = result.second;
      log.synthesis_time += (double)1.0*(Abc_Clock() - synthesis_start)/CLOCKS_PER_SEC;
      if (mini != nullptr) {
        size = sz;
        if (pMini) {
          Mini_AigStop(pMini);
        }
        pMini = mini;
        OutCompl = OutCompl_alt;
      }
    }
    Abc_RDataStop(relation);

    if (pMini != nullptr) {
      Mini_Aig_t* pTemp = pMini;
      pMini = Mini_AigDupCompl(pTemp, DivCompl, OutCompl);
      Mini_AigStop(pTemp);    

      log.nof_replaced_circuits_per_size[original_size]++;
      if (size < original_size) {
        log.nof_reduced_circuits_per_size[original_size]++;
      }
    }
    Vec_WrdFreeP( &vSimsDiv );
    Vec_WrdFreeP( &vSimsOut );
    Vec_WrdFreeP( &vSimsDiv2 );
    Vec_WrdFreeP( &vSimsOut2 );
    return pMini;
  }

  template <typename Y, typename R, typename S>
  word eSLIM_Man<Y, R, S>::getAllFalseBehaviour(const Subcircuit& subcir) {
    Gia_ManIncrementTravId(gia_man);
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObjVec( subcir.nodes, gia_man, pObj, i ) {
      Gia_ObjSetTravIdCurrent(gia_man, pObj);
    }
    long assm_idx = 0;
    Gia_ManForEachObjVecStart(subcir.io, gia_man, pObj, i, subcir.nof_inputs) {
      int bit_idx = i - subcir.nof_inputs;
      if (getAllFalseBehaviourRec(pObj)) {
        assm_idx |= (1 << bit_idx);
      }
    }
    return 1L << assm_idx;
  }

  template <typename Y, typename R, typename S>
  bool eSLIM_Man<Y, R, S>::getAllFalseBehaviourRec(Gia_Obj_t * pObj) {
    bool valuefanin0, valuefanin1;
    if (Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin0(pObj))) {
      valuefanin0 = getAllFalseBehaviourRec(Gia_ObjFanin0(pObj));
    } else {
      valuefanin0 = false;
    }
    valuefanin0 = valuefanin0 != pObj->fCompl0; 
    if (Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin1(pObj))) {
      valuefanin1 = getAllFalseBehaviourRec(Gia_ObjFanin1(pObj));
    } else {
      valuefanin1 = false;
    }
    valuefanin1 = valuefanin1 != pObj->fCompl1;
    return valuefanin0 && valuefanin1;
  }

  template <typename Y, typename R, typename S>
  int eSLIM_Man<Y, R, S>::getInsertionLiteral(Gia_Man_t* gia_man, const Subcircuit& subcir, Mini_Aig_t* replacement, const std::vector<int>& replacement_values, int fanin_lit) {
    int fanin_idx = Abc_Lit2Var(fanin_lit);
    bool fanin_negated = Abc_LitIsCompl(fanin_lit);
    int fanin_value;
    if (fanin_idx == 0) { // constant fanin
      fanin_value = 0; 
    } else if (fanin_idx <= subcir.nof_inputs) {
      Gia_Obj_t* pObj = Gia_ManObj(gia_man, Vec_IntEntry(subcir.io, fanin_idx - 1));
      if (Gia_ObjIsTravIdCurrent(gia_man, pObj)) { //the node has already been added
        fanin_value = Gia_ManObj(gia_man, Vec_IntEntry(subcir.io, fanin_idx - 1))->Value;
      } else {
        return -1;
      }
    } else {
      if (replacement_values[fanin_idx - subcir.nof_inputs - 1] == -1) {
        return -1;
      }
      fanin_value = replacement_values[fanin_idx - subcir.nof_inputs - 1];
    }
    return Abc_LitNotCond( fanin_value, fanin_negated );
  }

  template <typename Y, typename R, typename S>
  std::vector<int> eSLIM_Man<Y, R, S>::processReplacement(Gia_Man_t* gia_man, Gia_Man_t* pNew, const Subcircuit& subcir, Mini_Aig_t* replacement, std::vector<int>&& to_process, std::vector<int>& replacement_values) {
    std::vector<int> to_process_next_it;
    for (int id : to_process) {
      int fanin0 = getInsertionLiteral(gia_man, subcir, replacement, replacement_values, Mini_AigNodeFanin0( replacement, id));
      int fanin1 = getInsertionLiteral(gia_man, subcir, replacement, replacement_values, Mini_AigNodeFanin1( replacement, id));
      if (fanin0 == -1 || fanin1 == -1) {
        to_process_next_it.push_back(id);
      } else {
        int idx = id - subcir.nof_inputs - 1;
        replacement_values[idx] = Gia_ManAppendAnd2(pNew, fanin0, fanin1);
      }
    }
    int i, j = 0;
    Mini_AigForEachPo(replacement, i) {
      int old_id = Vec_IntEntry(subcir.io, j + subcir.nof_inputs);
      j++;
      Gia_Obj_t* obj = Gia_ManObj(gia_man, old_id);
      int fanin0_lit = Mini_AigNodeFanin0( replacement, i);
      int fanin0_idx = Abc_Lit2Var(fanin0_lit);
      if (fanin0_idx == 0) { // constant fanin
        obj->Value = Gia_ManConst0(gia_man)->Value;
      } else if (fanin0_idx <= subcir.nof_inputs) {
        Gia_Obj_t* fanin_obj = Gia_ManObj(gia_man, Vec_IntEntry(subcir.io, fanin0_idx - 1));
        if (Gia_ObjIsTravIdCurrent(gia_man, fanin_obj)) {
          obj->Value = fanin_obj->Value;
        } else {
          continue;
        }
      } else {
        if (replacement_values[fanin0_idx - subcir.nof_inputs - 1] == -1 ) {
          continue;
        } else {
          obj->Value = replacement_values[fanin0_idx - subcir.nof_inputs - 1];
        }
      }
      Gia_ObjSetTravIdCurrent(gia_man, obj);
      bool fanin0_negated = Abc_LitIsCompl(fanin0_lit);
      if (fanin0_negated) {
        obj->fMark0 = 1;
      }
    }
    return to_process_next_it;
  }

  template <typename Y, typename R, typename S>
  Vec_Int_t * eSLIM_Man<Y, R, S>::processEncompassing(Gia_Man_t* gia_man, Gia_Man_t* pNew,  Vec_Int_t * to_process) {
    int i;
    Gia_Obj_t * pObj;
    Vec_Int_t * to_process_remaining = Vec_IntAlloc( Vec_IntSize(to_process) );
    Gia_ManForEachObjVec( to_process, gia_man, pObj, i ) {
      if (Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin0(pObj)) && Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin1(pObj))) {
        Gia_ObjSetTravIdCurrent(gia_man, pObj);
        bool fanin0_negated = Gia_ObjFaninC0(pObj) ^ Gia_ObjFanin0(pObj)->fMark0;
        bool fanin1_negated = Gia_ObjFaninC1(pObj) ^ Gia_ObjFanin1(pObj)->fMark0;
        int fanin0 = Abc_LitNotCond(Gia_ObjFanin0(pObj)->Value, fanin0_negated);
        int fanin1 = Abc_LitNotCond(Gia_ObjFanin1(pObj)->Value, fanin1_negated);
        pObj->Value = Gia_ManAppendAnd2( pNew, fanin0, fanin1);
      } else {
        Vec_IntPush(to_process_remaining, Gia_ObjId(gia_man, pObj));
      }
    }
    Vec_IntFree(to_process);
    return to_process_remaining;
  }

  template <typename Y, typename R, typename S>
  void eSLIM_Man<Y, R, S>::insertReplacement(Mini_Aig_t* replacement, const Subcircuit& subcir) {
    // A miniaig contains a constant node, nodes for each PI/PO and for each and.
    int repalcement_size = Mini_AigNodeNum(replacement) - Vec_IntSize(subcir.io) - 1;
    int size_diff = Vec_IntSize(subcir.nodes) - repalcement_size;
    Gia_Man_t * pNew = Gia_ManStart( Gia_ManObjNum(gia_man) - size_diff );
    pNew->pName = Abc_UtilStrsav( gia_man->pName );
    pNew->pSpec = Abc_UtilStrsav( gia_man->pSpec );
    Gia_Obj_t * pObj;
    int i;
    Gia_ManConst0(gia_man)->Value = 0;
    Gia_ManForEachPi( gia_man, pObj, i ) {
      pObj->Value = Gia_ManAppendCi(pNew);
    }
    Gia_ManIncrementTravId(gia_man);
    Gia_ManForEachObjVec( subcir.nodes, gia_man, pObj, i ) {
      Gia_ObjSetTravIdCurrent(gia_man, pObj);
    }
    Gia_ManIncrementTravId(gia_man);
    Gia_ManForEachPi( gia_man, pObj, i ) {
      Gia_ObjSetTravIdCurrent(gia_man, pObj);
    }
    Vec_Int_t * to_process_encompassing  = Vec_IntAlloc( gia_man->nObjs );
    Gia_ManForEachAnd( gia_man, pObj, i ) {
      // nodes from the subcircuit shall not be added
      if (Gia_ObjIsTravIdPrevious(gia_man, pObj)) {
        continue;
      } else if (Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin0(pObj)) && Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin1(pObj))) {
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ObjSetTravIdCurrent(gia_man, pObj);
      } else {
        Vec_IntPush(to_process_encompassing, Gia_ObjId(gia_man, pObj));
      }
    }
    std::vector<int> to_process_replacement;
    Mini_AigForEachAnd(replacement, i) {
      to_process_replacement.push_back(i);
    }
    std::vector<int> replacement_values(to_process_replacement.size(), -1);

    while (Vec_IntSize(to_process_encompassing) > 0 || to_process_replacement.size() > 0) {
      to_process_replacement = processReplacement(gia_man, pNew, subcir, replacement, std::move(to_process_replacement), replacement_values);
      to_process_encompassing = processEncompassing(gia_man, pNew, to_process_encompassing);
    }
    Vec_IntFree(to_process_encompassing);
    int po_idx = 0;
    Mini_AigForEachPo(replacement, i) {
      Gia_Obj_t* pObj = Gia_ManObj(gia_man, Vec_IntEntry(subcir.io, subcir.nof_inputs + po_idx));
      if (!Gia_ObjIsTravIdCurrent(gia_man, pObj)) {
        int fanin_lit = Mini_AigNodeFanin0( replacement, i);
        int fanin_idx = Abc_Lit2Var(fanin_lit);
        assert (fanin_idx <= subcir.nof_inputs);
        Gia_Obj_t* fanin_obj = Gia_ManObj(gia_man, Vec_IntEntry(subcir.io, fanin_idx - 1));
        pObj->Value = fanin_obj->Value;
        if (Abc_LitIsCompl(fanin_lit)) {
          pObj->fMark0 = 1;
        }
        Gia_ObjSetTravIdCurrent(gia_man, pObj);
      }
      po_idx++;
    }
    
    Gia_ManForEachPo( gia_man, pObj, i ) {
      assert(Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin0(pObj)));
      bool fanin_negated = Gia_ObjFaninC0(pObj) ^ Gia_ObjFanin0(pObj)->fMark0;
      int fanin0 = Abc_LitNotCond(Gia_ObjFanin0(pObj)->Value, fanin_negated);
      pObj->Value = Gia_ManAppendCo( pNew, fanin0 );
    }
    Gia_ManStop(gia_man);
    gia_man = pNew;
  }

  template <typename Y, typename R, typename S>
  std::pair<int, Mini_Aig_t*> eSLIM_Man<Y, R, S>::reduce(Vec_Wrd_t* vSimsDiv, Vec_Wrd_t* vSimsOut, const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, 
                                                        int nVars, int nDivs, int nOuts, int size) {
    Y synth_man(vSimsDiv,vSimsOut,nVars,1+nVars+nDivs,nOuts,size, forbidden_pairs, log, cfg);
    Mini_Aig_t* result = nullptr;
    while (size >= 0) {
      Mini_Aig_t* pTemp = computeReplacement(synth_man, size);
      if (pTemp) {
        if (result) {
          Mini_AigStop(result);
        }
        result = pTemp;
        size--;
      } else {
        break;
      }
    }
    return std::make_pair(size + 1, result);
  }

  template <typename Y, typename R, typename S>
  double eSLIM_Man<Y, R, S>::getDynamicTimeout(int size) {
    if (this->log.nof_sat_calls_per_size[size] > cfg.minimum_dynamic_timeout_sample_size) {
      return std::max(static_cast<double>(cfg.minimum_sat_timeout), static_cast<double>(log.cummulative_sat_runtimes_per_size[size]) / (1000*log.nof_sat_calls_per_size[size])); //logged timings are given in ms
    } else {
      return cfg.base_sat_timeout;
    }
  }

  template <typename Y, typename R, typename S>
  Mini_Aig_t* eSLIM_Man<Y, R, S>::computeReplacement(Y& syn_man, int size) {
    return syn_man.getCircuit(size, getDynamicTimeout(size));
  }

}

ABC_NAMESPACE_CXX_HEADER_END