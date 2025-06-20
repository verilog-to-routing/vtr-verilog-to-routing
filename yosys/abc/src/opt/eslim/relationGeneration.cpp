/**CFile****************************************************************

  FileName    [relationGeneration.cpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Using Exact Synthesis with the SAT-based Local Improvement Method (eSLIM).]

  Synopsis    [Procedures for computing Boolean relations.]

  Author      [Franz-Xaver Reichl]
  
  Affiliation [University of Freiburg]

  Date        [Ver. 1.0. Started - March 2025.]

  Revision    [$Id: relationGeneration.cpp,v 1.00 2025/03/17 00:00:00 Exp $]

***********************************************************************/

#include <cassert>

#include "misc/vec/vec.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

#include "relationGeneration.hpp"

ABC_NAMESPACE_HEADER_START
  void *       Mf_ManGenerateCnf( Gia_Man_t * pGia, int nLutSize, int fCnfObjIds, int fAddOrCla, int fMapping, int fVerbose );
  void *       Cnf_DataWriteIntoSolver( Cnf_Dat_t * p, int nFrames, int fInit );
  Vec_Int_t * Gia_ManCollectNodeTfos( Gia_Man_t * p, int * pNodes, int nNodes );
  Vec_Int_t * Gia_ManCollectNodeTfis( Gia_Man_t * p, Vec_Int_t * vNodes );
ABC_NAMESPACE_HEADER_END

ABC_NAMESPACE_IMPL_START


namespace eSLIM {

  RelationGeneratorABC::RelationGeneratorABC(Gia_Man_t* pGia, const Subcircuit& subcir)
                      : RelationGenerator(pGia, subcir) {

  // for (const auto& [out, inputs] : subcir.forbidden_pairs) {
  for (const auto& it: subcir.forbidden_pairs) {
    const auto& inputs = it.second;
    inputs_in_forbidden_pairs.insert(inputs.begin(), inputs.end());
  }
}

