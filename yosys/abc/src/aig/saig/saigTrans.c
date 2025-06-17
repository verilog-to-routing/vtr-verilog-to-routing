/**CFile****************************************************************

  FileName    [saigTrans.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Dynamic simplification of the transition relation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigTrans.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "proof/fra/fra.h"

ABC_NAMESPACE_IMPL_START


/* 
    A similar approach is presented in the his paper:
    A. Kuehlmann. Dynamic transition relation simplification for 
    bounded property checking. ICCAD'04, pp. 50-57.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Maps a node/frame into a node of a different manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Saig_ManStartMap1( Aig_Man_t * p, int nFrames )
{
    Vec_Int_t * vMap;
    int i;
    assert( p->pData == NULL );
    vMap = Vec_IntAlloc( Aig_ManObjNumMax(p) * nFrames );
    for ( i = 0; i < vMap->nCap; i++ )
        vMap->pArray[i] = -1;
    vMap->nSize = vMap->nCap;
    p->pData = vMap;
}
static inline void Saig_ManStopMap1( Aig_Man_t * p )
{
    assert( p->pData != NULL );
    Vec_IntFree( (Vec_Int_t *)p->pData );
    p->pData = NULL;
}
static inline int Saig_ManHasMap1( Aig_Man_t * p )
{
    return (int)(p->pData != NULL);
}
static inline void Saig_ManSetMap1( Aig_Man_t * p, Aig_Obj_t * pOld, int f1, Aig_Obj_t * pNew ) 
{
    Vec_Int_t * vMap = (Vec_Int_t *)p->pData;
    int nOffset = f1 * Aig_ManObjNumMax(p) + pOld->Id;
    assert( !Aig_IsComplement(pOld) );
    assert( !Aig_IsComplement(pNew) );
    Vec_IntWriteEntry( vMap, nOffset, pNew->Id );
}
static inline int Saig_ManGetMap1( Aig_Man_t * p, Aig_Obj_t * pOld, int f1 )
{
    Vec_Int_t * vMap = (Vec_Int_t *)p->pData;
    int nOffset = f1 * Aig_ManObjNumMax(p) + pOld->Id;
    return Vec_IntEntry( vMap, nOffset );
}

/**Function*************************************************************

  Synopsis    [Maps a node/frame into a node/frame of a different manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Saig_ManStartMap2( Aig_Man_t * p, int nFrames )
{
    Vec_Int_t * vMap;
    int i;
    assert( p->pData2 == NULL );
    vMap = Vec_IntAlloc( Aig_ManObjNumMax(p) * nFrames * 2 );
    for ( i = 0; i < vMap->nCap; i++ )
        vMap->pArray[i] = -1;
    vMap->nSize = vMap->nCap;
    p->pData2 = vMap;
}
static inline void Saig_ManStopMap2( Aig_Man_t * p )
{
    assert( p->pData2 != NULL );
    Vec_IntFree( (Vec_Int_t *)p->pData2 );
    p->pData2 = NULL;
}
static inline int Saig_ManHasMap2( Aig_Man_t * p )
{
    return (int)(p->pData2 != NULL);
}
static inline void Saig_ManSetMap2( Aig_Man_t * p, Aig_Obj_t * pOld, int f1, Aig_Obj_t * pNew, int f2 ) 
{
    Vec_Int_t * vMap = (Vec_Int_t *)p->pData2;
    int nOffset = f1 * Aig_ManObjNumMax(p) + pOld->Id;
    assert( !Aig_IsComplement(pOld) );
    assert( !Aig_IsComplement(pNew) );
    Vec_IntWriteEntry( vMap, 2*nOffset + 0, pNew->Id );
    Vec_IntWriteEntry( vMap, 2*nOffset + 1, f2 );
}
static inline int Saig_ManGetMap2( Aig_Man_t * p, Aig_Obj_t * pOld, int f1, int * pf2 )
{
    Vec_Int_t * vMap = (Vec_Int_t *)p->pData2;
    int nOffset = f1 * Aig_ManObjNumMax(p) + pOld->Id;
    *pf2 = Vec_IntEntry( vMap, 2*nOffset + 1 );
    return Vec_IntEntry( vMap, 2*nOffset );
}

/**Function*************************************************************

  Synopsis    [Create mapping for the first nFrames timeframes of pAig.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManCreateMapping( Aig_Man_t * pAig, Aig_Man_t * pFrames, int nFrames )
{
    Aig_Obj_t * pObj, * pObjFrame, * pObjRepr;
    int i, f, iNum, iFrame;
    assert( pFrames->pReprs != NULL ); // mapping from nodes into their representatives
    // start step mapping for both orignal manager and fraig
    Saig_ManStartMap2( pAig, nFrames );
    Saig_ManStartMap2( pFrames, 1 );
    // for each object in each frame
    for ( f = 0; f < nFrames; f++ )
    Aig_ManForEachObj( pAig, pObj, i )
    {
        // get the frame object
        iNum = Saig_ManGetMap1( pAig, pObj, f );
        pObjFrame = Aig_ManObj( pFrames, iNum );
        // if the node has no prototype, map it into itself
        if ( pObjFrame == NULL )
        {
            Saig_ManSetMap2( pAig, pObj, f, pObj, f );
            continue;
        }
        // get the representative object
        pObjRepr = Aig_ObjRepr( pFrames, pObjFrame );
        if ( pObjRepr == NULL )
            pObjRepr = pObjFrame;
        // check if this is the first time this object is reached
        if ( Saig_ManGetMap2( pFrames, pObjRepr, 0, &iFrame ) == -1 )
            Saig_ManSetMap2( pFrames, pObjRepr, 0, pObj, f );
        // set the map for the main object
        iNum = Saig_ManGetMap2( pFrames, pObjRepr, 0, &iFrame );
        Saig_ManSetMap2( pAig, pObj, f, Aig_ManObj(pAig, iNum), iFrame );
    }
    Saig_ManStopMap2( pFrames );
    assert( Saig_ManHasMap2(pAig) );
}

/**Function*************************************************************

  Synopsis    [Unroll without initialization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManFramesNonInitial( Aig_Man_t * pAig, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f;
    assert( Saig_ManRegNum(pAig) > 0 );
    // start node map
    Saig_ManStartMap1( pAig, nFrames ); 
    // start the new manager
    pFrames = Aig_ManStart( Aig_ManNodeNum(pAig) * nFrames );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames );
    // create variables for register outputs
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pFrames );
    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // create PI nodes for this frame
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCi( pFrames );
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        // create POs for this frame
        Saig_ManForEachPo( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCo( pFrames, Aig_ObjChild0Copy(pObj) );
        // save register inputs
        Saig_ManForEachLi( pAig, pObj, i )
            pObj->pData = Aig_ObjChild0Copy(pObj);
        // save the mapping
        Aig_ManForEachObj( pAig, pObj, i )
        {
            assert( pObj->pData != NULL );
            Saig_ManSetMap1( pAig, pObj, f, Aig_Regular((Aig_Obj_t *)pObj->pData) );
        }
        // quit if the last frame
        if ( f == nFrames - 1 )
            break;
        // transfer to register outputs
        Saig_ManForEachLiLo(  pAig, pObjLi, pObjLo, i )
            pObjLo->pData = pObjLi->pData;
    }
    // remember register outputs
    Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
        Aig_ObjCreateCo( pFrames, (Aig_Obj_t *)pObjLi->pData );
    Aig_ManCleanup( pFrames );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Unroll with initialization and mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManFramesInitialMapped( Aig_Man_t * pAig, int nFrames, int nFramesMax, int fInit )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pRepr;
    int i, f, iNum1, iNum2, iFrame2;
    assert( nFrames <= nFramesMax );
    assert( Saig_ManRegNum(pAig) > 0 );
    // start node map
    Saig_ManStartMap1( pAig, nFramesMax );
    // start the new manager
    pFrames = Aig_ManStart( Aig_ManNodeNum(pAig) * nFramesMax );
    // create variables for register outputs
    if ( fInit )
    {
        Saig_ManForEachLo( pAig, pObj, i )
        {
            pObj->pData = Aig_ManConst0( pFrames );
            Saig_ManSetMap1( pAig, pObj, 0, Aig_Regular((Aig_Obj_t *)pObj->pData) );
        }
    }
    else
    {
        // create PIs first
        for ( f = 0; f < nFramesMax; f++ )
            Saig_ManForEachPi( pAig, pObj, i )
                Aig_ObjCreateCi( pFrames );
        // create registers second
        Saig_ManForEachLo( pAig, pObj, i )
        {
            pObj->pData = Aig_ObjCreateCi( pFrames );
            Saig_ManSetMap1( pAig, pObj, 0, Aig_Regular((Aig_Obj_t *)pObj->pData) );
        }
    }
    // add timeframes
    for ( f = 0; f < nFramesMax; f++ )
    {
        // map the constant node
        pObj = Aig_ManConst1(pAig);
        pObj->pData = Aig_ManConst1( pFrames );
        Saig_ManSetMap1( pAig, pObj, f, Aig_Regular((Aig_Obj_t *)pObj->pData) );
        // create PI nodes for this frame
        Saig_ManForEachPi( pAig, pObj, i )
        {
            if ( fInit )
                pObj->pData = Aig_ObjCreateCi( pFrames );
            else
                pObj->pData = Aig_ManCi( pFrames, f * Saig_ManPiNum(pAig) + i );
            Saig_ManSetMap1( pAig, pObj, f, Aig_Regular((Aig_Obj_t *)pObj->pData) );
        }
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
        {
            pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
            Saig_ManSetMap1( pAig, pObj, f, Aig_Regular((Aig_Obj_t *)pObj->pData) );
            if ( !Saig_ManHasMap2(pAig) )
                continue;
            if ( f < nFrames )
            {
                // get the mapping for this node
                iNum2 = Saig_ManGetMap2( pAig, pObj, f, &iFrame2 );
            }
            else
            {
                // get the mapping for this node
                iNum2 = Saig_ManGetMap2( pAig, pObj, nFrames-1, &iFrame2 );
                iFrame2 += f - (nFrames-1);
            }
            assert( iNum2 != -1 );
            assert( f >= iFrame2 );
            // get the corresponding frames node
            iNum1 = Saig_ManGetMap1( pAig, Aig_ManObj(pAig, iNum2), iFrame2 );
            pRepr = Aig_ManObj( pFrames, iNum1 );
            // compare the phases of these nodes
            pObj->pData = Aig_NotCond( pRepr, pRepr->fPhase ^ Aig_ObjPhaseReal((Aig_Obj_t *)pObj->pData) );
        }
        // create POs for this frame
        Saig_ManForEachPo( pAig, pObj, i )
        {
            pObj->pData = Aig_ObjCreateCo( pFrames, Aig_ObjChild0Copy(pObj) );
            Saig_ManSetMap1( pAig, pObj, f, Aig_Regular((Aig_Obj_t *)pObj->pData) );
        }
        // save register inputs
        Saig_ManForEachLi( pAig, pObj, i )
        {
            pObj->pData = Aig_ObjChild0Copy(pObj);
            Saig_ManSetMap1( pAig, pObj, f, Aig_Regular((Aig_Obj_t *)pObj->pData) );
        }
        // quit if the last frame
        if ( f == nFramesMax - 1 )
            break;
        // transfer to register outputs
        Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
        {
            pObjLo->pData = pObjLi->pData;
            if ( !fInit )
                Saig_ManSetMap1( pAig, pObjLo, f+1, Aig_Regular((Aig_Obj_t *)pObjLo->pData) );
        }
    }
    if ( !fInit )
    {
        // create registers
        Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
            Aig_ObjCreateCo( pFrames, (Aig_Obj_t *)pObjLi->pData );
        // set register number
        Aig_ManSetRegNum( pFrames, pAig->nRegs );
    }
    Aig_ManCleanup( pFrames );
    Saig_ManStopMap1( pAig );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Implements dynamic simplification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManTimeframeSimplify( Aig_Man_t * pAig, int nFrames, int nFramesMax, int fInit, int fVerbose )
{
//    extern Aig_Man_t * Fra_FraigEquivence( Aig_Man_t * pManAig, int nConfMax, int fProve );
    Aig_Man_t * pFrames, * pFraig, * pRes1, * pRes2;
    abctime clk;
    // create uninitialized timeframes with map1
    pFrames = Saig_ManFramesNonInitial( pAig, nFrames );
    // perform fraiging for the unrolled timeframes
clk = Abc_Clock();
    pFraig = Fra_FraigEquivence( pFrames, 1000, 0 );
    // report the results
    if ( fVerbose )
    {
        Aig_ManPrintStats( pFrames );
        Aig_ManPrintStats( pFraig );
ABC_PRT( "Fraiging", Abc_Clock() - clk );
    }
    Aig_ManStop( pFraig );
    assert( pFrames->pReprs != NULL );
    // create AIG with map2
    Saig_ManCreateMapping( pAig, pFrames, nFrames );
    Aig_ManStop( pFrames );
    Saig_ManStopMap1( pAig );
    // create reduced initialized timeframes
clk = Abc_Clock();
    pRes2 = Saig_ManFramesInitialMapped( pAig, nFrames, nFramesMax, fInit );
ABC_PRT( "Mapped", Abc_Clock() - clk );
    // free mapping
    Saig_ManStopMap2( pAig );
clk = Abc_Clock();
    pRes1 = Saig_ManFramesInitialMapped( pAig, nFrames, nFramesMax, fInit );
ABC_PRT( "Normal", Abc_Clock() - clk );
    // report the results
    if ( fVerbose )
    {
        Aig_ManPrintStats( pRes1 );
        Aig_ManPrintStats( pRes2 );
    }
    Aig_ManStop( pRes1 );
    assert( !Saig_ManHasMap1(pAig) );
    assert( !Saig_ManHasMap2(pAig) );
    return pRes2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

