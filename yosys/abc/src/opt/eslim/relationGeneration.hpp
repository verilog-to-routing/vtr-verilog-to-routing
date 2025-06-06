/**CFile****************************************************************

  FileName    [relationGeneration.hpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Procedures for computing Boolean relations.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: relationGeneration.hpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#ifndef ABC__OPT__ESLIM__RELATIONGENERATION_h
#define ABC__OPT__ESLIM__RELATIONGENERATION_h

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "misc/util/abc_namespaces.h"
#include <misc/util/abc_global.h>
#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h" // ioResub.h depends on utilTruth.h
#include "base/io/ioResub.h"

#include "utils.hpp"
// #include "satInterfaces.hpp"


ABC_NAMESPACE_CXX_HEADER_START

namespace eSLIM {

  template<class Derived>
  class RelationGenerator {

    public:
      static Abc_RData_t* computeRelation(Gia_Man_t* gia_man, const Subcircuit& subcir);
      
    private:
      Gia_Man_t* pGia;
      const Subcircuit& subcir; 
      
      RelationGenerator(Gia_Man_t* pGia, const Subcircuit& subcir);
      void setup();
      Vec_Int_t* getRelation();

      static Abc_RData_t* constructABCRelationRepresentation(Vec_Int_t * patterns, int nof_inputs, int nof_outputs);
      // Reimplementation with disabled prints
      // We do not want to modify code in other parts of ABC
      static Abc_RData_t * Abc_RData2Rel( Abc_RData_t * p );
      

    friend Derived;
  };

  // The class almost entirely duplicates the functionality provided by Gia_ManGenIoCombs in giaQBF.c
  // But the implementation in this class allows to take forbidden pairs into account.
  class RelationGeneratorABC : public RelationGenerator<RelationGeneratorABC> {
      
    private:
      RelationGeneratorABC(Gia_Man_t* pGia, const Subcircuit& subcir);
      void setupImpl() {};
      Vec_Int_t* getRelationImpl();
      Gia_Man_t * generateMiter();

      std::unordered_set<int> inputs_in_forbidden_pairs;
    
    friend RelationGenerator;
  };


  // Add other engine for the generation of relations
  // class MyRelationGenerator : public RelationGenerator<MyRelationGenerator> {
  //   private:
  //     void setupImpl()
  //     Vec_Int_t* getRelationImpl();
  //   friend RelationGenerator;
  // };


  template <typename T>
  inline RelationGenerator<T>::RelationGenerator(Gia_Man_t* pGia, const Subcircuit& subcir)
                      : pGia(pGia), subcir(subcir) {
  }

  template <typename T>
  inline Abc_RData_t* RelationGenerator<T>::computeRelation(Gia_Man_t* gia_man, const Subcircuit& subcir) {
    T generator(gia_man, subcir);
    generator.setup();
    Vec_Int_t* relation_patterns_masks = generator.getRelation();
    if ( relation_patterns_masks == NULL ) {
      return nullptr;
    }
    int nof_inputs = subcir.nof_inputs;
    int nof_outputs = Vec_IntSize(subcir.io) - subcir.nof_inputs;
    Abc_RData_t* r = constructABCRelationRepresentation(relation_patterns_masks, nof_inputs, nof_outputs);
    Vec_IntFree(relation_patterns_masks);
    return r;
  }

  template <typename T>
  inline Abc_RData_t* RelationGenerator<T>::constructABCRelationRepresentation(Vec_Int_t * patterns,  int nof_inputs, int nof_outputs) {
    int i, mask;
    int nof_vars = nof_inputs + nof_outputs;
    Abc_RData_t* p = Abc_RDataStart( nof_inputs, nof_outputs, Vec_IntSize(patterns) );
    Vec_IntForEachEntry( patterns, mask, i ) {
      for ( int k = 0; k < nof_vars; k++ ) {
        if ( (mask >> (nof_vars-1-k)) & 1 ) { 
          if ( k < nof_inputs ) {
            Abc_RDataSetIn( p, k, i );
          } else {
            Abc_RDataSetOut( p, 2*(k-nof_inputs)+1, i );
          }      
        } else { 
          if ( k >= nof_inputs ) {
            Abc_RDataSetOut( p, 2*(k-nof_inputs), i );
          }
        }
      }
    }
    Abc_RData_t* p2 = Abc_RData2Rel(p);
    Abc_RDataStop(p);
    return p2;
  }

  template <typename T>
  inline Abc_RData_t * RelationGenerator<T>::Abc_RData2Rel( Abc_RData_t * p ) {
    assert( p->nIns < 64 );
    assert( p->nOuts < 32 );
    int w;
    Vec_Wrd_t * vSimsIn2  = Vec_WrdStart( 64*p->nSimWords );
    Vec_Wrd_t * vSimsOut2 = Vec_WrdStart( 64*p->nSimWords );
    for ( w = 0; w < p->nIns; w++ )
        Abc_TtCopy( Vec_WrdEntryP(vSimsIn2,  w*p->nSimWords), Vec_WrdEntryP(p->vSimsIn, w*p->nSimWords), p->nSimWords, 0 );
    for ( w = 0; w < p->nOuts; w++ )
        Abc_TtCopy( Vec_WrdEntryP(vSimsOut2, w*p->nSimWords), Vec_WrdEntryP(p->vSimsOut, (2*w+1)*p->nSimWords), p->nSimWords, 0 );
    Vec_Wrd_t * vTransIn  = Vec_WrdStart( 64*p->nSimWords );
    Vec_Wrd_t * vTransOut = Vec_WrdStart( 64*p->nSimWords );
    Extra_BitMatrixTransposeP( vSimsIn2,  p->nSimWords, vTransIn,  1 );
    Extra_BitMatrixTransposeP( vSimsOut2, p->nSimWords, vTransOut, 1 );
    Vec_WrdShrink( vTransIn,  p->nPats );
    Vec_WrdShrink( vTransOut, p->nPats );
    Vec_Wrd_t * vTransInCopy = Vec_WrdDup(vTransIn); 
    Vec_WrdUniqify( vTransInCopy );
    // if ( Vec_WrdSize(vTransInCopy) == p->nPats )
    //     printf( "This resub problem is not a relation.\n" );
    // create the relation
    Abc_RData_t * pNew = Abc_RDataStart( p->nIns, 1 << (p->nOuts-1), Vec_WrdSize(vTransInCopy) );
    pNew->nOuts = p->nOuts;
    int i, k, n, iLine = 0; word Entry, Entry2;
    Vec_WrdForEachEntry( vTransInCopy, Entry, i ) {
        for ( n = 0; n < p->nIns; n++ )
            if ( (Entry >> n) & 1 )
                Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(pNew->vSimsIn, n*pNew->nSimWords), iLine );
        Vec_WrdForEachEntry( vTransIn, Entry2, k ) {
            if ( Entry != Entry2 )
                continue;
            Entry2 = Vec_WrdEntry( vTransOut, k );
            assert( Entry2 < (1 << p->nOuts) );
            Abc_InfoSetBit( (unsigned *)Vec_WrdEntryP(pNew->vSimsOut, Entry2*pNew->nSimWords), iLine );
        }
        iLine++;
    }
    assert( iLine == pNew->nPats );
    Vec_WrdFree( vTransOut );
    Vec_WrdFree( vTransInCopy );
    Vec_WrdFree( vTransIn );
    Vec_WrdFree( vSimsIn2 );
    Vec_WrdFree( vSimsOut2 );
    return pNew;    
  }

  template <typename T>
  inline void RelationGenerator<T>::setup() {
    static_cast<T*>(this)->setupImpl();
  }

  template <typename T>
  inline Vec_Int_t* RelationGenerator<T>::getRelation() {
    return static_cast<T*>(this)->getRelationImpl();
  }
}

ABC_NAMESPACE_CXX_HEADER_END

#endif
