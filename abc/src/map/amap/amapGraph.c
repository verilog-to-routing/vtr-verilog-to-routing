/**CFile****************************************************************

  FileName    [amapGraph.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Internal AIG manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapGraph.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManSetupObj( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    pObj = (Amap_Obj_t *)Aig_MmFixedEntryFetch( p->pMemObj );
    memset( pObj, 0, sizeof(Amap_Obj_t) );
    pObj->nFouts[0] = 1; // needed for flow to work in the first pass
    pObj->Id = Vec_PtrSize(p->vObjs);
    Vec_PtrPush( p->vObjs, pObj );
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManCreateConst1( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    pObj = Amap_ManSetupObj( p );
    pObj->Type   = AMAP_OBJ_CONST1;
    pObj->fPhase = 1;
    p->nObjs[AMAP_OBJ_CONST1]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManCreatePi( Amap_Man_t * p )
{
    Amap_Obj_t * pObj;
    pObj = Amap_ManSetupObj( p );
    pObj->Type  = AMAP_OBJ_PI;
    pObj->IdPio = Vec_PtrSize( p->vPis );
    Vec_PtrPush( p->vPis, pObj );
    p->nObjs[AMAP_OBJ_PI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary output with the given driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManCreatePo( Amap_Man_t * p, Amap_Obj_t * pFan0 )
{
    Amap_Obj_t * pObj;
    pObj = Amap_ManSetupObj( p );
    pObj->IdPio  = Vec_PtrSize( p->vPos );
    Vec_PtrPush( p->vPos, pObj );
    pObj->Type   = AMAP_OBJ_PO;
    pObj->Fan[0] = Amap_ObjToLit(pFan0);  Amap_Regular(pFan0)->nRefs++;
    pObj->Level  = Amap_Regular(pFan0)->Level;
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    assert( p->nLevelMax < 4094 ); // 2^12-2
    p->nObjs[AMAP_OBJ_PO]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManCreateAnd( Amap_Man_t * p, Amap_Obj_t * pFan0, Amap_Obj_t * pFan1 )
{
    Amap_Obj_t * pObj;
    pObj = Amap_ManSetupObj( p );
    pObj->Type   = AMAP_OBJ_AND;
    pObj->Fan[0] = Amap_ObjToLit(pFan0);  Amap_Regular(pFan0)->nRefs++;
    pObj->Fan[1] = Amap_ObjToLit(pFan1);  Amap_Regular(pFan1)->nRefs++;
    assert( Abc_Lit2Var(pObj->Fan[0]) != Abc_Lit2Var(pObj->Fan[1]) );
    pObj->fPhase = Amap_ObjPhaseReal(pFan0) & Amap_ObjPhaseReal(pFan1);
    pObj->Level  = 1 + Abc_MaxInt( Amap_Regular(pFan0)->Level, Amap_Regular(pFan1)->Level );
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    assert( p->nLevelMax < 4094 ); // 2^12-2
    p->nObjs[AMAP_OBJ_AND]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManCreateXor( Amap_Man_t * p, Amap_Obj_t * pFan0, Amap_Obj_t * pFan1 )
{
    Amap_Obj_t * pObj;
    pObj = Amap_ManSetupObj( p );
    pObj->Type   = AMAP_OBJ_XOR;
    pObj->Fan[0] = Amap_ObjToLit(pFan0);  Amap_Regular(pFan0)->nRefs++;
    pObj->Fan[1] = Amap_ObjToLit(pFan1);  Amap_Regular(pFan1)->nRefs++;
    pObj->fPhase = Amap_ObjPhaseReal(pFan0) ^ Amap_ObjPhaseReal(pFan1);
    pObj->Level  = 2 + Abc_MaxInt( Amap_Regular(pFan0)->Level, Amap_Regular(pFan1)->Level );
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    assert( p->nLevelMax < 4094 ); // 2^12-2
    p->nObjs[AMAP_OBJ_XOR]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManCreateMux( Amap_Man_t * p, Amap_Obj_t * pFan0, Amap_Obj_t * pFan1, Amap_Obj_t * pFanC )
{
    Amap_Obj_t * pObj;
    pObj = Amap_ManSetupObj( p );
    pObj->Type   = AMAP_OBJ_MUX;
    pObj->Fan[0] = Amap_ObjToLit(pFan0);  Amap_Regular(pFan0)->nRefs++;
    pObj->Fan[1] = Amap_ObjToLit(pFan1);  Amap_Regular(pFan1)->nRefs++;
    pObj->Fan[2] = Amap_ObjToLit(pFanC);  Amap_Regular(pFanC)->nRefs++;
    pObj->fPhase = (Amap_ObjPhaseReal(pFan1) &  Amap_ObjPhaseReal(pFanC)) | 
                   (Amap_ObjPhaseReal(pFan0) & ~Amap_ObjPhaseReal(pFanC));
    pObj->Level  = Abc_MaxInt( Amap_Regular(pFan0)->Level, Amap_Regular(pFan1)->Level );
    pObj->Level  = 2 + Abc_MaxInt( pObj->Level, Amap_Regular(pFanC)->Level );
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    assert( p->nLevelMax < 4094 ); // 2^12-2
    p->nObjs[AMAP_OBJ_MUX]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates the choice node.]

  Description [Should be called after the equivalence class nodes are linked.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManCreateChoice( Amap_Man_t * p, Amap_Obj_t * pObj )
{
    Amap_Obj_t * pTemp;
    // mark the node as a representative if its class
//    assert( pObj->fRepr == 0 );
    pObj->fRepr = 1;
    // update the level of this node (needed for correct required time computation)
    for ( pTemp = pObj; pTemp; pTemp = Amap_ObjChoice(p, pTemp) )
    {
        pObj->Level = Abc_MaxInt( pObj->Level, pTemp->Level );
//        pTemp->nVisits++; pTemp->nVisitsCopy++;
    }
    // mark the largest level
    if ( p->nLevelMax < (int)pObj->Level )
        p->nLevelMax = (int)pObj->Level;
    assert( p->nLevelMax < 4094 ); // 2^12-2
}

/**Function*************************************************************

  Synopsis    [Creates XOR/MUX choices for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManCreateXorChoices( Amap_Man_t * p, Amap_Obj_t * pFan0, Amap_Obj_t * pFan1, Amap_Obj_t * pChoices[] )
{
    pChoices[0] = Amap_ManCreateXor( p, pFan0,           pFan1 );
    pChoices[1] = Amap_ManCreateXor( p, Amap_Not(pFan0), pFan1 );
    pChoices[2] = Amap_ManCreateXor( p, pFan0,           Amap_Not(pFan1) );
    pChoices[3] = Amap_ManCreateXor( p, Amap_Not(pFan0), Amap_Not(pFan1) );
}

/**Function*************************************************************

  Synopsis    [Creates XOR/MUX choices for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManCreateMuxChoices( Amap_Man_t * p, Amap_Obj_t * pFan0, Amap_Obj_t * pFan1, Amap_Obj_t * pFanC, Amap_Obj_t * pChoices[] )
{
    pChoices[0] = Amap_ManCreateMux( p, pFan0,           pFan1,           pFanC           );
    pChoices[1] = Amap_ManCreateMux( p, Amap_Not(pFan0), Amap_Not(pFan1), pFanC           );
    pChoices[2] = Amap_ManCreateMux( p, pFan1,           pFan0,           Amap_Not(pFanC) );
    pChoices[3] = Amap_ManCreateMux( p, Amap_Not(pFan1), Amap_Not(pFan0), Amap_Not(pFanC) );
}

/**Function*************************************************************

  Synopsis    [Drags pointer out through the copy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Amap_Obj_t * Amap_AndToObj( Aig_Obj_t * pObj )
{
    return Amap_NotCond( (Amap_Obj_t *)Aig_Regular(pObj)->pData, Aig_IsComplement(pObj) );
}

/**Function*************************************************************

  Synopsis    [Starts the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Obj_t * Amap_ManGetLast_rec( Amap_Man_t * p, Amap_Obj_t * pObj )
{
    if ( pObj->Equiv == 0 )
        return pObj;
    return Amap_ManGetLast_rec( p, Amap_ObjChoice(p, pObj) );
}

/**Function*************************************************************

  Synopsis    [Starts the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_ManCreate( Amap_Man_t * p, Aig_Man_t * pAig )
{
    Vec_Ptr_t * vNodes;
    Amap_Obj_t * pChoices[4];
    Aig_Obj_t * pObj, * pFanin, * pPrev, * pFan0, * pFan1, * pFanC;
    int i, fChoices;
    if ( pAig->pEquivs )
        vNodes = Aig_ManDfsChoices( pAig );
    else
        vNodes = Aig_ManDfs( pAig, 1 );
    p->pConst1 = Amap_ManCreateConst1( p );
    // print warning about excessive memory usage
    if ( p->pPars->fVerbose )
    {
        if ( 1.0 * Aig_ManObjNum(pAig) * sizeof(Amap_Obj_t) / (1<<30) > 0.1 )
        printf( "Warning: Mapper allocates %.3f GB for subject graph with %d objects.\n", 
            1.0 * Aig_ManObjNum(pAig) * sizeof(Amap_Obj_t) / (1<<30), Aig_ManObjNum(pAig) );
    }
    // create PIs and remember them in the old nodes
    Aig_ManCleanData(pAig);
    Aig_ManConst1(pAig)->pData = Amap_ManConst1( p );
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Amap_ManCreatePi( p );
    // load the AIG into the mapper
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        fChoices = 0;
        if ( p->fUseXor && Aig_ObjRecognizeExor(pObj, &pFan0, &pFan1 ) )
        {
            Amap_ManCreateXorChoices( p, Amap_AndToObj(pFan0), Amap_AndToObj(pFan1), pChoices );
            fChoices = 1;
        }
        else if ( p->fUseMux && Aig_ObjIsMuxType(pObj) )
        {
            pFanC = Aig_ObjRecognizeMux( pObj, &pFan1, &pFan0 );
            Amap_ManCreateMuxChoices( p, Amap_AndToObj(pFan0), Amap_AndToObj(pFan1), Amap_AndToObj(pFanC), pChoices );
            fChoices = 1;
        }
        pObj->pData = Amap_ManCreateAnd( p, (Amap_Obj_t *)Aig_ObjChild0Copy(pObj), (Amap_Obj_t *)Aig_ObjChild1Copy(pObj) );
        if ( fChoices )
        {
            p->nChoicesAdded++;
            Amap_ObjSetChoice( (Amap_Obj_t *)pObj->pData, pChoices[0] );
            Amap_ObjSetChoice( pChoices[0], pChoices[1] );
            Amap_ObjSetChoice( pChoices[1], pChoices[2] );
            Amap_ObjSetChoice( pChoices[2], pChoices[3] );
            Amap_ManCreateChoice( p, (Amap_Obj_t *)pObj->pData );
        }
        if ( Aig_ObjIsChoice( pAig, pObj ) )
        {
//            assert( !fChoices );
            p->nChoicesGiven++;
            for ( pPrev = pObj, pFanin = Aig_ObjEquiv(pAig, pObj); pFanin; pPrev = pFanin, pFanin = Aig_ObjEquiv(pAig, pFanin) )
            {
                ((Amap_Obj_t *)pFanin->pData)->fRepr = 0;
                Amap_ObjSetChoice( Amap_ManGetLast_rec(p, (Amap_Obj_t *)pPrev->pData), 
                    (Amap_Obj_t *)pFanin->pData );
            }
            Amap_ManCreateChoice( p, (Amap_Obj_t *)pObj->pData );
        }
    }
    Vec_PtrFree( vNodes );
    // set the primary outputs without copying the phase
    Aig_ManForEachCo( pAig, pObj, i )
        pObj->pData = Amap_ManCreatePo( p, (Amap_Obj_t *)Aig_ObjChild0Copy(pObj) );
    if ( p->pPars->fVerbose )
        printf( "Performing mapping with %d given and %d created choices.\n", 
            p->nChoicesGiven, p->nChoicesAdded );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