  Vec_Int_t* RelationGeneratorABC::getRelationImpl() {
    abctime clkStart  = Abc_Clock();
    int nTimeOut = 600, nConfLimit = 1000000;
    int i, iNode, iSatVar, Iter, Mask, nSolutions = 0, RetValue = 0;
    // --------------- Modification ---------------
    Gia_Man_t* pMiter = generateMiter();
    // --------------- Modification ---------------
    Cnf_Dat_t * pCnf   = (Cnf_Dat_t*)Mf_ManGenerateCnf( pMiter, 8, 0, 0, 0, 0 );
    sat_solver * pSat  = (sat_solver*)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    int iLit = Abc_Var2Lit( 1, 0 ); // enumerating the care set (the miter output is 1)
    int status = sat_solver_addclause( pSat, &iLit, &iLit + 1 );  assert( status );
    Vec_Int_t * vSatVars = Vec_IntAlloc( Vec_IntSize(subcir.io) );
    Vec_IntForEachEntry( subcir.io, iNode, i )
        Vec_IntPush( vSatVars, i < subcir.nof_inputs  ? 2+i : pCnf->nVars-Vec_IntSize(subcir.io)+i );
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    Vec_Int_t * vRes  = Vec_IntAlloc( 1000 );
    for ( Iter = 0; Iter < 1000000; Iter++ )
    {        
        int status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, 0, 0, 0 );
        if ( status == l_False ) { RetValue =  1; break; }
        if ( status == l_Undef ) { RetValue =  0; break; }
        nSolutions++;
        // extract SAT assignment
        Mask = 0;
        Vec_IntClear( vLits );
        Vec_IntForEachEntry( vSatVars, iSatVar, i ) {
            Vec_IntPush( vLits, Abc_Var2Lit(iSatVar, sat_solver_var_value(pSat, iSatVar)) );
            if ( sat_solver_var_value(pSat, iSatVar) )
                Mask |= 1 << (Vec_IntSize(subcir.io)-1-i); 
        }
        Vec_IntPush( vRes, Mask );
        if ( !sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) ) )
            { RetValue =  1; break; }
        if ( nTimeOut && (Abc_Clock() - clkStart)/CLOCKS_PER_SEC >= nTimeOut ) { RetValue = 0; break; }
    }
    // complement the set of input/output minterms
    Vec_Int_t * vBits = Vec_IntStart( 1 << Vec_IntSize(subcir.io) );
    Vec_IntForEachEntry( vRes, Mask, i )
        Vec_IntWriteEntry( vBits, Mask, 1 );
    Vec_IntClear( vRes );
    Vec_IntForEachEntry( vBits, Mask, i )
        if ( !Mask )
            Vec_IntPush( vRes, i );
    Vec_IntFree( vBits );
    // cleanup
    Vec_IntFree( vLits );
    Vec_IntFree( vSatVars );
    sat_solver_delete( pSat );
    Gia_ManStop( pMiter ); 
    Cnf_DataFree( pCnf );
    if ( RetValue == 0 )
        Vec_IntFreeP( &vRes );        
    return vRes;   
  }

  Gia_Man_t * RelationGeneratorABC::generateMiter() {
    Vec_Int_t * vInsOuts = subcir.io;
    int nIns = subcir.nof_inputs;

    Vec_Int_t * vTfo = Gia_ManCollectNodeTfos( pGia, Vec_IntEntryP(vInsOuts, nIns), Vec_IntSize(vInsOuts)-nIns );
    Vec_Int_t * vTfi = Gia_ManCollectNodeTfis( pGia, vTfo );
    Vec_Int_t * vInLits  = Vec_IntAlloc( nIns );
    Vec_Int_t * vOutLits = Vec_IntAlloc( Vec_IntSize(vInsOuts) - nIns );
    Gia_Man_t * pNew, * pTemp; Gia_Obj_t * pObj; int i, iLit = 0;
    Gia_ManFillValue( pGia );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( pGia->pName );
    Gia_ManHashAlloc( pNew );
     Gia_ManForEachObjVec( vTfi, pGia, pObj, i )
        if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi(pNew);
    for ( i = 0; i < Vec_IntSize(vInsOuts)-nIns; i++ )
        Vec_IntPush( vInLits, Gia_ManAppendCi(pNew) );
    Gia_ManForEachObjVec( vTfi, pGia, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachObjVec( vTfo, pGia, pObj, i )
        if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
    Gia_ManForEachObjVec( vInsOuts, pGia, pObj, i )
        if ( i < nIns )
            Vec_IntPush( vOutLits, pObj->Value );
        else
            pObj->Value = Vec_IntEntry( vInLits, i-nIns );       
    
    Gia_ManIncrementTravId(pGia);
    Gia_ManForEachObjVec( subcir.nodes, pGia, pObj, i ) {
      Gia_ObjSetTravIdCurrent(pGia, pObj);
    }

    Gia_ManForEachObjVec( vTfo, pGia, pObj, i )
        // in case there are forbidden pairs we have to ensure that nodes from the sucircuit are not added
        if ( Gia_ObjIsAnd(pObj) && !Gia_ObjIsTravIdCurrent(pGia, pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachObjVec( vTfo, pGia, pObj, i )
        if ( Gia_ObjIsCo(pObj) )
            iLit = Gia_ManHashOr( pNew, iLit, Gia_ManHashXor(pNew, Gia_ObjFanin0Copy(pObj), pObj->Value) );
    // --------------- Modification ---------------
    // In case there are forbidden pairs, the outputs of the subcircuit influence the inputs of the subcircuit.
    // Therefore, we need the values of the inputs in the version of the circuit that is affected by the new outputs.
    for (int inp : inputs_in_forbidden_pairs) {
      int node_id = Vec_IntEntry(vInsOuts, inp); 
      pObj = Gia_ManObj(pGia, node_id);
      Vec_IntWriteEntry(vOutLits, inp, pObj->Value);
    }
    // --------------- Modification ---------------
    Gia_ManAppendCo( pNew, iLit );
    Vec_IntForEachEntry( vOutLits, iLit, i )
        Gia_ManAppendCo( pNew, iLit );
    Vec_IntFree( vTfo );
    Vec_IntFree( vTfi );
    Vec_IntFree( vInLits );
    Vec_IntFree( vOutLits );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pGia) );
    return pNew;
  }

} 

ABC_NAMESPACE_IMPL_END