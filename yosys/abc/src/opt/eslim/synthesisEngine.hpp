/**CFile****************************************************************

  FileName    [synthesisEngine.hpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Procedures for synthesising Boolean relations.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: synthesisEngine.hpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__OPT__ESLIM__SYNTHESISENGINE_h
#define ABC__OPT__ESLIM__SYNTHESISENGINE_h

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#include "misc/util/abc_namespaces.h"
#include "aig/miniaig/miniaig.h"
#include "misc/vec/vec.h"

#include "satInterfaces.hpp"

ABC_NAMESPACE_CXX_HEADER_START

#define MAJ_NOBJS  64 // Const0 + Const1 + nVars + nNodes

namespace eSLIM {

  // Based on Exa6_ManGenStart
  template<class Derived>
  class exactSynthesisEngine {

    private:
      Vec_Wrd_t *       vSimsIn;   // input signatures  (nWords = 1, nIns <= 64)
      Vec_Wrd_t *       vSimsOut;  // output signatures (nWords = 1, nOuts <= 6)
      int               nIns;     
      int               nDivs;     // divisors (const + inputs + tfi + sidedivs)
      int               nNodes;
      int               nOuts; 
      int               nObjs;
      int               VarMarks[MAJ_NOBJS][2][MAJ_NOBJS] = {}; // default init the array
      int               nCnfVars; 
      int               nCnfVars2; 
      int               nCnfClauses;

      // Assign outputs to their connectivity variables
      std::unordered_map<int, std::vector<int>> connection_variables;
      const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs;
      eSLIMLog& log;
      const eSLIMConfig& cfg;

      exactSynthesisEngine( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
                          const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg);
      int addClause(int lit1, int lit2, int lit3, int lit4);
      static bool isNormalized( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut );
      int markup();
      int findFanin( Vec_Int_t * vValues, int i, int k, int nof_objs);
      Mini_Aig_t * getAig(Vec_Int_t * assignment, int size);
      Mini_Aig_t * getAig(Vec_Int_t * assignment);
      void generateCNF(int fOrderNodes, int fUniqFans);
      int startEncoding(int fOrderNodes, int fUniqFans );
      void generateMinterm( int iMint);

      int getGateEnablingLiteral(int index, bool negated);
      int getAuxilaryVariableCount();

      void introduceConnectionVariables();
      void setupConnectionVariables();
      void addConnectivityConstraints();
      void addCombinedCycleConstraints();
      std::unordered_map<int, std::unordered_set<int>> computeNotInPair(const std::unordered_map<int, std::unordered_set<int>>& pairs);

      int prepareLits(int * pLits, int& nLits);

      friend Derived;
  };

  // applies Cadical incrementally
  class CadicalEngine : public exactSynthesisEngine<CadicalEngine> {
    
    public:
      CadicalEngine(Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
                    const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg);
      Mini_Aig_t* getCircuit(int size, double timeout);

    private:
      using exactSynthesisEngine::addClause;
      std::vector<int> gate_enabling_assumptions;
      CadicalSolver solver;

      
      int addClause( int * pLits, int nLits );
      Vec_Int_t * solve(int size, double timeout);
      int getAuxilaryVariableCountDerived();
      int getGateEnablingLiteralImpl(int index, bool negated);
      void addAssumptions(int size);
      void addGateDeactivatedConstraint(int out);

      std::vector<int> getAssumptions(int size);
      
    friend exactSynthesisEngine<CadicalEngine>;
  };

  class OneshotEngine {
    protected:
      int getAuxilaryVariableCountDerived() {return 0;};
      int getGateEnablingLiteralImpl(int index, bool negated) {return 0;};
      // void addAssumptions(int size) {};
      void addGateDeactivatedConstraint(int idx) {};
  };

  template <typename T>
  class OneshotManager {
    public:
      OneshotManager(Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int maxnNodes, 
        const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg);
      Mini_Aig_t* getCircuit(int size, double timeout);
      
    private:
      T getOneshotEngine(int size);

      Vec_Wrd_t *       vSimsIn;   // input signatures  (nWords = 1, nIns <= 64)
      Vec_Wrd_t *       vSimsOut;  // output signatures (nWords = 1, nOuts <= 6)
      int               nIns;     
      int               nDivs;     // divisors (const + inputs + tfi + sidedivs)
      int               maxnNodes;
      int               nOuts; 

      const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs;
      eSLIMLog& log;
      const eSLIMConfig& cfg;
  };

  class CadicalEngineOneShot : public exactSynthesisEngine<CadicalEngineOneShot>, public OneshotEngine {

    public:
      CadicalEngineOneShot( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
          const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg);
      Mini_Aig_t* getCircuit(int size, double timeout);

    private:
      using exactSynthesisEngine::addClause;
      
      int addClause( int * pLits, int nLits );
      Vec_Int_t * solve(int size, double timeout);

      CadicalSolver solver;

    friend exactSynthesisEngine<CadicalEngineOneShot>;
  };

  class KissatEngineOneShot : public exactSynthesisEngine<KissatEngineOneShot>, public OneshotEngine {

    public:
      KissatEngineOneShot( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
          const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg);
      Mini_Aig_t* getCircuit(int size, double timeout);

    private:
      using exactSynthesisEngine::addClause;
      
      int addClause( int * pLits, int nLits );
      Vec_Int_t * solve(int size, double timeout);

      KissatSolver solver;

    friend exactSynthesisEngine<KissatEngineOneShot>;
  };


  class KissatCmdEngine : public exactSynthesisEngine<KissatCmdEngine>, public OneshotEngine {

    public:
      KissatCmdEngine( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, 
          const std::unordered_map<int, std::unordered_set<int>>& forbidden_pairs, eSLIMLog& log, const eSLIMConfig& cfg);
      Mini_Aig_t* getCircuit(int size, double timeout);

    private:
      using exactSynthesisEngine::addClause;
      

      int addClause( int * pLits, int nLits );
      Vec_Int_t * solve(int size, double timeout);

      FILE * encoding_file;

      // Ensure that there is no file with the same name in the diretory
      constexpr static char * pFileNameIn = "_temp_.cnf";
      constexpr static char * pFileNameOut = "_temp_out.cnf";

      #ifdef WIN32
        constexpr static char * pKissat = "kissat.exe";
      #else
        constexpr static char * pKissat = "kissat";
      #endif

    friend exactSynthesisEngine<KissatCmdEngine>;
  };


  template <typename T>
  inline int exactSynthesisEngine<T>::addClause(int lit1, int lit2, int lit3, int lit4) {
    int clause[4] = {lit1, lit2, lit3, lit4};
    return static_cast<T*>(this)->addClause(clause, 4);
  }

  template <typename T>
  inline int exactSynthesisEngine<T>::getGateEnablingLiteral(int index, bool negated) {
    return static_cast<T*>(this)->getGateEnablingLiteralImpl(index, negated);
  }

  template <typename T>
  inline int exactSynthesisEngine<T>::getAuxilaryVariableCount() {
    int ncvars = connection_variables.empty() ? 0 : connection_variables.begin()->second.size() * connection_variables.size();
    return ncvars + static_cast<T*>(this)->getAuxilaryVariableCountDerived();
  }

  typedef OneshotManager<CadicalEngineOneShot> CadicalOneShot;
  typedef OneshotManager<KissatEngineOneShot> KissatOneShot;
  typedef OneshotManager<KissatCmdEngine> KissatCmdOneShot;

}

ABC_NAMESPACE_CXX_HEADER_END

#include "synthesisEngine.tpp"

#endif