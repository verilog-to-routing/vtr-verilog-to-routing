/**CFile****************************************************************

  FileName    [synthesisEngine.tpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Procedures for synthesising Boolean relations.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: synthesisEngine.tpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#include <numeric>
#ifdef WIN32
  #include <process.h> 
  #define unlink _unlink
#else
  #include <unistd.h>
#endif

#include "misc/util/utilTruth.h"

#include "synthesisEngine.hpp"
#include "utils.hpp"

ABC_NAMESPACE_HEADER_START
  Vec_Int_t * Exa4_ManParse( char * pFileName );
ABC_NAMESPACE_HEADER_END

ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  template <typename T>
  exactSynthesisEngine<T>::exactSynthesisEngine( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
                                                 const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg) 
                          : vSimsIn(vSimsIn), vSimsOut(vSimsOut), nIns(nIns), nDivs(nDivs), nNodes(nNodes),
                            nOuts(nOuts), forbidden_pairs(forbidden_pairs), log(log), cfg(cfg) {
    assert( Vec_WrdSize(vSimsIn) == Vec_WrdSize(vSimsOut) );
    nObjs       = nDivs + nNodes + nOuts;
    nCnfVars    = markup();
    introduceConnectionVariables();
    nCnfVars2   = 0;
    nCnfClauses = 0;
    assert( nObjs < 64 );
  }

  template <typename T>
  bool exactSynthesisEngine<T>::isNormalized( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut ) {
      int i, Count = 0; word Temp;
      Vec_WrdForEachEntry( vSimsIn, Temp, i ) {
        if ( Temp & 1 ) {
          Count++;
        }    
      }
      if ( Count ) {
        return false;
      }
      if ( !(Vec_WrdEntry(vSimsOut, 0) & 1) ) {
        return false;
      }
      return true;
  }

  template <typename T>
  int exactSynthesisEngine<T>::markup() {
    int i, k, j, nVars[3] = {1 + 3*nNodes, 0, 3*nNodes*Vec_WrdSize(vSimsIn)};
    assert( nObjs <= MAJ_NOBJS );
    for ( i = nDivs; i < nDivs + nNodes; i++ ) {
      for ( k = 0; k < 2; k++ ) {
        for ( j = 1+!k; j < i-k; j++ ) {
          VarMarks[i][k][j] = nVars[0] + nVars[1]++;
        }
      }
    }
    for ( i = nDivs + nNodes; i < nObjs; i++ ) {
      for ( j = 0; j < nDivs + nNodes; j++ ) {
        VarMarks[i][0][j] = nVars[0] + nVars[1]++;
      }
    }
    return nVars[0] + nVars[1] + nVars[2];
  }

  template <typename T>
  int exactSynthesisEngine<T>::findFanin( Vec_Int_t * vValues, int i, int k, int nof_objs ) {
    int j, Count = 0, iVar = -1;
    for ( j = 0; j < nof_objs; j++ ) {
      if ( VarMarks[i][k][j] && Vec_IntEntry(vValues, VarMarks[i][k][j]) ){
        iVar = j;
        Count++;
      }
    }
    assert( Count == 1 );
    return iVar;
  }

  template <typename T>
  Mini_Aig_t * exactSynthesisEngine<T>::getAig(Vec_Int_t * vValues) {
    return getAig(vValues, nNodes);
  }

  template <typename T>
  Mini_Aig_t * exactSynthesisEngine<T>::getAig(Vec_Int_t * vValues, int size) {
    assert (size >= 0 && size <= nNodes && "Invalid size for AIG construction given.");
    int i, k, Compl[MAJ_NOBJS] = {0};
    int nof_Objs = nDivs + size + nOuts;
    Mini_Aig_t * pMini = Mini_AigStartSupport( nDivs-1, nof_Objs );
    for ( i = nDivs; i < nDivs + size; i++ ) {
        int iNodes[2] = {0};
        int iVarStart = 1 + 3*(i - nDivs);//
        int Val1 = Vec_IntEntry(vValues, iVarStart);
        int Val2 = Vec_IntEntry(vValues, iVarStart+1);
        int Val3 = Vec_IntEntry(vValues, iVarStart+2);
        Compl[i] = Val1 && Val2 && Val3;
        for ( k = 0; k < 2; k++ ) {
            int iNode = findFanin(vValues, i, !k, nof_Objs);
            int fComp = k ? !Val1 && Val2 && !Val3 : Val1 && !Val2 && !Val3;
            iNodes[k] = Abc_Var2Lit(iNode, fComp ^ Compl[iNode]);
        }
        if ( Val1 && Val2 ) {
            if ( Val3 ) {
                Mini_AigOr( pMini, iNodes[0], iNodes[1] );
            } else {
                Mini_AigXorSpecial( pMini, iNodes[0], iNodes[1] );
            }
        } else {
          Mini_AigAnd( pMini, iNodes[0], iNodes[1] );
        }
    }
    for ( i = nDivs + nNodes; i < nObjs; i++ ) {
        int iVar = findFanin( vValues, i, 0, nof_Objs);
        Mini_AigCreatePo( pMini, Abc_Var2Lit(iVar, Compl[iVar]) );
    }
    assert( nof_Objs == Mini_AigNodeNum(pMini) );
    return pMini;
  }

  template <typename T>
  void exactSynthesisEngine<T>::generateCNF( int fOrderNodes, int fUniqFans ) {
    int m;
    startEncoding( fOrderNodes, fUniqFans );
    // Ignore pattern for all false input pattern (as we start with m=1)
    for ( m = 1; m < Vec_WrdSize(vSimsIn); m++ ) {
      generateMinterm( m );
    }

    if (forbidden_pairs.size() > 0) {
      addConnectivityConstraints();
    }
  }

  template <typename T>
  void exactSynthesisEngine<T>::introduceConnectionVariables() {
    std::unordered_set<int> inputs_in_pairs;
    // for (const auto& [out, inputs] : forbidden_pairs) {
    for (const auto& it: forbidden_pairs) {
      const auto& inputs = it.second;
      inputs_in_pairs.insert(inputs.begin(), inputs.end());
    }
    int nCVars = nObjs - 1; // The constant node does not depend on any node
    for (int in : inputs_in_pairs) {
      connection_variables.emplace(in, std::vector<int> (nCVars, 0));
      std::iota(connection_variables[in].begin(), connection_variables[in].end(), nCnfVars);
      nCnfVars += nCVars;
    }
  }

  template <typename T>
  void exactSynthesisEngine<T>::setupConnectionVariables() {
    // for (const auto& [in, cvars] : connection_variables) {
    for (const auto& it: connection_variables) {
      const auto& in = it.first;
      const auto& cvars = it.second;
      addClause(Abc_Var2Lit(cvars[in],0),0,0,0); 
      for (int i = nDivs; i < nDivs + nNodes; i++ ) {
        int gate_idx = i - nDivs;
        for (int j = 1; j < i - 1; j++) {
          addClause(Abc_Var2Lit(VarMarks[i][0][j + 1], 1), Abc_Var2Lit(cvars[j], 1), Abc_Var2Lit(cvars[i - 1], 0), getGateEnablingLiteral(gate_idx, 1)); 
          addClause(Abc_Var2Lit(VarMarks[i][1][j], 1), Abc_Var2Lit(cvars[j - 1], 1), Abc_Var2Lit(cvars[i - 1], 0), getGateEnablingLiteral(gate_idx, 1)); 
        }
      }
      for (int i = nDivs + nNodes; i < nObjs; i++) {
        for (int j = 1; j < nDivs + nNodes; j++) {
          addClause(Abc_Var2Lit(VarMarks[i][0][j], 1), Abc_Var2Lit(cvars[j - 1], 1), Abc_Var2Lit(cvars[i - 1], 0),0); 
        }
      }
    }
  }

  template <typename T>
  void exactSynthesisEngine<T>::addConnectivityConstraints() {
    assert (forbidden_pairs.size() > 0 && "Connectivity constraints are only needed if there are forbidden pairs.");
    setupConnectionVariables();
    // If there are forbidden pairs for more than one input it is possible that a loop is obtained via two pairs
    if (forbidden_pairs.size() > 1) {
      addCombinedCycleConstraints();
    }

    // for (const auto& [out, inputs] : forbidden_pairs) {
    for (const auto& it: forbidden_pairs) {
      const auto& out = it.first;
      const auto& inputs = it.second;
      for (int in : inputs) {
        const auto& cvars = connection_variables.at(in);
        int out_obj_idx = nDivs + nNodes + out;
        addClause(Abc_Var2Lit(cvars[out_obj_idx - 1], 1), 0, 0, 0);
      }
    }
  }

  template <typename T>
  void exactSynthesisEngine<T>::addCombinedCycleConstraints() {
    assert (forbidden_pairs.size() > 1 && "Combined connectivity constraints are only needed if there are forbidden pairs for multiple inputs.");
    std::unordered_map<int, std::unordered_set<int>> not_in_pair = computeNotInPair(forbidden_pairs);

    // for (const auto& [out, inputs] : not_in_pair) {
    for (const auto& it: not_in_pair) {
      const auto& out = it.first;
      const auto& inputs = it.second;
      for (int in : inputs) {
        const auto& cvars = connection_variables.at(in);
        int out_obj_idx = nDivs + nNodes + out;
        for (int in2 : forbidden_pairs.at(out)) {
          addClause(Abc_Var2Lit(cvars[out_obj_idx - 1], 1), Abc_Var2Lit(cvars[in2], 0),0,0);
        }
      }
    }
  }

  template <typename T>
  std::unordered_map<int, std::unordered_set<int>> exactSynthesisEngine<T>::computeNotInPair(const std::unordered_map<int, std::unordered_set<int>>& pairs) {
    std::unordered_set<int> all_inputs_in_pairs;
    // for (const auto& [out, in] : pairs) {
    for (const auto& it: pairs) {
      const auto& in = it.second;
      all_inputs_in_pairs.insert(in.begin(), in.end());
    }
    std::unordered_map<int, std::unordered_set<int>> not_in_pair;
    // for (const auto& [out, in] : pairs) {
    for (const auto& it: pairs) {
      const auto& out = it.first;
      const auto& in = it.second;
      if (in.size() == all_inputs_in_pairs.size()) {
        continue;
      }
      for (int i : all_inputs_in_pairs) {
        if (in.find(i) == in.end()) {
          not_in_pair[out].insert(i);
        }
      }
    }
    return not_in_pair;
  }

  template <typename T>
  int exactSynthesisEngine<T>::startEncoding(int fOrderNodes, int fUniqFans )
  {
      int pLits[2*MAJ_NOBJS], i, j, k, n, m, nLits;
      for ( i = nDivs; i < nDivs + nNodes; i++ )
      {
          int gate_idx = i - nDivs;
          int iVarStart = 1 + 3*(i - nDivs);//
          for ( k = 0; k < 2; k++ )
          {
              nLits = 0;
              for ( j = 0; j < nObjs; j++ )
                  if ( VarMarks[i][k][j] )
                      pLits[nLits++] = Abc_Var2Lit( VarMarks[i][k][j], 0 );
              assert( nLits > 0 );
              static_cast<T*>(this)->addClause( pLits, nLits );
              for ( n = 0;   n < nLits; n++ )
              for ( m = n+1; m < nLits; m++ )
                  addClause( Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), getGateEnablingLiteral(gate_idx, 1), 0 );
              if ( k == 1 )
                  break;
              for ( j = 0; j < nObjs; j++ ) if ( VarMarks[i][0][j] )
              for ( n = j; n < nObjs; n++ ) if ( VarMarks[i][1][n] )
                  addClause( Abc_Var2Lit(VarMarks[i][0][j], 1), Abc_Var2Lit(VarMarks[i][1][n], 1), getGateEnablingLiteral(gate_idx, 1), 0 );
          }
          if ( fOrderNodes )
          for ( j = nDivs; j < i; j++ )
          for ( n = 0;   n < nObjs; n++ ) if ( VarMarks[i][0][n] )
          for ( m = n+1; m < nObjs; m++ ) if ( VarMarks[j][0][m] )
              addClause( Abc_Var2Lit(VarMarks[i][0][n], 1), Abc_Var2Lit(VarMarks[j][0][m], 1), getGateEnablingLiteral(gate_idx, 1), 0 );
          for ( j = nDivs; j < i; j++ )
          for ( k = 0; k < 2; k++ )        if ( VarMarks[i][k][j] )
          for ( n = 0; n < nObjs; n++ ) if ( VarMarks[i][!k][n] )
          for ( m = 0; m < 2; m++ )        if ( VarMarks[j][m][n] )
              addClause( Abc_Var2Lit(VarMarks[i][k][j], 1), Abc_Var2Lit(VarMarks[i][!k][n], 1), Abc_Var2Lit(VarMarks[j][m][n], 1), getGateEnablingLiteral(gate_idx, 1) );
          for ( k = 0; k < 3; k++ )
              addClause( Abc_Var2Lit(iVarStart, k==1), Abc_Var2Lit(iVarStart+1, k==2), Abc_Var2Lit(iVarStart+2, k!=0), getGateEnablingLiteral(gate_idx, 1));
          if ( !cfg.allow_xors )
              addClause( Abc_Var2Lit(iVarStart, 1), Abc_Var2Lit(iVarStart+1, 1), Abc_Var2Lit(iVarStart+2, 0), getGateEnablingLiteral(gate_idx, 1));

      }
      for ( i = nDivs; i < nDivs + nNodes; i++ )
      {
          int gate_idx = i - nDivs;
          nLits = 0;
          for ( k = 0; k < 2; k++ )
              for ( j = i+1; j < nObjs; j++ )
                  if ( VarMarks[j][k][i] )
                      pLits[nLits++] = Abc_Var2Lit( VarMarks[j][k][i], 0 );
          if ( fUniqFans )
              for ( n = 0;   n < nLits; n++ )
              for ( m = n+1; m < nLits; m++ )
                  addClause( Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), getGateEnablingLiteral(gate_idx, 1), 0 );
      }
      for ( i = nDivs + nNodes; i < nObjs; i++ )
      {
          static_cast<T*>(this)->addGateDeactivatedConstraint(i);
          nLits = 0;
          for ( j = 0; j < nDivs + nNodes; j++ ) if ( VarMarks[i][0][j] ) {
              pLits[nLits++] = Abc_Var2Lit( VarMarks[i][0][j], 0 );
          }
          static_cast<T*>(this)->addClause( pLits, nLits );
          for ( n = 0;   n < nLits; n++ )
          for ( m = n+1; m < nLits; m++ )
              addClause( Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
      }
      return 1;
  }

  template <typename T>
  void exactSynthesisEngine<T>::generateMinterm( int iMint) {
      int internalCnfVars = nCnfVars - getAuxilaryVariableCount(); // for each gate we add an assumption
      int iNodeVar = internalCnfVars + 3*nNodes*(iMint - Vec_WrdSize(vSimsIn));
      int iOutMint = Abc_Tt6FirstBit( Vec_WrdEntry(vSimsOut, iMint) );
      int fOnlyOne = Abc_TtSuppOnlyOne( (int)Vec_WrdEntry(vSimsOut, iMint) );
      int i, k, n, j, VarVals[MAJ_NOBJS];
      int fAllOnes = Abc_TtCountOnes( Vec_WrdEntry(vSimsOut, iMint) ) == (1 << nOuts);
      if ( fAllOnes )
          return;
      assert( nObjs <= MAJ_NOBJS );
      assert( iMint < Vec_WrdSize(vSimsIn) );
      assert( nOuts <= 6 );
      for ( i = 0; i < nDivs; i++ ) {
          VarVals[i] = (Vec_WrdEntry(vSimsIn, iMint) >> i) & 1;
      }
      for ( i = 0; i < nNodes; i++ ) {
          VarVals[nDivs + i] = Abc_Var2Lit(iNodeVar + 3*i + 2, 0);
      }
      if ( fOnlyOne ) {
          for ( i = 0; i < nOuts; i++ )
              VarVals[nDivs + nNodes + i] = (iOutMint >> i) & 1;
      } else {
          word t = Abc_Tt6Stretch( Vec_WrdEntry(vSimsOut, iMint), nOuts );
          int i, c, nCubes = 0, pCover[100], pLits[10];
          int iOutVar = nCnfVars + nCnfVars2;  nCnfVars2 += nOuts;
          for ( i = 0; i < nOuts; i++ ) {
            VarVals[nDivs + nNodes + i] = Abc_Var2Lit(iOutVar + i, 0);
          }
          assert( t );
          if ( ~t ) {
              Abc_Tt6IsopCover( ~t, ~t, nOuts, pCover, &nCubes );
              for ( c = 0; c < nCubes; c++ ) {
                  int nLits = 0;
                  for ( i = 0; i < nOuts; i++ ) {
                      int iLit = (pCover[c] >> (2*i)) & 3;
                      if ( iLit == 1 || iLit == 2 )
                          pLits[nLits++] = Abc_Var2Lit(iOutVar + i, iLit != 1);
                  }
                  static_cast<T*>(this)->addClause( pLits, nLits );
              }
          }
      }

      for ( i = nDivs; i < nDivs + nNodes; i++ ) {
          int iVarStart = 1 + 3*(i - nDivs);//
          int iBaseVarI = iNodeVar + 3*(i - nDivs);
          for ( k = 0; k < 2; k++ ) {
            for ( j = 0; j < i; j++ ) {
              if ( VarMarks[i][k][j] ) {
                for ( n = 0; n < 2; n++ ) {
                  addClause( Abc_Var2Lit(VarMarks[i][k][j], 1), Abc_Var2Lit(iBaseVarI + k, n), Abc_LitNotCond(VarVals[j], !n), 0);
                }
              }
            }
          }
          for ( k = 0; k < 4; k++ ) {
            for ( n = 0; n < 2; n++ ) {
              addClause( Abc_Var2Lit(iBaseVarI + 0, k&1), Abc_Var2Lit(iBaseVarI + 1, k>>1), Abc_Var2Lit(iBaseVarI + 2, !n), Abc_Var2Lit(k ? iVarStart + k-1 : 0, n));
            }
          }
      }
      for ( i = nDivs + nNodes; i < nObjs; i++ ) {
        for ( j = 0; j < nDivs + nNodes; j++ ) {
          if ( VarMarks[i][0][j] ) {
            for ( n = 0; n < 2; n++ ) {
              addClause( Abc_Var2Lit(VarMarks[i][0][j], 1), Abc_LitNotCond(VarVals[j], n), Abc_LitNotCond(VarVals[i], !n), 0);
            }
          }
        }
      }
  }

  template <typename T>
  int exactSynthesisEngine<T>::prepareLits(int * pLits, int& nLits) {
    int k = 0;
    for ( int i = 0; i < nLits; i++ ) {
      if ( pLits[i] == 1 )
          return 0;
      else if ( pLits[i] == 0 )
          continue;
      else if ( pLits[i] <= 2*(nCnfVars + nCnfVars2) )
          pLits[k++] = pLits[i];
      else assert( 0 );
    }
    nLits = k;
    return 1;
  }

  inline CadicalEngine::CadicalEngine(Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
                                      const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg) 
                                    :  exactSynthesisEngine(vSimsIn, vSimsOut, nIns, nDivs, nOuts, nNodes, forbidden_pairs, log, cfg) {
    gate_enabling_assumptions.reserve(nNodes);
    for (int i = 0; i < nNodes; i++) {
      gate_enabling_assumptions.push_back(nCnfVars + i);
    }
    nCnfVars += nNodes;
    generateCNF(0, 0);
  }

  inline int CadicalEngine::addClause( int * pLits, int nLits ) {
    if (prepareLits(pLits, nLits) == 0) {
      return 0;
    }
    assert( nLits > 0 );
    solver.addClause(pLits, nLits);
    return 1;
  }

  inline Vec_Int_t * CadicalEngine::solve( int size, double timeout) {
    std::vector<int> assumptions = getAssumptions(size);
    solver.assume(assumptions);
    int status = timeout > 0 ? solver.solve(timeout) : solver.solve();
    Vec_Int_t * vRes = status == 10 ? solver.getModelVec() : nullptr;
    if (status == 10) {
      log.cummulative_sat_runtimes_per_size[size] += solver.getRunTime();
      log.nof_sat_calls_per_size[size] ++;
    } else if (status == 20) {
      log.cummulative_unsat_runtimes_per_size[size] += solver.getRunTime();
      log.nof_unsat_calls_per_size[size] ++;
    }
    return vRes;
  }

  Mini_Aig_t* CadicalEngine::getCircuit(int size, double timeout) {
    addAssumptions(size);
    Vec_Int_t * vValues = solve(size, timeout);
    Mini_Aig_t * pMini = vValues ? getAig(vValues, size) : nullptr;
    Vec_IntFreeP( &vValues );
    return pMini;
  }

  inline std::vector<int> CadicalEngine::getAssumptions(int size) {
    std::vector<int> assumptions(nNodes);
    for (int i = 0; i < size; i++) {
      assumptions[i] = gate_enabling_assumptions[i];
    }
    for (int i = size; i < nNodes; i++) {
      assumptions[i] = -gate_enabling_assumptions[i];
    }
    return assumptions;
  }

  inline void CadicalEngine::addGateDeactivatedConstraint(int out_idx) {
    for (int j = nDivs; j < nDivs + nNodes; j++ ) {
      int gate_idx = j - nDivs;
      // if a gate is disabled then it shall not be an output
      addClause( Abc_Var2Lit(VarMarks[out_idx][0][j],1), Abc_Var2Lit( gate_enabling_assumptions[gate_idx], 0 ),  0, 0 );
    }
  }

  inline int CadicalEngine::getGateEnablingLiteralImpl(int index, bool negated) {
    return Abc_Var2Lit( gate_enabling_assumptions[index], negated );
  }

  inline int CadicalEngine::getAuxilaryVariableCountDerived() {
    return nNodes; // We add an assumption for each gate
  }

  inline void CadicalEngine::addAssumptions(int size) {
    solver.assume(getAssumptions(size));
  }

  template <typename T>
  OneshotManager<T>::OneshotManager(Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int maxnNodes, 
                                    const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg) 
                    : vSimsIn(vSimsIn), vSimsOut(vSimsOut), nIns(nIns), nDivs(nDivs), maxnNodes(maxnNodes), nOuts(nOuts), forbidden_pairs(forbidden_pairs), log(log), cfg(cfg) {
  }

  template <typename T>
  T OneshotManager<T>::getOneshotEngine(int size) {
    assert(size <= maxnNodes);
    T oneshotEngine(vSimsIn, vSimsOut, nIns, nDivs, nOuts, size, forbidden_pairs, log, cfg);
    return oneshotEngine;
  }

  template <typename T>
  Mini_Aig_t* OneshotManager<T>::getCircuit(int size, double timeout) {
    T synth = getOneshotEngine(size);
    return synth.getCircuit(size, timeout);
  }

  inline CadicalEngineOneShot::CadicalEngineOneShot( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
                                                  const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg)
                              : exactSynthesisEngine<CadicalEngineOneShot>(vSimsIn, vSimsOut, nIns, nDivs, nOuts, nNodes, forbidden_pairs, log, cfg) {                       
  }

  inline Vec_Int_t* CadicalEngineOneShot::solve( int size, double timeout) {
    int status = timeout > 0 ? solver.solve(timeout) : solver.solve();
    Vec_Int_t * vRes = status != 10 ? nullptr : solver.getModelVec();
    return vRes;
  }

  Mini_Aig_t* CadicalEngineOneShot::getCircuit(int size, double timeout) {
    generateCNF(0, 0);
    Vec_Int_t * vValues = solve(size, timeout);
    Mini_Aig_t * pMini = vValues ? getAig(vValues, size) : nullptr;
    Vec_IntFreeP( &vValues );
    return pMini;
  }

  inline int CadicalEngineOneShot::addClause( int * pLits, int nLits ) {
    if (prepareLits(pLits, nLits) == 0) {
      return 0;
    }
    assert( nLits > 0 );
    solver.addClause(pLits, nLits);
    return 1;
  }

  inline KissatEngineOneShot::KissatEngineOneShot(Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
                                                  const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg) 
                            : exactSynthesisEngine<KissatEngineOneShot>(vSimsIn, vSimsOut, nIns, nDivs, nOuts, nNodes, forbidden_pairs, log, cfg) {
  }

  Mini_Aig_t* KissatEngineOneShot::getCircuit(int size, double timeout) {
    solver.init(nCnfVars);
    generateCNF(0, 0);
    Vec_Int_t * vValues = solve(size, timeout);
    Mini_Aig_t * pMini = vValues ? getAig(vValues, size) : nullptr;
    Vec_IntFreeP( &vValues );
    return pMini;
  }

  inline Vec_Int_t *KissatEngineOneShot::solve( int size, double timeout) {
    // TODO: The used interface does not yet allow timeouts
    int status =  solver.solve();
    Vec_Int_t * vRes = status != 10 ? nullptr : solver.getModelVec();
    return vRes;
  }

  inline int KissatEngineOneShot::addClause( int * pLits, int nLits ) {
    if (prepareLits(pLits, nLits) == 0) {
      return 0;
    }
    assert( nLits > 0 );
    solver.addClause(pLits, nLits);
    return 1;
  }

  inline KissatCmdEngine::KissatCmdEngine(Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
                                          const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg) 
                        : exactSynthesisEngine<KissatCmdEngine>(vSimsIn, vSimsOut, nIns, nDivs, nOuts, nNodes, forbidden_pairs, log, cfg) {
  }

  Mini_Aig_t* KissatCmdEngine::getCircuit(int size, double timeout) {
    encoding_file = fopen( pFileNameIn, "wb" );
    fputs( "p cnf                \n", encoding_file );
    generateCNF(0, 0);
    Vec_Int_t * vValues = solve(size, timeout);
    Mini_Aig_t * pMini = vValues ? getAig(vValues, size) : nullptr;
    Vec_IntFreeP( &vValues );
    return pMini;
  }

  inline int KissatCmdEngine::addClause( int * pLits, int nLits ) {
    if (prepareLits(pLits, nLits) == 0) {
      return 0;
    }
    assert( nLits > 0 );
    if ( encoding_file ) {
      nCnfClauses++;
      for ( int i = 0; i < nLits; i++ ) {
        fprintf( encoding_file, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
      }
      fprintf( encoding_file, "0\n" );
    }
    return 1;
  }

  inline Vec_Int_t * KissatCmdEngine::solve(int size, double timeout) {
    rewind( encoding_file );
    fprintf( encoding_file, "p cnf %d %d", nCnfVars + nCnfVars2, nCnfClauses );
    fclose( encoding_file );
    encoding_file = nullptr;

    char Command[1000], * pCommand = (char *)&Command;

    // TODO:
    // if ( TimeOut )
    //     sprintf( pCommand, "%s --time=%d -q %s > %s", pKissat, TimeOut, pFileNameIn, pFileNameOut );
    // else
    //     sprintf( pCommand, "%s -q %s > %s", pKissat, pFileNameIn, pFileNameOut );

    sprintf( pCommand, "%s -q %s > %s", pKissat, pFileNameIn, pFileNameOut );
#ifdef __wasm
    if ( 1 ) {
#else
    if ( system( pCommand ) == -1 ) {
#endif  
      std::cerr << "Command " << pCommand << " failed\n";
      return nullptr;
    }
    Vec_Int_t * vRes = Exa4_ManParse( pFileNameOut );
    unlink( pFileNameIn );
    return vRes;
  }

}

ABC_NAMESPACE_CXX_HEADER_END