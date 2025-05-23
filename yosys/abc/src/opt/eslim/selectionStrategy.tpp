/**CFile****************************************************************

  FileName    [selectionStrategy.tpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Procedures for selecting subcircuits.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: selectionStrategy.tpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#include <queue>

#include "misc/util/abc_namespaces.h"

#include "selectionStrategy.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  template<typename T>
  Subcircuit SelectionStrategy<T>::getSubcircuit() {
    for (int i = 0; i < cfg.nselection_trials; i++) {
      Subcircuit subcir = static_cast<T*>(this)->getSubcircuitImpl();
      if (filterSubcircuit(subcir)) {
        return subcir;
      } else {
        subcir.free();
      }
    }
    status = false;
    Subcircuit empty_subcir;
    return empty_subcir;
  }

  template<typename T>
  bool SelectionStrategy<T>::filterSubcircuit(const Subcircuit& subcir) {
    // if (subcir.nof_inputs > 8) {
    if (subcir.nof_inputs > 10) {
      return false;
    }
    // ABC internally requires that the subcircuit has not more than 6 outputs (e.g. generateMinterm)
    if (Vec_IntSize(subcir.io)-subcir.nof_inputs > 6) {
      return false;
    }
    if (Vec_IntSize(subcir.nodes) < min_size) {
      return false;
    }
    return static_cast<T*>(this)->filterSubcircuitImpl(subcir);
  }

  template <typename T>
  int SelectionStrategy<T>::getSubcircuitIO(Vec_Int_t* subcircuit, Vec_Int_t* io) {
    assert(Vec_IntSize(io) == 0);
    Gia_ManIncrementTravId(gia_man);
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObjVec( subcircuit, gia_man, pObj, i ) {
      Gia_ObjSetTravIdCurrent(gia_man, pObj);
    }
    Gia_ManIncrementTravId(gia_man);

    // inputs
    Gia_ManForEachObjVec( subcircuit, gia_man, pObj, i ) {
      // fanin0 is not in the subcircuit and was not considered yet
      if (!Gia_ObjIsTravIdPrevious(gia_man, Gia_ObjFanin0(pObj)) && !Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin0(pObj))) {
        Gia_ObjSetTravIdCurrent(gia_man, Gia_ObjFanin0(pObj));
        Vec_IntPush(io, Gia_ObjId(gia_man, Gia_ObjFanin0(pObj)));
      } 
      // fanin1 is not in the subcircuit and was not considered yet
      if (!Gia_ObjIsTravIdPrevious(gia_man, Gia_ObjFanin1(pObj)) && !Gia_ObjIsTravIdCurrent(gia_man, Gia_ObjFanin1(pObj))) {
        Gia_ObjSetTravIdCurrent(gia_man, Gia_ObjFanin1(pObj));
        Vec_IntPush(io, Gia_ObjId(gia_man, Gia_ObjFanin1(pObj)));
      } 
    }
    int nof_inputs = Vec_IntSize(io);

    // outputs
    Gia_ManForEachAnd( gia_man, pObj, i ) {
      // If there is an object that is not contained in the subcircuit that has a fanin in the subcircuit then this fanin is an output
      if (!Gia_ObjIsTravIdPrevious(gia_man, pObj)) {
        if (Gia_ObjIsTravIdPrevious(gia_man, Gia_ObjFanin0(pObj))) {
          Gia_ObjSetTravIdCurrent(gia_man, Gia_ObjFanin0(pObj)); 
          Vec_IntPush(io, Gia_ObjId(gia_man, Gia_ObjFanin0(pObj)));
        }
        if (Gia_ObjIsTravIdPrevious(gia_man, Gia_ObjFanin1(pObj))) {
          Gia_ObjSetTravIdCurrent(gia_man, Gia_ObjFanin1(pObj)); 
          Vec_IntPush(io, Gia_ObjId(gia_man, Gia_ObjFanin1(pObj)));
        }
      }
    }

    Gia_ManForEachPo( gia_man, pObj, i ) {
      if (Gia_ObjIsTravIdPrevious(gia_man, Gia_ObjFanin0(pObj))) {
        Gia_ObjSetTravIdCurrent(gia_man, Gia_ObjFanin0(pObj)); 
        Vec_IntPush(io, Gia_ObjId(gia_man, Gia_ObjFanin0(pObj)));
      }
    }
    return nof_inputs;
  }

  template <typename T>
  std::unordered_map<int, std::unordered_set<int>> SelectionStrategy<T>::computeForbiddenPairs(const Subcircuit& subcir) {
    Gia_Obj_t * pObj;
    int i;
    unsigned int min_output_level = Gia_ManLevelNum(gia_man);
    Gia_ManIncrementTravId(gia_man);
    std::unordered_map<int,int> out_id;
    Gia_ManForEachObjVecStart(subcir.io, gia_man, pObj, i, subcir.nof_inputs) {
      min_output_level = Gia_ObjLevel(gia_man, pObj) < min_output_level ? Gia_ObjLevel(gia_man, pObj) : min_output_level;
      Gia_ObjSetTravIdCurrent(gia_man, pObj);
      auto id = Gia_ObjId(gia_man, pObj);
      out_id[id] = i - subcir.nof_inputs;
    }
    std::unordered_map<int, std::unordered_set<int>> forbidden_pairs;
    Gia_ManForEachObjVecStop(subcir.io, gia_man, pObj, i, subcir.nof_inputs) {
      forbiddenPairsRec(pObj, i, min_output_level, forbidden_pairs, out_id);
    }
    return forbidden_pairs;
  }

  template <typename T>
  void SelectionStrategy<T>::forbiddenPairsRec(Gia_Obj_t * pObj, int input, int min_level, std::unordered_map<int, std::unordered_set<int>>& pairs, const std::unordered_map<int,int>& out_id) {
    if (Gia_ObjIsTravIdCurrent(gia_man, pObj)) { 
      auto id = Gia_ObjId(gia_man, pObj);
      pairs[out_id.at(id)].insert(input);
      return;
    }
    if (Gia_ObjLevel(gia_man, pObj) < min_level) {
      return;
    }
    forbiddenPairsRec(Gia_ObjFanin0(pObj), input, min_level, pairs, out_id);
    forbiddenPairsRec(Gia_ObjFanin1(pObj), input, min_level, pairs, out_id);
  }

  template<typename T>
  int randomizedBFS<T>::selectRoot() {
    int nof_objs = Gia_ManObjNum(this->gia_man);
    int root_id = this->getUniformRandomNumber(this->nPis + 1, nof_objs - this->nPos - 1);
    return root_id;
  }

  template<typename T>
  void randomizedBFS<T>::checkCandidate(Subcircuit& subcircuit, std::queue<int>& candidates, std::queue<int>& rejected, bool add) {
    int obj_id = candidates.front();
    candidates.pop();
    Gia_Obj_t * gia_obj = Gia_ManObj(this->gia_man, obj_id);
    if (Gia_ObjIsAnd(gia_obj) && !Gia_ObjIsTravIdCurrent(this->gia_man, gia_obj)) {
      if (add) {
        Gia_ObjSetTravIdCurrent(this->gia_man, gia_obj);
        Vec_IntPush(subcircuit.nodes, obj_id);
        candidates.push(Gia_ObjFaninId0(gia_obj, obj_id));
        candidates.push(Gia_ObjFaninId1(gia_obj, obj_id));
      } else {
        rejected.push(obj_id);
      }
    }
  }

  template<typename T>
  Subcircuit randomizedBFS<T>::findSubcircuit() {
    Subcircuit result;
    assert (Gia_ManIsNormalized(this->gia_man));
    int root_id = selectRoot();
    Gia_Obj_t * gia_obj = Gia_ManObj(this->gia_man, root_id);
    assert (Gia_ObjIsAnd(gia_obj));
    result.nodes = Vec_IntAlloc(this->cfg.subcircuit_size_bound);
    result.io = Vec_IntAlloc(this->cfg.subcircuit_size_bound);

    Vec_IntPush(result.nodes, root_id);

    std::queue<int> candidates;
    candidates.push(Gia_ObjFaninId0(gia_obj, root_id));
    candidates.push(Gia_ObjFaninId1(gia_obj, root_id));
    std::queue<int> rejected_nodes;

    Gia_ManIncrementTravId(this->gia_man);

    while (!candidates.empty() && Vec_IntSize(result.nodes) < this->cfg.subcircuit_size_bound) {
      checkCandidate(result, candidates, rejected_nodes, this->getRandomBool());
    }

    if (this->cfg.fill_subcircuits) {
      while (!rejected_nodes.empty() && Vec_IntSize(result.nodes) < this->cfg.subcircuit_size_bound) {
        checkCandidate(result, rejected_nodes, rejected_nodes, true);
      }
    }
    result.nof_inputs = this->getSubcircuitIO(result.nodes, result.io);
    return result;
  }

  inline bool randomizedBFSnoFP::filterSubcircuitImpl(const Subcircuit& subcir) {
    Gia_Obj_t * pObj;
    int i;
    unsigned int min_output_level = Gia_ManLevelNum(gia_man);
    Gia_ManIncrementTravId(gia_man);
    Gia_ManForEachObjVecStart(subcir.io, gia_man, pObj, i, subcir.nof_inputs) {
      min_output_level = Gia_ObjLevel(gia_man, pObj) < min_output_level ? Gia_ObjLevel(gia_man, pObj) : min_output_level;
      Gia_ObjSetTravIdCurrent(gia_man, pObj);
    }
    Gia_ManIncrementTravId(gia_man);
    Gia_ManForEachObjVecStop(subcir.io, gia_man, pObj, i, subcir.nof_inputs) {
      if (!filterSubcircuitRec(pObj, min_output_level)) {
        return false;
      }
    }
    return true;
  }

  inline bool randomizedBFSnoFP::filterSubcircuitRec(Gia_Obj_t * pObj, unsigned int min_level) {
    if (Gia_ObjIsTravIdPrevious(gia_man, pObj)) {
      return false;
    } else if (Gia_ObjIsTravIdCurrent(gia_man, pObj)) {
      return true;
    } else if (Gia_ObjLevel(gia_man, pObj) < min_level) {
      return true;
    }
    Gia_ObjSetTravIdCurrent(gia_man, pObj);
    return filterSubcircuitRec(Gia_ObjFanin0(pObj), min_level) && filterSubcircuitRec(Gia_ObjFanin1(pObj), min_level);
  }
}

ABC_NAMESPACE_CXX_HEADER_END